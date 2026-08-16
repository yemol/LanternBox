#include "FT02_Gnss.h"

#include <HardwareSerial.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
constexpr int FT02_GNSS_RX_PIN = 39; // ESP32 RX <- GNSS TX
constexpr int FT02_GNSS_TX_PIN = 38; // ESP32 TX -> GNSS RX
constexpr uint32_t FT02_GNSS_BAUD = 38400;
constexpr uint32_t FT02_GNSS_LINK_STALE_MS = 5000;
constexpr uint32_t FT02_GNSS_FIX_STALE_MS = 6000;
constexpr uint32_t FT02_GNSS_GSV_STALE_MS = 10000;
constexpr size_t FT02_GNSS_GSV_TALKER_SLOTS = 8;
constexpr uint32_t FT02_GNSS_CLOCK_RESYNC_MS = 10UL * 60UL * 1000UL;
constexpr size_t FT02_GNSS_LINE_CAPACITY = 160;
constexpr size_t FT02_GNSS_FILTER_SAMPLES = 5;
constexpr float FT02_GNSS_ALTITUDE_MAX_HDOP = 2.5f;
constexpr uint8_t FT02_GNSS_ALTITUDE_MIN_SATELLITES = 6;
constexpr float FT02_GNSS_POSITION_DEADBAND_METERS = 3.0f;
constexpr float FT02_GNSS_MOVE_ENTER_KMH = 1.2f;
constexpr float FT02_GNSS_MOVE_EXIT_KMH = 0.6f;
constexpr uint8_t FT02_GNSS_MOVE_ENTER_SAMPLES = 3;
constexpr uint8_t FT02_GNSS_MOVE_EXIT_SAMPLES = 5;
constexpr float FT02_GNSS_COURSE_MIN_KMH = 2.0f;
constexpr double FT02_GNSS_EARTH_RADIUS_METERS = 6371000.0;

HardwareSerial g_ft02GnssSerial(1);
FT02GnssSnapshot g_ft02Gnss = {};
size_t g_ft02GnssLineLength = 0;
char g_ft02GnssLine[FT02_GNSS_LINE_CAPACITY] = {};
uint32_t g_ft02GnssStartedMs = 0;
uint32_t g_ft02GnssLastByteMs = 0;
uint32_t g_ft02GnssLastSentenceMs = 0;
uint32_t g_ft02GnssLastFixMs = 0;
uint32_t g_ft02GnssLastTimeSyncMs = 0;

struct FT02GnssGsvTalkerState
{
    char talker[3];
    uint8_t satellitesVisible;
    uint32_t lastSeenMs;
};

FT02GnssGsvTalkerState g_ft02GnssGsvTalkers[FT02_GNSS_GSV_TALKER_SLOTS] = {};

float g_ft02AltitudeSamples[FT02_GNSS_FILTER_SAMPLES] = {};
size_t g_ft02AltitudeSampleCount = 0;
size_t g_ft02AltitudeSampleCursor = 0;
float g_ft02SpeedSamples[FT02_GNSS_FILTER_SAMPLES] = {};
size_t g_ft02SpeedSampleCount = 0;
size_t g_ft02SpeedSampleCursor = 0;
float g_ft02FilteredSpeedKmh = 0.0f;
uint8_t g_ft02MoveEnterCount = 0;
uint8_t g_ft02MoveExitCount = 0;

static uint8_t FT02_GnssHexNibble(char value)
{
    if(value >= '0' && value <= '9') return (uint8_t)(value - '0');
    if(value >= 'A' && value <= 'F') return (uint8_t)(value - 'A' + 10);
    if(value >= 'a' && value <= 'f') return (uint8_t)(value - 'a' + 10);
    return 0xFF;
}

static bool FT02_GnssChecksumValid(const char* line)
{
    if(line == nullptr || line[0] != '$') return false;

    const char* star = strchr(line, '*');
    if(star == nullptr || star[1] == 0 || star[2] == 0) return false;

    const uint8_t expectedHigh = FT02_GnssHexNibble(star[1]);
    const uint8_t expectedLow = FT02_GnssHexNibble(star[2]);
    if(expectedHigh > 0x0F || expectedLow > 0x0F) return false;

    uint8_t actual = 0;
    for(const char* cursor = line + 1; cursor < star; cursor++)
    {
        actual ^= (uint8_t)(*cursor);
    }

    const uint8_t expected = (uint8_t)((expectedHigh << 4) | expectedLow);
    return actual == expected;
}

static double FT02_GnssParseCoordinate(const char* text, const char* hemisphere)
{
    if(text == nullptr || text[0] == 0) return 0.0;

    const double raw = atof(text);
    const int degrees = (int)(raw / 100.0);
    const double minutes = raw - (double)degrees * 100.0;
    double value = (double)degrees + minutes / 60.0;

    if(hemisphere != nullptr &&
       (hemisphere[0] == 'S' || hemisphere[0] == 'W'))
    {
        value = -value;
    }

    return value;
}

static int FT02_GnssSplitFields(char* line, char* fields[], int capacity)
{
    int count = 0;
    char* cursor = line;

    while(cursor != nullptr && count < capacity)
    {
        fields[count++] = cursor;
        char* comma = strchr(cursor, ',');
        if(comma == nullptr) break;
        *comma = 0;
        cursor = comma + 1;
    }

    return count;
}

static bool FT02_GnssAllDigits(const char* text, size_t count)
{
    if(text == nullptr || strlen(text) < count) return false;
    for(size_t i = 0; i < count; i++)
    {
        if(text[i] < '0' || text[i] > '9') return false;
    }
    return true;
}

static bool FT02_GnssLeapYear(int year)
{
    if((year % 400) == 0) return true;
    if((year % 100) == 0) return false;
    return (year % 4) == 0;
}

static int FT02_GnssDaysInMonth(int year, int month)
{
    static const int days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    if(month < 1 || month > 12) return 0;
    if(month == 2 && FT02_GnssLeapYear(year)) return 29;
    return days[month - 1];
}

// Howard Hinnant's civil-date conversion, returning days since 1970-01-01.
static int64_t FT02_GnssDaysFromCivil(int year, unsigned month, unsigned day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned adjustedMonth = month > 2 ? month - 3U : month + 9U;
    const unsigned doy = (153U * adjustedMonth + 2U) / 5U + day - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return (int64_t)era * 146097LL + (int64_t)doe - 719468LL;
}

static bool FT02_GnssParseUtcEpoch(
    const char* rawTime,
    const char* rawDate,
    time_t& utcEpoch
)
{
    if(!FT02_GnssAllDigits(rawTime, 6) || !FT02_GnssAllDigits(rawDate, 6))
    {
        return false;
    }

    const int hour = (rawTime[0] - '0') * 10 + (rawTime[1] - '0');
    const int minute = (rawTime[2] - '0') * 10 + (rawTime[3] - '0');
    const int second = (rawTime[4] - '0') * 10 + (rawTime[5] - '0');
    const int day = (rawDate[0] - '0') * 10 + (rawDate[1] - '0');
    const int month = (rawDate[2] - '0') * 10 + (rawDate[3] - '0');
    const int year = 2000 + (rawDate[4] - '0') * 10 + (rawDate[5] - '0');

    if(year < 2020 || year > 2099 || month < 1 || month > 12 ||
       day < 1 || day > FT02_GnssDaysInMonth(year, month) ||
       hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
       second < 0 || second > 60)
    {
        return false;
    }

    const int64_t days = FT02_GnssDaysFromCivil(year, (unsigned)month, (unsigned)day);
    const int64_t seconds = days * 86400LL + hour * 3600LL + minute * 60LL + second;
    if(seconds <= 0) return false;
    utcEpoch = (time_t)seconds;
    return true;
}

static void FT02_GnssFormatUtcTime(const char* raw)
{
    if(!FT02_GnssAllDigits(raw, 6))
    {
        snprintf(g_ft02Gnss.utcTime, sizeof(g_ft02Gnss.utcTime), "--:--:--");
        return;
    }

    snprintf(
        g_ft02Gnss.utcTime,
        sizeof(g_ft02Gnss.utcTime),
        "%c%c:%c%c:%c%c",
        raw[0], raw[1], raw[2], raw[3], raw[4], raw[5]
    );
}

static void FT02_GnssFormatUtcDate(const char* raw)
{
    if(!FT02_GnssAllDigits(raw, 6))
    {
        snprintf(g_ft02Gnss.utcDate, sizeof(g_ft02Gnss.utcDate), "--/--/--");
        return;
    }

    snprintf(
        g_ft02Gnss.utcDate,
        sizeof(g_ft02Gnss.utcDate),
        "%c%c/%c%c/%c%c",
        raw[0], raw[1], raw[2], raw[3], raw[4], raw[5]
    );
}

static void FT02_GnssMarkUiChanged()
{
    g_ft02Gnss.uiGeneration++;
}

static void FT02_GnssResetAltitudeFilter()
{
    memset(g_ft02AltitudeSamples, 0, sizeof(g_ft02AltitudeSamples));
    g_ft02AltitudeSampleCount = 0;
    g_ft02AltitudeSampleCursor = 0;
}

static void FT02_GnssResetSpeedFilter()
{
    memset(g_ft02SpeedSamples, 0, sizeof(g_ft02SpeedSamples));
    g_ft02SpeedSampleCount = 0;
    g_ft02SpeedSampleCursor = 0;
    g_ft02FilteredSpeedKmh = 0.0f;
    g_ft02MoveEnterCount = 0;
    g_ft02MoveExitCount = 0;
}

static float FT02_GnssMedian(const float* values, size_t count)
{
    if(values == nullptr || count == 0) return 0.0f;
    float sorted[FT02_GNSS_FILTER_SAMPLES] = {};
    if(count > FT02_GNSS_FILTER_SAMPLES) count = FT02_GNSS_FILTER_SAMPLES;
    for(size_t i = 0; i < count; i++) sorted[i] = values[i];
    for(size_t i = 1; i < count; i++)
    {
        const float value = sorted[i];
        size_t j = i;
        while(j > 0 && sorted[j - 1] > value)
        {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = value;
    }
    if((count & 1U) != 0U) return sorted[count / 2U];
    return (sorted[count / 2U - 1U] + sorted[count / 2U]) * 0.5f;
}

static double FT02_GnssDistanceMeters(
    double lat1,
    double lon1,
    double lat2,
    double lon2
)
{
    const double toRadians = M_PI / 180.0;
    const double p1 = lat1 * toRadians;
    const double p2 = lat2 * toRadians;
    const double dp = (lat2 - lat1) * toRadians;
    const double dl = (lon2 - lon1) * toRadians;
    const double a = sin(dp * 0.5) * sin(dp * 0.5) +
        cos(p1) * cos(p2) * sin(dl * 0.5) * sin(dl * 0.5);
    const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return FT02_GNSS_EARTH_RADIUS_METERS * c;
}

static void FT02_GnssUpdatePosition(double latitude, double longitude)
{
    if(!isfinite(latitude) || !isfinite(longitude) ||
       fabs(latitude) > 90.0 || fabs(longitude) > 180.0 ||
       (latitude == 0.0 && longitude == 0.0))
    {
        return;
    }

    if(!g_ft02Gnss.hasPosition)
    {
        g_ft02Gnss.latitude = latitude;
        g_ft02Gnss.longitude = longitude;
        g_ft02Gnss.hasPosition = true;
        FT02_GnssMarkUiChanged();
        return;
    }

    const double distance = FT02_GnssDistanceMeters(
        g_ft02Gnss.latitude,
        g_ft02Gnss.longitude,
        latitude,
        longitude
    );
    if(distance >= FT02_GNSS_POSITION_DEADBAND_METERS)
    {
        g_ft02Gnss.latitude = latitude;
        g_ft02Gnss.longitude = longitude;
        FT02_GnssMarkUiChanged();
    }
}

static void FT02_GnssInvalidateMotion()
{
    bool changed = false;
    if(g_ft02Gnss.speedValid) { g_ft02Gnss.speedValid = false; changed = true; }
    if(g_ft02Gnss.moving) { g_ft02Gnss.moving = false; changed = true; }
    if(g_ft02Gnss.courseValid) { g_ft02Gnss.courseValid = false; changed = true; }
    if(g_ft02Gnss.speedKmh != 0.0f) { g_ft02Gnss.speedKmh = 0.0f; changed = true; }
    FT02_GnssResetSpeedFilter();
    if(changed) FT02_GnssMarkUiChanged();
}

static void FT02_GnssInvalidateAltitude()
{
    if(g_ft02Gnss.altitudeValid)
    {
        g_ft02Gnss.altitudeValid = false;
        FT02_GnssMarkUiChanged();
    }
    FT02_GnssResetAltitudeFilter();
}

static void FT02_GnssUpdateFixState(bool valid)
{
    if(valid)
    {
        g_ft02GnssLastFixMs = millis();
    }

    if(g_ft02Gnss.fixValid != valid)
    {
        g_ft02Gnss.fixValid = valid;
        if(!valid)
        {
            FT02_GnssInvalidateMotion();
            FT02_GnssInvalidateAltitude();
        }
        FT02_GnssMarkUiChanged();
    }
}

static void FT02_GnssUpdateAltitude(float rawAltitude)
{
    g_ft02Gnss.rawAltitudeMeters = rawAltitude;

    const bool trusted =
        g_ft02Gnss.fixValid &&
        g_ft02Gnss.fixType >= 3 &&
        g_ft02Gnss.fixQuality > 0 &&
        g_ft02Gnss.satellites >= FT02_GNSS_ALTITUDE_MIN_SATELLITES &&
        g_ft02Gnss.hdop > 0.0f &&
        g_ft02Gnss.hdop <= FT02_GNSS_ALTITUDE_MAX_HDOP &&
        isfinite(rawAltitude) &&
        rawAltitude > -1000.0f &&
        rawAltitude < 10000.0f;

    if(!trusted)
    {
        FT02_GnssInvalidateAltitude();
        return;
    }

    g_ft02AltitudeSamples[g_ft02AltitudeSampleCursor] = rawAltitude;
    g_ft02AltitudeSampleCursor =
        (g_ft02AltitudeSampleCursor + 1U) % FT02_GNSS_FILTER_SAMPLES;
    if(g_ft02AltitudeSampleCount < FT02_GNSS_FILTER_SAMPLES)
    {
        g_ft02AltitudeSampleCount++;
    }

    const float median = FT02_GnssMedian(
        g_ft02AltitudeSamples,
        g_ft02AltitudeSampleCount
    );
    const float filtered = !g_ft02Gnss.altitudeValid
        ? median
        : g_ft02Gnss.altitudeMeters + 0.25f * (median - g_ft02Gnss.altitudeMeters);

    const bool changed =
        !g_ft02Gnss.altitudeValid ||
        fabsf(filtered - g_ft02Gnss.altitudeMeters) >= 0.8f;
    g_ft02Gnss.altitudeMeters = filtered;
    g_ft02Gnss.altitudeValid = true;
    if(changed) FT02_GnssMarkUiChanged();
}

static float FT02_GnssCourseDelta(float a, float b)
{
    float delta = fabsf(a - b);
    if(delta > 180.0f) delta = 360.0f - delta;
    return delta;
}

static void FT02_GnssUpdateSpeedAndCourse(float rawSpeedKmh, float rawCourse)
{
    if(!isfinite(rawSpeedKmh) || rawSpeedKmh < 0.0f) rawSpeedKmh = 0.0f;
    g_ft02Gnss.rawSpeedKmh = rawSpeedKmh;

    g_ft02SpeedSamples[g_ft02SpeedSampleCursor] = rawSpeedKmh;
    g_ft02SpeedSampleCursor =
        (g_ft02SpeedSampleCursor + 1U) % FT02_GNSS_FILTER_SAMPLES;
    if(g_ft02SpeedSampleCount < FT02_GNSS_FILTER_SAMPLES)
    {
        g_ft02SpeedSampleCount++;
    }

    const float median = FT02_GnssMedian(g_ft02SpeedSamples, g_ft02SpeedSampleCount);
    if(g_ft02SpeedSampleCount == 1)
    {
        g_ft02FilteredSpeedKmh = median;
    }
    else
    {
        g_ft02FilteredSpeedKmh += 0.35f * (median - g_ft02FilteredSpeedKmh);
    }

    bool stateChanged = false;
    if(g_ft02Gnss.moving)
    {
        g_ft02MoveEnterCount = 0;
        if(median <= FT02_GNSS_MOVE_EXIT_KMH)
        {
            if(g_ft02MoveExitCount < 255) g_ft02MoveExitCount++;
            if(g_ft02MoveExitCount >= FT02_GNSS_MOVE_EXIT_SAMPLES)
            {
                g_ft02Gnss.moving = false;
                g_ft02Gnss.courseValid = false;
                g_ft02MoveExitCount = 0;
                stateChanged = true;
            }
        }
        else
        {
            g_ft02MoveExitCount = 0;
        }
    }
    else
    {
        g_ft02MoveExitCount = 0;
        if(median >= FT02_GNSS_MOVE_ENTER_KMH)
        {
            if(g_ft02MoveEnterCount < 255) g_ft02MoveEnterCount++;
            if(g_ft02MoveEnterCount >= FT02_GNSS_MOVE_ENTER_SAMPLES)
            {
                g_ft02Gnss.moving = true;
                g_ft02MoveEnterCount = 0;
                stateChanged = true;
            }
        }
        else
        {
            g_ft02MoveEnterCount = 0;
        }
    }

    const float shownSpeed = g_ft02Gnss.moving ? g_ft02FilteredSpeedKmh : 0.0f;
    const bool speedChanged =
        !g_ft02Gnss.speedValid ||
        fabsf(shownSpeed - g_ft02Gnss.speedKmh) >= 0.2f;
    g_ft02Gnss.speedValid = true;
    g_ft02Gnss.speedKmh = shownSpeed;

    bool courseChanged = false;
    if(g_ft02Gnss.moving && g_ft02FilteredSpeedKmh >= FT02_GNSS_COURSE_MIN_KMH &&
       isfinite(rawCourse) && rawCourse >= 0.0f && rawCourse < 360.0f)
    {
        courseChanged =
            !g_ft02Gnss.courseValid ||
            FT02_GnssCourseDelta(rawCourse, g_ft02Gnss.courseDegrees) >= 5.0f;
        g_ft02Gnss.courseDegrees = rawCourse;
        g_ft02Gnss.courseValid = true;
    }

    if(stateChanged || speedChanged || courseChanged)
    {
        FT02_GnssMarkUiChanged();
    }
}

static void FT02_GnssUpdateTime(const char* rawTime, const char* rawDate)
{
    FT02_GnssFormatUtcTime(rawTime);
    FT02_GnssFormatUtcDate(rawDate);

    time_t utcEpoch = 0;
    if(!FT02_GnssParseUtcEpoch(rawTime, rawDate, utcEpoch))
    {
        if(g_ft02Gnss.timeValid)
        {
            g_ft02Gnss.timeValid = false;
            FT02_GnssMarkUiChanged();
        }
        return;
    }

    const bool wasTimeValid = g_ft02Gnss.timeValid;
    char previousLocalMinute[6] = {};
    snprintf(previousLocalMinute, sizeof(previousLocalMinute), "%.5s", g_ft02Gnss.localTime);

    g_ft02Gnss.timeValid = true;
    const time_t localEpoch = utcEpoch + 8 * 3600;
    struct tm localTm = {};
    gmtime_r(&localEpoch, &localTm);
    strftime(
        g_ft02Gnss.localTime,
        sizeof(g_ft02Gnss.localTime),
        "%H:%M:%S",
        &localTm
    );
    strftime(
        g_ft02Gnss.localDate,
        sizeof(g_ft02Gnss.localDate),
        "%d/%m/%y",
        &localTm
    );

    const uint32_t nowMs = millis();
    if(!g_ft02Gnss.systemTimeSynchronized ||
       g_ft02GnssLastTimeSyncMs == 0 ||
       nowMs - g_ft02GnssLastTimeSyncMs >= FT02_GNSS_CLOCK_RESYNC_MS)
    {
        struct timeval tv = {};
        tv.tv_sec = utcEpoch;
        tv.tv_usec = 0;
        if(settimeofday(&tv, nullptr) == 0)
        {
            g_ft02Gnss.systemTimeSynchronized = true;
            g_ft02GnssLastTimeSyncMs = nowMs;
            g_ft02Gnss.timeSyncCount++;
            Serial.printf(
                "[GNSS-TIME] system clock synchronized UTC=%s %s local=%s %s count=%lu\n",
                g_ft02Gnss.utcDate,
                g_ft02Gnss.utcTime,
                g_ft02Gnss.localDate,
                g_ft02Gnss.localTime,
                (unsigned long)g_ft02Gnss.timeSyncCount
            );
            FT02_GnssMarkUiChanged();
        }
        else
        {
            Serial.println("[GNSS-TIME] settimeofday failed");
        }
    }

    if(!wasTimeValid || strncmp(previousLocalMinute, g_ft02Gnss.localTime, 5) != 0)
    {
        FT02_GnssMarkUiChanged();
    }
}

static void FT02_GnssResetGsvState()
{
    memset(g_ft02GnssGsvTalkers, 0, sizeof(g_ft02GnssGsvTalkers));
    g_ft02Gnss.gsvSeen = false;
    g_ft02Gnss.satellitesVisible = 0;
}

static void FT02_GnssRefreshVisibleSatellites(uint32_t nowMs)
{
    uint16_t totalVisible = 0;
    bool anyFresh = false;
    bool mixedFresh = false;
    uint8_t mixedVisible = 0;

    for(size_t i = 0; i < FT02_GNSS_GSV_TALKER_SLOTS; i++)
    {
        const FT02GnssGsvTalkerState& slot = g_ft02GnssGsvTalkers[i];
        if(slot.talker[0] == 0 || slot.lastSeenMs == 0 ||
           nowMs - slot.lastSeenMs > FT02_GNSS_GSV_STALE_MS)
        {
            continue;
        }

        anyFresh = true;
        if(slot.talker[0] == 'G' && slot.talker[1] == 'N')
        {
            // A GN talker is a mixed-constellation total. Prefer it over
            // constellation-specific GP/GL/GA/GB/GQ totals to avoid double-counting.
            mixedFresh = true;
            mixedVisible = slot.satellitesVisible;
            break;
        }

        totalVisible += slot.satellitesVisible;
    }

    const uint8_t visible = mixedFresh
        ? mixedVisible
        : (uint8_t)(totalVisible > 255U ? 255U : totalVisible);
    const bool gsvSeen = anyFresh;

    if(g_ft02Gnss.gsvSeen != gsvSeen ||
       g_ft02Gnss.satellitesVisible != visible)
    {
        g_ft02Gnss.gsvSeen = gsvSeen;
        g_ft02Gnss.satellitesVisible = visible;
        FT02_GnssMarkUiChanged();
    }
}

static void FT02_GnssParseGsv(char* fields[], int count)
{
    // NMEA GSV: $ttGSV,total_messages,message_number,total_satellites,...
    if(count < 4 || fields[0] == nullptr || strlen(fields[0]) < 3) return;

    const char talker0 = fields[0][1];
    const char talker1 = fields[0][2];
    if(talker0 == 0 || talker1 == 0) return;

    int visibleInt = atoi(fields[3]);
    if(visibleInt < 0) visibleInt = 0;
    if(visibleInt > 255) visibleInt = 255;
    const uint8_t visible = (uint8_t)visibleInt;
    const uint32_t nowMs = millis();

    FT02GnssGsvTalkerState* target = nullptr;
    FT02GnssGsvTalkerState* empty = nullptr;
    FT02GnssGsvTalkerState* oldest = &g_ft02GnssGsvTalkers[0];

    for(size_t i = 0; i < FT02_GNSS_GSV_TALKER_SLOTS; i++)
    {
        FT02GnssGsvTalkerState& slot = g_ft02GnssGsvTalkers[i];
        if(slot.talker[0] == talker0 && slot.talker[1] == talker1)
        {
            target = &slot;
            break;
        }
        if(empty == nullptr && slot.talker[0] == 0) empty = &slot;
        if(slot.lastSeenMs < oldest->lastSeenMs) oldest = &slot;
    }

    if(target == nullptr) target = empty != nullptr ? empty : oldest;
    target->talker[0] = talker0;
    target->talker[1] = talker1;
    target->talker[2] = 0;
    target->satellitesVisible = visible;
    target->lastSeenMs = nowMs;

    FT02_GnssRefreshVisibleSatellites(nowMs);
}

static void FT02_GnssParseGga(char* fields[], int count)
{
    if(count < 10) return;

    FT02_GnssFormatUtcTime(fields[1]);

    const double latitude = FT02_GnssParseCoordinate(fields[2], fields[3]);
    const double longitude = FT02_GnssParseCoordinate(fields[4], fields[5]);
    const uint8_t quality = (uint8_t)atoi(fields[6]);
    const uint8_t satellites = (uint8_t)atoi(fields[7]);
    const float hdop = (float)atof(fields[8]);
    const float altitude = (float)atof(fields[9]);

    bool changed = false;
    if(quality != g_ft02Gnss.fixQuality)
    {
        g_ft02Gnss.fixQuality = quality;
        changed = true;
    }
    if(satellites != g_ft02Gnss.satellites)
    {
        g_ft02Gnss.satellites = satellites;
        changed = true;
    }
    if(hdop > 0.0f && fabsf(hdop - g_ft02Gnss.hdop) >= 0.2f)
    {
        g_ft02Gnss.hdop = hdop;
        changed = true;
    }

    const bool valid = quality > 0;
    FT02_GnssUpdateFixState(valid);
    if(valid)
    {
        FT02_GnssUpdatePosition(latitude, longitude);
    }
    FT02_GnssUpdateAltitude(altitude);
    if(changed) FT02_GnssMarkUiChanged();
}

static void FT02_GnssParseRmc(char* fields[], int count)
{
    if(count < 10) return;

    FT02_GnssUpdateTime(fields[1], fields[9]);

    const bool valid = fields[2] != nullptr && fields[2][0] == 'A';
    if(valid)
    {
        const double latitude = FT02_GnssParseCoordinate(fields[3], fields[4]);
        const double longitude = FT02_GnssParseCoordinate(fields[5], fields[6]);
        const float speedKmh = (float)atof(fields[7]) * 1.852f;
        const float course = (float)atof(fields[8]);

        FT02_GnssUpdatePosition(latitude, longitude);
        FT02_GnssUpdateSpeedAndCourse(speedKmh, course);
    }
    else
    {
        // RMC status V means speed/course are not trustworthy even when a
        // nearby GGA sentence still reports a positional fix.
        FT02_GnssInvalidateMotion();
    }

    FT02_GnssUpdateFixState(valid || g_ft02Gnss.fixQuality > 0);
}

static void FT02_GnssParseGsa(char* fields[], int count)
{
    if(count < 3) return;

    const uint8_t fixType = (uint8_t)atoi(fields[2]);
    if(fixType != g_ft02Gnss.fixType)
    {
        g_ft02Gnss.fixType = fixType;
        if(fixType < 3) FT02_GnssInvalidateAltitude();
        FT02_GnssMarkUiChanged();
    }

    if(count > 16 && fields[16] != nullptr && fields[16][0] != 0)
    {
        const float hdop = (float)atof(fields[16]);
        if(hdop > 0.0f && fabsf(hdop - g_ft02Gnss.hdop) >= 0.2f)
        {
            g_ft02Gnss.hdop = hdop;
            if(hdop > FT02_GNSS_ALTITUDE_MAX_HDOP) FT02_GnssInvalidateAltitude();
            FT02_GnssMarkUiChanged();
        }
    }
}

static void FT02_GnssHandleLine(const char* sourceLine)
{
    if(sourceLine == nullptr || sourceLine[0] != '$') return;

    if(!FT02_GnssChecksumValid(sourceLine))
    {
        g_ft02Gnss.checksumErrors++;
        return;
    }

    if(!g_ft02Gnss.nmeaSeen)
    {
        g_ft02Gnss.nmeaSeen = true;
        FT02_GnssMarkUiChanged();
    }

    g_ft02Gnss.validSentences++;
    g_ft02GnssLastSentenceMs = millis();

    char line[FT02_GNSS_LINE_CAPACITY];
    snprintf(line, sizeof(line), "%s", sourceLine);
    char* star = strchr(line, '*');
    if(star != nullptr) *star = 0;

    char* fields[24] = {};
    const int count = FT02_GnssSplitFields(line, fields, 24);
    if(count < 1 || fields[0] == nullptr) return;

    const size_t typeLength = strlen(fields[0]);
    if(typeLength < 3) return;
    const char* type = fields[0] + typeLength - 3;

    if(strcmp(type, "GGA") == 0)
    {
        FT02_GnssParseGga(fields, count);
    }
    else if(strcmp(type, "RMC") == 0)
    {
        FT02_GnssParseRmc(fields, count);
    }
    else if(strcmp(type, "GSA") == 0)
    {
        FT02_GnssParseGsa(fields, count);
    }
    else if(strcmp(type, "GSV") == 0)
    {
        FT02_GnssParseGsv(fields, count);
    }
}

static void FT02_GnssOpenSerial()
{
    g_ft02GnssSerial.end();
    delay(20);
    g_ft02GnssSerial.begin(
        FT02_GNSS_BAUD,
        SERIAL_8N1,
        FT02_GNSS_RX_PIN,
        FT02_GNSS_TX_PIN
    );

    g_ft02Gnss.currentBaud = FT02_GNSS_BAUD;
    g_ft02Gnss.detectedBaud = FT02_GNSS_BAUD;
    g_ft02GnssLineLength = 0;
    FT02_GnssMarkUiChanged();

    Serial.printf(
        "[GNSS] open baud=%lu RX=%d TX=%d\n",
        (unsigned long)FT02_GNSS_BAUD,
        FT02_GNSS_RX_PIN,
        FT02_GNSS_TX_PIN
    );
}

static void FT02_GnssResetRuntimeFields(bool preserveClockState)
{
    const bool systemTimeSynchronized = preserveClockState
        ? g_ft02Gnss.systemTimeSynchronized
        : false;
    const uint32_t timeSyncCount = preserveClockState
        ? g_ft02Gnss.timeSyncCount
        : 0;
    const uint32_t uiGeneration = g_ft02Gnss.uiGeneration;

    memset(&g_ft02Gnss, 0, sizeof(g_ft02Gnss));
    g_ft02Gnss.systemTimeSynchronized = systemTimeSynchronized;
    g_ft02Gnss.timeSyncCount = timeSyncCount;
    g_ft02Gnss.uiGeneration = uiGeneration;
    snprintf(g_ft02Gnss.utcTime, sizeof(g_ft02Gnss.utcTime), "--:--:--");
    snprintf(g_ft02Gnss.utcDate, sizeof(g_ft02Gnss.utcDate), "--/--/--");
    snprintf(g_ft02Gnss.localTime, sizeof(g_ft02Gnss.localTime), "--:--:--");
    snprintf(g_ft02Gnss.localDate, sizeof(g_ft02Gnss.localDate), "--/--/--");
    FT02_GnssResetAltitudeFilter();
    FT02_GnssResetSpeedFilter();
    FT02_GnssResetGsvState();
}
}

void FT02_GnssBegin()
{
    FT02_GnssResetRuntimeFields(false);

    // POSIX TZ signs are reversed: CST-8 means UTC+8 with no daylight saving.
    setenv("TZ", "CST-8", 1);
    tzset();

    g_ft02Gnss.started = true;
    g_ft02GnssStartedMs = millis();
    g_ft02GnssLastByteMs = 0;
    g_ft02GnssLastSentenceMs = 0;
    g_ft02GnssLastFixMs = 0;
    g_ft02GnssLastTimeSyncMs = 0;

    FT02_GnssOpenSerial();
    Serial.println("[GNSS] runtime A2 started: altitude/speed filters + UTC+8 system clock sync");
}

void FT02_GnssReconnect()
{
    Serial.println("[GNSS] manual UART reconnect requested");

    FT02_GnssResetRuntimeFields(true);
    g_ft02Gnss.started = true;
    g_ft02GnssStartedMs = millis();
    g_ft02GnssLastByteMs = 0;
    g_ft02GnssLastSentenceMs = 0;
    g_ft02GnssLastFixMs = 0;
    g_ft02GnssLineLength = 0;
    FT02_GnssOpenSerial();
}

void FT02_GnssPoll()
{
    if(!g_ft02Gnss.started) return;

    int budget = 512;
    while(budget-- > 0 && g_ft02GnssSerial.available() > 0)
    {
        const char value = (char)g_ft02GnssSerial.read();
        g_ft02Gnss.bytesReceived++;
        g_ft02GnssLastByteMs = millis();

        if(!g_ft02Gnss.serialDataSeen)
        {
            g_ft02Gnss.serialDataSeen = true;
            FT02_GnssMarkUiChanged();
        }

        if(value == '$')
        {
            g_ft02GnssLineLength = 0;
            g_ft02GnssLine[g_ft02GnssLineLength++] = value;
        }
        else if(value == '\n')
        {
            if(g_ft02GnssLineLength > 0)
            {
                while(g_ft02GnssLineLength > 0 &&
                      (g_ft02GnssLine[g_ft02GnssLineLength - 1] == '\r' ||
                       g_ft02GnssLine[g_ft02GnssLineLength - 1] == '\n'))
                {
                    g_ft02GnssLineLength--;
                }
                g_ft02GnssLine[g_ft02GnssLineLength] = 0;
                FT02_GnssHandleLine(g_ft02GnssLine);
            }
            g_ft02GnssLineLength = 0;
        }
        else if(g_ft02GnssLineLength > 0)
        {
            if(g_ft02GnssLineLength < FT02_GNSS_LINE_CAPACITY - 1)
            {
                g_ft02GnssLine[g_ft02GnssLineLength++] = value;
            }
            else
            {
                g_ft02GnssLineLength = 0;
            }
        }
    }

    const uint32_t now = millis();

    const bool communicationActive =
        g_ft02GnssLastSentenceMs > 0 &&
        now - g_ft02GnssLastSentenceMs <= FT02_GNSS_LINK_STALE_MS;

    if(communicationActive != g_ft02Gnss.communicationActive)
    {
        g_ft02Gnss.communicationActive = communicationActive;
        FT02_GnssMarkUiChanged();
    }

    if(g_ft02Gnss.fixValid &&
       g_ft02GnssLastFixMs > 0 &&
       now - g_ft02GnssLastFixMs > FT02_GNSS_FIX_STALE_MS)
    {
        FT02_GnssUpdateFixState(false);
    }

    // Keep the diagnostic "visible" count honest if GSV output stops.
    FT02_GnssRefreshVisibleSatellites(now);
}

FT02GnssSnapshot FT02_GnssSnapshotCurrent()
{
    FT02GnssSnapshot snapshot = g_ft02Gnss;
    const uint32_t now = millis();

    snapshot.startAgeMs = g_ft02GnssStartedMs > 0
        ? now - g_ft02GnssStartedMs
        : 0;
    snapshot.lastByteAgeMs = g_ft02GnssLastByteMs > 0
        ? now - g_ft02GnssLastByteMs
        : UINT32_MAX;
    snapshot.lastSentenceAgeMs = g_ft02GnssLastSentenceMs > 0
        ? now - g_ft02GnssLastSentenceMs
        : UINT32_MAX;
    snapshot.lastFixAgeMs = g_ft02GnssLastFixMs > 0
        ? now - g_ft02GnssLastFixMs
        : UINT32_MAX;
    snapshot.lastTimeSyncAgeMs = g_ft02GnssLastTimeSyncMs > 0
        ? now - g_ft02GnssLastTimeSyncMs
        : UINT32_MAX;

    return snapshot;
}
