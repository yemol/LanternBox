#include "FT02_Gnss.h"

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

namespace
{
FT02GnssSnapshot g_nav = {};
uint32_t g_startedMs = 0;
uint32_t g_lastNavMs = 0;
uint32_t g_lastFixMs = 0;
uint32_t g_lastTimeSyncMs = 0;
uint32_t g_lastUnixApplied = 0;

void markChanged() { ++g_nav.uiGeneration; }

void clearTimeStrings()
{
    snprintf(g_nav.utcTime, sizeof(g_nav.utcTime), "--:--:--");
    snprintf(g_nav.utcDate, sizeof(g_nav.utcDate), "--/--/--");
    snprintf(g_nav.localTime, sizeof(g_nav.localTime), "--:--:--");
    snprintf(g_nav.localDate, sizeof(g_nav.localDate), "--/--/--");
}

void updateTimeStrings(uint32_t unixTime)
{
    time_t t = static_cast<time_t>(unixTime);
    struct tm utc = {};
    struct tm local = {};
    gmtime_r(&t, &utc);
    localtime_r(&t, &local);
    snprintf(g_nav.utcTime, sizeof(g_nav.utcTime), "%02d:%02d:%02d", utc.tm_hour, utc.tm_min, utc.tm_sec);
    snprintf(g_nav.utcDate, sizeof(g_nav.utcDate), "%02d/%02d/%02d", utc.tm_mday, utc.tm_mon + 1, (utc.tm_year + 1900) % 100);
    snprintf(g_nav.localTime, sizeof(g_nav.localTime), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    snprintf(g_nav.localDate, sizeof(g_nav.localDate), "%02d/%02d/%02d", local.tm_mday, local.tm_mon + 1, (local.tm_year + 1900) % 100);
}

void maybeSetSystemClock(uint32_t unixTime)
{
    if(unixTime < 1600000000u) return;
    if(g_lastUnixApplied == unixTime) return;
    struct timeval tv = {};
    tv.tv_sec = static_cast<time_t>(unixTime);
    tv.tv_usec = 0;
    if(settimeofday(&tv, nullptr) == 0)
    {
        g_lastUnixApplied = unixTime;
        g_nav.systemTimeSynchronized = true;
        ++g_nav.timeSyncCount;
        g_lastTimeSyncMs = millis();
    }
}
}

void FT02_GnssBegin()
{
    const uint32_t generation = g_nav.uiGeneration;
    memset(&g_nav, 0, sizeof(g_nav));
    g_nav.uiGeneration = generation;
    clearTimeStrings();
    setenv("TZ", "CST-8", 1);
    tzset();
    g_nav.started = true;
    g_startedMs = millis();
    g_lastNavMs = 0;
    g_lastFixMs = 0;
    g_lastTimeSyncMs = 0;
    markChanged();
    Serial.println("[NAV] source=LR01 only; Core GNSS UART/parser absent");
}

void FT02_GnssPoll()
{
    if(!g_nav.started) return;
    const uint32_t now = millis();
    g_nav.startAgeMs = now - g_startedMs;
    g_nav.lastFixAgeMs = g_lastFixMs == 0 ? 0xFFFFFFFFu : now - g_lastFixMs;
    g_nav.lastTimeSyncAgeMs = g_lastTimeSyncMs == 0 ? 0xFFFFFFFFu : now - g_lastTimeSyncMs;

    // NAV_STATE is expected every ~1 s. If it becomes stale, invalidate the
    // position but do not infer that the GNSS hardware itself is disconnected;
    // SYSTEM_STATE gnss= is authoritative for that.
    if(g_nav.fixValid && g_lastNavMs != 0 && now - g_lastNavMs > 3500u)
    {
        g_nav.fixValid = false;
        g_nav.hasPosition = false;
        g_nav.altitudeValid = false;
        g_nav.speedValid = false;
        markChanged();
    }
}

FT02GnssSnapshot FT02_GnssSnapshotCurrent()
{
    FT02GnssSnapshot out = g_nav;
    const uint32_t now = millis();
    out.startAgeMs = now - g_startedMs;
    out.lastFixAgeMs = g_lastFixMs == 0 ? 0xFFFFFFFFu : now - g_lastFixMs;
    out.lastTimeSyncAgeMs = g_lastTimeSyncMs == 0 ? 0xFFFFFFFFu : now - g_lastTimeSyncMs;
    return out;
}

void FT02_GnssApplyLR01State(bool gnssReady,
                             bool fix,
                             uint8_t fixType,
                             int32_t latitudeE7,
                             int32_t longitudeE7,
                             int16_t altitudeDm,
                             uint8_t satellitesUsed,
                             uint8_t satellitesVisible,
                             uint16_t hdopX100,
                             uint16_t speedCms,
                             uint16_t headingX10,
                             bool compassValid,
                             uint8_t compassQuality,
                             uint32_t gnssBytes,
                             uint32_t unixTime,
                             bool timeValid)
{
    (void)compassQuality; // quality lives in FT02LR01State; snapshot keeps the established UI shape.
    const uint32_t now = millis();
    const double lat = static_cast<double>(latitudeE7) / 10000000.0;
    const double lon = static_cast<double>(longitudeE7) / 10000000.0;
    const bool validPos = fix && latitudeE7 != 0 && longitudeE7 != 0 &&
                          lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
    bool changed = false;
#define SET_FIELD(field, value) do { auto _v=(value); if(g_nav.field != _v) { g_nav.field=_v; changed=true; } } while(0)
    SET_FIELD(communicationActive, gnssReady);
    SET_FIELD(bytesReceived, gnssBytes);
    SET_FIELD(fixValid, fix);
    SET_FIELD(hasPosition, validPos);
    SET_FIELD(fixType, fixType);
    SET_FIELD(fixQuality, fixType);
    SET_FIELD(satellites, satellitesUsed);
    SET_FIELD(satellitesVisible, satellitesVisible);
    const float hdop = static_cast<float>(hdopX100) / 100.0f;
    if(fabsf(g_nav.hdop - hdop) > 0.005f) { g_nav.hdop=hdop; changed=true; }
    if(validPos && (fabs(g_nav.latitude-lat)>1e-8 || fabs(g_nav.longitude-lon)>1e-8)) { g_nav.latitude=lat; g_nav.longitude=lon; changed=true; }
    const float altitude = static_cast<float>(altitudeDm) / 10.0f;
    SET_FIELD(altitudeValid, fix);
    if(fabsf(g_nav.altitudeMeters-altitude)>0.05f) { g_nav.altitudeMeters=altitude; g_nav.rawAltitudeMeters=altitude; changed=true; }
    const float speedKmh = static_cast<float>(speedCms) * 0.036f;
    SET_FIELD(speedValid, fix);
    SET_FIELD(moving, fix && speedCms >= 20u);
    if(fabsf(g_nav.speedKmh-speedKmh)>0.05f) { g_nav.speedKmh=speedKmh; g_nav.rawSpeedKmh=speedKmh; changed=true; }
    const float heading = static_cast<float>(headingX10) / 10.0f;
    SET_FIELD(courseValid, compassValid);
    if(fabsf(g_nav.courseDegrees-heading)>0.05f) { g_nav.courseDegrees=heading; changed=true; }
    SET_FIELD(timeValid, timeValid && unixTime >= 1600000000u);
#undef SET_FIELD

    g_lastNavMs = now;
    if(fix) g_lastFixMs = now;
    if(timeValid && unixTime >= 1600000000u)
    {
        updateTimeStrings(unixTime);
        maybeSetSystemClock(unixTime);
    }
    if(changed) markChanged();
}
