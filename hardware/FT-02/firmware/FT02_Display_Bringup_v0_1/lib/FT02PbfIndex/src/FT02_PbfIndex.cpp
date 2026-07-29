#include "FT02_PbfIndex.h"

#include "FT02_Storage.h"

#include <esp_heap_caps.h>
#include <miniz.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FT-02 PBF Persistent Index A1
//
// This stage keeps the original .osm.pbf untouched. On first use it scans the
// source and writes a compact, validated block catalog beside it. Later opens
// verify a 96 KiB source signature and load the catalog instead of repeating
// the ~65 second full decode.
//
// A1 is deliberately a block catalog plus node envelopes. It is the durable
// foundation for A2 local feature lookup; it is not yet the final way/POI
// spatial index or map renderer.

static const uint32_t FT02_PBF_MAX_HEADER_BYTES = 64U * 1024U;
static const uint32_t FT02_PBF_MAX_BLOB_BYTES = 4U * 1024U * 1024U;
static const uint32_t FT02_PBF_MAX_RAW_BYTES = 4U * 1024U * 1024U;
static const uint32_t FT02_PBF_SIGNATURE_SAMPLE_BYTES = 32U * 1024U;
static const uint32_t FT02_PBF_INDEX_VERSION = 1U;
static const uint16_t FT02_PBF_INDEX_FLAG_SOURCE_BOUNDS = 0x0001U;
static const uint16_t FT02_PBF_ENTRY_FLAG_NODE_BOUNDS = 0x0001U;

#pragma pack(push, 1)
struct FT02PbfIndexHeaderDisk
{
    char magic[8];
    uint16_t version;
    uint16_t headerBytes;
    uint16_t entryBytes;
    uint16_t flags;

    uint64_t sourceFileBytes;
    uint32_t sourceSignature;
    uint32_t entryCount;
    uint32_t fileBlocks;
    uint32_t headerBlocks;
    uint32_t dataBlocks;

    uint64_t nodes;
    uint64_t ways;
    uint64_t relations;
    uint64_t rawPayloadBytes;
    uint64_t compressedPayloadBytes;

    uint32_t maxBlobBytes;
    uint32_t maxRawBlockBytes;
    uint32_t buildElapsedMs;
    uint32_t entryCrc32;

    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;

    uint8_t reserved[12];
};

struct FT02PbfIndexEntryDisk
{
    uint64_t fileOffset;
    uint32_t blobHeaderBytes;
    uint32_t blobBytes;
    uint32_t rawBytes;
    uint32_t compressedBytes;

    uint32_t nodes;
    uint32_t ways;
    uint32_t relations;

    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;

    uint8_t blockType;
    uint8_t compression;
    uint16_t flags;
    uint8_t reserved[8];
};
#pragma pack(pop)

static_assert(sizeof(FT02PbfIndexHeaderDisk) == 128, "PBI header must be 128 bytes");
static_assert(sizeof(FT02PbfIndexEntryDisk) == 64, "PBI entry must be 64 bytes");

static const char FT02_PBF_INDEX_MAGIC[8] = {
    'F', 'T', 'P', 'B', 'I', '1', 0, 0
};

enum FT02PbfIndexLoadResult
{
    FT02_PBF_LOAD_MISSING = 0,
    FT02_PBF_LOAD_VALID,
    FT02_PBF_LOAD_INVALID,
    FT02_PBF_LOAD_IO_ERROR
};

struct FT02PbCursor
{
    const uint8_t* data;
    size_t size;
    size_t offset;
};

struct FT02PbSlice
{
    const uint8_t* data;
    size_t size;
};

struct FT02PackedIterator
{
    const FT02PbSlice* slices;
    size_t sliceCount;
    size_t sliceIndex;
    FT02PbCursor cursor;
};

struct FT02PbfBlobHeader
{
    char type[16];
    uint32_t dataSize;
};

struct FT02PbfBlobView
{
    uint32_t rawSize;
    const uint8_t* rawData;
    size_t rawLength;
    const uint8_t* zlibData;
    size_t zlibLength;
    bool hasOtherCompression;
};

struct FT02GeoBounds
{
    bool valid;
    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;
};

struct FT02BlockStats
{
    uint64_t nodes;
    uint64_t ways;
    uint64_t relations;
    FT02GeoBounds nodeBounds;
};

static FT02PbfIndexReport g_report;

static uint32_t FT02_Crc32Update(
    uint32_t crc,
    const uint8_t* data,
    size_t size
)
{
    for(size_t i = 0; i < size; i++)
    {
        crc ^= data[i];

        for(uint8_t bit = 0; bit < 8; bit++)
        {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }

    return crc;
}

static uint32_t FT02_Crc32Finish(uint32_t crc)
{
    return ~crc;
}

static bool FT02_PbReadVarint(FT02PbCursor& cursor, uint64_t& value)
{
    value = 0;
    uint32_t shift = 0;

    while(cursor.offset < cursor.size && shift <= 63)
    {
        uint8_t byte = cursor.data[cursor.offset++];
        value |= (uint64_t)(byte & 0x7FU) << shift;

        if((byte & 0x80U) == 0)
        {
            return true;
        }

        shift += 7;
    }

    return false;
}

static int64_t FT02_PbDecodeZigZag64(uint64_t value)
{
    return (int64_t)((value >> 1) ^ (uint64_t)-(int64_t)(value & 1U));
}

static bool FT02_PbReadSInt64(FT02PbCursor& cursor, int64_t& value)
{
    uint64_t encoded = 0;

    if(!FT02_PbReadVarint(cursor, encoded))
    {
        return false;
    }

    value = FT02_PbDecodeZigZag64(encoded);
    return true;
}

static bool FT02_PbReadKey(
    FT02PbCursor& cursor,
    uint32_t& fieldNumber,
    uint8_t& wireType
)
{
    uint64_t key = 0;

    if(!FT02_PbReadVarint(cursor, key))
    {
        return false;
    }

    fieldNumber = (uint32_t)(key >> 3);
    wireType = (uint8_t)(key & 0x07U);
    return fieldNumber != 0;
}

static bool FT02_PbReadBytes(
    FT02PbCursor& cursor,
    const uint8_t*& bytes,
    size_t& length
)
{
    uint64_t encodedLength = 0;

    if(!FT02_PbReadVarint(cursor, encodedLength))
    {
        return false;
    }

    if(encodedLength > (uint64_t)(cursor.size - cursor.offset))
    {
        return false;
    }

    bytes = cursor.data + cursor.offset;
    length = (size_t)encodedLength;
    cursor.offset += length;
    return true;
}

static bool FT02_PbSkipField(FT02PbCursor& cursor, uint8_t wireType)
{
    if(wireType == 0)
    {
        uint64_t ignored = 0;
        return FT02_PbReadVarint(cursor, ignored);
    }

    if(wireType == 1)
    {
        if(cursor.size - cursor.offset < 8) return false;
        cursor.offset += 8;
        return true;
    }

    if(wireType == 2)
    {
        const uint8_t* ignored = nullptr;
        size_t ignoredLength = 0;
        return FT02_PbReadBytes(cursor, ignored, ignoredLength);
    }

    if(wireType == 5)
    {
        if(cursor.size - cursor.offset < 4) return false;
        cursor.offset += 4;
        return true;
    }

    return false;
}

static void FT02_PackedIteratorBegin(
    FT02PackedIterator& iterator,
    const FT02PbSlice* slices,
    size_t sliceCount
)
{
    iterator.slices = slices;
    iterator.sliceCount = sliceCount;
    iterator.sliceIndex = 0;
    iterator.cursor = {nullptr, 0, 0};

    if(sliceCount > 0)
    {
        iterator.cursor = {slices[0].data, slices[0].size, 0};
    }
}

static bool FT02_PackedIteratorNext(
    FT02PackedIterator& iterator,
    uint64_t& value,
    bool& hasValue
)
{
    hasValue = false;

    while(iterator.sliceIndex < iterator.sliceCount)
    {
        if(iterator.cursor.offset < iterator.cursor.size)
        {
            if(!FT02_PbReadVarint(iterator.cursor, value))
            {
                return false;
            }

            hasValue = true;
            return true;
        }

        iterator.sliceIndex++;

        if(iterator.sliceIndex < iterator.sliceCount)
        {
            const FT02PbSlice& next = iterator.slices[iterator.sliceIndex];
            iterator.cursor = {next.data, next.size, 0};
        }
    }

    return true;
}

static void FT02_BoundsReset(FT02GeoBounds& bounds)
{
    memset(&bounds, 0, sizeof(bounds));
}

static void FT02_BoundsAdd(
    FT02GeoBounds& bounds,
    int32_t latE7,
    int32_t lonE7
)
{
    if(!bounds.valid)
    {
        bounds.valid = true;
        bounds.minLatE7 = latE7;
        bounds.maxLatE7 = latE7;
        bounds.minLonE7 = lonE7;
        bounds.maxLonE7 = lonE7;
        return;
    }

    if(latE7 < bounds.minLatE7) bounds.minLatE7 = latE7;
    if(latE7 > bounds.maxLatE7) bounds.maxLatE7 = latE7;
    if(lonE7 < bounds.minLonE7) bounds.minLonE7 = lonE7;
    if(lonE7 > bounds.maxLonE7) bounds.maxLonE7 = lonE7;
}

static void FT02_BoundsMerge(
    FT02GeoBounds& destination,
    const FT02GeoBounds& source
)
{
    if(!source.valid)
    {
        return;
    }

    FT02_BoundsAdd(destination, source.minLatE7, source.minLonE7);
    FT02_BoundsAdd(destination, source.maxLatE7, source.maxLonE7);
}

static int32_t FT02_NanodegreesToE7(int64_t nanodegrees)
{
    return (int32_t)(nanodegrees / 100LL);
}

static bool FT02_ParseBlobHeader(
    const uint8_t* data,
    size_t size,
    FT02PbfBlobHeader& result
)
{
    memset(&result, 0, sizeof(result));
    FT02PbCursor cursor = {data, size, 0};
    bool hasType = false;
    bool hasDataSize = false;

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;

        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(field == 1 && wire == 2)
        {
            const uint8_t* value = nullptr;
            size_t length = 0;
            if(!FT02_PbReadBytes(cursor, value, length)) return false;

            size_t copyLength = length;
            if(copyLength >= sizeof(result.type)) copyLength = sizeof(result.type) - 1;
            memcpy(result.type, value, copyLength);
            result.type[copyLength] = 0;
            hasType = true;
        }
        else if(field == 3 && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(cursor, value) || value > UINT32_MAX) return false;
            result.dataSize = (uint32_t)value;
            hasDataSize = true;
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    return hasType && hasDataSize;
}

static bool FT02_ParseBlob(
    const uint8_t* data,
    size_t size,
    FT02PbfBlobView& result
)
{
    memset(&result, 0, sizeof(result));
    FT02PbCursor cursor = {data, size, 0};

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;

        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(field == 1 && wire == 2)
        {
            if(!FT02_PbReadBytes(cursor, result.rawData, result.rawLength)) return false;
        }
        else if(field == 2 && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(cursor, value) || value > UINT32_MAX) return false;
            result.rawSize = (uint32_t)value;
        }
        else if(field == 3 && wire == 2)
        {
            if(!FT02_PbReadBytes(cursor, result.zlibData, result.zlibLength)) return false;
        }
        else if((field == 4 || field == 5 || field == 6 || field == 7) && wire == 2)
        {
            const uint8_t* ignored = nullptr;
            size_t ignoredLength = 0;
            if(!FT02_PbReadBytes(cursor, ignored, ignoredLength)) return false;
            result.hasOtherCompression = true;
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    if(result.rawData != nullptr)
    {
        if(result.rawSize == 0) result.rawSize = (uint32_t)result.rawLength;
        return true;
    }

    return result.zlibData != nullptr && result.rawSize > 0;
}

static bool FT02_ParseHeaderBBox(
    const uint8_t* data,
    size_t size,
    FT02GeoBounds& bounds
)
{
    FT02PbCursor cursor = {data, size, 0};
    int64_t left = 0;
    int64_t right = 0;
    int64_t top = 0;
    int64_t bottom = 0;
    bool hasLeft = false;
    bool hasRight = false;
    bool hasTop = false;
    bool hasBottom = false;

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(wire == 0 && field >= 1 && field <= 4)
        {
            int64_t value = 0;
            if(!FT02_PbReadSInt64(cursor, value)) return false;

            if(field == 1) { left = value; hasLeft = true; }
            if(field == 2) { right = value; hasRight = true; }
            if(field == 3) { top = value; hasTop = true; }
            if(field == 4) { bottom = value; hasBottom = true; }
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    if(!hasLeft || !hasRight || !hasTop || !hasBottom)
    {
        return false;
    }

    bounds.valid = true;
    bounds.minLatE7 = FT02_NanodegreesToE7(bottom);
    bounds.minLonE7 = FT02_NanodegreesToE7(left);
    bounds.maxLatE7 = FT02_NanodegreesToE7(top);
    bounds.maxLonE7 = FT02_NanodegreesToE7(right);
    return true;
}

static bool FT02_ParseHeaderBlockBounds(
    const uint8_t* data,
    size_t size,
    FT02GeoBounds& bounds
)
{
    FT02PbCursor cursor = {data, size, 0};

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(field == 1 && wire == 2)
        {
            const uint8_t* message = nullptr;
            size_t messageLength = 0;
            if(!FT02_PbReadBytes(cursor, message, messageLength)) return false;
            return FT02_ParseHeaderBBox(message, messageLength, bounds);
        }

        if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    return true;
}

static bool FT02_CountPackedVarints(
    const uint8_t* data,
    size_t size,
    uint64_t& count
)
{
    FT02PbCursor cursor = {data, size, 0};

    while(cursor.offset < cursor.size)
    {
        uint64_t ignored = 0;
        if(!FT02_PbReadVarint(cursor, ignored)) return false;
        count++;
    }

    return true;
}

static bool FT02_ParseRegularNode(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    FT02GeoBounds& bounds
)
{
    FT02PbCursor cursor = {data, size, 0};
    int64_t lat = 0;
    int64_t lon = 0;
    bool hasLat = false;
    bool hasLon = false;

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(field == 8 && wire == 0)
        {
            if(!FT02_PbReadSInt64(cursor, lat)) return false;
            hasLat = true;
        }
        else if(field == 9 && wire == 0)
        {
            if(!FT02_PbReadSInt64(cursor, lon)) return false;
            hasLon = true;
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    if(hasLat && hasLon)
    {
        int64_t latNano = latOffset + granularity * lat;
        int64_t lonNano = lonOffset + granularity * lon;
        FT02_BoundsAdd(
            bounds,
            FT02_NanodegreesToE7(latNano),
            FT02_NanodegreesToE7(lonNano)
        );
    }

    return true;
}

static bool FT02_ParseDenseNodes(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    uint64_t& nodeCount,
    FT02GeoBounds& bounds
)
{
    static const size_t MAX_PACKED_SLICES = 8;
    FT02PbSlice idSlices[MAX_PACKED_SLICES];
    FT02PbSlice latSlices[MAX_PACKED_SLICES];
    FT02PbSlice lonSlices[MAX_PACKED_SLICES];
    size_t idSliceCount = 0;
    size_t latSliceCount = 0;
    size_t lonSliceCount = 0;
    uint64_t scalarIds = 0;

    FT02PbCursor cursor = {data, size, 0};

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if((field == 1 || field == 8 || field == 9) && wire == 2)
        {
            const uint8_t* packed = nullptr;
            size_t packedLength = 0;
            if(!FT02_PbReadBytes(cursor, packed, packedLength)) return false;

            FT02PbSlice* target = nullptr;
            size_t* targetCount = nullptr;

            if(field == 1) { target = idSlices; targetCount = &idSliceCount; }
            if(field == 8) { target = latSlices; targetCount = &latSliceCount; }
            if(field == 9) { target = lonSlices; targetCount = &lonSliceCount; }

            if(target == nullptr || targetCount == nullptr || *targetCount >= MAX_PACKED_SLICES)
            {
                return false;
            }

            target[*targetCount] = {packed, packedLength};
            (*targetCount)++;
        }
        else if(field == 1 && wire == 0)
        {
            uint64_t ignored = 0;
            if(!FT02_PbReadVarint(cursor, ignored)) return false;
            scalarIds++;
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    uint64_t packedIds = 0;
    for(size_t i = 0; i < idSliceCount; i++)
    {
        if(!FT02_CountPackedVarints(idSlices[i].data, idSlices[i].size, packedIds))
        {
            return false;
        }
    }

    nodeCount += scalarIds + packedIds;

    FT02PackedIterator latIterator;
    FT02PackedIterator lonIterator;
    FT02_PackedIteratorBegin(latIterator, latSlices, latSliceCount);
    FT02_PackedIteratorBegin(lonIterator, lonSlices, lonSliceCount);

    int64_t accumulatedLat = 0;
    int64_t accumulatedLon = 0;

    while(true)
    {
        uint64_t encodedLat = 0;
        uint64_t encodedLon = 0;
        bool hasLat = false;
        bool hasLon = false;

        if(!FT02_PackedIteratorNext(latIterator, encodedLat, hasLat)) return false;
        if(!FT02_PackedIteratorNext(lonIterator, encodedLon, hasLon)) return false;

        if(hasLat != hasLon)
        {
            return false;
        }

        if(!hasLat)
        {
            break;
        }

        accumulatedLat += FT02_PbDecodeZigZag64(encodedLat);
        accumulatedLon += FT02_PbDecodeZigZag64(encodedLon);

        int64_t latNano = latOffset + granularity * accumulatedLat;
        int64_t lonNano = lonOffset + granularity * accumulatedLon;

        FT02_BoundsAdd(
            bounds,
            FT02_NanodegreesToE7(latNano),
            FT02_NanodegreesToE7(lonNano)
        );
    }

    return true;
}

static bool FT02_ParsePrimitiveGroup(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    FT02BlockStats& stats
)
{
    FT02PbCursor cursor = {data, size, 0};

    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;

        if(wire == 2 && (field == 1 || field == 2 || field == 3 || field == 4))
        {
            const uint8_t* message = nullptr;
            size_t messageLength = 0;
            if(!FT02_PbReadBytes(cursor, message, messageLength)) return false;

            if(field == 1)
            {
                stats.nodes++;
                if(!FT02_ParseRegularNode(
                    message,
                    messageLength,
                    granularity,
                    latOffset,
                    lonOffset,
                    stats.nodeBounds
                )) return false;
            }
            else if(field == 2)
            {
                if(!FT02_ParseDenseNodes(
                    message,
                    messageLength,
                    granularity,
                    latOffset,
                    lonOffset,
                    stats.nodes,
                    stats.nodeBounds
                )) return false;
            }
            else if(field == 3)
            {
                stats.ways++;
            }
            else if(field == 4)
            {
                stats.relations++;
            }
        }
        else if(!FT02_PbSkipField(cursor, wire))
        {
            return false;
        }
    }

    return true;
}

static bool FT02_ParsePrimitiveBlockStats(
    const uint8_t* data,
    size_t size,
    FT02BlockStats& stats
)
{
    memset(&stats, 0, sizeof(stats));
    FT02_BoundsReset(stats.nodeBounds);

    int64_t granularity = 100;
    int64_t latOffset = 0;
    int64_t lonOffset = 0;

    // Pass 1: protobuf field order is not guaranteed, so collect coordinate
    // parameters before decoding any PrimitiveGroup.
    FT02PbCursor metadataCursor = {data, size, 0};

    while(metadataCursor.offset < metadataCursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(metadataCursor, field, wire)) return false;

        if(field == 17 && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(metadataCursor, value)) return false;
            granularity = (int64_t)value;
        }
        else if((field == 19 || field == 20) && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(metadataCursor, value)) return false;
            if(field == 19) latOffset = (int64_t)value;
            if(field == 20) lonOffset = (int64_t)value;
        }
        else if(!FT02_PbSkipField(metadataCursor, wire))
        {
            return false;
        }
    }

    // Pass 2: decode groups with the resolved coordinate parameters.
    FT02PbCursor groupCursor = {data, size, 0};

    while(groupCursor.offset < groupCursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(groupCursor, field, wire)) return false;

        if(field == 2 && wire == 2)
        {
            const uint8_t* group = nullptr;
            size_t groupLength = 0;
            if(!FT02_PbReadBytes(groupCursor, group, groupLength)) return false;

            if(!FT02_ParsePrimitiveGroup(
                group,
                groupLength,
                granularity,
                latOffset,
                lonOffset,
                stats
            )) return false;
        }
        else if(!FT02_PbSkipField(groupCursor, wire))
        {
            return false;
        }
    }

    return true;
}

static void* FT02_PbfAllocate(size_t bytes)
{
    if(bytes == 0) return nullptr;

    if(bytes <= 64U * 1024U)
    {
        return heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void FT02_PbfFree(void* memory)
{
    if(memory != nullptr)
    {
        heap_caps_free(memory);
    }
}

static bool FT02_ReadExact(FILE* file, void* output, size_t bytes)
{
    if(bytes == 0) return true;
    return file != nullptr && fread(output, 1, bytes, file) == bytes;
}

static bool FT02_WriteExact(FILE* file, const void* data, size_t bytes)
{
    if(bytes == 0) return true;
    return file != nullptr && fwrite(data, 1, bytes, file) == bytes;
}

static bool FT02_InflateZlibToBuffer(
    const uint8_t* input,
    size_t inputLength,
    uint8_t* output,
    size_t outputCapacity,
    size_t& outputLength
)
{
    outputLength = 0;

    if(
        input == nullptr
        || inputLength == 0
        || output == nullptr
        || outputCapacity == 0
    )
    {
        return false;
    }

    tinfl_decompressor* decompressor =
        (tinfl_decompressor*)heap_caps_calloc(
            1,
            sizeof(tinfl_decompressor),
            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
        );

    if(decompressor == nullptr)
    {
        return false;
    }

    tinfl_init(decompressor);

    size_t consumedInput = inputLength;
    size_t producedOutput = outputCapacity;

    tinfl_status status = tinfl_decompress(
        decompressor,
        input,
        &consumedInput,
        output,
        output,
        &producedOutput,
        TINFL_FLAG_PARSE_ZLIB_HEADER
            | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
    );

    outputLength = producedOutput;
    heap_caps_free(decompressor);

    return status == TINFL_STATUS_DONE
        && producedOutput == outputCapacity
        && consumedInput <= inputLength;
}

static void FT02_SetError(
    FT02PbfIndexError error,
    uint32_t blockIndex,
    const char* blockType
)
{
    g_report.state = FT02_PBF_INDEX_ERROR;
    g_report.error = error;
    g_report.failedBlockIndex = blockIndex;

    if(blockType == nullptr) blockType = "";
    strncpy(g_report.failedBlockType, blockType, sizeof(g_report.failedBlockType) - 1);
    g_report.failedBlockType[sizeof(g_report.failedBlockType) - 1] = 0;
}

static bool FT02_ComputeSourceSignature(
    FILE* source,
    uint64_t fileBytes,
    uint32_t& signature
)
{
    signature = 0;

    if(source == nullptr)
    {
        return false;
    }

    uint8_t* buffer = (uint8_t*)FT02_PbfAllocate(FT02_PBF_SIGNATURE_SAMPLE_BYTES);
    if(buffer == nullptr)
    {
        return false;
    }

    uint64_t positions[3] = {0, 0, 0};
    positions[0] = 0;
    positions[1] = fileBytes > FT02_PBF_SIGNATURE_SAMPLE_BYTES
        ? (fileBytes / 2U) - (FT02_PBF_SIGNATURE_SAMPLE_BYTES / 2U)
        : 0;
    positions[2] = fileBytes > FT02_PBF_SIGNATURE_SAMPLE_BYTES
        ? fileBytes - FT02_PBF_SIGNATURE_SAMPLE_BYTES
        : 0;

    uint32_t crc = 0xFFFFFFFFU;
    crc = FT02_Crc32Update(crc, (const uint8_t*)&fileBytes, sizeof(fileBytes));

    for(size_t sample = 0; sample < 3; sample++)
    {
        if(sample > 0 && positions[sample] == positions[sample - 1])
        {
            continue;
        }

        uint64_t remaining = fileBytes - positions[sample];
        size_t bytesToRead = remaining < FT02_PBF_SIGNATURE_SAMPLE_BYTES
            ? (size_t)remaining
            : (size_t)FT02_PBF_SIGNATURE_SAMPLE_BYTES;

        if(
            positions[sample] > (uint64_t)LONG_MAX
            || fseek(source, (long)positions[sample], SEEK_SET) != 0
            || !FT02_ReadExact(source, buffer, bytesToRead)
        )
        {
            FT02_PbfFree(buffer);
            return false;
        }

        crc = FT02_Crc32Update(
            crc,
            (const uint8_t*)&positions[sample],
            sizeof(positions[sample])
        );
        crc = FT02_Crc32Update(crc, buffer, bytesToRead);
    }

    FT02_PbfFree(buffer);
    signature = FT02_Crc32Finish(crc);
    return fseek(source, 0, SEEK_SET) == 0;
}

bool FT02_PbfIndexComputeSourceIdentity(
    const char* sourcePath,
    uint64_t& sourceFileBytes,
    uint32_t& sourceSignature
)
{
    sourceFileBytes = 0;
    sourceSignature = 0;

    if(sourcePath == nullptr || !FT02_StorageFileExists(sourcePath)) return false;

    FILE* source = FT02_StorageOpenReadFile(sourcePath);
    if(source == nullptr) return false;

    if(fseek(source, 0, SEEK_END) != 0)
    {
        fclose(source);
        return false;
    }

    const long fileLength = ftell(source);
    if(fileLength < 0 || fseek(source, 0, SEEK_SET) != 0)
    {
        fclose(source);
        return false;
    }

    sourceFileBytes = (uint64_t)fileLength;
    const bool ok = FT02_ComputeSourceSignature(
        source,
        sourceFileBytes,
        sourceSignature
    );
    fclose(source);
    return ok;
}

static void FT02_CopyHeaderToReport(
    const FT02PbfIndexHeaderDisk& header,
    uint64_t indexFileBytes,
    bool loadedFromIndex,
    uint32_t operationElapsedMs
)
{
    g_report.loadedFromIndex = loadedFromIndex;
    g_report.rebuiltIndex = !loadedFromIndex;
    g_report.sourceBoundsValid =
        (header.flags & FT02_PBF_INDEX_FLAG_SOURCE_BOUNDS) != 0;

    g_report.sourceFileBytes = header.sourceFileBytes;
    g_report.sourceSignature = header.sourceSignature;
    g_report.indexFileBytes = indexFileBytes;
    g_report.indexEntries = header.entryCount;

    g_report.fileBlocks = header.fileBlocks;
    g_report.headerBlocks = header.headerBlocks;
    g_report.dataBlocks = header.dataBlocks;
    g_report.nodes = header.nodes;
    g_report.ways = header.ways;
    g_report.relations = header.relations;
    g_report.rawPayloadBytes = header.rawPayloadBytes;
    g_report.compressedPayloadBytes = header.compressedPayloadBytes;
    g_report.maxBlobBytes = header.maxBlobBytes;
    g_report.maxRawBlockBytes = header.maxRawBlockBytes;
    g_report.originalBuildElapsedMs = header.buildElapsedMs;
    g_report.operationElapsedMs = operationElapsedMs;

    g_report.minLatE7 = header.minLatE7;
    g_report.minLonE7 = header.minLonE7;
    g_report.maxLatE7 = header.maxLatE7;
    g_report.maxLonE7 = header.maxLonE7;

    g_report.state = FT02_PBF_INDEX_READY;
    g_report.error = FT02_PBF_INDEX_ERROR_NONE;
}

static FT02PbfIndexLoadResult FT02_TryLoadIndex(
    const char* indexPath,
    uint64_t sourceFileBytes,
    uint32_t sourceSignature,
    uint32_t startedAt
)
{
    if(!FT02_StorageFileExists(indexPath))
    {
        return FT02_PBF_LOAD_MISSING;
    }

    uint64_t indexFileBytes = 0;
    if(!FT02_StorageFileSize(indexPath, indexFileBytes))
    {
        return FT02_PBF_LOAD_IO_ERROR;
    }

    FILE* indexFile = FT02_StorageOpenReadFile(indexPath);
    if(indexFile == nullptr)
    {
        return FT02_PBF_LOAD_IO_ERROR;
    }

    FT02PbfIndexHeaderDisk header;
    memset(&header, 0, sizeof(header));
    bool readOk = FT02_ReadExact(indexFile, &header, sizeof(header));

    bool headerValid = readOk
        && memcmp(header.magic, FT02_PBF_INDEX_MAGIC, sizeof(header.magic)) == 0
        && header.version == FT02_PBF_INDEX_VERSION
        && header.headerBytes == sizeof(FT02PbfIndexHeaderDisk)
        && header.entryBytes == sizeof(FT02PbfIndexEntryDisk)
        && header.sourceFileBytes == sourceFileBytes
        && header.sourceSignature == sourceSignature
        && header.entryCount > 0
        && header.entryCount < 100000U;

    uint64_t expectedBytes = sizeof(FT02PbfIndexHeaderDisk)
        + (uint64_t)header.entryCount * sizeof(FT02PbfIndexEntryDisk);

    if(!headerValid || expectedBytes != indexFileBytes)
    {
        fclose(indexFile);
        return FT02_PBF_LOAD_INVALID;
    }

    uint8_t buffer[1024];
    uint64_t remaining = (uint64_t)header.entryCount * sizeof(FT02PbfIndexEntryDisk);
    uint32_t crc = 0xFFFFFFFFU;

    while(remaining > 0)
    {
        size_t chunk = remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer);

        if(!FT02_ReadExact(indexFile, buffer, chunk))
        {
            fclose(indexFile);
            return FT02_PBF_LOAD_IO_ERROR;
        }

        crc = FT02_Crc32Update(crc, buffer, chunk);
        remaining -= chunk;
    }

    fclose(indexFile);

    if(FT02_Crc32Finish(crc) != header.entryCrc32)
    {
        return FT02_PBF_LOAD_INVALID;
    }

    FT02_CopyHeaderToReport(
        header,
        indexFileBytes,
        true,
        millis() - startedAt
    );

    Serial.println("[PBF-A1] persistent index loaded");
    Serial.print("[PBF-A1] index_bytes=");
    Serial.print((unsigned long long)indexFileBytes);
    Serial.print(" entries=");
    Serial.print(header.entryCount);
    Serial.print(" open_ms=");
    Serial.println(g_report.operationElapsedMs);

    return FT02_PBF_LOAD_VALID;
}

static bool FT02_CommitIndex(
    const char* tempPath,
    const char* finalPath
)
{
    if(!FT02_StorageDeleteFile(finalPath))
    {
        return false;
    }

    return FT02_StorageRenameFile(tempPath, finalPath);
}

static bool FT02_BuildIndex(
    const char* sourcePath,
    const char* indexPath,
    uint64_t sourceFileBytes,
    uint32_t sourceSignature,
    uint32_t startedAt
)
{
    if(g_report.psramBytes < (2U * 1024U * 1024U))
    {
        FT02_SetError(FT02_PBF_INDEX_ERROR_PSRAM_UNAVAILABLE, 0, "");
        return false;
    }

    FILE* source = FT02_StorageOpenReadFile(sourcePath);
    if(source == nullptr)
    {
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_OPEN, 0, "");
        return false;
    }

    FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
    FILE* indexFile = FT02_StorageOpenWriteFile(FT02_PBF_INDEX_TEMP_PATH, true);

    if(indexFile == nullptr)
    {
        fclose(source);
        FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_OPEN, 0, "");
        return false;
    }

    FT02PbfIndexHeaderDisk header;
    memset(&header, 0, sizeof(header));

    if(!FT02_WriteExact(indexFile, &header, sizeof(header)))
    {
        fclose(indexFile);
        fclose(source);
        FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
        FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_WRITE, 0, "");
        return false;
    }

    g_report.state = FT02_PBF_INDEX_BUILDING;
    g_report.sourceFileBytes = sourceFileBytes;
    g_report.sourceSignature = sourceSignature;

    Serial.println("[PBF-A1] persistent index build begin");
    Serial.print("[PBF-A1] source=");
    Serial.println(sourcePath);
    Serial.print("[PBF-A1] index=");
    Serial.println(indexPath);
    Serial.print("[PBF-A1] source_bytes=");
    Serial.print((unsigned long long)sourceFileBytes);
    Serial.print(" signature=0x");
    Serial.println(sourceSignature, HEX);

    uint32_t entryCrc = 0xFFFFFFFFU;
    uint32_t blockIndex = 0;
    FT02GeoBounds sourceBounds;
    FT02GeoBounds observedNodeBounds;
    FT02_BoundsReset(sourceBounds);
    FT02_BoundsReset(observedNodeBounds);

    while(true)
    {
        long blockOffsetLong = ftell(source);
        if(blockOffsetLong < 0)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_SEEK, blockIndex, "");
            return false;
        }

        uint8_t networkLength[4];
        size_t prefixRead = fread(networkLength, 1, sizeof(networkLength), source);

        if(prefixRead == 0)
        {
            if(feof(source)) break;

            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_TRUNCATED, blockIndex, "");
            return false;
        }

        if(prefixRead != sizeof(networkLength))
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_TRUNCATED, blockIndex, "");
            return false;
        }

        uint32_t headerLength =
            ((uint32_t)networkLength[0] << 24)
            | ((uint32_t)networkLength[1] << 16)
            | ((uint32_t)networkLength[2] << 8)
            | (uint32_t)networkLength[3];

        if(headerLength == 0 || headerLength > FT02_PBF_MAX_HEADER_BYTES)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_HEADER_TOO_LARGE, blockIndex, "");
            return false;
        }

        uint8_t* headerBytes = (uint8_t*)FT02_PbfAllocate(headerLength);
        if(headerBytes == nullptr)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_OUT_OF_MEMORY, blockIndex, "");
            return false;
        }

        if(!FT02_ReadExact(source, headerBytes, headerLength))
        {
            FT02_PbfFree(headerBytes);
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_TRUNCATED, blockIndex, "");
            return false;
        }

        FT02PbfBlobHeader blobHeader;
        bool headerOk = FT02_ParseBlobHeader(headerBytes, headerLength, blobHeader);
        FT02_PbfFree(headerBytes);

        if(!headerOk)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_PROTOBUF, blockIndex, "HEADER");
            return false;
        }

        if(blobHeader.dataSize == 0 || blobHeader.dataSize > FT02_PBF_MAX_BLOB_BYTES)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_BLOB_TOO_LARGE, blockIndex, blobHeader.type);
            return false;
        }

        uint8_t* blobBytes = (uint8_t*)FT02_PbfAllocate(blobHeader.dataSize);
        if(blobBytes == nullptr)
        {
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_OUT_OF_MEMORY, blockIndex, blobHeader.type);
            return false;
        }

        if(!FT02_ReadExact(source, blobBytes, blobHeader.dataSize))
        {
            FT02_PbfFree(blobBytes);
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_TRUNCATED, blockIndex, blobHeader.type);
            return false;
        }

        FT02PbfBlobView blob;
        if(!FT02_ParseBlob(blobBytes, blobHeader.dataSize, blob))
        {
            FT02_PbfFree(blobBytes);
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(
                blob.hasOtherCompression
                    ? FT02_PBF_INDEX_ERROR_UNSUPPORTED_COMPRESSION
                    : FT02_PBF_INDEX_ERROR_PROTOBUF,
                blockIndex,
                blobHeader.type
            );
            return false;
        }

        if(blob.rawSize > FT02_PBF_MAX_RAW_BYTES)
        {
            FT02_PbfFree(blobBytes);
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_RAW_BLOCK_TOO_LARGE, blockIndex, blobHeader.type);
            return false;
        }

        const uint8_t* rawBytes = blob.rawData;
        size_t rawLength = blob.rawLength;
        uint8_t* ownedRawBytes = nullptr;
        uint8_t compression = 0;
        uint32_t compressedBytes = (uint32_t)blob.rawLength;

        if(blob.zlibData != nullptr)
        {
            compression = 1;
            compressedBytes = (uint32_t)blob.zlibLength;
            ownedRawBytes = (uint8_t*)FT02_PbfAllocate(blob.rawSize);

            if(ownedRawBytes == nullptr)
            {
                FT02_PbfFree(blobBytes);
                fclose(indexFile);
                fclose(source);
                FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
                FT02_SetError(FT02_PBF_INDEX_ERROR_OUT_OF_MEMORY, blockIndex, blobHeader.type);
                return false;
            }

            size_t inflated = 0;
            bool inflateOk = FT02_InflateZlibToBuffer(
                blob.zlibData,
                blob.zlibLength,
                ownedRawBytes,
                blob.rawSize,
                inflated
            );

            if(!inflateOk || inflated != blob.rawSize)
            {
                FT02_PbfFree(ownedRawBytes);
                FT02_PbfFree(blobBytes);
                fclose(indexFile);
                fclose(source);
                FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
                FT02_SetError(FT02_PBF_INDEX_ERROR_DECOMPRESSION, blockIndex, blobHeader.type);
                return false;
            }

            rawBytes = ownedRawBytes;
            rawLength = inflated;
        }

        FT02PbfIndexEntryDisk entry;
        memset(&entry, 0, sizeof(entry));
        entry.fileOffset = (uint64_t)blockOffsetLong;
        entry.blobHeaderBytes = headerLength;
        entry.blobBytes = blobHeader.dataSize;
        entry.rawBytes = (uint32_t)rawLength;
        entry.compressedBytes = compressedBytes;
        entry.compression = compression;

        if(strcmp(blobHeader.type, "OSMHeader") == 0)
        {
            entry.blockType = 1;
            g_report.headerBlocks++;

            FT02GeoBounds headerBounds;
            FT02_BoundsReset(headerBounds);
            if(!FT02_ParseHeaderBlockBounds(rawBytes, rawLength, headerBounds))
            {
                if(ownedRawBytes != nullptr) FT02_PbfFree(ownedRawBytes);
                FT02_PbfFree(blobBytes);
                fclose(indexFile);
                fclose(source);
                FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
                FT02_SetError(FT02_PBF_INDEX_ERROR_PROTOBUF, blockIndex, blobHeader.type);
                return false;
            }

            FT02_BoundsMerge(sourceBounds, headerBounds);
        }
        else if(strcmp(blobHeader.type, "OSMData") == 0)
        {
            entry.blockType = 2;
            FT02BlockStats blockStats;

            if(!FT02_ParsePrimitiveBlockStats(rawBytes, rawLength, blockStats))
            {
                if(ownedRawBytes != nullptr) FT02_PbfFree(ownedRawBytes);
                FT02_PbfFree(blobBytes);
                fclose(indexFile);
                fclose(source);
                FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
                FT02_SetError(FT02_PBF_INDEX_ERROR_PROTOBUF, blockIndex, blobHeader.type);
                return false;
            }

            if(
                blockStats.nodes > UINT32_MAX
                || blockStats.ways > UINT32_MAX
                || blockStats.relations > UINT32_MAX
            )
            {
                if(ownedRawBytes != nullptr) FT02_PbfFree(ownedRawBytes);
                FT02_PbfFree(blobBytes);
                fclose(indexFile);
                fclose(source);
                FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
                FT02_SetError(FT02_PBF_INDEX_ERROR_PROTOBUF, blockIndex, blobHeader.type);
                return false;
            }

            entry.nodes = (uint32_t)blockStats.nodes;
            entry.ways = (uint32_t)blockStats.ways;
            entry.relations = (uint32_t)blockStats.relations;

            if(blockStats.nodeBounds.valid)
            {
                entry.flags |= FT02_PBF_ENTRY_FLAG_NODE_BOUNDS;
                entry.minLatE7 = blockStats.nodeBounds.minLatE7;
                entry.minLonE7 = blockStats.nodeBounds.minLonE7;
                entry.maxLatE7 = blockStats.nodeBounds.maxLatE7;
                entry.maxLonE7 = blockStats.nodeBounds.maxLonE7;
                FT02_BoundsMerge(observedNodeBounds, blockStats.nodeBounds);
            }

            g_report.dataBlocks++;
            g_report.nodes += blockStats.nodes;
            g_report.ways += blockStats.ways;
            g_report.relations += blockStats.relations;
        }
        else
        {
            entry.blockType = 0;
        }

        if(!FT02_WriteExact(indexFile, &entry, sizeof(entry)))
        {
            if(ownedRawBytes != nullptr) FT02_PbfFree(ownedRawBytes);
            FT02_PbfFree(blobBytes);
            fclose(indexFile);
            fclose(source);
            FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
            FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_WRITE, blockIndex, blobHeader.type);
            return false;
        }

        entryCrc = FT02_Crc32Update(
            entryCrc,
            (const uint8_t*)&entry,
            sizeof(entry)
        );

        g_report.fileBlocks++;
        g_report.indexEntries++;
        g_report.bytesRead += 4ULL + headerLength + blobHeader.dataSize;
        g_report.rawPayloadBytes += rawLength;
        g_report.compressedPayloadBytes += compressedBytes;
        if(blobHeader.dataSize > g_report.maxBlobBytes) g_report.maxBlobBytes = blobHeader.dataSize;
        if(rawLength > g_report.maxRawBlockBytes) g_report.maxRawBlockBytes = (uint32_t)rawLength;
        g_report.minimumFreeHeap = ESP.getMinFreeHeap();

        if(ownedRawBytes != nullptr) FT02_PbfFree(ownedRawBytes);
        FT02_PbfFree(blobBytes);

        if((blockIndex % 16U) == 0U)
        {
            unsigned long percent = sourceFileBytes > 0
                ? (unsigned long)((g_report.bytesRead * 100ULL) / sourceFileBytes)
                : 0;

            Serial.print("[PBF-A1] block=");
            Serial.print(blockIndex);
            Serial.print(" type=");
            Serial.print(blobHeader.type);
            Serial.print(" progress=");
            Serial.print(percent);
            Serial.print("% nodes=");
            Serial.print((unsigned long long)g_report.nodes);
            Serial.print(" ways=");
            Serial.print((unsigned long long)g_report.ways);
            Serial.print(" rels=");
            Serial.println((unsigned long long)g_report.relations);
        }

        blockIndex++;
        delay(1);
    }

    fclose(source);

    if(!sourceBounds.valid)
    {
        FT02_BoundsMerge(sourceBounds, observedNodeBounds);
    }

    memcpy(header.magic, FT02_PBF_INDEX_MAGIC, sizeof(header.magic));
    header.version = FT02_PBF_INDEX_VERSION;
    header.headerBytes = sizeof(FT02PbfIndexHeaderDisk);
    header.entryBytes = sizeof(FT02PbfIndexEntryDisk);
    header.flags = sourceBounds.valid ? FT02_PBF_INDEX_FLAG_SOURCE_BOUNDS : 0;
    header.sourceFileBytes = sourceFileBytes;
    header.sourceSignature = sourceSignature;
    header.entryCount = g_report.indexEntries;
    header.fileBlocks = g_report.fileBlocks;
    header.headerBlocks = g_report.headerBlocks;
    header.dataBlocks = g_report.dataBlocks;
    header.nodes = g_report.nodes;
    header.ways = g_report.ways;
    header.relations = g_report.relations;
    header.rawPayloadBytes = g_report.rawPayloadBytes;
    header.compressedPayloadBytes = g_report.compressedPayloadBytes;
    header.maxBlobBytes = g_report.maxBlobBytes;
    header.maxRawBlockBytes = g_report.maxRawBlockBytes;
    header.buildElapsedMs = millis() - startedAt;
    header.entryCrc32 = FT02_Crc32Finish(entryCrc);

    if(sourceBounds.valid)
    {
        header.minLatE7 = sourceBounds.minLatE7;
        header.minLonE7 = sourceBounds.minLonE7;
        header.maxLatE7 = sourceBounds.maxLatE7;
        header.maxLonE7 = sourceBounds.maxLonE7;
    }

    if(
        fseek(indexFile, 0, SEEK_SET) != 0
        || !FT02_WriteExact(indexFile, &header, sizeof(header))
    )
    {
        fclose(indexFile);
        FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
        FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_WRITE, blockIndex, "INDEX");
        return false;
    }

    if(!FT02_StorageSyncFile(indexFile))
    {
        fclose(indexFile);
        FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
        FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_SYNC, blockIndex, "INDEX");
        return false;
    }

    if(fclose(indexFile) != 0)
    {
        FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
        FT02_SetError(FT02_PBF_INDEX_ERROR_TEMP_SYNC, blockIndex, "INDEX");
        return false;
    }

    if(!FT02_CommitIndex(FT02_PBF_INDEX_TEMP_PATH, indexPath))
    {
        FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);
        FT02_SetError(FT02_PBF_INDEX_ERROR_COMMIT, blockIndex, "INDEX");
        return false;
    }

    uint64_t indexFileBytes = sizeof(FT02PbfIndexHeaderDisk)
        + (uint64_t)header.entryCount * sizeof(FT02PbfIndexEntryDisk);

    FT02_CopyHeaderToReport(
        header,
        indexFileBytes,
        false,
        millis() - startedAt
    );
    g_report.minimumFreeHeap = ESP.getMinFreeHeap();

    Serial.println("[PBF-A1] INDEX BUILT");
    Serial.print("[PBF-A1] entries=");
    Serial.print(g_report.indexEntries);
    Serial.print(" index_bytes=");
    Serial.print((unsigned long long)g_report.indexFileBytes);
    Serial.print(" build_ms=");
    Serial.println(g_report.originalBuildElapsedMs);
    Serial.print("[PBF-A1] blocks=");
    Serial.print(g_report.fileBlocks);
    Serial.print(" header=");
    Serial.print(g_report.headerBlocks);
    Serial.print(" data=");
    Serial.println(g_report.dataBlocks);
    Serial.print("[PBF-A1] nodes=");
    Serial.print((unsigned long long)g_report.nodes);
    Serial.print(" ways=");
    Serial.print((unsigned long long)g_report.ways);
    Serial.print(" relations=");
    Serial.println((unsigned long long)g_report.relations);
    Serial.flush();

    return true;
}

void FT02_PbfIndexReset()
{
    memset(&g_report, 0, sizeof(g_report));
    g_report.state = FT02_PBF_INDEX_NOT_STARTED;
    g_report.error = FT02_PBF_INDEX_ERROR_NONE;
}

void FT02_PbfIndexPrepare(bool forceRebuild)
{
    FT02_PbfIndexReset();
    g_report.state = forceRebuild
        ? FT02_PBF_INDEX_BUILDING
        : FT02_PBF_INDEX_CHECKING;
}

bool FT02_PbfIndexEnsure(
    bool forceRebuild,
    const char* sourcePath,
    const char* indexPath
)
{
    FT02_PbfIndexReset();
    g_report.state = FT02_PBF_INDEX_CHECKING;
    g_report.freeHeapBefore = ESP.getFreeHeap();
    g_report.minimumFreeHeap = ESP.getMinFreeHeap();
    g_report.psramBytes = ESP.getPsramSize();
    g_report.freePsramBefore = ESP.getFreePsram();

    const uint32_t startedAt = millis();

    if(!FT02_StorageIsReady())
    {
        FT02_SetError(FT02_PBF_INDEX_ERROR_STORAGE_NOT_READY, 0, "");
        return false;
    }

    if(!FT02_StorageFileExists(sourcePath))
    {
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_NOT_FOUND, 0, "");
        return false;
    }

    FILE* source = FT02_StorageOpenReadFile(sourcePath);
    if(source == nullptr)
    {
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_OPEN, 0, "");
        return false;
    }

    if(fseek(source, 0, SEEK_END) != 0)
    {
        fclose(source);
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_SEEK, 0, "");
        return false;
    }

    long fileLength = ftell(source);
    if(fileLength < 0 || fseek(source, 0, SEEK_SET) != 0)
    {
        fclose(source);
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_SEEK, 0, "");
        return false;
    }

    uint64_t sourceFileBytes = (uint64_t)fileLength;
    uint32_t sourceSignature = 0;

    if(!FT02_ComputeSourceSignature(source, sourceFileBytes, sourceSignature))
    {
        fclose(source);
        FT02_SetError(FT02_PBF_INDEX_ERROR_SOURCE_SIGNATURE, 0, "");
        return false;
    }

    fclose(source);
    g_report.sourceFileBytes = sourceFileBytes;
    g_report.sourceSignature = sourceSignature;

    Serial.println("[PBF-A1] source check");
    Serial.print("[PBF-A1] source_bytes=");
    Serial.print((unsigned long long)sourceFileBytes);
    Serial.print(" signature=0x");
    Serial.println(sourceSignature, HEX);

    if(!forceRebuild)
    {
        FT02PbfIndexLoadResult loadResult = FT02_TryLoadIndex(
            indexPath,
            sourceFileBytes,
            sourceSignature,
            startedAt
        );

        if(loadResult == FT02_PBF_LOAD_VALID)
        {
            return true;
        }

        if(loadResult == FT02_PBF_LOAD_INVALID)
        {
            Serial.println("[PBF-A1] existing index invalid; rebuilding");
            FT02_StorageDeleteFile(indexPath);
        }
        else if(loadResult == FT02_PBF_LOAD_IO_ERROR)
        {
            Serial.println("[PBF-A1] existing index unreadable; rebuilding");
            FT02_StorageDeleteFile(indexPath);
        }
        else
        {
            Serial.println("[PBF-A1] index missing; first build required");
        }
    }
    else
    {
        Serial.println("[PBF-A1] force rebuild requested");
    }

    g_report.state = FT02_PBF_INDEX_BUILDING;
    return FT02_BuildIndex(
        sourcePath,
        indexPath,
        sourceFileBytes,
        sourceSignature,
        startedAt
    );
}

const FT02PbfIndexReport& FT02_PbfIndexReportCurrent()
{
    return g_report;
}

const char* FT02_PbfIndexStateText()
{
    switch(g_report.state)
    {
        case FT02_PBF_INDEX_NOT_STARTED: return "NOT STARTED";
        case FT02_PBF_INDEX_CHECKING: return "CHECKING";
        case FT02_PBF_INDEX_BUILDING: return "BUILDING";
        case FT02_PBF_INDEX_READY: return "READY";
        case FT02_PBF_INDEX_ERROR: return "ERROR";
    }

    return "UNKNOWN";
}

const char* FT02_PbfIndexSourceText()
{
    if(g_report.state != FT02_PBF_INDEX_READY)
    {
        return "-";
    }

    return g_report.loadedFromIndex ? "CACHED INDEX" : "NEW BUILD";
}

const char* FT02_PbfIndexErrorText()
{
    switch(g_report.error)
    {
        case FT02_PBF_INDEX_ERROR_NONE: return "NONE";
        case FT02_PBF_INDEX_ERROR_STORAGE_NOT_READY: return "STORAGE NOT READY";
        case FT02_PBF_INDEX_ERROR_SOURCE_NOT_FOUND: return "PBF FILE NOT FOUND";
        case FT02_PBF_INDEX_ERROR_SOURCE_OPEN: return "PBF OPEN FAILED";
        case FT02_PBF_INDEX_ERROR_SOURCE_SEEK: return "PBF SEEK FAILED";
        case FT02_PBF_INDEX_ERROR_SOURCE_SIGNATURE: return "SOURCE SIGNATURE FAILED";
        case FT02_PBF_INDEX_ERROR_TRUNCATED: return "PBF TRUNCATED";
        case FT02_PBF_INDEX_ERROR_HEADER_TOO_LARGE: return "HEADER TOO LARGE";
        case FT02_PBF_INDEX_ERROR_BLOB_TOO_LARGE: return "BLOB TOO LARGE";
        case FT02_PBF_INDEX_ERROR_RAW_BLOCK_TOO_LARGE: return "RAW BLOCK TOO LARGE";
        case FT02_PBF_INDEX_ERROR_PROTOBUF: return "PROTOBUF PARSE FAILED";
        case FT02_PBF_INDEX_ERROR_UNSUPPORTED_COMPRESSION: return "COMPRESSION UNSUPPORTED";
        case FT02_PBF_INDEX_ERROR_OUT_OF_MEMORY: return "OUT OF MEMORY";
        case FT02_PBF_INDEX_ERROR_PSRAM_UNAVAILABLE: return "PSRAM UNAVAILABLE";
        case FT02_PBF_INDEX_ERROR_DECOMPRESSION: return "ZLIB DECOMPRESS FAILED";
        case FT02_PBF_INDEX_ERROR_TEMP_OPEN: return "INDEX TEMP OPEN FAILED";
        case FT02_PBF_INDEX_ERROR_TEMP_WRITE: return "INDEX WRITE FAILED";
        case FT02_PBF_INDEX_ERROR_TEMP_SYNC: return "INDEX SYNC FAILED";
        case FT02_PBF_INDEX_ERROR_COMMIT: return "INDEX COMMIT FAILED";
        case FT02_PBF_INDEX_ERROR_INDEX_READ: return "INDEX READ FAILED";
        case FT02_PBF_INDEX_ERROR_INDEX_INVALID: return "INDEX INVALID";
    }

    return "UNKNOWN";
}
