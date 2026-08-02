#include "FT02_Gnss.h"

#include <HardwareSerial.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace
{
constexpr int FT02_GNSS_RX_PIN = 39; // ESP32 RX <- GNSS TX
constexpr int FT02_GNSS_TX_PIN = 38; // ESP32 TX -> GNSS RX
constexpr uint32_t FT02_GNSS_BAUD = 38400;
constexpr uint32_t FT02_GNSS_LINK_STALE_MS = 5000;
constexpr uint32_t FT02_GNSS_FIX_STALE_MS = 6000;
constexpr size_t FT02_GNSS_LINE_CAPACITY = 160;



HardwareSerial g_ft02GnssSerial(1);
FT02GnssSnapshot g_ft02Gnss = {};
size_t g_ft02GnssLineLength = 0;
char g_ft02GnssLine[FT02_GNSS_LINE_CAPACITY] = {};
uint32_t g_ft02GnssStartedMs = 0;
uint32_t g_ft02GnssLastByteMs = 0;
uint32_t g_ft02GnssLastSentenceMs = 0;
uint32_t g_ft02GnssLastFixMs = 0;
uint32_t g_ft02GnssLastSummaryMs = 0;
uint32_t g_ft02GnssRawPrintCount = 0;

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

    uint8_t expectedHigh = FT02_GnssHexNibble(star[1]);
    uint8_t expectedLow = FT02_GnssHexNibble(star[2]);
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

static void FT02_GnssFormatUtcTime(const char* raw)
{
    if(raw == nullptr || strlen(raw) < 6)
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
    if(raw == nullptr || strlen(raw) < 6)
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

static void FT02_GnssUpdateFixState(bool valid)
{
    if(valid)
    {
        g_ft02GnssLastFixMs = millis();
    }

    if(g_ft02Gnss.fixValid != valid)
    {
        g_ft02Gnss.fixValid = valid;
        FT02_GnssMarkUiChanged();
    }
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
    if(fabs(latitude - g_ft02Gnss.latitude) >= 0.00005 ||
       fabs(longitude - g_ft02Gnss.longitude) >= 0.00005)
    {
        g_ft02Gnss.latitude = latitude;
        g_ft02Gnss.longitude = longitude;
        changed = true;
    }
    if(fabs(hdop - g_ft02Gnss.hdop) >= 0.3f)
    {
        g_ft02Gnss.hdop = hdop;
        changed = true;
    }
    if(fabs(altitude - g_ft02Gnss.altitudeMeters) >= 2.0f)
    {
        g_ft02Gnss.altitudeMeters = altitude;
        changed = true;
    }

    FT02_GnssUpdateFixState(quality > 0);
    if(changed) FT02_GnssMarkUiChanged();
}

static void FT02_GnssParseRmc(char* fields[], int count)
{
    if(count < 10) return;

    FT02_GnssFormatUtcTime(fields[1]);
    FT02_GnssFormatUtcDate(fields[9]);

    const bool valid = fields[2] != nullptr && fields[2][0] == 'A';
    if(valid)
    {
        const double latitude = FT02_GnssParseCoordinate(fields[3], fields[4]);
        const double longitude = FT02_GnssParseCoordinate(fields[5], fields[6]);
        const float speedKmh = (float)atof(fields[7]) * 1.852f;
        const float course = (float)atof(fields[8]);

        bool changed = false;
        if(fabs(latitude - g_ft02Gnss.latitude) >= 0.00005 ||
           fabs(longitude - g_ft02Gnss.longitude) >= 0.00005)
        {
            g_ft02Gnss.latitude = latitude;
            g_ft02Gnss.longitude = longitude;
            changed = true;
        }
        if(fabs(speedKmh - g_ft02Gnss.speedKmh) >= 1.0f)
        {
            g_ft02Gnss.speedKmh = speedKmh;
            changed = true;
        }
        if(fabs(course - g_ft02Gnss.courseDegrees) >= 5.0f)
        {
            g_ft02Gnss.courseDegrees = course;
            changed = true;
        }
        if(changed) FT02_GnssMarkUiChanged();
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
        FT02_GnssMarkUiChanged();
    }

    if(count > 16 && fields[16] != nullptr && fields[16][0] != 0)
    {
        const float hdop = (float)atof(fields[16]);
        if(hdop > 0.0f && fabs(hdop - g_ft02Gnss.hdop) >= 0.3f)
        {
            g_ft02Gnss.hdop = hdop;
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

    if(g_ft02GnssRawPrintCount < 12 || (g_ft02GnssRawPrintCount % 20) == 0)
    {
        Serial.print("[GNSS-NMEA] ");
        Serial.println(sourceLine);
    }
    g_ft02GnssRawPrintCount++;

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
}

void FT02_GnssBegin()
{
    memset(&g_ft02Gnss, 0, sizeof(g_ft02Gnss));
    snprintf(g_ft02Gnss.utcTime, sizeof(g_ft02Gnss.utcTime), "--:--:--");
    snprintf(g_ft02Gnss.utcDate, sizeof(g_ft02Gnss.utcDate), "--/--/--");

    g_ft02Gnss.started = true;
    g_ft02GnssStartedMs = millis();
    g_ft02GnssLastByteMs = 0;
    g_ft02GnssLastSentenceMs = 0;
    g_ft02GnssLastFixMs = 0;
    g_ft02GnssLastSummaryMs = 0;
    g_ft02GnssRawPrintCount = 0;

    FT02_GnssOpenSerial();
    Serial.println("[GNSS] runtime A1 started at fixed 38400 baud; PPS/P pin is not used");
}

void FT02_GnssReconnect()
{
    Serial.println("[GNSS] manual UART reconnect requested");

    g_ft02Gnss.serialDataSeen = false;
    g_ft02Gnss.nmeaSeen = false;
    g_ft02Gnss.communicationActive = false;
    g_ft02Gnss.fixValid = false;
    g_ft02Gnss.bytesReceived = 0;
    g_ft02Gnss.validSentences = 0;
    g_ft02Gnss.checksumErrors = 0;
    g_ft02Gnss.fixType = 0;
    g_ft02Gnss.fixQuality = 0;
    g_ft02Gnss.satellites = 0;
    g_ft02Gnss.latitude = 0.0;
    g_ft02Gnss.longitude = 0.0;
    g_ft02Gnss.altitudeMeters = 0.0f;
    g_ft02Gnss.hdop = 0.0f;
    g_ft02Gnss.speedKmh = 0.0f;
    g_ft02Gnss.courseDegrees = 0.0f;
    snprintf(g_ft02Gnss.utcTime, sizeof(g_ft02Gnss.utcTime), "--:--:--");
    snprintf(g_ft02Gnss.utcDate, sizeof(g_ft02Gnss.utcDate), "--/--/--");

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
        g_ft02Gnss.fixValid = false;
        FT02_GnssMarkUiChanged();
    }

    if(now - g_ft02GnssLastSummaryMs >= 2000)
    {
        g_ft02GnssLastSummaryMs = now;
        Serial.printf(
            "[GNSS] baud=%lu link=%d bytes=%lu nmea=%lu err=%lu fix=%d type=%u sats=%u lat=%.6f lon=%.6f hdop=%.1f\n",
            (unsigned long)g_ft02Gnss.currentBaud,
            g_ft02Gnss.communicationActive ? 1 : 0,
            (unsigned long)g_ft02Gnss.bytesReceived,
            (unsigned long)g_ft02Gnss.validSentences,
            (unsigned long)g_ft02Gnss.checksumErrors,
            g_ft02Gnss.fixValid ? 1 : 0,
            (unsigned int)g_ft02Gnss.fixType,
            (unsigned int)g_ft02Gnss.satellites,
            g_ft02Gnss.latitude,
            g_ft02Gnss.longitude,
            g_ft02Gnss.hdop
        );
    }
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

    return snapshot;
}
