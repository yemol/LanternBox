#include "FT02_LocationLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "FT02_LocationRecorder.h"
#include "FT02_Storage.h"

namespace
{
constexpr const char* FT02_TRACK_PATH = "/lanternbox/tracks/path_points.jsonl";
constexpr const char* FT02_TRACK_TEMP_PATH = "/lanternbox/tracks/path_points.tmp";
constexpr const char* FT02_TRACK_BACKUP_PATH = "/lanternbox/tracks/path_points.bak";
constexpr size_t FT02_LOCATION_LOG_LINE_BYTES = 1200;
constexpr uint16_t FT02_LOCATION_LOG_LINES_PER_YIELD = 6;
constexpr uint32_t FT02_LOCATION_LOG_WORKER_STACK = 8192;
constexpr UBaseType_t FT02_LOCATION_LOG_WORKER_PRIORITY = 2;
constexpr BaseType_t FT02_LOCATION_LOG_WORKER_CORE = 0;

struct FT02LocationLogBuildState
{
    FT02LocationLogEntry entries[FT02_LOCATION_LOG_MAX_SESSIONS];
    FT02LocationLogStatus status;
};

FT02LocationLogEntry g_ft02LocationEntries[FT02_LOCATION_LOG_MAX_SESSIONS] = {};
FT02LocationLogStatus g_ft02LocationLogStatus = {};
FT02LocationRoutePoint* g_ft02LocationRoutePoints = nullptr;
FT02LocationRouteStatus g_ft02LocationRouteStatus = {};
FT02LocationLogBuildState g_ft02LocationLogWorkerBuild = {};
TaskHandle_t g_ft02LocationLogReloadTaskHandle = nullptr;
SemaphoreHandle_t g_ft02LocationLogDataMutex = nullptr;
volatile bool g_ft02LocationLogCancelRequested = false;
volatile bool g_ft02LocationLogCompletionPending = false;

static void FT02_LocationLogSetMessage(const char* message)
{
    snprintf(
        g_ft02LocationLogStatus.message,
        sizeof(g_ft02LocationLogStatus.message),
        "%s",
        message != nullptr ? message : ""
    );
}

static void FT02_LocationLogChanged()
{
    g_ft02LocationLogStatus.generation++;
}

static void FT02_LocationRouteSetMessage(const char* message)
{
    snprintf(
        g_ft02LocationRouteStatus.message,
        sizeof(g_ft02LocationRouteStatus.message),
        "%s",
        message != nullptr ? message : ""
    );
}

static void FT02_LocationRouteChanged()
{
    g_ft02LocationRouteStatus.generation++;
}

static void* FT02_LocationRouteAlloc(size_t bytes)
{
    if(bytes == 0) return nullptr;
#if defined(ARDUINO_ARCH_ESP32)
    void* memory = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(memory != nullptr) return memory;
#endif
    return malloc(bytes);
}

static void FT02_LocationRouteFree(void* memory)
{
    if(memory == nullptr) return;
#if defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(memory);
#else
    free(memory);
#endif
}

static void FT02_LocationRouteClear(bool preserveGeneration)
{
    const uint32_t generation = g_ft02LocationRouteStatus.generation;
    if(g_ft02LocationRoutePoints != nullptr)
    {
        FT02_LocationRouteFree(g_ft02LocationRoutePoints);
        g_ft02LocationRoutePoints = nullptr;
    }
    memset(&g_ft02LocationRouteStatus, 0, sizeof(g_ft02LocationRouteStatus));
    if(preserveGeneration) g_ft02LocationRouteStatus.generation = generation;
}

static void FT02_LocationLogClearEntries()
{
    memset(g_ft02LocationEntries, 0, sizeof(g_ft02LocationEntries));
    g_ft02LocationLogStatus.count = 0;
    g_ft02LocationLogStatus.totalSessions = 0;
    g_ft02LocationLogStatus.lineCount = 0;
    g_ft02LocationLogStatus.parseErrorCount = 0;
    g_ft02LocationLogStatus.truncated = false;
}

static const char* FT02_FindJsonValue(const char* line, const char* key)
{
    if(line == nullptr || key == nullptr) return nullptr;

    char pattern[64];
    const int written = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if(written <= 0 || static_cast<size_t>(written) >= sizeof(pattern)) return nullptr;

    const char* cursor = strstr(line, pattern);
    if(cursor == nullptr) return nullptr;
    cursor += strlen(pattern);

    while(*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') cursor++;
    if(*cursor != ':') return nullptr;
    cursor++;
    while(*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') cursor++;
    return cursor;
}

static bool FT02_JsonString(
    const char* line,
    const char* key,
    char* output,
    size_t outputSize
)
{
    if(output == nullptr || outputSize == 0) return false;
    output[0] = '\0';

    const char* cursor = FT02_FindJsonValue(line, key);
    if(cursor == nullptr || *cursor != '"') return false;
    cursor++;

    size_t used = 0;
    bool escaped = false;
    while(*cursor != '\0')
    {
        const char ch = *cursor++;
        if(escaped)
        {
            if(used + 1 < outputSize) output[used++] = ch;
            escaped = false;
            continue;
        }
        if(ch == '\\')
        {
            escaped = true;
            continue;
        }
        if(ch == '"')
        {
            output[used] = '\0';
            return true;
        }
        if(used + 1 < outputSize) output[used++] = ch;
    }

    output[0] = '\0';
    return false;
}

static bool FT02_JsonBool(const char* line, const char* key, bool& output)
{
    const char* cursor = FT02_FindJsonValue(line, key);
    if(cursor == nullptr) return false;
    if(strncmp(cursor, "true", 4) == 0)
    {
        output = true;
        return true;
    }
    if(strncmp(cursor, "false", 5) == 0)
    {
        output = false;
        return true;
    }
    return false;
}

static bool FT02_JsonDouble(const char* line, const char* key, double& output)
{
    const char* cursor = FT02_FindJsonValue(line, key);
    if(cursor == nullptr) return false;

    char* end = nullptr;
    const double value = strtod(cursor, &end);
    if(end == cursor) return false;
    output = value;
    return true;
}

static int FT02_LocationLogFindSession(FT02LocationLogBuildState& build, const char* sessionId)
{
    for(uint16_t i = 0; i < build.status.count; i++)
    {
        if(strcmp(build.entries[i].sessionId, sessionId) == 0)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

static FT02LocationLogEntry* FT02_LocationLogCreateSession(FT02LocationLogBuildState& build, const char* sessionId)
{
    build.status.totalSessions++;

    uint16_t index = build.status.count;
    if(build.status.count >= FT02_LOCATION_LOG_MAX_SESSIONS)
    {
        memmove(
            &build.entries[0],
            &build.entries[1],
            sizeof(FT02LocationLogEntry) * (FT02_LOCATION_LOG_MAX_SESSIONS - 1)
        );
        index = FT02_LOCATION_LOG_MAX_SESSIONS - 1;
        build.status.truncated = true;
    }
    else
    {
        build.status.count++;
    }

    FT02LocationLogEntry& entry = build.entries[index];
    memset(&entry, 0, sizeof(entry));
    entry.valid = true;
    snprintf(entry.sessionId, sizeof(entry.sessionId), "%s", sessionId);
    return &entry;
}

static bool FT02_ParseDateTime(
    const char* dateText,
    const char* timeText,
    int& year,
    int& month,
    int& day,
    int& hour,
    int& minute,
    int& second
)
{
    if(dateText == nullptr || timeText == nullptr) return false;
    if(strlen(dateText) != 10 || strlen(timeText) < 8) return false;
    if(dateText[4] != '-' || dateText[7] != '-' ||
       timeText[2] != ':' || timeText[5] != ':') return false;

    year = atoi(dateText);
    month = atoi(dateText + 5);
    day = atoi(dateText + 8);
    hour = atoi(timeText);
    minute = atoi(timeText + 3);
    second = atoi(timeText + 6);

    return year >= 2020 && year <= 2199 &&
           month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
           hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59;
}

// Howard Hinnant's civil-date conversion, adapted for embedded use.
static int64_t FT02_DaysFromCivil(int year, unsigned month, unsigned day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const int adjustedMonth = static_cast<int>(month) + (month > 2 ? -3 : 9);
    const unsigned dayOfYear =
        (153U * static_cast<unsigned>(adjustedMonth) + 2U) / 5U + day - 1U;
    const unsigned dayOfEra = yearOfEra * 365U + yearOfEra / 4U - yearOfEra / 100U + dayOfYear;
    return static_cast<int64_t>(era) * 146097LL + static_cast<int64_t>(dayOfEra) - 719468LL;
}

static bool FT02_DateTimeSeconds(
    const char* dateText,
    const char* timeText,
    int64_t& output
)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if(!FT02_ParseDateTime(dateText, timeText, year, month, day, hour, minute, second))
    {
        return false;
    }

    const int64_t days = FT02_DaysFromCivil(
        year,
        static_cast<unsigned>(month),
        static_cast<unsigned>(day)
    );
    output = days * 86400LL + hour * 3600LL + minute * 60LL + second;
    return true;
}

static void FT02_LocationLogFinalizeDurations(FT02LocationLogBuildState& build)
{
    for(uint16_t i = 0; i < build.status.count; i++)
    {
        FT02LocationLogEntry& entry = build.entries[i];
        int64_t startSeconds = 0;
        int64_t endSeconds = 0;
        if(FT02_DateTimeSeconds(entry.startDate, entry.startTime, startSeconds) &&
           FT02_DateTimeSeconds(entry.endDate, entry.endTime, endSeconds) &&
           endSeconds >= startSeconds)
        {
            const int64_t duration = endSeconds - startSeconds;
            entry.durationSeconds = duration > 0xFFFFFFFFLL
                ? 0xFFFFFFFFUL
                : static_cast<uint32_t>(duration);
            entry.durationValid = true;
        }
    }
}

static void FT02_LocationLogApplyLine(FT02LocationLogBuildState& build, const char* line)
{
    char sessionId[40];
    char type[32];
    char dateText[16];
    char timeText[20];
    char note[32];

    if(!FT02_JsonString(line, "session_id", sessionId, sizeof(sessionId)) ||
       sessionId[0] == '\0' || strcmp(sessionId, "--") == 0)
    {
        build.status.parseErrorCount++;
        return;
    }

    int index = FT02_LocationLogFindSession(build, sessionId);
    FT02LocationLogEntry* entry = index >= 0
        ? &build.entries[index]
        : FT02_LocationLogCreateSession(build, sessionId);
    if(entry == nullptr) return;

    const bool hasType = FT02_JsonString(line, "type", type, sizeof(type));
    const bool hasDate = FT02_JsonString(line, "device_date", dateText, sizeof(dateText));
    const bool hasTime = FT02_JsonString(line, "device_time", timeText, sizeof(timeText));
    const bool hasNote = FT02_JsonString(line, "note", note, sizeof(note));

    entry->eventCount++;

    const bool isSessionStart = hasType && strcmp(type, "session_start") == 0;
    if((entry->startDate[0] == '\0' || isSessionStart) && hasDate)
    {
        snprintf(entry->startDate, sizeof(entry->startDate), "%s", dateText);
    }
    if((entry->startTime[0] == '\0' || isSessionStart) && hasTime)
    {
        snprintf(entry->startTime, sizeof(entry->startTime), "%s", timeText);
    }
    if(hasDate) snprintf(entry->endDate, sizeof(entry->endDate), "%s", dateText);
    if(hasTime) snprintf(entry->endTime, sizeof(entry->endTime), "%s", timeText);

    if(hasType && strcmp(type, "session_stop") == 0)
    {
        entry->completed = true;
    }
    else if(hasType && strcmp(type, "path_point") == 0)
    {
        entry->pointCount++;
        if(hasNote && strcmp(note, "manual") == 0) entry->manualPointCount++;
        else
        {
            entry->autoPointCount++;
            entry->autoTrackUsed = true;
        }
    }
    else if(hasType && strcmp(type, "path_point_failed") == 0)
    {
        entry->failedPointCount++;
    }
    else if(hasType && strcmp(type, "auto_track_on") == 0)
    {
        entry->autoTrackUsed = true;
    }

    bool fixValid = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    const bool hasFix = FT02_JsonBool(line, "gnss_fix", fixValid);
    const bool hasLat = FT02_JsonDouble(line, "lat", latitude);
    const bool hasLon = FT02_JsonDouble(line, "lon", longitude);
    const bool hasAltitude = FT02_JsonDouble(line, "altitude_m", altitude);

    if(hasFix && fixValid && hasLat && hasLon)
    {
        if(!entry->hasStartPosition)
        {
            entry->hasStartPosition = true;
            entry->startLatitude = latitude;
            entry->startLongitude = longitude;
            if(hasAltitude) entry->startAltitudeMeters = static_cast<float>(altitude);
        }

        entry->hasEndPosition = true;
        entry->endLatitude = latitude;
        entry->endLongitude = longitude;
        if(hasAltitude) entry->endAltitudeMeters = static_cast<float>(altitude);
    }
}

static bool FT02_LineBelongsToSession(const char* line, const char* sessionId)
{
    char lineSessionId[40];
    return FT02_JsonString(line, "session_id", lineSessionId, sizeof(lineSessionId)) &&
           strcmp(lineSessionId, sessionId) == 0;
}


static bool FT02_LocationRoutePointFromLine(
    const char* line,
    const char* requestedSessionId,
    FT02LocationRoutePoint& output
)
{
    char sessionId[40];
    char type[32];
    if(!FT02_JsonString(line, "session_id", sessionId, sizeof(sessionId)) ||
       strcmp(sessionId, requestedSessionId) != 0 ||
       !FT02_JsonString(line, "type", type, sizeof(type)) ||
       strcmp(type, "path_point") != 0)
    {
        return false;
    }

    bool fixValid = false;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    const bool hasFix = FT02_JsonBool(line, "gnss_fix", fixValid);
    const bool hasLatitude = FT02_JsonDouble(line, "lat", latitude);
    const bool hasLongitude = FT02_JsonDouble(line, "lon", longitude);
    const bool hasAltitude = FT02_JsonDouble(line, "altitude_m", altitude);

    if(!hasFix || !fixValid || !hasLatitude || !hasLongitude ||
       !isfinite(latitude) || !isfinite(longitude) ||
       latitude < -85.0 || latitude > 85.0 ||
       longitude < -180.0 || longitude > 180.0)
    {
        return false;
    }

    output.latitude = latitude;
    output.longitude = longitude;
    output.altitudeValid = hasAltitude && isfinite(altitude);
    output.altitudeMeters = output.altitudeValid
        ? static_cast<float>(altitude)
        : 0.0f;
    return true;
}
}

static bool FT02_LocationLogLock(TickType_t waitTicks = pdMS_TO_TICKS(20))
{
    return g_ft02LocationLogDataMutex == nullptr ||
           xSemaphoreTake(g_ft02LocationLogDataMutex, waitTicks) == pdTRUE;
}

static void FT02_LocationLogUnlock()
{
    if(g_ft02LocationLogDataMutex != nullptr)
    {
        xSemaphoreGive(g_ft02LocationLogDataMutex);
    }
}

static void FT02_LocationLogCommitBuild(
    const FT02LocationLogBuildState& build,
    bool completionPending
)
{
    if(FT02_LocationLogLock(portMAX_DELAY))
    {
        const uint32_t nextGeneration = g_ft02LocationLogStatus.generation + 1U;
        memcpy(g_ft02LocationEntries, build.entries, sizeof(g_ft02LocationEntries));
        g_ft02LocationLogStatus = build.status;
        g_ft02LocationLogStatus.generation = nextGeneration;
        FT02_LocationLogUnlock();
    }
    g_ft02LocationLogCompletionPending = completionPending;
}

static void FT02_LocationLogWorkerSetMessage(
    FT02LocationLogBuildState& build,
    const char* message
)
{
    snprintf(
        build.status.message,
        sizeof(build.status.message),
        "%s",
        message != nullptr ? message : ""
    );
}

static void FT02_LocationLogReloadTask(void*)
{
    FT02LocationLogBuildState& build = g_ft02LocationLogWorkerBuild;
    memset(&build, 0, sizeof(build));
    build.status.storageReady = FT02_StorageIsReady();
    build.status.loading = true;
    FT02_LocationLogWorkerSetMessage(build, "正在后台加载定位记录...");

    bool readOk = true;
    bool cancelled = g_ft02LocationLogCancelRequested;
    FILE* file = nullptr;

    if(!cancelled && !build.status.storageReady)
    {
        readOk = false;
        FT02_LocationLogWorkerSetMessage(build, "SD未就绪，无法读取记录");
    }

    if(!cancelled && readOk)
    {
        // All potentially slow SD operations, including stat/fopen, live in
        // this worker. The Arduino main loop never waits for them.
        if(!FT02_StorageFileExists(FT02_TRACK_PATH))
        {
            build.status.fileExists = false;
            build.status.loaded = true;
            build.status.loading = false;
            FT02_LocationLogWorkerSetMessage(build, "暂无定位记录");
        }
        else
        {
            build.status.fileExists = true;
            file = FT02_StorageOpenReadFile(FT02_TRACK_PATH);
            if(file == nullptr)
            {
                readOk = false;
                FT02_LocationLogWorkerSetMessage(build, "定位记录文件打开失败");
            }
        }
    }

    if(!cancelled && readOk && file != nullptr)
    {
        char line[FT02_LOCATION_LOG_LINE_BYTES];
        uint16_t linesSinceYield = 0;
        while(!g_ft02LocationLogCancelRequested && fgets(line, sizeof(line), file) != nullptr)
        {
            build.status.lineCount++;
            FT02_LocationLogApplyLine(build, line);
            if(++linesSinceYield >= FT02_LOCATION_LOG_LINES_PER_YIELD)
            {
                linesSinceYield = 0;
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        cancelled = g_ft02LocationLogCancelRequested;
        if(!cancelled && ferror(file) != 0)
        {
            readOk = false;
        }
    }

    if(file != nullptr)
    {
        fclose(file);
        file = nullptr;
    }

    if(cancelled)
    {
        build.status.loading = false;
        build.status.loaded = false;
        FT02_LocationLogWorkerSetMessage(build, "定位记录后台加载已取消");
        Serial.println("[LOCATION-LOG] background reload cancelled");
        FT02_LocationLogCommitBuild(build, false);
    }
    else
    {
        FT02_LocationLogFinalizeDurations(build);
        build.status.loading = false;
        build.status.loaded = readOk;

        if(!readOk)
        {
            if(build.status.message[0] == '\0')
            {
                FT02_LocationLogWorkerSetMessage(build, "读取定位记录时发生错误");
            }
        }
        else if(build.status.count == 0 && build.status.message[0] == '\0')
        {
            FT02_LocationLogWorkerSetMessage(build, "暂无可显示的定位记录");
        }
        else if(build.status.truncated)
        {
            FT02_LocationLogWorkerSetMessage(build, "记录较多，仅显示最近64条");
        }
        else if(build.status.count > 0)
        {
            snprintf(
                build.status.message,
                sizeof(build.status.message),
                "已加载 %u 条行程",
                static_cast<unsigned int>(build.status.count)
            );
        }

        Serial.printf(
            "[LOCATION-LOG] background reload done sessions=%u total=%lu lines=%lu parse_errors=%lu truncated=%d ok=%d\n",
            static_cast<unsigned int>(build.status.count),
            static_cast<unsigned long>(build.status.totalSessions),
            static_cast<unsigned long>(build.status.lineCount),
            static_cast<unsigned long>(build.status.parseErrorCount),
            build.status.truncated ? 1 : 0,
            readOk ? 1 : 0
        );
        FT02_LocationLogCommitBuild(build, true);
    }

    g_ft02LocationLogCancelRequested = false;
    g_ft02LocationLogReloadTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void FT02_LocationLogBegin()
{
    FT02_LocationRouteClear(false);
    memset(&g_ft02LocationLogStatus, 0, sizeof(g_ft02LocationLogStatus));
    FT02_LocationLogClearEntries();
    if(g_ft02LocationLogDataMutex == nullptr)
    {
        g_ft02LocationLogDataMutex = xSemaphoreCreateMutex();
    }
    g_ft02LocationLogStatus.storageReady = FT02_StorageIsReady();
    FT02_LocationLogSetMessage("尚未加载定位记录");
    FT02_LocationLogChanged();
    Serial.println("[LOCATION-LOG] background session-list service ready");
}

bool FT02_LocationLogStartReload()
{
    // A reload already in flight is still useful. Do not launch a second SD
    // reader against the same JSONL file or block the UI waiting for it.
    if(g_ft02LocationLogReloadTaskHandle != nullptr || g_ft02LocationLogStatus.loading)
    {
        Serial.println("[LOCATION-LOG] background reload already active");
        return true;
    }

    FT02_LocationLogReleaseRoute();
    g_ft02LocationLogCancelRequested = false;
    g_ft02LocationLogCompletionPending = false;

    if(FT02_LocationLogLock(portMAX_DELAY))
    {
        FT02_LocationLogClearEntries();
        g_ft02LocationLogStatus.storageReady = FT02_StorageIsReady();
        g_ft02LocationLogStatus.fileExists = false;
        g_ft02LocationLogStatus.loaded = false;
        g_ft02LocationLogStatus.loading = true;
        FT02_LocationLogSetMessage("正在后台加载定位记录...");
        FT02_LocationLogChanged();
        FT02_LocationLogUnlock();
    }

    const BaseType_t created = xTaskCreatePinnedToCore(
        FT02_LocationLogReloadTask,
        "ft02-loclog",
        FT02_LOCATION_LOG_WORKER_STACK,
        nullptr,
        FT02_LOCATION_LOG_WORKER_PRIORITY,
        &g_ft02LocationLogReloadTaskHandle,
        FT02_LOCATION_LOG_WORKER_CORE
    );
    if(created != pdPASS)
    {
        g_ft02LocationLogReloadTaskHandle = nullptr;
        if(FT02_LocationLogLock(portMAX_DELAY))
        {
            g_ft02LocationLogStatus.loading = false;
            g_ft02LocationLogStatus.loaded = false;
            FT02_LocationLogSetMessage("定位记录后台任务启动失败");
            FT02_LocationLogChanged();
            FT02_LocationLogUnlock();
        }
        Serial.println("[LOCATION-LOG] background reload task create failed");
        return false;
    }

    Serial.println("[LOCATION-LOG] background reload queued");
    return true;
}

bool FT02_LocationLogPollReload()
{
    // The main loop only consumes a completion flag. No stat/fopen/fgets or
    // other SD operation is allowed on this path.
    if(!g_ft02LocationLogCompletionPending) return false;
    g_ft02LocationLogCompletionPending = false;
    return true;
}

void FT02_LocationLogCancelReload()
{
    if(g_ft02LocationLogReloadTaskHandle == nullptr && !g_ft02LocationLogStatus.loading)
    {
        return;
    }
    // Never fclose a FILE owned by the worker from the main task. A blocked SD
    // call may take time to return, but input/GNSS/UI remain responsive.
    g_ft02LocationLogCancelRequested = true;
    Serial.println("[LOCATION-LOG] background reload cancel requested");
}

bool FT02_LocationLogIsLoading()
{
    return g_ft02LocationLogStatus.loading;
}

bool FT02_LocationLogReload()
{
    if(!FT02_LocationLogStartReload()) return false;
    while(FT02_LocationLogIsLoading())
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return FT02_LocationLogStatusCurrent().loaded;
}

FT02LocationLogStatus FT02_LocationLogStatusCurrent()
{
    FT02LocationLogStatus snapshot = {};
    if(FT02_LocationLogLock(portMAX_DELAY))
    {
        snapshot = g_ft02LocationLogStatus;
        FT02_LocationLogUnlock();
    }
    return snapshot;
}

bool FT02_LocationLogGetNewest(
    uint16_t newestIndex,
    FT02LocationLogEntry& output
)
{
    memset(&output, 0, sizeof(output));
    if(!FT02_LocationLogLock(portMAX_DELAY)) return false;
    if(newestIndex >= g_ft02LocationLogStatus.count)
    {
        FT02_LocationLogUnlock();
        return false;
    }

    const uint16_t internalIndex =
        static_cast<uint16_t>(g_ft02LocationLogStatus.count - 1U - newestIndex);
    output = g_ft02LocationEntries[internalIndex];
    FT02_LocationLogUnlock();

    const FT02LocationRecorderSnapshot recorder = FT02_LocationRecorderSnapshotCurrent();
    if(recorder.sessionActive && strcmp(recorder.sessionId, output.sessionId) == 0)
    {
        output.active = true;
        output.completed = false;
        output.durationSeconds = recorder.durationSeconds;
        output.durationValid = true;
        if(recorder.pointCount > output.pointCount)
        {
            output.pointCount = recorder.pointCount;
        }
        if(recorder.failedPointCount > output.failedPointCount)
        {
            output.failedPointCount = recorder.failedPointCount;
        }
    }

    return output.valid;
}


uint16_t FT02_LocationLogWrapIndex(
    uint16_t current,
    uint16_t count,
    int32_t step
)
{
    if(count == 0) return 0;
    const int32_t modulus = static_cast<int32_t>(count);
    int32_t normalized = static_cast<int32_t>(current % count);
    int32_t delta = step % modulus;
    normalized = (normalized + delta) % modulus;
    if(normalized < 0) normalized += modulus;
    return static_cast<uint16_t>(normalized);
}

bool FT02_LocationLogLoadRoute(const char* sessionId)
{
    FT02_LocationLogReleaseRoute();
    g_ft02LocationRouteStatus.storageReady = FT02_StorageIsReady();
    snprintf(
        g_ft02LocationRouteStatus.sessionId,
        sizeof(g_ft02LocationRouteStatus.sessionId),
        "%s",
        sessionId != nullptr ? sessionId : ""
    );

    if(sessionId == nullptr || sessionId[0] == '\0')
    {
        FT02_LocationRouteSetMessage("行程编号无效");
        FT02_LocationRouteChanged();
        return false;
    }
    if(!g_ft02LocationRouteStatus.storageReady)
    {
        FT02_LocationRouteSetMessage("SD未就绪，无法读取轨迹");
        FT02_LocationRouteChanged();
        return false;
    }
    if(!FT02_StorageFileExists(FT02_TRACK_PATH))
    {
        FT02_LocationRouteSetMessage("定位记录文件不存在");
        FT02_LocationRouteChanged();
        return false;
    }

    g_ft02LocationRouteStatus.fileExists = true;
    FILE* file = FT02_StorageOpenReadFile(FT02_TRACK_PATH);
    if(file == nullptr)
    {
        FT02_LocationRouteSetMessage("轨迹文件打开失败");
        FT02_LocationRouteChanged();
        return false;
    }

    uint32_t validPointCount = 0;
    char line[FT02_LOCATION_LOG_LINE_BYTES];
    FT02LocationRoutePoint point = {};
    while(fgets(line, sizeof(line), file) != nullptr)
    {
        if(FT02_LocationRoutePointFromLine(line, sessionId, point))
        {
            if(validPointCount == 0xFFFFFFFFUL)
            {
                fclose(file);
                FT02_LocationRouteSetMessage("轨迹点数量超出范围");
                FT02_LocationRouteChanged();
                return false;
            }
            validPointCount++;
        }
    }
    const bool firstPassOk = ferror(file) == 0;
    fclose(file);
    if(!firstPassOk)
    {
        FT02_LocationRouteSetMessage("扫描轨迹文件失败");
        FT02_LocationRouteChanged();
        return false;
    }

    g_ft02LocationRouteStatus.totalPointCount = validPointCount;
    if(validPointCount == 0)
    {
        g_ft02LocationRouteStatus.loaded = true;
        FT02_LocationRouteSetMessage("本次行程没有有效定位点");
        FT02_LocationRouteChanged();
        return true;
    }

    const size_t bytes = static_cast<size_t>(validPointCount) * sizeof(FT02LocationRoutePoint);
    if(bytes / sizeof(FT02LocationRoutePoint) != validPointCount)
    {
        FT02_LocationRouteSetMessage("轨迹内存尺寸溢出");
        FT02_LocationRouteChanged();
        return false;
    }

    g_ft02LocationRoutePoints = static_cast<FT02LocationRoutePoint*>(
        FT02_LocationRouteAlloc(bytes)
    );
    if(g_ft02LocationRoutePoints == nullptr)
    {
        FT02_LocationRouteSetMessage("内存不足，无法加载全部轨迹点");
        FT02_LocationRouteChanged();
        return false;
    }

    file = FT02_StorageOpenReadFile(FT02_TRACK_PATH);
    if(file == nullptr)
    {
        FT02_LocationLogReleaseRoute();
        snprintf(
            g_ft02LocationRouteStatus.sessionId,
            sizeof(g_ft02LocationRouteStatus.sessionId),
            "%s",
            sessionId
        );
        FT02_LocationRouteSetMessage("轨迹文件重新打开失败");
        FT02_LocationRouteChanged();
        return false;
    }

    uint32_t stored = 0;
    while(fgets(line, sizeof(line), file) != nullptr)
    {
        if(!FT02_LocationRoutePointFromLine(line, sessionId, point)) continue;
        if(stored >= validPointCount)
        {
            g_ft02LocationRouteStatus.parseErrorCount++;
            break;
        }
        g_ft02LocationRoutePoints[stored] = point;
        if(stored == 0)
        {
            g_ft02LocationRouteStatus.minLatitude = point.latitude;
            g_ft02LocationRouteStatus.maxLatitude = point.latitude;
            g_ft02LocationRouteStatus.minLongitude = point.longitude;
            g_ft02LocationRouteStatus.maxLongitude = point.longitude;
        }
        else
        {
            if(point.latitude < g_ft02LocationRouteStatus.minLatitude)
                g_ft02LocationRouteStatus.minLatitude = point.latitude;
            if(point.latitude > g_ft02LocationRouteStatus.maxLatitude)
                g_ft02LocationRouteStatus.maxLatitude = point.latitude;
            if(point.longitude < g_ft02LocationRouteStatus.minLongitude)
                g_ft02LocationRouteStatus.minLongitude = point.longitude;
            if(point.longitude > g_ft02LocationRouteStatus.maxLongitude)
                g_ft02LocationRouteStatus.maxLongitude = point.longitude;
        }
        stored++;
    }
    const bool secondPassOk = ferror(file) == 0;
    fclose(file);

    if(!secondPassOk || stored != validPointCount)
    {
        const uint32_t expected = validPointCount;
        FT02_LocationLogReleaseRoute();
        snprintf(
            g_ft02LocationRouteStatus.sessionId,
            sizeof(g_ft02LocationRouteStatus.sessionId),
            "%s",
            sessionId
        );
        g_ft02LocationRouteStatus.totalPointCount = expected;
        FT02_LocationRouteSetMessage("轨迹文件在读取期间发生变化");
        FT02_LocationRouteChanged();
        return false;
    }

    g_ft02LocationRouteStatus.count = stored;
    g_ft02LocationRouteStatus.loaded = true;
    g_ft02LocationRouteStatus.hasBounds = stored > 0;
    snprintf(
        g_ft02LocationRouteStatus.message,
        sizeof(g_ft02LocationRouteStatus.message),
        "已加载全部 %lu 个有效定位点",
        static_cast<unsigned long>(stored)
    );
    Serial.printf(
        "[LOCATION-LOG] route loaded session=%s points=%lu bounds=%.7f,%.7f..%.7f,%.7f\n",
        sessionId,
        static_cast<unsigned long>(stored),
        g_ft02LocationRouteStatus.minLatitude,
        g_ft02LocationRouteStatus.minLongitude,
        g_ft02LocationRouteStatus.maxLatitude,
        g_ft02LocationRouteStatus.maxLongitude
    );
    FT02_LocationRouteChanged();
    return true;
}

void FT02_LocationLogReleaseRoute()
{
    FT02_LocationRouteClear(true);
    FT02_LocationRouteSetMessage("轨迹尚未加载");
    FT02_LocationRouteChanged();
}

FT02LocationRouteStatus FT02_LocationLogRouteStatusCurrent()
{
    return g_ft02LocationRouteStatus;
}

const FT02LocationRoutePoint* FT02_LocationLogRoutePoints()
{
    return g_ft02LocationRoutePoints;
}

bool FT02_LocationLogDeleteSession(const char* sessionId)
{
    if(sessionId == nullptr || sessionId[0] == '\0')
    {
        FT02_LocationLogSetMessage("删除失败：行程编号无效");
        FT02_LocationLogChanged();
        return false;
    }

    const FT02LocationRecorderSnapshot recorder = FT02_LocationRecorderSnapshotCurrent();
    if(recorder.sessionActive && strcmp(recorder.sessionId, sessionId) == 0)
    {
        FT02_LocationLogSetMessage("记录进行中，不能删除");
        FT02_LocationLogChanged();
        return false;
    }

    if(!FT02_StorageIsReady() || !FT02_StorageFileExists(FT02_TRACK_PATH))
    {
        FT02_LocationLogSetMessage("删除失败：记录文件不可用");
        FT02_LocationLogChanged();
        return false;
    }

    FT02_StorageDeleteFile(FT02_TRACK_TEMP_PATH);
    FT02_StorageDeleteFile(FT02_TRACK_BACKUP_PATH);

    FILE* source = FT02_StorageOpenReadFile(FT02_TRACK_PATH);
    FILE* target = FT02_StorageOpenWriteFile(FT02_TRACK_TEMP_PATH, true);
    if(source == nullptr || target == nullptr)
    {
        if(source != nullptr) fclose(source);
        if(target != nullptr) fclose(target);
        FT02_StorageDeleteFile(FT02_TRACK_TEMP_PATH);
        FT02_LocationLogSetMessage("删除失败：无法创建临时文件");
        FT02_LocationLogChanged();
        return false;
    }

    uint32_t removedLines = 0;
    bool copyOk = true;
    char line[FT02_LOCATION_LOG_LINE_BYTES];
    while(fgets(line, sizeof(line), source) != nullptr)
    {
        if(FT02_LineBelongsToSession(line, sessionId))
        {
            removedLines++;
            continue;
        }
        if(fputs(line, target) < 0)
        {
            copyOk = false;
            break;
        }
    }

    if(ferror(source) != 0) copyOk = false;
    if(copyOk && !FT02_StorageSyncFile(target)) copyOk = false;
    fclose(source);
    fclose(target);

    if(!copyOk || removedLines == 0)
    {
        FT02_StorageDeleteFile(FT02_TRACK_TEMP_PATH);
        FT02_LocationLogSetMessage(
            removedLines == 0 ? "删除失败：未找到该行程" : "删除失败：重写记录文件失败"
        );
        FT02_LocationLogChanged();
        return false;
    }

    if(!FT02_StorageRenameFile(FT02_TRACK_PATH, FT02_TRACK_BACKUP_PATH))
    {
        FT02_StorageDeleteFile(FT02_TRACK_TEMP_PATH);
        FT02_LocationLogSetMessage("删除失败：无法备份原记录");
        FT02_LocationLogChanged();
        return false;
    }

    if(!FT02_StorageRenameFile(FT02_TRACK_TEMP_PATH, FT02_TRACK_PATH))
    {
        FT02_StorageRenameFile(FT02_TRACK_BACKUP_PATH, FT02_TRACK_PATH);
        FT02_StorageDeleteFile(FT02_TRACK_TEMP_PATH);
        FT02_LocationLogSetMessage("删除失败：原记录已恢复");
        FT02_LocationLogChanged();
        return false;
    }

    FT02_StorageDeleteFile(FT02_TRACK_BACKUP_PATH);
    Serial.printf(
        "[LOCATION-LOG] deleted session=%s lines=%lu\n",
        sessionId,
        static_cast<unsigned long>(removedLines)
    );

    // Rebuild the list incrementally. Deletion itself is complete at this
    // point, so do not trap the UI in a second whole-file synchronous scan.
    const bool reloadStarted = FT02_LocationLogStartReload();
    if(reloadStarted && !FT02_LocationLogIsLoading())
    {
        FT02_LocationLogSetMessage("定位记录已删除");
        FT02_LocationLogChanged();
    }
    return reloadStarted;
}
