#include "FT02_PbfMapRuntime.h"

#include "FT02_PbfIndex.h"
#include "FT02_Storage.h"

#include <esp_heap_caps.h>
#include <errno.h>
#include <miniz.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FT-02 Direct PBF Map A3.13
//
// A3 keeps the untouched .osm.pbf as the source of truth, but builds a
// device-side persistent regional geometry cache. The first view in a region
// performs the full raw-PBF scan. Later launches, pans and zooms inside that
// region load a compact cache from SD and only re-project local geometry.

static const uint32_t FT02_PBF_MAP_INDEX_VERSION = 1U;
static const uint32_t FT02_PBF_MAP_MAX_BLOB_BYTES = 4U * 1024U * 1024U;
static const uint32_t FT02_PBF_MAP_MAX_RAW_BYTES = 4U * 1024U * 1024U;
static const uint32_t FT02_PBF_MAP_NODE_CAPACITY = 131072U;
static const uint32_t FT02_PBF_MAP_NODE_MAX_LOAD = 100000U;
static const uint32_t FT02_PBF_MAP_SEGMENT_CAPACITY = 70000U;
static const uint32_t FT02_PBF_CACHE_GEO_SEGMENT_CAPACITY = 90000U;
static const uint32_t FT02_PBF_CACHE_GEO_LABEL_CAPACITY = 1024U;
static const uint32_t FT02_PBF_CACHE_REGION_PIXELS_Z18 = 3072U;
static const uint32_t FT02_PBF_CACHE_REGION_MARGIN_PIXELS_Z18 = 192U;
static const uint32_t FT02_PBF_CACHE_VERSION = 5U;
static const char* FT02_PBF_CACHE_PATH = "/maps/raw/shanghai-260726.osm.pbc5";
static const int FT02_PBF_MAP_WIDTH = 800;
static const int FT02_PBF_MAP_HEIGHT = 364;
static const int FT02_PBF_MAP_MARGIN_PIXELS = 160;
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

struct FT02PbfCacheHeaderDisk
{
    char magic[8];
    uint16_t version;
    uint16_t headerBytes;
    uint16_t segmentBytes;
    uint16_t labelBytes;
    uint64_t sourceFileBytes;
    uint32_t sourceSignature;
    uint32_t segmentCount;
    uint32_t labelCount;
    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;
    int32_t centerLatE7;
    int32_t centerLonE7;
    uint32_t buildElapsedMs;
    uint32_t payloadCrc32;
    uint32_t fileBytes;
    uint8_t reserved[56];
};

struct FT02PbfGeoSegmentDisk
{
    int32_t lat1E7;
    int32_t lon1E7;
    int32_t lat2E7;
    int32_t lon2E7;
    uint8_t style;
    uint8_t reserved[3];
};

struct FT02PbfGeoLabelDisk
{
    int32_t latE7;
    int32_t lonE7;
    uint8_t priority;
    uint8_t textBytes;
    char text[FT02_PBF_MAP_LABEL_TEXT_BYTES];
    uint8_t reserved[6];
};
#pragma pack(pop)

static_assert(sizeof(FT02PbfIndexHeaderDisk) == 128, "PBI header size mismatch");
static_assert(sizeof(FT02PbfIndexEntryDisk) == 64, "PBI entry size mismatch");
static_assert(sizeof(FT02PbfCacheHeaderDisk) == 128, "PBC5 header size mismatch");
static_assert(sizeof(FT02PbfGeoSegmentDisk) == 20, "PBC5 segment size mismatch");
static_assert(sizeof(FT02PbfGeoLabelDisk) == 64, "PBC5 label size mismatch");

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

struct FT02PbfBlobView
{
    uint32_t rawSize;
    const uint8_t* rawData;
    size_t rawLength;
    const uint8_t* zlibData;
    size_t zlibLength;
    bool unsupportedCompression;
};

struct FT02NodeEntry
{
    int64_t id;
    int32_t latE7;
    int32_t lonE7;
};

struct FT02StringTable
{
    const uint8_t** data;
    uint16_t* lengths;
    uint32_t count;
};

struct FT02ViewBounds
{
    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;
};

static FT02PbfMapReport g_report;
static FT02PbfIndexEntryDisk* g_indexEntries = nullptr;
static uint32_t g_indexEntryCount = 0;
static uint64_t g_sourceFileBytes = 0;
static uint32_t g_sourceSignature = 0;
static FT02NodeEntry* g_nodeTable = nullptr;
static FT02PbfMapSegment* g_segments = nullptr;
static size_t g_segmentCount = 0;
static FT02PbfMapLabel* g_labels = nullptr;
static size_t g_labelCount = 0;
static FT02PbfGeoSegmentDisk* g_geoSegments = nullptr;
static size_t g_geoSegmentCount = 0;
static FT02PbfGeoLabelDisk* g_geoLabels = nullptr;
static size_t g_geoLabelCount = 0;
static FT02PbfCacheHeaderDisk g_cacheHeader;
static bool g_cacheResident = false;
static bool g_buildingCache = false;
static bool g_forceCacheRebuild = false;
static FT02ViewBounds g_cacheBuildBounds;
static double g_centerLon = FT02_PBF_MAP_DEFAULT_LON;
static double g_centerLat = FT02_PBF_MAP_DEFAULT_LAT;
static int g_zoom = FT02_PBF_MAP_DEFAULT_ZOOM;
static double g_centerWorldX = 0.0;
static double g_centerWorldY = 0.0;

// Forward declarations used by the node pass. The definitions live beside
// the shared string-table and label helpers below.
static bool FT02_BuildStringTable(const uint8_t* data, size_t size, FT02StringTable& table);
static void FT02_FreeStringTable(FT02StringTable& table);
static bool FT02_ReadPackedToArray(
    const FT02PbSlice* slices,
    size_t sliceCount,
    uint32_t* output,
    size_t capacity,
    size_t& count
);
static void FT02_AddNamedPoiLabel(
    const FT02StringTable& strings,
    const uint32_t* keys,
    const uint32_t* values,
    size_t tagCount,
    int32_t latE7,
    int32_t lonE7
);

static bool FT02_PbReadVarint(FT02PbCursor& cursor, uint64_t& value)
{
    value = 0;
    uint32_t shift = 0;
    while(cursor.offset < cursor.size && shift <= 63)
    {
        uint8_t byte = cursor.data[cursor.offset++];
        value |= (uint64_t)(byte & 0x7FU) << shift;
        if((byte & 0x80U) == 0) return true;
        shift += 7;
    }
    return false;
}

static int64_t FT02_ZigZag64(uint64_t value)
{
    return (int64_t)((value >> 1) ^ (uint64_t)-(int64_t)(value & 1U));
}

static bool FT02_PbReadKey(FT02PbCursor& cursor, uint32_t& field, uint8_t& wire)
{
    uint64_t key = 0;
    if(!FT02_PbReadVarint(cursor, key)) return false;
    field = (uint32_t)(key >> 3);
    wire = (uint8_t)(key & 7U);
    return field != 0;
}

static bool FT02_PbReadBytes(FT02PbCursor& cursor, const uint8_t*& data, size_t& size)
{
    uint64_t length = 0;
    if(!FT02_PbReadVarint(cursor, length)) return false;
    if(length > (uint64_t)(cursor.size - cursor.offset)) return false;
    data = cursor.data + cursor.offset;
    size = (size_t)length;
    cursor.offset += size;
    return true;
}

static bool FT02_PbSkip(FT02PbCursor& cursor, uint8_t wire)
{
    if(wire == 0)
    {
        uint64_t ignored = 0;
        return FT02_PbReadVarint(cursor, ignored);
    }
    if(wire == 1)
    {
        if(cursor.size - cursor.offset < 8) return false;
        cursor.offset += 8;
        return true;
    }
    if(wire == 2)
    {
        const uint8_t* ignored = nullptr;
        size_t length = 0;
        return FT02_PbReadBytes(cursor, ignored, length);
    }
    if(wire == 5)
    {
        if(cursor.size - cursor.offset < 4) return false;
        cursor.offset += 4;
        return true;
    }
    return false;
}

static void FT02_PackedBegin(FT02PackedIterator& iterator, const FT02PbSlice* slices, size_t count)
{
    iterator.slices = slices;
    iterator.sliceCount = count;
    iterator.sliceIndex = 0;
    iterator.cursor = {nullptr, 0, 0};
    if(count > 0) iterator.cursor = {slices[0].data, slices[0].size, 0};
}

static bool FT02_PackedNext(FT02PackedIterator& iterator, uint64_t& value, bool& hasValue)
{
    hasValue = false;
    while(iterator.sliceIndex < iterator.sliceCount)
    {
        if(iterator.cursor.offset < iterator.cursor.size)
        {
            if(!FT02_PbReadVarint(iterator.cursor, value)) return false;
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

static void* FT02_MapAlloc(size_t bytes, bool internalPreferred = false)
{
    if(bytes == 0) return nullptr;
    if(internalPreferred || bytes <= 32768U)
    {
        void* result = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if(result != nullptr) return result;
    }
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void FT02_MapFree(void* memory)
{
    if(memory != nullptr) heap_caps_free(memory);
}

static bool FT02_ReadExact(FILE* file, void* output, size_t bytes)
{
    return bytes == 0 || (file != nullptr && fread(output, 1, bytes, file) == bytes);
}

static bool FT02_WriteExact(FILE* file, const void* input, size_t bytes)
{
    return bytes == 0 || (file != nullptr && fwrite(input, 1, bytes, file) == bytes);
}

// Cache geometry lives in PSRAM. Passing a large PSRAM pointer directly into
// The storage VFS must not consume large PSRAM payloads directly. Stage every
// cache write through a DMA-capable internal-RAM buffer so the fixed SPI40
// multi-block transport receives bounded, internal-memory chunks.
static bool FT02_WriteDmaStaged(
    FILE* file,
    const void* input,
    size_t bytes,
    const char* section,
    size_t& totalWritten
)
{
    if(bytes == 0) return true;
    if(file == nullptr || input == nullptr) return false;

    static const size_t FT02_CACHE_WRITE_CHUNK = 8U * 1024U;
    uint8_t* stage = (uint8_t*)heap_caps_aligned_alloc(
        16,
        FT02_CACHE_WRITE_CHUNK,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT
    );
    if(stage == nullptr)
    {
        Serial.printf(
            "[PBF-A3.13] cache writer DMA buffer allocation failed heap=%lu\n",
            (unsigned long)ESP.getFreeHeap()
        );
        return false;
    }

    const uint8_t* source = (const uint8_t*)input;
    size_t offset = 0;
    size_t nextReport = 64U * 1024U;
    bool ok = true;

    while(offset < bytes)
    {
        const size_t remaining = bytes - offset;
        const size_t chunk = remaining < FT02_CACHE_WRITE_CHUNK
            ? remaining
            : FT02_CACHE_WRITE_CHUNK;

        memcpy(stage, source + offset, chunk);
        const size_t written = fwrite(stage, 1, chunk, file);
        if(written != chunk)
        {
            Serial.printf(
                "[PBF-A3.13] cache write failed section=%s offset=%lu requested=%lu written=%lu errno=%d ferror=%d\n",
                section != nullptr ? section : "?",
                (unsigned long)offset,
                (unsigned long)chunk,
                (unsigned long)written,
                errno,
                ferror(file)
            );
            ok = false;
            break;
        }

        offset += chunk;
        totalWritten += chunk;

        if(offset >= nextReport || offset == bytes)
        {
            Serial.printf(
                "[PBF-A3.13] cache write %s %lu/%lu bytes total=%lu\n",
                section != nullptr ? section : "?",
                (unsigned long)offset,
                (unsigned long)bytes,
                (unsigned long)totalWritten
            );
            nextReport += 64U * 1024U;
        }

        delay(1);
    }

    heap_caps_free(stage);
    return ok;
}

static uint32_t FT02_Crc32Update(uint32_t crc, const uint8_t* data, size_t size)
{
    crc = ~crc;
    for(size_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int32_t)(crc & 1U));
    }
    return ~crc;
}

static bool FT02_FileExists(const char* path)
{
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;
    fclose(file);
    return true;
}

static bool FT02_ParseBlob(const uint8_t* data, size_t size, FT02PbfBlobView& blob)
{
    memset(&blob, 0, sizeof(blob));
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0;
        uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if(field == 1 && wire == 2)
        {
            if(!FT02_PbReadBytes(cursor, blob.rawData, blob.rawLength)) return false;
        }
        else if(field == 2 && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(cursor, value) || value > UINT32_MAX) return false;
            blob.rawSize = (uint32_t)value;
        }
        else if(field == 3 && wire == 2)
        {
            if(!FT02_PbReadBytes(cursor, blob.zlibData, blob.zlibLength)) return false;
        }
        else if((field == 4 || field == 5 || field == 6 || field == 7) && wire == 2)
        {
            blob.unsupportedCompression = true;
            if(!FT02_PbSkip(cursor, wire)) return false;
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    if(blob.rawData != nullptr) blob.rawSize = (uint32_t)blob.rawLength;
    return blob.rawData != nullptr || blob.zlibData != nullptr;
}

static bool FT02_Inflate(const uint8_t* input, size_t inputLength, uint8_t* output, size_t capacity, size_t& outputLength)
{
    outputLength = 0;
    tinfl_decompressor* decompressor = (tinfl_decompressor*)FT02_MapAlloc(sizeof(tinfl_decompressor), true);
    if(decompressor == nullptr) return false;
    tinfl_init(decompressor);
    size_t inSize = inputLength;
    size_t outSize = capacity;
    tinfl_status status = tinfl_decompress(
        decompressor,
        input,
        &inSize,
        output,
        output,
        &outSize,
        TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
    );
    FT02_MapFree(decompressor);
    outputLength = outSize;
    return status == TINFL_STATUS_DONE
        && outputLength == capacity
        && inSize <= inputLength;
}

static bool FT02_LoadRawBlock(FILE* source, const FT02PbfIndexEntryDisk& entry, uint8_t*& raw, size_t& rawLength)
{
    raw = nullptr;
    rawLength = 0;
    if(entry.blobBytes == 0 || entry.blobBytes > FT02_PBF_MAP_MAX_BLOB_BYTES) return false;
    if(entry.rawBytes == 0 || entry.rawBytes > FT02_PBF_MAP_MAX_RAW_BYTES) return false;
    if(fseek(source, (long)entry.fileOffset, SEEK_SET) != 0) return false;

    uint8_t prefix[4];
    if(!FT02_ReadExact(source, prefix, sizeof(prefix))) return false;
    uint32_t headerLength = ((uint32_t)prefix[0] << 24) | ((uint32_t)prefix[1] << 16) |
        ((uint32_t)prefix[2] << 8) | (uint32_t)prefix[3];
    if(headerLength != entry.blobHeaderBytes) return false;
    if(fseek(source, (long)headerLength, SEEK_CUR) != 0) return false;

    uint8_t* blobBytes = (uint8_t*)FT02_MapAlloc(entry.blobBytes);
    if(blobBytes == nullptr) return false;
    if(!FT02_ReadExact(source, blobBytes, entry.blobBytes))
    {
        FT02_MapFree(blobBytes);
        return false;
    }

    FT02PbfBlobView blob;
    if(!FT02_ParseBlob(blobBytes, entry.blobBytes, blob) || blob.unsupportedCompression)
    {
        FT02_MapFree(blobBytes);
        return false;
    }

    if(blob.rawData != nullptr)
    {
        raw = (uint8_t*)FT02_MapAlloc(blob.rawLength);
        if(raw == nullptr)
        {
            FT02_MapFree(blobBytes);
            return false;
        }
        memcpy(raw, blob.rawData, blob.rawLength);
        rawLength = blob.rawLength;
        FT02_MapFree(blobBytes);
        return true;
    }

    raw = (uint8_t*)FT02_MapAlloc(blob.rawSize);
    if(raw == nullptr)
    {
        FT02_MapFree(blobBytes);
        return false;
    }
    size_t inflated = 0;
    bool ok = FT02_Inflate(blob.zlibData, blob.zlibLength, raw, blob.rawSize, inflated);
    FT02_MapFree(blobBytes);
    if(!ok || inflated != blob.rawSize)
    {
        FT02_MapFree(raw);
        raw = nullptr;
        return false;
    }
    rawLength = inflated;
    return true;
}

static double FT02_ClampLatitude(double latitude)
{
    if(latitude > 85.05112878) return 85.05112878;
    if(latitude < -85.05112878) return -85.05112878;
    return latitude;
}

static double FT02_WorldSize(int zoom)
{
    return 256.0 * (double)(1UL << zoom);
}

static double FT02_LonToWorldX(double lon, int zoom)
{
    return (lon + 180.0) / 360.0 * FT02_WorldSize(zoom);
}

static double FT02_LatToWorldY(double lat, int zoom)
{
    lat = FT02_ClampLatitude(lat);
    double rad = lat * M_PI / 180.0;
    return (1.0 - log(tan(rad) + 1.0 / cos(rad)) / M_PI) * 0.5 * FT02_WorldSize(zoom);
}

static double FT02_WorldXToLon(double x, int zoom)
{
    return x / FT02_WorldSize(zoom) * 360.0 - 180.0;
}

static double FT02_WorldYToLat(double y, int zoom)
{
    double n = M_PI - 2.0 * M_PI * y / FT02_WorldSize(zoom);
    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

static FT02ViewBounds FT02_CurrentViewBounds()
{
    // The regional cache was designed around Z18. At the new Z16 overview
    // level, adding the normal 160 px prefetch margin would make the requested
    // geographic window larger than the validated cache region. Use the
    // visible viewport only at Z16; Z18-Z20 retain the original margin.
    const int marginPixels = g_zoom <= 16 ? 0 : FT02_PBF_MAP_MARGIN_PIXELS;
    double halfW = FT02_PBF_MAP_WIDTH * 0.5 + marginPixels;
    double halfH = FT02_PBF_MAP_HEIGHT * 0.5 + marginPixels;
    double minLon = FT02_WorldXToLon(g_centerWorldX - halfW, g_zoom);
    double maxLon = FT02_WorldXToLon(g_centerWorldX + halfW, g_zoom);
    double maxLat = FT02_WorldYToLat(g_centerWorldY - halfH, g_zoom);
    double minLat = FT02_WorldYToLat(g_centerWorldY + halfH, g_zoom);
    FT02ViewBounds bounds;
    bounds.minLatE7 = (int32_t)llround(minLat * 10000000.0);
    bounds.minLonE7 = (int32_t)llround(minLon * 10000000.0);
    bounds.maxLatE7 = (int32_t)llround(maxLat * 10000000.0);
    bounds.maxLonE7 = (int32_t)llround(maxLon * 10000000.0);
    return bounds;
}

static FT02ViewBounds FT02_CacheRegionBoundsForCenter(double centerLon, double centerLat)
{
    const int baseZoom = 18;
    double cx = FT02_LonToWorldX(centerLon, baseZoom);
    double cy = FT02_LatToWorldY(centerLat, baseZoom);
    double half = FT02_PBF_CACHE_REGION_PIXELS_Z18 * 0.5 + FT02_PBF_CACHE_REGION_MARGIN_PIXELS_Z18;
    FT02ViewBounds bounds;
    bounds.minLonE7 = (int32_t)llround(FT02_WorldXToLon(cx - half, baseZoom) * 10000000.0);
    bounds.maxLonE7 = (int32_t)llround(FT02_WorldXToLon(cx + half, baseZoom) * 10000000.0);
    bounds.maxLatE7 = (int32_t)llround(FT02_WorldYToLat(cy - half, baseZoom) * 10000000.0);
    bounds.minLatE7 = (int32_t)llround(FT02_WorldYToLat(cy + half, baseZoom) * 10000000.0);
    return bounds;
}

static bool FT02_ViewInsideCache(const FT02ViewBounds& view)
{
    if(!g_cacheResident) return false;
    // Z16 almost fills the validated Z18 regional cache. Do not consume the
    // remaining edge allowance with the normal 2.5% safety pad at this level.
    const int32_t padLat = g_zoom <= 16
        ? 0
        : (g_cacheHeader.maxLatE7 - g_cacheHeader.minLatE7) / 40;
    const int32_t padLon = g_zoom <= 16
        ? 0
        : (g_cacheHeader.maxLonE7 - g_cacheHeader.minLonE7) / 40;
    return view.minLatE7 >= g_cacheHeader.minLatE7 + padLat &&
        view.maxLatE7 <= g_cacheHeader.maxLatE7 - padLat &&
        view.minLonE7 >= g_cacheHeader.minLonE7 + padLon &&
        view.maxLonE7 <= g_cacheHeader.maxLonE7 - padLon;
}

static bool FT02_BoundsIntersect(const FT02ViewBounds& view, const FT02PbfIndexEntryDisk& entry)
{
    if((entry.flags & FT02_PBF_ENTRY_FLAG_NODE_BOUNDS) == 0) return false;
    if(entry.maxLatE7 < view.minLatE7 || entry.minLatE7 > view.maxLatE7) return false;
    if(entry.maxLonE7 < view.minLonE7 || entry.minLonE7 > view.maxLonE7) return false;
    return true;
}

static uint32_t FT02_HashNodeId(int64_t id)
{
    uint64_t x = (uint64_t)id;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)x;
}

static bool FT02_NodeInsert(int64_t id, int32_t latE7, int32_t lonE7)
{
    if(id == 0) return true;
    if(g_report.nodesKept >= FT02_PBF_MAP_NODE_MAX_LOAD) return false;
    uint32_t slot = FT02_HashNodeId(id) & (FT02_PBF_MAP_NODE_CAPACITY - 1U);
    for(uint32_t probe = 0; probe < FT02_PBF_MAP_NODE_CAPACITY; probe++)
    {
        FT02NodeEntry& entry = g_nodeTable[slot];
        if(entry.id == 0)
        {
            entry.id = id;
            entry.latE7 = latE7;
            entry.lonE7 = lonE7;
            g_report.nodesKept++;
            return true;
        }
        if(entry.id == id)
        {
            entry.latE7 = latE7;
            entry.lonE7 = lonE7;
            return true;
        }
        slot = (slot + 1U) & (FT02_PBF_MAP_NODE_CAPACITY - 1U);
    }
    return false;
}

static bool FT02_NodeFind(int64_t id, int32_t& latE7, int32_t& lonE7)
{
    if(id == 0) return false;
    uint32_t slot = FT02_HashNodeId(id) & (FT02_PBF_MAP_NODE_CAPACITY - 1U);
    for(uint32_t probe = 0; probe < FT02_PBF_MAP_NODE_CAPACITY; probe++)
    {
        const FT02NodeEntry& entry = g_nodeTable[slot];
        if(entry.id == id) { latE7 = entry.latE7; lonE7 = entry.lonE7; return true; }
        if(entry.id == 0) return false;
        slot = (slot + 1U) & (FT02_PBF_MAP_NODE_CAPACITY - 1U);
    }
    return false;
}

static bool FT02_PointInside(const FT02ViewBounds& bounds, int32_t latE7, int32_t lonE7)
{
    return latE7 >= bounds.minLatE7 && latE7 <= bounds.maxLatE7 &&
        lonE7 >= bounds.minLonE7 && lonE7 <= bounds.maxLonE7;
}

static int32_t FT02_NanodegreesToE7(int64_t value)
{
    return (int32_t)(value / 100LL);
}

static bool FT02_ParseRegularNodeForMap(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    const FT02ViewBounds& bounds,
    const FT02StringTable& strings
)
{
    static const size_t MAX_SLICES = 8;
    static const size_t MAX_TAGS = 32;
    FT02PbSlice keySlices[MAX_SLICES], valueSlices[MAX_SLICES];
    size_t keySliceCount = 0, valueSliceCount = 0;
    FT02PbCursor cursor = {data, size, 0};
    int64_t id = 0, lat = 0, lon = 0;
    bool hasId = false, hasLat = false, hasLon = false;
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if((field == 1 || field == 8 || field == 9) && wire == 0)
        {
            uint64_t encoded = 0;
            if(!FT02_PbReadVarint(cursor, encoded)) return false;
            int64_t value = FT02_ZigZag64(encoded);
            if(field == 1) { id = value; hasId = true; }
            if(field == 8) { lat = value; hasLat = true; }
            if(field == 9) { lon = value; hasLon = true; }
        }
        else if((field == 2 || field == 3) && wire == 2)
        {
            const uint8_t* packed = nullptr; size_t packedSize = 0;
            if(!FT02_PbReadBytes(cursor, packed, packedSize)) return false;
            FT02PbSlice* target = field == 2 ? keySlices : valueSlices;
            size_t* count = field == 2 ? &keySliceCount : &valueSliceCount;
            if(*count >= MAX_SLICES) return false;
            target[(*count)++] = {packed, packedSize};
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    if(hasId && hasLat && hasLon)
    {
        int32_t latE7 = FT02_NanodegreesToE7(latOffset + granularity * lat);
        int32_t lonE7 = FT02_NanodegreesToE7(lonOffset + granularity * lon);
        if(FT02_PointInside(bounds, latE7, lonE7))
        {
            if(!FT02_NodeInsert(id, latE7, lonE7)) return false;
            if(g_buildingCache && (keySliceCount > 0 || valueSliceCount > 0))
            {
                uint32_t keys[MAX_TAGS], values[MAX_TAGS];
                size_t keyCount = 0, valueCount = 0;
                if(!FT02_ReadPackedToArray(keySlices, keySliceCount, keys, MAX_TAGS, keyCount)) return false;
                if(!FT02_ReadPackedToArray(valueSlices, valueSliceCount, values, MAX_TAGS, valueCount)) return false;
                size_t tagCount = keyCount < valueCount ? keyCount : valueCount;
                FT02_AddNamedPoiLabel(strings, keys, values, tagCount, latE7, lonE7);
            }
        }
    }
    return true;
}

static bool FT02_ParseDenseNodesForMap(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    const FT02ViewBounds& bounds,
    const FT02StringTable& strings
)
{
    static const size_t MAX_SLICES = 8;
    static const size_t MAX_TAGS = 32;
    FT02PbSlice idSlices[MAX_SLICES], latSlices[MAX_SLICES], lonSlices[MAX_SLICES], keyValueSlices[MAX_SLICES];
    size_t idCount = 0, latCount = 0, lonCount = 0, keyValueCount = 0;
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if((field == 1 || field == 8 || field == 9 || field == 10) && wire == 2)
        {
            const uint8_t* packed = nullptr; size_t packedSize = 0;
            if(!FT02_PbReadBytes(cursor, packed, packedSize)) return false;
            FT02PbSlice* target = nullptr; size_t* count = nullptr;
            if(field == 1) { target = idSlices; count = &idCount; }
            if(field == 8) { target = latSlices; count = &latCount; }
            if(field == 9) { target = lonSlices; count = &lonCount; }
            if(field == 10) { target = keyValueSlices; count = &keyValueCount; }
            if(target == nullptr || count == nullptr || *count >= MAX_SLICES) return false;
            target[(*count)++] = {packed, packedSize};
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }

    FT02PackedIterator ids, lats, lons, keyValues;
    FT02_PackedBegin(ids, idSlices, idCount);
    FT02_PackedBegin(lats, latSlices, latCount);
    FT02_PackedBegin(lons, lonSlices, lonCount);
    FT02_PackedBegin(keyValues, keyValueSlices, keyValueCount);
    int64_t id = 0, lat = 0, lon = 0;
    while(true)
    {
        uint64_t encodedId = 0, encodedLat = 0, encodedLon = 0;
        bool hasId = false, hasLat = false, hasLon = false;
        if(!FT02_PackedNext(ids, encodedId, hasId)) return false;
        if(!FT02_PackedNext(lats, encodedLat, hasLat)) return false;
        if(!FT02_PackedNext(lons, encodedLon, hasLon)) return false;
        if(hasId != hasLat || hasId != hasLon) return false;
        if(!hasId) break;
        id += FT02_ZigZag64(encodedId);
        lat += FT02_ZigZag64(encodedLat);
        lon += FT02_ZigZag64(encodedLon);
        int32_t latE7 = FT02_NanodegreesToE7(latOffset + granularity * lat);
        int32_t lonE7 = FT02_NanodegreesToE7(lonOffset + granularity * lon);

        uint32_t keys[MAX_TAGS], values[MAX_TAGS];
        size_t tagCount = 0;
        if(keyValueCount > 0)
        {
            while(true)
            {
                uint64_t key = 0; bool hasKey = false;
                if(!FT02_PackedNext(keyValues, key, hasKey)) return false;
                if(!hasKey) break;
                if(key == 0) break;
                uint64_t value = 0; bool hasValue = false;
                if(!FT02_PackedNext(keyValues, value, hasValue) || !hasValue) return false;
                if(tagCount < MAX_TAGS)
                {
                    keys[tagCount] = (uint32_t)key;
                    values[tagCount] = (uint32_t)value;
                    tagCount++;
                }
            }
        }

        if(FT02_PointInside(bounds, latE7, lonE7))
        {
            if(!FT02_NodeInsert(id, latE7, lonE7)) return false;
            if(g_buildingCache && tagCount > 0)
                FT02_AddNamedPoiLabel(strings, keys, values, tagCount, latE7, lonE7);
        }
    }
    return true;
}

static bool FT02_ParseNodeGroup(
    const uint8_t* data,
    size_t size,
    int64_t granularity,
    int64_t latOffset,
    int64_t lonOffset,
    const FT02ViewBounds& bounds,
    const FT02StringTable& strings
)
{
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if((field == 1 || field == 2) && wire == 2)
        {
            const uint8_t* message = nullptr; size_t messageSize = 0;
            if(!FT02_PbReadBytes(cursor, message, messageSize)) return false;
            if(field == 1)
            {
                if(!FT02_ParseRegularNodeForMap(message, messageSize, granularity, latOffset, lonOffset, bounds, strings)) return false;
            }
            else
            {
                if(!FT02_ParseDenseNodesForMap(message, messageSize, granularity, latOffset, lonOffset, bounds, strings)) return false;
            }
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    return true;
}

static bool FT02_ParsePrimitiveMetadata(const uint8_t* data, size_t size, int64_t& granularity, int64_t& latOffset, int64_t& lonOffset)
{
    granularity = 100; latOffset = 0; lonOffset = 0;
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if((field == 17 || field == 19 || field == 20) && wire == 0)
        {
            uint64_t value = 0;
            if(!FT02_PbReadVarint(cursor, value)) return false;
            if(field == 17) granularity = (int64_t)value;
            if(field == 19) latOffset = (int64_t)value;
            if(field == 20) lonOffset = (int64_t)value;
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    return true;
}

static bool FT02_ParseNodeBlock(const uint8_t* data, size_t size, const FT02ViewBounds& bounds)
{
    const uint8_t* stringMessage = nullptr; size_t stringMessageSize = 0;
    FT02PbCursor first = {data, size, 0};
    while(first.offset < first.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(first, field, wire)) return false;
        if(field == 1 && wire == 2)
        {
            if(!FT02_PbReadBytes(first, stringMessage, stringMessageSize)) return false;
        }
        else if(!FT02_PbSkip(first, wire)) return false;
    }
    if(stringMessage == nullptr) return false;

    FT02StringTable strings;
    if(!FT02_BuildStringTable(stringMessage, stringMessageSize, strings))
    {
        FT02_FreeStringTable(strings);
        return false;
    }

    int64_t granularity = 100, latOffset = 0, lonOffset = 0;
    if(!FT02_ParsePrimitiveMetadata(data, size, granularity, latOffset, lonOffset))
    {
        FT02_FreeStringTable(strings);
        return false;
    }

    FT02PbCursor cursor = {data, size, 0};
    bool ok = true;
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) { ok = false; break; }
        if(field == 2 && wire == 2)
        {
            const uint8_t* group = nullptr; size_t groupSize = 0;
            if(!FT02_PbReadBytes(cursor, group, groupSize) ||
               !FT02_ParseNodeGroup(group, groupSize, granularity, latOffset, lonOffset, bounds, strings))
            { ok = false; break; }
        }
        else if(!FT02_PbSkip(cursor, wire)) { ok = false; break; }
    }
    FT02_FreeStringTable(strings);
    return ok;
}

static bool FT02_CountStringTable(const uint8_t* data, size_t size, uint32_t& count)
{
    count = 0;
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if(field == 1 && wire == 2)
        {
            const uint8_t* value = nullptr; size_t length = 0;
            if(!FT02_PbReadBytes(cursor, value, length)) return false;
            count++;
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    return true;
}

static bool FT02_BuildStringTable(const uint8_t* data, size_t size, FT02StringTable& table)
{
    memset(&table, 0, sizeof(table));
    if(!FT02_CountStringTable(data, size, table.count)) return false;
    if(table.count == 0) return true;
    table.data = (const uint8_t**)FT02_MapAlloc(sizeof(uint8_t*) * table.count);
    table.lengths = (uint16_t*)FT02_MapAlloc(sizeof(uint16_t) * table.count);
    if(table.data == nullptr || table.lengths == nullptr) return false;
    FT02PbCursor cursor = {data, size, 0};
    uint32_t index = 0;
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if(field == 1 && wire == 2)
        {
            const uint8_t* value = nullptr; size_t length = 0;
            if(!FT02_PbReadBytes(cursor, value, length) || index >= table.count || length > UINT16_MAX) return false;
            table.data[index] = value;
            table.lengths[index] = (uint16_t)length;
            index++;
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    return index == table.count;
}

static void FT02_FreeStringTable(FT02StringTable& table)
{
    FT02_MapFree((void*)table.data);
    FT02_MapFree(table.lengths);
    memset(&table, 0, sizeof(table));
}

static bool FT02_StringEquals(const FT02StringTable& table, uint32_t index, const char* literal)
{
    if(literal == nullptr || index >= table.count) return false;
    size_t length = strlen(literal);
    return table.lengths[index] == length && memcmp(table.data[index], literal, length) == 0;
}

static bool FT02_StringNotNo(const FT02StringTable& table, uint32_t index)
{
    return index < table.count && !FT02_StringEquals(table, index, "no");
}

static bool FT02_ReadPackedToArray(const FT02PbSlice* slices, size_t sliceCount, uint32_t* output, size_t capacity, size_t& count)
{
    count = 0;
    FT02PackedIterator iterator;
    FT02_PackedBegin(iterator, slices, sliceCount);
    while(true)
    {
        uint64_t value = 0; bool hasValue = false;
        if(!FT02_PackedNext(iterator, value, hasValue)) return false;
        if(!hasValue) break;
        if(count < capacity) output[count++] = (uint32_t)value;
    }
    return true;
}

static bool FT02_TagEquals(
    const FT02StringTable& table,
    const uint32_t* keys,
    const uint32_t* values,
    size_t count,
    const char* keyText,
    const char* valueText
)
{
    for(size_t i = 0; i < count; i++)
    {
        if(FT02_StringEquals(table, keys[i], keyText) &&
           FT02_StringEquals(table, values[i], valueText)) return true;
    }
    return false;
}

static bool FT02_HasTagKey(
    const FT02StringTable& table,
    const uint32_t* keys,
    size_t count,
    const char* keyText
)
{
    for(size_t i = 0; i < count; i++)
        if(FT02_StringEquals(table, keys[i], keyText)) return true;
    return false;
}

static uint8_t FT02_ClassifyWay(const FT02StringTable& table, const uint32_t* keys, const uint32_t* values, size_t count)
{
    uint8_t style = 0;
    for(size_t i = 0; i < count; i++)
    {
        uint32_t key = keys[i]; uint32_t value = values[i];
        if(FT02_StringEquals(table, key, "highway"))
        {
            if(FT02_StringEquals(table, value, "motorway") || FT02_StringEquals(table, value, "trunk") ||
               FT02_StringEquals(table, value, "primary") || FT02_StringEquals(table, value, "motorway_link") ||
               FT02_StringEquals(table, value, "trunk_link") || FT02_StringEquals(table, value, "primary_link"))
                return FT02_PBF_MAP_STYLE_MAJOR;
            if(FT02_StringEquals(table, value, "secondary") || FT02_StringEquals(table, value, "tertiary") ||
               FT02_StringEquals(table, value, "secondary_link") || FT02_StringEquals(table, value, "tertiary_link"))
                style = FT02_PBF_MAP_STYLE_MEDIUM;
            else if(FT02_StringEquals(table, value, "footway") || FT02_StringEquals(table, value, "path") ||
                    FT02_StringEquals(table, value, "pedestrian") || FT02_StringEquals(table, value, "steps") ||
                    FT02_StringEquals(table, value, "cycleway") || FT02_StringEquals(table, value, "track"))
                style = FT02_PBF_MAP_STYLE_PATH;
            else
                style = FT02_PBF_MAP_STYLE_MINOR;
        }
        else if(FT02_StringEquals(table, key, "building") && FT02_StringNotNo(table, value))
        {
            if(style == 0) style = FT02_PBF_MAP_STYLE_BUILDING;
        }
        else if(FT02_StringEquals(table, key, "railway"))
        {
            if(style == 0) style = FT02_PBF_MAP_STYLE_RAIL;
        }
        else if(FT02_StringEquals(table, key, "waterway"))
        {
            if(style == 0) style = FT02_PBF_MAP_STYLE_WATER;
        }
        else if(FT02_StringEquals(table, key, "natural") && FT02_StringEquals(table, value, "water"))
        {
            if(style == 0) style = FT02_PBF_MAP_STYLE_WATER;
        }
    }

    // Named POI areas are often mapped as closed ways without a building tag.
    // Keep their outline so hospitals, schools, parks, malls and stations can
    // receive a stable label anchor in the regional cache.
    if(style == 0 && (
        FT02_HasTagKey(table, keys, count, "amenity") ||
        FT02_HasTagKey(table, keys, count, "shop") ||
        FT02_HasTagKey(table, keys, count, "tourism") ||
        FT02_HasTagKey(table, keys, count, "leisure") ||
        FT02_HasTagKey(table, keys, count, "office") ||
        FT02_HasTagKey(table, keys, count, "public_transport") ||
        FT02_HasTagKey(table, keys, count, "place")))
        style = FT02_PBF_MAP_STYLE_BUILDING;

    return style;
}

static int FT02_FindTagValueIndex(
    const FT02StringTable& table,
    const uint32_t* keys,
    const uint32_t* values,
    size_t count,
    const char* wanted
)
{
    for(size_t i = 0; i < count; i++)
    {
        if(FT02_StringEquals(table, keys[i], wanted) && values[i] < table.count)
            return (int)values[i];
    }
    return -1;
}

static uint8_t FT02_LabelPriorityForTags(
    const FT02StringTable& table,
    const uint32_t* keys,
    const uint32_t* values,
    size_t count,
    uint8_t style
)
{
    if(style == FT02_PBF_MAP_STYLE_MAJOR) return 4;
    if(style == FT02_PBF_MAP_STYLE_MEDIUM) return 3;
    if(style == FT02_PBF_MAP_STYLE_MINOR) return 2;
    if(style == FT02_PBF_MAP_STYLE_PATH) return 1;

    if(FT02_TagEquals(table, keys, values, count, "railway", "station") ||
       FT02_TagEquals(table, keys, values, count, "railway", "subway_entrance") ||
       FT02_TagEquals(table, keys, values, count, "public_transport", "station") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "hospital") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "university") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "college") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "school") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "police") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "fire_station") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "townhall"))
        return 4;

    if(FT02_TagEquals(table, keys, values, count, "amenity", "clinic") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "library") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "theatre") ||
       FT02_TagEquals(table, keys, values, count, "amenity", "cinema") ||
       FT02_TagEquals(table, keys, values, count, "tourism", "museum") ||
       FT02_TagEquals(table, keys, values, count, "tourism", "attraction") ||
       FT02_TagEquals(table, keys, values, count, "tourism", "hotel") ||
       FT02_TagEquals(table, keys, values, count, "shop", "mall") ||
       FT02_TagEquals(table, keys, values, count, "shop", "department_store") ||
       FT02_TagEquals(table, keys, values, count, "shop", "supermarket") ||
       FT02_TagEquals(table, keys, values, count, "leisure", "park") ||
       FT02_TagEquals(table, keys, values, count, "leisure", "stadium") ||
       FT02_TagEquals(table, keys, values, count, "leisure", "sports_centre") ||
       FT02_TagEquals(table, keys, values, count, "office", "government"))
        return 3;

    if(style == FT02_PBF_MAP_STYLE_WATER || style == FT02_PBF_MAP_STYLE_RAIL) return 2;
    if(style == FT02_PBF_MAP_STYLE_BUILDING ||
       FT02_HasTagKey(table, keys, count, "amenity") ||
       FT02_HasTagKey(table, keys, count, "shop") ||
       FT02_HasTagKey(table, keys, count, "tourism") ||
       FT02_HasTagKey(table, keys, count, "leisure") ||
       FT02_HasTagKey(table, keys, count, "office") ||
       FT02_HasTagKey(table, keys, count, "public_transport") ||
       FT02_HasTagKey(table, keys, count, "place"))
        return 2;

    return 0;
}

static size_t FT02_Utf8SafeLength(const uint8_t* text, size_t bytes, size_t capacity)
{
    if(text == nullptr || capacity <= 1 || bytes == 0) return 0;
    if(bytes <= capacity - 1) return bytes;
    size_t limit = capacity - 1;
    while(limit > 0 && (text[limit] & 0xC0U) == 0x80U) limit--;
    if(limit == 0) return 0;
    uint8_t lead = text[limit];
    size_t needed = 1;
    if((lead & 0xE0U) == 0xC0U) needed = 2;
    else if((lead & 0xF0U) == 0xE0U) needed = 3;
    else if((lead & 0xF8U) == 0xF0U) needed = 4;
    if(limit + needed <= capacity - 1) return limit + needed;
    return limit;
}

static bool FT02_LabelTextEquals(const FT02PbfGeoLabelDisk& label, const uint8_t* text, size_t bytes)
{
    return label.textBytes == bytes && bytes > 0 && memcmp(label.text, text, bytes) == 0;
}

static void FT02_AddGeoLabel(
    int32_t latE7,
    int32_t lonE7,
    uint8_t priority,
    const uint8_t* text,
    size_t textBytes
)
{
    if(priority == 0 || text == nullptr || textBytes == 0 || g_geoLabels == nullptr) return;
    size_t safeBytes = FT02_Utf8SafeLength(text, textBytes, FT02_PBF_MAP_LABEL_TEXT_BYTES);
    if(safeBytes == 0) return;
    for(size_t i = 0; i < g_geoLabelCount; i++)
    {
        if(!FT02_LabelTextEquals(g_geoLabels[i], text, safeBytes)) continue;
        int64_t dLat = llabs((int64_t)g_geoLabels[i].latE7 - latE7);
        int64_t dLon = llabs((int64_t)g_geoLabels[i].lonE7 - lonE7);
        // Allow the same long road to carry more than one label, but avoid
        // repeated labels from adjacent OSM way fragments. Roughly 180 m.
        if(dLat + dLon < 18000) return;
    }
    if(g_geoLabelCount >= FT02_PBF_CACHE_GEO_LABEL_CAPACITY) return;
    FT02PbfGeoLabelDisk& label = g_geoLabels[g_geoLabelCount++];
    memset(&label, 0, sizeof(label));
    label.latE7 = latE7;
    label.lonE7 = lonE7;
    label.priority = priority;
    label.textBytes = (uint8_t)safeBytes;
    memcpy(label.text, text, safeBytes);
}

static void FT02_AddNamedPoiLabel(
    const FT02StringTable& strings,
    const uint32_t* keys,
    const uint32_t* values,
    size_t tagCount,
    int32_t latE7,
    int32_t lonE7
)
{
    int nameIndex = FT02_FindTagValueIndex(strings, keys, values, tagCount, "name:zh");
    if(nameIndex < 0) nameIndex = FT02_FindTagValueIndex(strings, keys, values, tagCount, "name");
    if(nameIndex < 0 || (uint32_t)nameIndex >= strings.count) return;
    uint8_t priority = FT02_LabelPriorityForTags(strings, keys, values, tagCount, 0);
    if(priority == 0) return;
    FT02_AddGeoLabel(
        latE7,
        lonE7,
        priority,
        strings.data[nameIndex],
        strings.lengths[nameIndex]
    );
}

static int16_t FT02_ProjectX(int32_t lonE7)
{
    double world = FT02_LonToWorldX(lonE7 / 10000000.0, g_zoom);
    return (int16_t)lround(world - g_centerWorldX + FT02_PBF_MAP_WIDTH * 0.5);
}

static int16_t FT02_ProjectY(int32_t latE7)
{
    double world = FT02_LatToWorldY(latE7 / 10000000.0, g_zoom);
    return (int16_t)lround(world - g_centerWorldY + FT02_PBF_MAP_HEIGHT * 0.5);
}

static bool FT02_SegmentMightBeVisible(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    const int margin = 8;
    if(x1 < -margin && x2 < -margin) return false;
    if(x1 > FT02_PBF_MAP_WIDTH + margin && x2 > FT02_PBF_MAP_WIDTH + margin) return false;
    if(y1 < -margin && y2 < -margin) return false;
    if(y1 > FT02_PBF_MAP_HEIGHT + margin && y2 > FT02_PBF_MAP_HEIGHT + margin) return false;
    return true;
}

static bool FT02_StyleVisibleAtZoom(uint8_t style)
{
    // Z16 is the widest overview and hides minor streets, footpaths and
    // buildings. Z17 restores minor streets but still hides paths/buildings,
    // creating a useful intermediate level before the existing Z18 detail.
    if(g_zoom <= 16)
    {
        if(style == FT02_PBF_MAP_STYLE_MINOR) return false;
        if(style == FT02_PBF_MAP_STYLE_PATH) return false;
        if(style == FT02_PBF_MAP_STYLE_BUILDING) return false;
    }
    if(style == FT02_PBF_MAP_STYLE_PATH && g_zoom < 19) return false;
    if(style == FT02_PBF_MAP_STYLE_BUILDING && g_zoom < 18) return false;
    return true;
}

static void FT02_AddSegment(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2, uint8_t style)
{
    if(g_buildingCache)
    {
        if(g_geoSegments == nullptr || g_geoSegmentCount >= FT02_PBF_CACHE_GEO_SEGMENT_CAPACITY)
        {
            g_report.segmentLimitReached = true;
            return;
        }
        FT02PbfGeoSegmentDisk& segment = g_geoSegments[g_geoSegmentCount++];
        segment.lat1E7 = lat1; segment.lon1E7 = lon1;
        segment.lat2E7 = lat2; segment.lon2E7 = lon2;
        segment.style = style;
        memset(segment.reserved, 0, sizeof(segment.reserved));
        return;
    }

    if(!FT02_StyleVisibleAtZoom(style)) return;
    int16_t x1 = FT02_ProjectX(lon1); int16_t y1 = FT02_ProjectY(lat1);
    int16_t x2 = FT02_ProjectX(lon2); int16_t y2 = FT02_ProjectY(lat2);
    if(!FT02_SegmentMightBeVisible(x1, y1, x2, y2)) return;
    if(g_segmentCount >= FT02_PBF_MAP_SEGMENT_CAPACITY)
    {
        g_report.segmentLimitReached = true;
        return;
    }
    FT02PbfMapSegment& segment = g_segments[g_segmentCount++];
    segment.x1 = x1; segment.y1 = y1; segment.x2 = x2; segment.y2 = y2; segment.style = style;
    g_report.segments = (uint32_t)g_segmentCount;
}

static bool FT02_ParseWay(
    const uint8_t* data,
    size_t size,
    const FT02StringTable& strings
)
{
    static const size_t MAX_SLICES = 8;
    static const size_t MAX_TAGS = 64;
    FT02PbSlice keySlices[MAX_SLICES], valueSlices[MAX_SLICES], refSlices[MAX_SLICES];
    size_t keySliceCount = 0, valueSliceCount = 0, refSliceCount = 0;
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if((field == 2 || field == 3 || field == 8) && wire == 2)
        {
            const uint8_t* packed = nullptr; size_t packedSize = 0;
            if(!FT02_PbReadBytes(cursor, packed, packedSize)) return false;
            FT02PbSlice* target = nullptr; size_t* count = nullptr;
            if(field == 2) { target = keySlices; count = &keySliceCount; }
            if(field == 3) { target = valueSlices; count = &valueSliceCount; }
            if(field == 8) { target = refSlices; count = &refSliceCount; }
            if(target == nullptr || count == nullptr || *count >= MAX_SLICES) return false;
            target[(*count)++] = {packed, packedSize};
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }

    g_report.waysScanned++;
    uint32_t keys[MAX_TAGS], values[MAX_TAGS];
    size_t keyCount = 0, valueCount = 0;
    if(!FT02_ReadPackedToArray(keySlices, keySliceCount, keys, MAX_TAGS, keyCount)) return false;
    if(!FT02_ReadPackedToArray(valueSlices, valueSliceCount, values, MAX_TAGS, valueCount)) return false;
    size_t tagCount = keyCount < valueCount ? keyCount : valueCount;
    uint8_t style = FT02_ClassifyWay(strings, keys, values, tagCount);
    int nameIndex = FT02_FindTagValueIndex(strings, keys, values, tagCount, "name:zh");
    if(nameIndex < 0) nameIndex = FT02_FindTagValueIndex(strings, keys, values, tagCount, "name");
    uint8_t labelPriority = FT02_LabelPriorityForTags(strings, keys, values, tagCount, style);
    if(style == 0) return true;

    FT02PackedIterator refs;
    FT02_PackedBegin(refs, refSlices, refSliceCount);
    int64_t nodeId = 0;
    bool previousResolved = false;
    int32_t previousLat = 0, previousLon = 0;
    bool accepted = false;
    int64_t longestScore = -1;
    int32_t labelLat = 0, labelLon = 0;
    while(true)
    {
        uint64_t encoded = 0; bool hasValue = false;
        if(!FT02_PackedNext(refs, encoded, hasValue)) return false;
        if(!hasValue) break;
        nodeId += FT02_ZigZag64(encoded);
        int32_t lat = 0, lon = 0;
        bool resolved = FT02_NodeFind(nodeId, lat, lon);
        if(resolved && previousResolved)
        {
            size_t before = g_buildingCache ? g_geoSegmentCount : g_segmentCount;
            FT02_AddSegment(previousLat, previousLon, lat, lon, style);
            size_t after = g_buildingCache ? g_geoSegmentCount : g_segmentCount;
            if(after > before)
            {
                accepted = true;
                int64_t dLat = (int64_t)lat - previousLat;
                int64_t dLon = (int64_t)lon - previousLon;
                int64_t score = llabs(dLat) + llabs(dLon);
                if(score > longestScore)
                {
                    longestScore = score;
                    labelLat = (int32_t)(((int64_t)lat + previousLat) / 2);
                    labelLon = (int32_t)(((int64_t)lon + previousLon) / 2);
                }
            }
        }
        if(!resolved) g_report.unresolvedNodeRefs++;
        previousResolved = resolved;
        if(resolved) { previousLat = lat; previousLon = lon; }
    }
    if(accepted)
    {
        g_report.waysAccepted++;
        if(g_buildingCache && nameIndex >= 0 && (uint32_t)nameIndex < strings.count)
        {
            FT02_AddGeoLabel(
                labelLat,
                labelLon,
                labelPriority,
                strings.data[nameIndex],
                strings.lengths[nameIndex]
            );
        }
    }
    return true;
}

static bool FT02_ParseWayGroup(const uint8_t* data, size_t size, const FT02StringTable& strings)
{
    FT02PbCursor cursor = {data, size, 0};
    while(cursor.offset < cursor.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(cursor, field, wire)) return false;
        if(field == 3 && wire == 2)
        {
            const uint8_t* way = nullptr; size_t waySize = 0;
            if(!FT02_PbReadBytes(cursor, way, waySize)) return false;
            if(!FT02_ParseWay(way, waySize, strings)) return false;
        }
        else if(!FT02_PbSkip(cursor, wire)) return false;
    }
    return true;
}

static bool FT02_ParseWayBlock(const uint8_t* data, size_t size)
{
    const uint8_t* stringMessage = nullptr; size_t stringMessageSize = 0;
    FT02PbCursor first = {data, size, 0};
    while(first.offset < first.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(first, field, wire)) return false;
        if(field == 1 && wire == 2)
        {
            if(!FT02_PbReadBytes(first, stringMessage, stringMessageSize)) return false;
        }
        else if(!FT02_PbSkip(first, wire)) return false;
    }
    if(stringMessage == nullptr) return false;
    FT02StringTable strings;
    if(!FT02_BuildStringTable(stringMessage, stringMessageSize, strings))
    {
        FT02_FreeStringTable(strings);
        return false;
    }
    FT02PbCursor second = {data, size, 0};
    bool ok = true;
    while(second.offset < second.size)
    {
        uint32_t field = 0; uint8_t wire = 0;
        if(!FT02_PbReadKey(second, field, wire)) { ok = false; break; }
        if(field == 2 && wire == 2)
        {
            const uint8_t* group = nullptr; size_t groupSize = 0;
            if(!FT02_PbReadBytes(second, group, groupSize) || !FT02_ParseWayGroup(group, groupSize, strings))
            { ok = false; break; }
        }
        else if(!FT02_PbSkip(second, wire)) { ok = false; break; }
    }
    FT02_FreeStringTable(strings);
    return ok;
}

static bool FT02_LoadIndex()
{
    // Index entries are temporary build-only data. Always start clean so a
    // failed read cannot leave stale pointers behind.
    FT02_MapFree(g_indexEntries);
    g_indexEntries = nullptr;
    g_indexEntryCount = 0;

    FILE* file = FT02_StorageOpenReadFile(FT02_PBF_INDEX_PATH);
    if(file == nullptr)
    {
        Serial.println("[PBF-A3.13] index open failed");
        return false;
    }

    FT02PbfIndexHeaderDisk header;
    if(!FT02_ReadExact(file, &header, sizeof(header)))
    {
        Serial.println("[PBF-A3.13] index header read failed");
        fclose(file);
        return false;
    }

    if(memcmp(header.magic, "FTPBI1", 6) != 0 ||
       header.version != FT02_PBF_MAP_INDEX_VERSION ||
       header.headerBytes != sizeof(header) ||
       header.entryBytes != sizeof(FT02PbfIndexEntryDisk) ||
       header.entryCount == 0 ||
       header.entryCount > 4096U)
    {
        Serial.printf(
            "[PBF-A3.13] index header invalid version=%u header=%u entry=%u count=%lu\n",
            (unsigned)header.version,
            (unsigned)header.headerBytes,
            (unsigned)header.entryBytes,
            (unsigned long)header.entryCount
        );
        fclose(file);
        return false;
    }

    const size_t entryBytes =
        sizeof(FT02PbfIndexEntryDisk) * (size_t)header.entryCount;

    g_indexEntries =
        (FT02PbfIndexEntryDisk*)FT02_MapAlloc(entryBytes);
    if(g_indexEntries == nullptr)
    {
        Serial.printf(
            "[PBF-A3.13] index allocation failed bytes=%lu heap=%lu psram=%lu\n",
            (unsigned long)entryBytes,
            (unsigned long)ESP.getFreeHeap(),
            (unsigned long)ESP.getFreePsram()
        );
        fclose(file);
        return false;
    }

    const bool readOk = FT02_ReadExact(file, g_indexEntries, entryBytes);
    fclose(file);

    if(!readOk)
    {
        Serial.println("[PBF-A3.13] index entries read failed");
        FT02_MapFree(g_indexEntries);
        g_indexEntries = nullptr;
        return false;
    }

    // FT02_Crc32Update() implements the standard CRC-32 result when started
    // with zero. This is equivalent to the A1 builder's 0xFFFFFFFF + finish
    // convention.
    const uint32_t actualCrc = FT02_Crc32Update(
        0U,
        (const uint8_t*)g_indexEntries,
        entryBytes
    );

    if(actualCrc != header.entryCrc32)
    {
        Serial.printf(
            "[PBF-A3.13] index CRC mismatch expected=0x%08lX actual=0x%08lX\n",
            (unsigned long)header.entryCrc32,
            (unsigned long)actualCrc
        );
        FT02_MapFree(g_indexEntries);
        g_indexEntries = nullptr;
        return false;
    }

    g_indexEntryCount = header.entryCount;
    g_sourceFileBytes = header.sourceFileBytes;
    g_sourceSignature = header.sourceSignature;
    g_report.indexEntries = header.entryCount;

    Serial.printf(
        "[PBF-A3.13] index loaded entries=%lu bytes=%lu\n",
        (unsigned long)g_indexEntryCount,
        (unsigned long)entryBytes
    );
    return true;
}

static bool FT02_EnsureIndexForCacheBuild()
{
    if(FT02_LoadIndex()) return true;

    Serial.println("[PBF-A3.13] validating/recovering persistent index");

    if(FT02_PbfIndexEnsure(
            false,
            FT02_PBF_SOURCE_PATH,
            FT02_PBF_INDEX_PATH
        ) && FT02_LoadIndex())
    {
        Serial.println("[PBF-A3.13] index recovered by normal ensure");
        return true;
    }

    // One deterministic recovery attempt. This path is reached only when a
    // new regional cache must be built. Normal open, pan, zoom and R recenter
    // never touch the index when a valid PBC4 cache is available.
    Serial.printf(
        "[PBF-A3.13] normal ensure failed: %s; forcing one rebuild\n",
        FT02_PbfIndexErrorText()
    );

    FT02_StorageDeleteFile(FT02_PBF_INDEX_PATH);
    FT02_StorageDeleteFile(FT02_PBF_INDEX_TEMP_PATH);

    if(!FT02_PbfIndexEnsure(
            true,
            FT02_PBF_SOURCE_PATH,
            FT02_PBF_INDEX_PATH
        ))
    {
        Serial.printf(
            "[PBF-A3.13] forced index rebuild failed: %s\n",
            FT02_PbfIndexErrorText()
        );
        return false;
    }

    if(!FT02_LoadIndex())
    {
        Serial.println("[PBF-A3.13] rebuilt index could not be loaded");
        return false;
    }

    Serial.println("[PBF-A3.13] index rebuilt and loaded");
    return true;
}

static void FT02_ReleaseRuntimeBuffers()
{
    FT02_MapFree(g_indexEntries); g_indexEntries = nullptr; g_indexEntryCount = 0;
    FT02_MapFree(g_nodeTable); g_nodeTable = nullptr;
}

static void FT02_ReleaseScreenBuffers()
{
    FT02_MapFree(g_segments); g_segments = nullptr; g_segmentCount = 0;
    FT02_MapFree(g_labels); g_labels = nullptr; g_labelCount = 0;
}

static void FT02_ReleaseCacheBuffers()
{
    FT02_MapFree(g_geoSegments); g_geoSegments = nullptr; g_geoSegmentCount = 0;
    FT02_MapFree(g_geoLabels); g_geoLabels = nullptr; g_geoLabelCount = 0;
    memset(&g_cacheHeader, 0, sizeof(g_cacheHeader));
    g_cacheResident = false;
}

static void FT02_ReleaseAllBuffers()
{
    FT02_ReleaseRuntimeBuffers();
    FT02_ReleaseScreenBuffers();
    FT02_ReleaseCacheBuffers();
}


static bool FT02_AllocateScreenBuffers()
{
    FT02_ReleaseScreenBuffers();
    g_segments = (FT02PbfMapSegment*)FT02_MapAlloc(sizeof(FT02PbfMapSegment) * FT02_PBF_MAP_SEGMENT_CAPACITY);
    if(g_segments == nullptr) return false;
    memset(g_segments, 0, sizeof(FT02PbfMapSegment) * FT02_PBF_MAP_SEGMENT_CAPACITY);
    g_labels = (FT02PbfMapLabel*)FT02_MapAlloc(sizeof(FT02PbfMapLabel) * FT02_PBF_CACHE_GEO_LABEL_CAPACITY);
    if(g_labels == nullptr)
    {
        FT02_ReleaseScreenBuffers();
        return false;
    }
    memset(g_labels, 0, sizeof(FT02PbfMapLabel) * FT02_PBF_CACHE_GEO_LABEL_CAPACITY);
    return true;
}

static uint32_t FT02_CachePayloadCrc()
{
    uint32_t crc = 0;
    crc = FT02_Crc32Update(
        crc,
        (const uint8_t*)g_geoSegments,
        g_geoSegmentCount * sizeof(FT02PbfGeoSegmentDisk)
    );
    crc = FT02_Crc32Update(
        crc,
        (const uint8_t*)g_geoLabels,
        g_geoLabelCount * sizeof(FT02PbfGeoLabelDisk)
    );
    return crc;
}

static bool FT02_SaveCache(uint32_t buildElapsedMs)
{
    const char* tempPath = "/maps/raw/shanghai-260726.osm.pbc5.tmp";
    FT02PbfCacheHeaderDisk header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "FTPBC5", 6);
    header.version = FT02_PBF_CACHE_VERSION;
    header.headerBytes = sizeof(header);
    header.segmentBytes = sizeof(FT02PbfGeoSegmentDisk);
    header.labelBytes = sizeof(FT02PbfGeoLabelDisk);
    header.sourceFileBytes = g_sourceFileBytes;
    header.sourceSignature = g_sourceSignature;
    header.segmentCount = (uint32_t)g_geoSegmentCount;
    header.labelCount = (uint32_t)g_geoLabelCount;
    header.minLatE7 = g_cacheBuildBounds.minLatE7;
    header.minLonE7 = g_cacheBuildBounds.minLonE7;
    header.maxLatE7 = g_cacheBuildBounds.maxLatE7;
    header.maxLonE7 = g_cacheBuildBounds.maxLonE7;
    header.centerLatE7 = (int32_t)llround(g_centerLat * 10000000.0);
    header.centerLonE7 = (int32_t)llround(g_centerLon * 10000000.0);
    header.buildElapsedMs = buildElapsedMs;
    header.payloadCrc32 = FT02_CachePayloadCrc();
    header.fileBytes = sizeof(header)
        + header.segmentCount * sizeof(FT02PbfGeoSegmentDisk)
        + header.labelCount * sizeof(FT02PbfGeoLabelDisk);

    FILE* file = FT02_StorageOpenWriteFile(tempPath, true);
    if(file == nullptr)
    {
        Serial.printf("[PBF-A3.13] cache temp open failed path=%s errno=%d\n", tempPath, errno);
        return false;
    }

    // Disable stdio's opaque buffer. Each explicit staged chunk below already
    // uses DMA-capable internal RAM, which makes the write path deterministic.
    setvbuf(file, nullptr, _IONBF, 0);

    size_t totalWritten = 0;
    bool ok = FT02_WriteDmaStaged(file, &header, sizeof(header), "header", totalWritten)
        && FT02_WriteDmaStaged(
            file,
            g_geoSegments,
            g_geoSegmentCount * sizeof(FT02PbfGeoSegmentDisk),
            "segments",
            totalWritten
        )
        && FT02_WriteDmaStaged(
            file,
            g_geoLabels,
            g_geoLabelCount * sizeof(FT02PbfGeoLabelDisk),
            "labels",
            totalWritten
        )
        && FT02_StorageSyncFile(file);

    const int closeResult = fclose(file);
    ok = ok && closeResult == 0;

    if(!ok)
    {
        Serial.printf(
            "[PBF-A3.13] cache temp write/sync failed total=%lu expected=%lu close=%d errno=%d\n",
            (unsigned long)totalWritten,
            (unsigned long)header.fileBytes,
            closeResult,
            errno
        );
        FT02_StorageDeleteFile(tempPath);
        return false;
    }

    uint64_t writtenBytes = 0;
    if(!FT02_StorageFileSize(tempPath, writtenBytes) || writtenBytes != header.fileBytes)
    {
        Serial.printf(
            "[PBF-A3.13] cache temp size mismatch actual=%llu expected=%lu\n",
            (unsigned long long)writtenBytes,
            (unsigned long)header.fileBytes
        );
        FT02_StorageDeleteFile(tempPath);
        return false;
    }
    FT02_StorageDeleteFile(FT02_PBF_CACHE_PATH);
    if(!FT02_StorageRenameFile(tempPath, FT02_PBF_CACHE_PATH))
    {
        FT02_StorageDeleteFile(tempPath);
        return false;
    }
    g_cacheHeader = header;
    g_cacheResident = true;
    g_report.cacheFileBytes = header.fileBytes;
    return true;
}

static bool FT02_LoadCacheFromSd()
{
    FILE* file = FT02_StorageOpenReadFile(FT02_PBF_CACHE_PATH);
    if(file == nullptr) return false;
    FT02PbfCacheHeaderDisk header;
    bool ok = FT02_ReadExact(file, &header, sizeof(header));
    if(!ok || memcmp(header.magic, "FTPBC5", 6) != 0
       || header.version != FT02_PBF_CACHE_VERSION
       || header.headerBytes != sizeof(header)
       || header.segmentBytes != sizeof(FT02PbfGeoSegmentDisk)
       || header.labelBytes != sizeof(FT02PbfGeoLabelDisk)
       || header.sourceFileBytes != g_sourceFileBytes
       || header.sourceSignature != g_sourceSignature
       || header.segmentCount == 0
       || header.segmentCount > FT02_PBF_CACHE_GEO_SEGMENT_CAPACITY
       || header.labelCount > FT02_PBF_CACHE_GEO_LABEL_CAPACITY)
    {
        fclose(file);
        return false;
    }

    FT02PbfGeoSegmentDisk* segments = (FT02PbfGeoSegmentDisk*)FT02_MapAlloc(
        header.segmentCount * sizeof(FT02PbfGeoSegmentDisk)
    );
    FT02PbfGeoLabelDisk* labels = nullptr;
    if(header.labelCount > 0)
        labels = (FT02PbfGeoLabelDisk*)FT02_MapAlloc(header.labelCount * sizeof(FT02PbfGeoLabelDisk));
    if(segments == nullptr || (header.labelCount > 0 && labels == nullptr))
    {
        FT02_MapFree(segments);
        FT02_MapFree(labels);
        fclose(file);
        return false;
    }
    ok = FT02_ReadExact(file, segments, header.segmentCount * sizeof(FT02PbfGeoSegmentDisk))
        && FT02_ReadExact(file, labels, header.labelCount * sizeof(FT02PbfGeoLabelDisk));
    fclose(file);
    if(!ok)
    {
        FT02_MapFree(segments);
        FT02_MapFree(labels);
        return false;
    }

    uint32_t crc = 0;
    crc = FT02_Crc32Update(crc, (const uint8_t*)segments, header.segmentCount * sizeof(FT02PbfGeoSegmentDisk));
    crc = FT02_Crc32Update(crc, (const uint8_t*)labels, header.labelCount * sizeof(FT02PbfGeoLabelDisk));
    if(crc != header.payloadCrc32)
    {
        FT02_MapFree(segments);
        FT02_MapFree(labels);
        return false;
    }

    FT02_ReleaseCacheBuffers();
    g_geoSegments = segments;
    g_geoSegmentCount = header.segmentCount;
    g_geoLabels = labels;
    g_geoLabelCount = header.labelCount;
    g_cacheHeader = header;
    g_cacheResident = true;
    g_report.cacheFileBytes = header.fileBytes;
    return true;
}

static bool FT02_LabelVisibleAtZoom(uint8_t priority)
{
    // Z16 keeps only the highest-priority labels. Z17 is the intermediate
    // overview/street layer and admits the next priority tier before Z18.
    if(priority >= 4) return g_zoom >= 16;
    if(priority >= 3) return g_zoom >= 17;
    if(priority == 2) return g_zoom >= 19;
    return g_zoom >= 20;
}

static bool FT02_ProjectCache()
{
    if(!g_cacheResident || g_geoSegments == nullptr) return false;
    if(!FT02_AllocateScreenBuffers()) return false;
    uint32_t started = millis();
    g_buildingCache = false;
    g_segmentCount = 0;
    g_labelCount = 0;

    for(size_t i = 0; i < g_geoSegmentCount; i++)
    {
        const FT02PbfGeoSegmentDisk& source = g_geoSegments[i];
        FT02_AddSegment(
            source.lat1E7,
            source.lon1E7,
            source.lat2E7,
            source.lon2E7,
            source.style
        );
    }

    for(int priority = 4; priority >= 1; priority--)
    {
        for(size_t i = 0; i < g_geoLabelCount; i++)
        {
            const FT02PbfGeoLabelDisk& source = g_geoLabels[i];
            if(source.priority != priority || !FT02_LabelVisibleAtZoom(source.priority)) continue;
            int16_t x = FT02_ProjectX(source.lonE7);
            int16_t y = FT02_ProjectY(source.latE7);
            if(x < 18 || x > FT02_PBF_MAP_WIDTH - 18 || y < 12 || y > FT02_PBF_MAP_HEIGHT - 12) continue;
            if(g_labelCount >= FT02_PBF_CACHE_GEO_LABEL_CAPACITY) break;
            FT02PbfMapLabel& target = g_labels[g_labelCount++];
            memset(&target, 0, sizeof(target));
            target.x = x;
            target.y = y;
            target.priority = source.priority;
            target.textBytes = source.textBytes;
            size_t copyBytes = source.textBytes;
            if(copyBytes >= FT02_PBF_MAP_LABEL_TEXT_BYTES) copyBytes = FT02_PBF_MAP_LABEL_TEXT_BYTES - 1;
            memcpy(target.text, source.text, copyBytes);
            target.text[copyBytes] = 0;
        }
    }

    g_report.projectMs = millis() - started;
    g_report.segments = (uint32_t)g_segmentCount;
    g_report.labels = (uint32_t)g_labelCount;
    g_report.cachedSegments = (uint32_t)g_geoSegmentCount;
    g_report.cachedLabels = (uint32_t)g_geoLabelCount;
    return g_segmentCount > 0;
}

static bool FT02_BuildRegionalCache()
{
    FT02_ReleaseCacheBuffers();
    g_cacheBuildBounds = FT02_CacheRegionBoundsForCenter(g_centerLon, g_centerLat);
    g_geoSegments = (FT02PbfGeoSegmentDisk*)FT02_MapAlloc(
        sizeof(FT02PbfGeoSegmentDisk) * FT02_PBF_CACHE_GEO_SEGMENT_CAPACITY
    );
    g_geoLabels = (FT02PbfGeoLabelDisk*)FT02_MapAlloc(
        sizeof(FT02PbfGeoLabelDisk) * FT02_PBF_CACHE_GEO_LABEL_CAPACITY
    );
    g_nodeTable = (FT02NodeEntry*)FT02_MapAlloc(sizeof(FT02NodeEntry) * FT02_PBF_MAP_NODE_CAPACITY);
    if(g_geoSegments == nullptr || g_geoLabels == nullptr || g_nodeTable == nullptr) return false;
    memset(g_geoSegments, 0, sizeof(FT02PbfGeoSegmentDisk) * FT02_PBF_CACHE_GEO_SEGMENT_CAPACITY);
    memset(g_geoLabels, 0, sizeof(FT02PbfGeoLabelDisk) * FT02_PBF_CACHE_GEO_LABEL_CAPACITY);
    memset(g_nodeTable, 0, sizeof(FT02NodeEntry) * FT02_PBF_MAP_NODE_CAPACITY);
    g_geoSegmentCount = 0;
    g_geoLabelCount = 0;
    g_buildingCache = true;

    FILE* source = FT02_StorageOpenReadFile(FT02_PBF_SOURCE_PATH);
    if(source == nullptr) return false;
    uint32_t started = millis();
    uint32_t nodePassStart = millis();
    for(uint32_t i = 0; i < g_indexEntryCount; i++)
    {
        const FT02PbfIndexEntryDisk& entry = g_indexEntries[i];
        if(entry.blockType != 2 || entry.nodes == 0 || !FT02_BoundsIntersect(g_cacheBuildBounds, entry)) continue;
        uint8_t* raw = nullptr; size_t rawLength = 0;
        if(!FT02_LoadRawBlock(source, entry, raw, rawLength))
        {
            fclose(source);
            return false;
        }
        bool ok = FT02_ParseNodeBlock(raw, rawLength, g_cacheBuildBounds);
        FT02_MapFree(raw);
        if(!ok)
        {
            fclose(source);
            return false;
        }
        g_report.nodeBlocksRead++;
        if((g_report.nodeBlocksRead & 7U) == 0U)
            Serial.printf("[PBF-A3] cache node blocks=%lu nodes=%lu\n",
                (unsigned long)g_report.nodeBlocksRead,
                (unsigned long)g_report.nodesKept);
        delay(1);
    }
    g_report.nodePassMs = millis() - nodePassStart;

    uint32_t wayPassStart = millis();
    for(uint32_t i = 0; i < g_indexEntryCount; i++)
    {
        const FT02PbfIndexEntryDisk& entry = g_indexEntries[i];
        if(entry.blockType != 2 || entry.ways == 0) continue;
        uint8_t* raw = nullptr; size_t rawLength = 0;
        if(!FT02_LoadRawBlock(source, entry, raw, rawLength))
        {
            fclose(source);
            return false;
        }
        bool ok = FT02_ParseWayBlock(raw, rawLength);
        FT02_MapFree(raw);
        if(!ok)
        {
            fclose(source);
            return false;
        }
        g_report.wayBlocksRead++;
        if((g_report.wayBlocksRead & 7U) == 0U)
            Serial.printf("[PBF-A3] cache way blocks=%lu ways=%lu accepted=%lu geo=%lu labels=%lu\n",
                (unsigned long)g_report.wayBlocksRead,
                (unsigned long)g_report.waysScanned,
                (unsigned long)g_report.waysAccepted,
                (unsigned long)g_geoSegmentCount,
                (unsigned long)g_geoLabelCount);
        delay(1);
    }
    fclose(source);
    g_report.wayPassMs = millis() - wayPassStart;
    g_report.cacheBuildMs = millis() - started;
    g_buildingCache = false;

    if(g_report.nodesKept >= FT02_PBF_MAP_NODE_MAX_LOAD) return false;
    if(g_geoSegmentCount == 0 || g_report.segmentLimitReached) return false;
    if(!FT02_SaveCache(g_report.cacheBuildMs)) return false;
    g_report.cacheBuilt = true;
    g_report.cachedSegments = (uint32_t)g_geoSegmentCount;
    g_report.cachedLabels = (uint32_t)g_geoLabelCount;
    return true;
}

static bool FT02_Fail(FT02PbfMapError error)
{
    g_buildingCache = false;
    g_report.state = FT02_PBF_MAP_ERROR;
    g_report.error = error;
    g_report.minimumFreeHeap = ESP.getMinFreeHeap();
    g_report.freePsramAfter = ESP.getFreePsram();
    Serial.print("[PBF-A3.13] FAIL error=");
    Serial.println((int)error);
    FT02_ReleaseAllBuffers();
    return false;
}

void FT02_PbfMapUnload()
{
    FT02_ReleaseAllBuffers();
}

void FT02_PbfMapPrepare()
{
    FT02_ReleaseRuntimeBuffers();
    FT02_ReleaseScreenBuffers();
    memset(&g_report, 0, sizeof(g_report));
    if(g_centerWorldX == 0.0 && g_centerWorldY == 0.0) FT02_PbfMapResetView();
    g_report.state = FT02_PBF_MAP_LOADING;
    g_report.error = FT02_PBF_MAP_ERROR_NONE;
    g_report.centerLon = g_centerLon;
    g_report.centerLat = g_centerLat;
    g_report.zoom = g_zoom;
}

void FT02_PbfMapResetView()
{
    g_centerLon = FT02_PBF_MAP_DEFAULT_LON;
    g_centerLat = FT02_PBF_MAP_DEFAULT_LAT;
    g_zoom = FT02_PBF_MAP_DEFAULT_ZOOM;
    g_centerWorldX = FT02_LonToWorldX(g_centerLon, g_zoom);
    g_centerWorldY = FT02_LatToWorldY(g_centerLat, g_zoom);
}

void FT02_PbfMapMovePixels(int dx, int dy)
{
    if(g_centerWorldX == 0.0 && g_centerWorldY == 0.0) FT02_PbfMapResetView();
    g_centerWorldX += dx;
    g_centerWorldY += dy;
    g_centerLon = FT02_WorldXToLon(g_centerWorldX, g_zoom);
    g_centerLat = FT02_WorldYToLat(g_centerWorldY, g_zoom);
}

bool FT02_PbfMapChangeZoom(int delta)
{
    if(delta == 0) return false;
    if(g_centerWorldX == 0.0 && g_centerWorldY == 0.0) FT02_PbfMapResetView();

    // Derive the anchor from the currently displayed world coordinates before
    // changing zoom. This prevents a stale/default geographic value from
    // becoming the new center after the zoom level changes.
    const int oldZoom = g_zoom;
    const double anchorLon = FT02_WorldXToLon(g_centerWorldX, oldZoom);
    const double anchorLat = FT02_WorldYToLat(g_centerWorldY, oldZoom);

    // The supported levels are continuous.  Use a direct one-step clamp
    // instead of searching a preset table, so Z18 -> Z17 -> Z16 is explicit
    // and cannot be skipped by an ordering or stale-table mistake.
    const int step = delta > 0 ? 1 : -1;
    int next = g_zoom + step;
    if(next < FT02_PBF_MAP_MIN_ZOOM) next = FT02_PBF_MAP_MIN_ZOOM;
    if(next > FT02_PBF_MAP_MAX_ZOOM) next = FT02_PBF_MAP_MAX_ZOOM;

    if(next == g_zoom)
    {
        Serial.printf(
            "[PBF-A3.13] zoom limit unchanged z=%d center=%.7f,%.7f; rebuild skipped\n",
            g_zoom,
            anchorLon,
            anchorLat
        );
        return false;
    }

    g_zoom = next;
    g_centerLon = anchorLon;
    g_centerLat = anchorLat;
    g_centerWorldX = FT02_LonToWorldX(anchorLon, g_zoom);
    g_centerWorldY = FT02_LatToWorldY(anchorLat, g_zoom);

    Serial.printf(
        "[PBF-A3.13] zoom anchor kept oldZ=%d newZ=%d center=%.7f,%.7f\n",
        oldZoom,
        g_zoom,
        g_centerLon,
        g_centerLat
    );
    return true;
}

void FT02_PbfMapInvalidateCache()
{
    g_forceCacheRebuild = true;
    FT02_StorageDeleteFile(FT02_PBF_CACHE_PATH);
    FT02_ReleaseCacheBuffers();
}

bool FT02_PbfMapBuild()
{
    FT02_ReleaseRuntimeBuffers();
    FT02_ReleaseScreenBuffers();
    memset(&g_report, 0, sizeof(g_report));

    if(g_centerWorldX == 0.0 && g_centerWorldY == 0.0)
    {
        FT02_PbfMapResetView();
    }

    g_report.state = FT02_PBF_MAP_LOADING;
    g_report.centerLon = g_centerLon;
    g_report.centerLat = g_centerLat;
    g_report.zoom = g_zoom;

    const uint32_t started = millis();

    Serial.println("[PBF-A3.13] regional cache map build begin");
    Serial.printf(
        "[PBF-A3.13] center=%.7f,%.7f zoom=%d\n",
        g_centerLon,
        g_centerLat,
        g_zoom
    );

    if(!FT02_StorageIsReady())
    {
        return FT02_Fail(FT02_PBF_MAP_ERROR_STORAGE);
    }

    const FT02ViewBounds view = FT02_CurrentViewBounds();
    bool cacheReady = false;

    // Cold boot previously compared a valid PBC5 header against zero-valued
    // source identity fields, deleted the cache, and rebuilt it on every first
    // map open. Prime the identity from three small source samples before the
    // cache header is validated. This does not load or rebuild the PBI index.
    if(!g_cacheResident && (g_sourceFileBytes == 0 || g_sourceSignature == 0))
    {
        const uint32_t identityStarted = millis();
        if(!FT02_PbfIndexComputeSourceIdentity(
                FT02_PBF_SOURCE_PATH,
                g_sourceFileBytes,
                g_sourceSignature
            ))
        {
            Serial.println("[PBF-A3.13] source identity check failed before cache load");
            return FT02_Fail(FT02_PBF_MAP_ERROR_SOURCE_OPEN);
        }
        Serial.printf(
            "[PBF-A3.13] source identity ready bytes=%llu signature=0x%08lX time=%lu ms\n",
            (unsigned long long)g_sourceFileBytes,
            (unsigned long)g_sourceSignature,
            (unsigned long)(millis() - identityStarted)
        );
    }

    if(g_forceCacheRebuild)
    {
        g_forceCacheRebuild = false;
        FT02_ReleaseCacheBuffers();
        FT02_StorageDeleteFile(FT02_PBF_CACHE_PATH);
        Serial.println("[PBF-A3.13] forced regional cache rebuild requested");
    }

    // Fastest path: R, pan and zoom inside the current resident cache are
    // projection-only operations. They do not read the SD index and do not
    // rescan the PBF.
    if(g_cacheResident && FT02_ViewInsideCache(view))
    {
        cacheReady = true;
        g_report.cacheReusedInMemory = true;
        g_report.cacheFileBytes = g_cacheHeader.fileBytes;
        Serial.println("[PBF-A3.13] RAM cache hit; index not accessed");
    }

    // Cold-start path: the regional cache is self-contained. Load and use it
    // before touching the .pbi block index. This allows the map to start even
    // when an old or damaged index is present on the card.
    if(!cacheReady && !g_cacheResident && FT02_FileExists(FT02_PBF_CACHE_PATH))
    {
        const uint32_t loadStarted = millis();

        if(FT02_LoadCacheFromSd())
        {
            g_report.cacheLoadMs = millis() - loadStarted;

            if(FT02_ViewInsideCache(view))
            {
                cacheReady = true;
                g_report.cacheLoaded = true;
                Serial.printf(
                    "[PBF-A3.13] SD cache hit segments=%lu labels=%lu time=%lu ms; index not accessed\n",
                    (unsigned long)g_geoSegmentCount,
                    (unsigned long)g_geoLabelCount,
                    (unsigned long)g_report.cacheLoadMs
                );
            }
            else
            {
                Serial.println("[PBF-A3.13] SD cache does not cover view; rebuilding region");
                FT02_ReleaseCacheBuffers();
            }
        }
        else
        {
            Serial.println("[PBF-A3.13] SD cache invalid; deleting and rebuilding region");
            FT02_StorageDeleteFile(FT02_PBF_CACHE_PATH);
            FT02_ReleaseCacheBuffers();
        }
    }

    // The persistent PBI index is needed only to build a new regional cache.
    if(!cacheReady)
    {
        if(!FT02_EnsureIndexForCacheBuild())
        {
            return FT02_Fail(FT02_PBF_MAP_ERROR_INDEX_INVALID);
        }

        Serial.println("[PBF-A3.13] building new regional cache from raw PBF");

        if(!FT02_BuildRegionalCache())
        {
            if(g_report.nodesKept >= FT02_PBF_MAP_NODE_MAX_LOAD)
            {
                return FT02_Fail(FT02_PBF_MAP_ERROR_NODE_TABLE_FULL);
            }

            if(g_report.segmentLimitReached)
            {
                return FT02_Fail(FT02_PBF_MAP_ERROR_CACHE_REGION_FULL);
            }

            return FT02_Fail(FT02_PBF_MAP_ERROR_CACHE_WRITE);
        }

        cacheReady = true;
    }

    FT02_ReleaseRuntimeBuffers();

    if(!FT02_ProjectCache())
    {
        return FT02_Fail(FT02_PBF_MAP_ERROR_NO_GEOMETRY);
    }

    g_report.elapsedMs = millis() - started;
    g_report.minimumFreeHeap = ESP.getMinFreeHeap();
    g_report.freePsramAfter = ESP.getFreePsram();
    g_report.centerLon = g_centerLon;
    g_report.centerLat = g_centerLat;
    g_report.zoom = g_zoom;
    g_report.state = FT02_PBF_MAP_READY;
    g_report.error = FT02_PBF_MAP_ERROR_NONE;

    const char* source = g_report.cacheBuilt
        ? "NEW CACHE"
        : (g_report.cacheLoaded ? "SD CACHE" : "RAM CACHE");

    Serial.printf(
        "[PBF-A3.13] PASS source=%s screen_segments=%lu labels=%lu project=%lu ms total=%lu ms\n",
        source,
        (unsigned long)g_report.segments,
        (unsigned long)g_report.labels,
        (unsigned long)g_report.projectMs,
        (unsigned long)g_report.elapsedMs
    );
    return true;
}

const FT02PbfMapReport& FT02_PbfMapReportCurrent() { return g_report; }
const FT02PbfMapSegment* FT02_PbfMapSegments() { return g_segments; }
size_t FT02_PbfMapSegmentCount() { return g_segmentCount; }
const FT02PbfMapLabel* FT02_PbfMapLabels() { return g_labels; }
size_t FT02_PbfMapLabelCount() { return g_labelCount; }

const char* FT02_PbfMapStateText()
{
    switch(g_report.state)
    {
        case FT02_PBF_MAP_LOADING: return "LOADING";
        case FT02_PBF_MAP_READY: return "MAP READY";
        case FT02_PBF_MAP_ERROR: return "ERROR";
        default: return "IDLE";
    }
}

const char* FT02_PbfMapErrorText()
{
    switch(g_report.error)
    {
        case FT02_PBF_MAP_ERROR_STORAGE: return "STORAGE NOT READY";
        case FT02_PBF_MAP_ERROR_INDEX_OPEN: return "INDEX OPEN FAILED";
        case FT02_PBF_MAP_ERROR_INDEX_INVALID: return "INDEX INVALID";
        case FT02_PBF_MAP_ERROR_SOURCE_OPEN: return "PBF OPEN FAILED";
        case FT02_PBF_MAP_ERROR_SOURCE_READ: return "PBF BLOCK READ FAILED";
        case FT02_PBF_MAP_ERROR_SOURCE_SEEK: return "PBF SEEK FAILED";
        case FT02_PBF_MAP_ERROR_OUT_OF_MEMORY: return "OUT OF MEMORY";
        case FT02_PBF_MAP_ERROR_DECOMPRESSION: return "DECOMPRESSION FAILED";
        case FT02_PBF_MAP_ERROR_PROTOBUF: return "PROTOBUF PARSE FAILED";
        case FT02_PBF_MAP_ERROR_NODE_TABLE_FULL: return "LOCAL NODE TABLE FULL";
        case FT02_PBF_MAP_ERROR_NO_GEOMETRY: return "NO VISIBLE GEOMETRY";
        case FT02_PBF_MAP_ERROR_CACHE_OPEN: return "CACHE OPEN FAILED";
        case FT02_PBF_MAP_ERROR_CACHE_INVALID: return "CACHE INVALID";
        case FT02_PBF_MAP_ERROR_CACHE_WRITE: return "CACHE BUILD/WRITE FAILED";
        case FT02_PBF_MAP_ERROR_CACHE_REGION_FULL: return "CACHE REGION TOO DENSE";
        default: return "NONE";
    }
}
