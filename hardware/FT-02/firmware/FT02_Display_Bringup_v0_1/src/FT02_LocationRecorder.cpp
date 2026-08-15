#include "FT02_LocationRecorder.h"

#include <stdio.h>
#include <string.h>
#include <esp_system.h>

#include "FT02_Storage.h"
#include "FT02_BuildInfo.h"

namespace
{
constexpr const char* FT02_TRACK_PATH = "/lanternbox/tracks/path_points.jsonl";
constexpr const char* FT02_DEVICE_ID = "FT-02A";
constexpr uint32_t FT02_AUTO_TRACK_INTERVAL_MS = 30000UL;

FT02LocationRecorderSnapshot g_ft02Recorder = {};
uint32_t g_ft02AutoLastAttemptMs = 0;
bool g_ft02AutoWaitingForFix = false;

static void FT02_RecorderChanged()
{
    g_ft02Recorder.uiGeneration++;
}

static void FT02_RecorderSetAction(const char* text)
{
    snprintf(
        g_ft02Recorder.lastAction,
        sizeof(g_ft02Recorder.lastAction),
        "%s",
        text != nullptr ? text : ""
    );
    FT02_RecorderChanged();
}

static bool FT02_RecorderLeapYear(int year)
{
    if((year % 400) == 0) return true;
    if((year % 100) == 0) return false;
    return (year % 4) == 0;
}

static int FT02_RecorderDaysInMonth(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if(month == 2 && FT02_RecorderLeapYear(year)) return 29;
    if(month < 1 || month > 12) return 31;
    return days[month - 1];
}

static bool FT02_RecorderParseUtc(
    const FT02GnssSnapshot& gnss,
    int& year,
    int& month,
    int& day,
    int& hour,
    int& minute,
    int& second
)
{
    if(strlen(gnss.utcDate) < 8 || strlen(gnss.utcTime) < 8) return false;
    if(gnss.utcDate[2] != '/' || gnss.utcDate[5] != '/') return false;
    if(gnss.utcTime[2] != ':' || gnss.utcTime[5] != ':') return false;

    day = (gnss.utcDate[0] - '0') * 10 + (gnss.utcDate[1] - '0');
    month = (gnss.utcDate[3] - '0') * 10 + (gnss.utcDate[4] - '0');
    year = 2000 + (gnss.utcDate[6] - '0') * 10 + (gnss.utcDate[7] - '0');
    hour = (gnss.utcTime[0] - '0') * 10 + (gnss.utcTime[1] - '0');
    minute = (gnss.utcTime[3] - '0') * 10 + (gnss.utcTime[4] - '0');
    second = (gnss.utcTime[6] - '0') * 10 + (gnss.utcTime[7] - '0');

    if(year < 2020 || month < 1 || month > 12 || day < 1 ||
       day > FT02_RecorderDaysInMonth(year, month) ||
       hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
       second < 0 || second > 59)
    {
        return false;
    }

    return true;
}

static bool FT02_RecorderLocalDateTime(
    const FT02GnssSnapshot& gnss,
    char* dateText,
    size_t dateSize,
    char* timeText,
    size_t timeSize
)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if(!FT02_RecorderParseUtc(gnss, year, month, day, hour, minute, second))
    {
        snprintf(dateText, dateSize, "unknown");
        snprintf(timeText, timeSize, "uptime-%lu", (unsigned long)(millis() / 1000UL));
        return false;
    }

    hour += 8;
    if(hour >= 24)
    {
        hour -= 24;
        day++;
        if(day > FT02_RecorderDaysInMonth(year, month))
        {
            day = 1;
            month++;
            if(month > 12)
            {
                month = 1;
                year++;
            }
        }
    }

    snprintf(dateText, dateSize, "%04d-%02d-%02d", year, month, day);
    snprintf(timeText, timeSize, "%02d:%02d:%02d", hour, minute, second);
    return true;
}

static void FT02_RecorderBuildSessionId(const FT02GnssSnapshot& gnss)
{
    char dateText[16];
    char timeText[20];
    const bool hasTime = FT02_RecorderLocalDateTime(
        gnss,
        dateText,
        sizeof(dateText),
        timeText,
        sizeof(timeText)
    );

    if(hasTime)
    {
        snprintf(
            g_ft02Recorder.sessionId,
            sizeof(g_ft02Recorder.sessionId),
            "FT02-%c%c%c%c%c%c%c%c-%c%c%c%c%c%c",
            dateText[0], dateText[1], dateText[2], dateText[3],
            dateText[5], dateText[6], dateText[8], dateText[9],
            timeText[0], timeText[1], timeText[3], timeText[4],
            timeText[6], timeText[7]
        );
        return;
    }

    const uint32_t randomPart = esp_random() & 0xFFFFU;
    snprintf(
        g_ft02Recorder.sessionId,
        sizeof(g_ft02Recorder.sessionId),
        "FT02-U%010lu-%04lX",
        (unsigned long)(millis() / 1000UL),
        (unsigned long)randomPart
    );
}

static bool FT02_RecorderWriteEvent(
    const char* type,
    const char* note,
    const FT02GnssSnapshot& gnss
)
{
    char deviceDate[16];
    char deviceTime[20];
    FT02_RecorderLocalDateTime(
        gnss,
        deviceDate,
        sizeof(deviceDate),
        deviceTime,
        sizeof(deviceTime)
    );

    char line[896];
    const double latitude = gnss.fixValid ? gnss.latitude : 0.0;
    const double longitude = gnss.fixValid ? gnss.longitude : 0.0;
    const float altitude = (gnss.fixValid && gnss.altitudeValid) ? gnss.altitudeMeters : 0.0f;
    const float hdop = gnss.hdop > 0.0f ? gnss.hdop : 0.0f;

    const int written = snprintf(
        line,
        sizeof(line),
        "{\"type\":\"%s\",\"device_id\":\"%s\",\"version\":\"%s\","
        "\"session_id\":\"%s\",\"device_date\":\"%s\",\"device_time\":\"%s\","
        "\"gnss_fix\":%s,\"fix_type\":%u,\"fix_quality\":%u,\"satellites\":%u,"
        "\"hdop\":%.1f,\"lat\":%.6f,\"lon\":%.6f,\"altitude_m\":%.1f,"
        "\"speed_kmh\":%.1f,\"course_deg\":%.1f,"
        "\"gnss_utc_time\":\"%s\",\"gnss_utc_date\":\"%s\","
        "\"timezone\":\"UTC+8\",\"note\":\"%s\"}",
        type,
        FT02_DEVICE_ID,
        FT02_FIRMWARE_VERSION,
        g_ft02Recorder.sessionId,
        deviceDate,
        deviceTime,
        gnss.fixValid ? "true" : "false",
        (unsigned int)gnss.fixType,
        (unsigned int)gnss.fixQuality,
        (unsigned int)gnss.satellites,
        hdop,
        latitude,
        longitude,
        altitude,
        gnss.speedKmh,
        gnss.courseDegrees,
        gnss.utcTime,
        gnss.utcDate,
        note != nullptr ? note : ""
    );

    if(written <= 0 || static_cast<size_t>(written) >= sizeof(line))
    {
        g_ft02Recorder.writeFailureCount++;
        FT02_RecorderSetAction("记录失败：数据过长");
        return false;
    }

    if(!FT02_StorageAppendLine(FT02_TRACK_PATH, line))
    {
        g_ft02Recorder.writeFailureCount++;
        FT02_RecorderSetAction("记录失败：SD不可写");
        Serial.printf("[RECORDER] append failed type=%s path=%s\n", type, FT02_TRACK_PATH);
        return false;
    }

    Serial.printf(
        "[RECORDER] wrote type=%s session=%s fix=%d sats=%u note=%s\n",
        type,
        g_ft02Recorder.sessionId,
        gnss.fixValid ? 1 : 0,
        (unsigned int)gnss.satellites,
        note != nullptr ? note : ""
    );
    return true;
}

static bool FT02_RecorderRequireActive()
{
    if(g_ft02Recorder.sessionActive) return true;
    FT02_RecorderSetAction("请先按 ENTER 开始记录");
    return false;
}

static bool FT02_RecorderTryRecordOrigin(const FT02GnssSnapshot& gnss)
{
    if(!g_ft02Recorder.sessionActive || g_ft02Recorder.originRecorded || !gnss.fixValid)
    {
        return false;
    }

    if(!FT02_RecorderWriteEvent("session_origin", "first valid fix", gnss))
    {
        return false;
    }

    g_ft02Recorder.originRecorded = true;
    FT02_RecorderSetAction("已记录行程起点");
    return true;
}

static bool FT02_RecorderRecordPoint(const char* note, bool recordFailureEvent)
{
    if(!FT02_RecorderRequireActive()) return false;

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    if(!gnss.fixValid)
    {
        g_ft02Recorder.failedPointCount++;
        if(recordFailureEvent)
        {
            FT02_RecorderWriteEvent("path_point_failed", "no gnss fix", gnss);
        }
        FT02_RecorderSetAction("记点失败：尚未定位");
        return false;
    }

    if(!g_ft02Recorder.originRecorded && !FT02_RecorderTryRecordOrigin(gnss))
    {
        return false;
    }

    if(!FT02_RecorderWriteEvent("path_point", note, gnss)) return false;

    g_ft02Recorder.pointCount++;
    if(strcmp(note, "manual") == 0)
    {
        FT02_RecorderSetAction("已保存手动路径点");
    }
    else if(strcmp(note, "auto_start") == 0)
    {
        FT02_RecorderSetAction("自动记录已开始");
    }
    else
    {
        FT02_RecorderSetAction("已保存自动路径点");
    }
    return true;
}
}

void FT02_LocationRecorderBegin()
{
    memset(&g_ft02Recorder, 0, sizeof(g_ft02Recorder));
    g_ft02Recorder.autoIntervalSeconds = FT02_AUTO_TRACK_INTERVAL_MS / 1000UL;
    g_ft02Recorder.storageReady = FT02_StorageIsReady();
    snprintf(g_ft02Recorder.sessionId, sizeof(g_ft02Recorder.sessionId), "--");
    snprintf(g_ft02Recorder.lastAction, sizeof(g_ft02Recorder.lastAction), "按 ENTER 开始路径记录");
    g_ft02AutoLastAttemptMs = 0;
    g_ft02AutoWaitingForFix = false;
    FT02_RecorderChanged();
    Serial.println("[RECORDER] location recorder A2 ready path=/lanternbox/tracks/path_points.jsonl");
}

void FT02_LocationRecorderPoll()
{
    const bool storageReady = FT02_StorageIsReady();
    if(storageReady != g_ft02Recorder.storageReady)
    {
        g_ft02Recorder.storageReady = storageReady;
        FT02_RecorderChanged();
    }

    if(!g_ft02Recorder.sessionActive) return;

    const uint32_t now = millis();
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();

    if(!g_ft02Recorder.originRecorded && gnss.fixValid)
    {
        FT02_RecorderTryRecordOrigin(gnss);
    }

    if(!g_ft02Recorder.autoTrackEnabled) return;

    if(g_ft02AutoWaitingForFix && gnss.fixValid)
    {
        g_ft02AutoWaitingForFix = false;
        g_ft02AutoLastAttemptMs = now;
        FT02_RecorderRecordPoint(
            g_ft02Recorder.pointCount == 0 ? "auto_start" : "auto",
            false
        );
        return;
    }

    if(now - g_ft02AutoLastAttemptMs < FT02_AUTO_TRACK_INTERVAL_MS) return;

    g_ft02AutoLastAttemptMs = now;
    if(!gnss.fixValid)
    {
        g_ft02AutoWaitingForFix = true;
        FT02_RecorderSetAction("自动记录暂停：等待定位");
        return;
    }

    FT02_RecorderRecordPoint("auto", false);
}

bool FT02_LocationRecorderStartSession()
{
    if(g_ft02Recorder.sessionActive) return true;

    if(!FT02_StorageIsReady())
    {
        FT02_RecorderSetAction("无法开始：SD未就绪");
        return false;
    }

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    FT02_RecorderBuildSessionId(gnss);

    g_ft02Recorder.sessionActive = true;
    g_ft02Recorder.autoTrackEnabled = false;
    g_ft02Recorder.originRecorded = false;
    g_ft02Recorder.sessionStartedMs = millis();
    g_ft02Recorder.durationSeconds = 0;
    g_ft02Recorder.pointCount = 0;
    g_ft02Recorder.failedPointCount = 0;
    g_ft02AutoLastAttemptMs = millis();
    g_ft02AutoWaitingForFix = false;

    if(!FT02_RecorderWriteEvent("session_start", "manual start", gnss))
    {
        g_ft02Recorder.sessionActive = false;
        snprintf(g_ft02Recorder.sessionId, sizeof(g_ft02Recorder.sessionId), "--");
        return false;
    }

    if(gnss.fixValid)
    {
        FT02_RecorderTryRecordOrigin(gnss);
    }
    else
    {
        FT02_RecorderSetAction("记录已开始：等待起点");
    }
    return true;
}

bool FT02_LocationRecorderStopSession(const char* note)
{
    if(!g_ft02Recorder.sessionActive) return true;

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();

    if(!g_ft02Recorder.originRecorded && gnss.fixValid)
    {
        FT02_RecorderTryRecordOrigin(gnss);
    }

    if(g_ft02Recorder.autoTrackEnabled)
    {
        FT02_RecorderWriteEvent("auto_track_off", "session stop", gnss);
        g_ft02Recorder.autoTrackEnabled = false;
        g_ft02AutoWaitingForFix = false;
    }

    const bool ok = FT02_RecorderWriteEvent(
        "session_stop",
        note != nullptr ? note : "manual stop",
        gnss
    );

    g_ft02Recorder.durationSeconds = g_ft02Recorder.sessionStartedMs > 0
        ? (millis() - g_ft02Recorder.sessionStartedMs) / 1000UL
        : 0;
    g_ft02Recorder.sessionActive = false;
    g_ft02Recorder.sessionStartedMs = 0;
    FT02_RecorderSetAction(ok ? "路径记录已停止" : "停止时写入失败");
    return ok;
}

bool FT02_LocationRecorderToggleSession()
{
    if(g_ft02Recorder.sessionActive)
    {
        return FT02_LocationRecorderStopSession("manual stop");
    }
    return FT02_LocationRecorderStartSession();
}

bool FT02_LocationRecorderRecordManualPoint()
{
    return FT02_RecorderRecordPoint("manual", true);
}

bool FT02_LocationRecorderToggleAutoTrack()
{
    if(!FT02_RecorderRequireActive()) return false;

    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();

    if(g_ft02Recorder.autoTrackEnabled)
    {
        if(!FT02_RecorderWriteEvent("auto_track_off", "toggle", gnss)) return false;
        g_ft02Recorder.autoTrackEnabled = false;
        g_ft02AutoWaitingForFix = false;
        FT02_RecorderSetAction("自动记录已关闭");
        return true;
    }

    if(!FT02_RecorderWriteEvent("auto_track_on", "toggle", gnss)) return false;

    g_ft02Recorder.autoTrackEnabled = true;
    g_ft02AutoLastAttemptMs = millis();
    g_ft02AutoWaitingForFix = !gnss.fixValid;

    if(gnss.fixValid)
    {
        return FT02_RecorderRecordPoint("auto_start", false);
    }

    FT02_RecorderSetAction("自动记录开启：等待定位");
    return true;
}

FT02LocationRecorderSnapshot FT02_LocationRecorderSnapshotCurrent()
{
    FT02LocationRecorderSnapshot snapshot = g_ft02Recorder;
    const uint32_t now = millis();

    if(snapshot.sessionActive && snapshot.sessionStartedMs > 0)
    {
        snapshot.durationSeconds = (now - snapshot.sessionStartedMs) / 1000UL;
    }

    if(snapshot.sessionActive && snapshot.autoTrackEnabled)
    {
        const uint32_t elapsed = now - g_ft02AutoLastAttemptMs;
        snapshot.nextAutoSeconds = elapsed >= FT02_AUTO_TRACK_INTERVAL_MS
            ? 0
            : (FT02_AUTO_TRACK_INTERVAL_MS - elapsed + 999UL) / 1000UL;
    }
    else
    {
        snapshot.nextAutoSeconds = 0;
    }

    return snapshot;
}
