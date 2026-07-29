#pragma once

#include <Arduino.h>
#include <stdint.h>

static const char* const FT02_PBF_SOURCE_PATH =
    "/maps/raw/shanghai-260726.osm.pbf";
static const char* const FT02_PBF_INDEX_PATH =
    "/maps/raw/shanghai-260726.osm.pbi";
static const char* const FT02_PBF_INDEX_TEMP_PATH =
    "/maps/raw/shanghai-260726.osm.pbi.tmp";

enum FT02PbfIndexState
{
    FT02_PBF_INDEX_NOT_STARTED = 0,
    FT02_PBF_INDEX_CHECKING,
    FT02_PBF_INDEX_BUILDING,
    FT02_PBF_INDEX_READY,
    FT02_PBF_INDEX_ERROR
};

enum FT02PbfIndexError
{
    FT02_PBF_INDEX_ERROR_NONE = 0,
    FT02_PBF_INDEX_ERROR_STORAGE_NOT_READY,
    FT02_PBF_INDEX_ERROR_SOURCE_NOT_FOUND,
    FT02_PBF_INDEX_ERROR_SOURCE_OPEN,
    FT02_PBF_INDEX_ERROR_SOURCE_SEEK,
    FT02_PBF_INDEX_ERROR_SOURCE_SIGNATURE,
    FT02_PBF_INDEX_ERROR_TRUNCATED,
    FT02_PBF_INDEX_ERROR_HEADER_TOO_LARGE,
    FT02_PBF_INDEX_ERROR_BLOB_TOO_LARGE,
    FT02_PBF_INDEX_ERROR_RAW_BLOCK_TOO_LARGE,
    FT02_PBF_INDEX_ERROR_PROTOBUF,
    FT02_PBF_INDEX_ERROR_UNSUPPORTED_COMPRESSION,
    FT02_PBF_INDEX_ERROR_OUT_OF_MEMORY,
    FT02_PBF_INDEX_ERROR_PSRAM_UNAVAILABLE,
    FT02_PBF_INDEX_ERROR_DECOMPRESSION,
    FT02_PBF_INDEX_ERROR_TEMP_OPEN,
    FT02_PBF_INDEX_ERROR_TEMP_WRITE,
    FT02_PBF_INDEX_ERROR_TEMP_SYNC,
    FT02_PBF_INDEX_ERROR_COMMIT,
    FT02_PBF_INDEX_ERROR_INDEX_READ,
    FT02_PBF_INDEX_ERROR_INDEX_INVALID
};

struct FT02PbfIndexReport
{
    FT02PbfIndexState state;
    FT02PbfIndexError error;

    bool loadedFromIndex;
    bool rebuiltIndex;
    bool sourceBoundsValid;

    uint64_t sourceFileBytes;
    uint32_t sourceSignature;
    uint64_t indexFileBytes;
    uint32_t indexEntries;

    uint64_t bytesRead;
    uint64_t compressedPayloadBytes;
    uint64_t rawPayloadBytes;

    uint32_t fileBlocks;
    uint32_t headerBlocks;
    uint32_t dataBlocks;

    uint64_t nodes;
    uint64_t ways;
    uint64_t relations;

    uint32_t maxBlobBytes;
    uint32_t maxRawBlockBytes;
    uint32_t operationElapsedMs;
    uint32_t originalBuildElapsedMs;

    uint32_t freeHeapBefore;
    uint32_t minimumFreeHeap;
    uint32_t psramBytes;
    uint32_t freePsramBefore;

    int32_t minLatE7;
    int32_t minLonE7;
    int32_t maxLatE7;
    int32_t maxLonE7;

    uint32_t failedBlockIndex;
    char failedBlockType[16];
};

void FT02_PbfIndexReset();
void FT02_PbfIndexPrepare(bool forceRebuild);
bool FT02_PbfIndexEnsure(
    bool forceRebuild,
    const char* sourcePath = FT02_PBF_SOURCE_PATH,
    const char* indexPath = FT02_PBF_INDEX_PATH
);

// Reads only the source file size and the existing 3 x 32 KiB signature samples.
// It does not load or rebuild the persistent index.
bool FT02_PbfIndexComputeSourceIdentity(
    const char* sourcePath,
    uint64_t& sourceFileBytes,
    uint32_t& sourceSignature
);

const FT02PbfIndexReport& FT02_PbfIndexReportCurrent();
const char* FT02_PbfIndexStateText();
const char* FT02_PbfIndexErrorText();
const char* FT02_PbfIndexSourceText();
