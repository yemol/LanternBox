#pragma once

#include <Arduino.h>

constexpr uint16_t FT02_LOCATION_LOG_MAX_SESSIONS = 64;

struct FT02LocationLogEntry
{
    bool valid;
    bool completed;
    bool active;
    bool autoTrackUsed;
    bool hasStartPosition;
    bool hasEndPosition;
    bool durationValid;

    uint32_t durationSeconds;
    uint32_t pointCount;
    uint32_t manualPointCount;
    uint32_t autoPointCount;
    uint32_t failedPointCount;
    uint32_t eventCount;

    double startLatitude;
    double startLongitude;
    double endLatitude;
    double endLongitude;
    float startAltitudeMeters;
    float endAltitudeMeters;

    char sessionId[40];
    char startDate[16];
    char startTime[20];
    char endDate[16];
    char endTime[20];
};

struct FT02LocationLogStatus
{
    bool storageReady;
    bool fileExists;
    bool loaded;
    bool loading;
    bool truncated;

    uint16_t count;
    uint32_t totalSessions;
    uint32_t lineCount;
    uint32_t parseErrorCount;
    uint32_t generation;

    char message[80];
};

struct FT02LocationRoutePoint
{
    double latitude;
    double longitude;
    float altitudeMeters;
    bool altitudeValid;
};

struct FT02LocationRouteStatus
{
    bool storageReady;
    bool fileExists;
    bool loaded;
    bool hasBounds;

    uint32_t count;
    uint32_t totalPointCount;
    uint32_t parseErrorCount;
    uint32_t generation;

    double minLatitude;
    double minLongitude;
    double maxLatitude;
    double maxLongitude;

    char sessionId[40];
    char message[80];
};

void FT02_LocationLogBegin();

// Background list loader. StartReload only schedules a FreeRTOS worker; all
// potentially slow SD operations (stat/fopen/fgets) run outside the Arduino
// main loop. PollReload only consumes a completion flag, so CardKB, GNSS and
// UI dispatch stay responsive even if the SD card stalls.
bool FT02_LocationLogStartReload();
bool FT02_LocationLogPollReload();
void FT02_LocationLogCancelReload();
bool FT02_LocationLogIsLoading();

// Legacy synchronous helper retained for non-UI/internal compatibility.
bool FT02_LocationLogReload();
FT02LocationLogStatus FT02_LocationLogStatusCurrent();

// List order is newest first. newestIndex=0 returns the latest session.
bool FT02_LocationLogGetNewest(
    uint16_t newestIndex,
    FT02LocationLogEntry& output
);

// Wraps an index by an arbitrary signed step. Used by both list and detail
// navigation so first/last records form one continuous loop.
uint16_t FT02_LocationLogWrapIndex(
    uint16_t current,
    uint16_t count,
    int32_t step
);

// Loads every valid path_point for one session into a dedicated route buffer.
// The buffer is exact-sized and prefers PSRAM on ESP32-S3, then falls back to
// normal heap. Existing JSONL files require no migration.
bool FT02_LocationLogLoadRoute(const char* sessionId);
void FT02_LocationLogReleaseRoute();
FT02LocationRouteStatus FT02_LocationLogRouteStatusCurrent();
const FT02LocationRoutePoint* FT02_LocationLogRoutePoints();

// Rewrites the JSONL file atomically through temp + backup files. Active
// sessions are protected and cannot be deleted.
bool FT02_LocationLogDeleteSession(const char* sessionId);
