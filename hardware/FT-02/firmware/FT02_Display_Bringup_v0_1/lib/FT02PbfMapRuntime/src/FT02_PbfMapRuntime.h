#pragma once

#include <Arduino.h>
#include <stdint.h>

// Supported user zoom presets are continuous from Z16 through Z20.
// Z16 is the widest overview; Z17 is an intermediate overview/street layer;
// Z18-Z20 retain the existing progressively detailed behavior.
static const int FT02_PBF_MAP_MIN_ZOOM = 16;
static const int FT02_PBF_MAP_MAX_ZOOM = 20;
static const int FT02_PBF_MAP_DEFAULT_ZOOM = 18;
// Coordinate contract for the map runtime:
// - OSM/PBF geometry and GNSS positions use WGS-84.
// - Mainland-China web-map coordinates (GCJ-02) must be converted before use.
// Original Google Maps point (GCJ-02): 31.210449492563992, 121.38134188066108
// Converted default center (WGS-84): 31.212306845416812, 121.37670781443698
static const double FT02_PBF_MAP_DEFAULT_LON = 121.37670781443698;
static const double FT02_PBF_MAP_DEFAULT_LAT = 31.212306845416812;

enum FT02PbfMapState
{
    FT02_PBF_MAP_NOT_STARTED = 0,
    FT02_PBF_MAP_LOADING,
    FT02_PBF_MAP_READY,
    FT02_PBF_MAP_ERROR
};

enum FT02PbfMapError
{
    FT02_PBF_MAP_ERROR_NONE = 0,
    FT02_PBF_MAP_ERROR_STORAGE,
    FT02_PBF_MAP_ERROR_INDEX_OPEN,
    FT02_PBF_MAP_ERROR_INDEX_INVALID,
    FT02_PBF_MAP_ERROR_SOURCE_OPEN,
    FT02_PBF_MAP_ERROR_SOURCE_READ,
    FT02_PBF_MAP_ERROR_SOURCE_SEEK,
    FT02_PBF_MAP_ERROR_OUT_OF_MEMORY,
    FT02_PBF_MAP_ERROR_DECOMPRESSION,
    FT02_PBF_MAP_ERROR_PROTOBUF,
    FT02_PBF_MAP_ERROR_NODE_TABLE_FULL,
    FT02_PBF_MAP_ERROR_NO_GEOMETRY,
    FT02_PBF_MAP_ERROR_CACHE_OPEN,
    FT02_PBF_MAP_ERROR_CACHE_INVALID,
    FT02_PBF_MAP_ERROR_CACHE_WRITE,
    FT02_PBF_MAP_ERROR_CACHE_REGION_FULL
};

enum FT02PbfMapStyle : uint8_t
{
    FT02_PBF_MAP_STYLE_MINOR = 1,
    FT02_PBF_MAP_STYLE_MEDIUM = 2,
    FT02_PBF_MAP_STYLE_MAJOR = 3,
    FT02_PBF_MAP_STYLE_PATH = 4,
    FT02_PBF_MAP_STYLE_BUILDING = 5,
    FT02_PBF_MAP_STYLE_RAIL = 6,
    FT02_PBF_MAP_STYLE_WATER = 7
};

struct FT02PbfMapSegment
{
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    uint8_t style;
    uint8_t reserved[3];
};

static const size_t FT02_PBF_MAP_LABEL_TEXT_BYTES = 48;

struct FT02PbfMapLabel
{
    int16_t x;
    int16_t y;
    uint8_t priority;
    uint8_t textBytes;
    char text[FT02_PBF_MAP_LABEL_TEXT_BYTES];
};

struct FT02PbfMapReport
{
    FT02PbfMapState state;
    FT02PbfMapError error;

    double centerLon;
    double centerLat;
    int zoom;

    uint32_t elapsedMs;
    uint32_t nodePassMs;
    uint32_t wayPassMs;
    uint32_t cacheLoadMs;
    uint32_t cacheBuildMs;
    uint32_t projectMs;

    uint32_t indexEntries;
    uint32_t nodeBlocksRead;
    uint32_t wayBlocksRead;
    uint32_t nodesKept;
    uint32_t waysScanned;
    uint32_t waysAccepted;
    uint32_t segments;
    uint32_t labels;
    uint32_t cachedSegments;
    uint32_t cachedLabels;
    uint32_t cacheFileBytes;
    uint32_t unresolvedNodeRefs;

    uint32_t minimumFreeHeap;
    uint32_t freePsramAfter;
    bool segmentLimitReached;
    bool cacheLoaded;
    bool cacheBuilt;
    bool cacheReusedInMemory;
};

void FT02_PbfMapPrepare();
void FT02_PbfMapUnload();
void FT02_PbfMapResetView();
void FT02_PbfMapMovePixels(int dx, int dy);
bool FT02_PbfMapChangeZoom(int delta);
void FT02_PbfMapInvalidateCache();
bool FT02_PbfMapBuild();

const FT02PbfMapReport& FT02_PbfMapReportCurrent();
const FT02PbfMapSegment* FT02_PbfMapSegments();
size_t FT02_PbfMapSegmentCount();
const FT02PbfMapLabel* FT02_PbfMapLabels();
size_t FT02_PbfMapLabelCount();
const char* FT02_PbfMapStateText();
const char* FT02_PbfMapErrorText();
