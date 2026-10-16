#include "FT02_Gnss.h"
#include "FT02_Log.h"

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

namespace
{
constexpr int FT02_GNSS_RX_PIN = 5;   // Core RX <- MAX-M10S/K12+ TX
constexpr int FT02_GNSS_TX_PIN = 6;   // Core TX -> MAX-M10S/K12+ RX (UBX power control + wake)
constexpr uint32_t FT02_GNSS_BAUD = 115200u;
constexpr size_t FT02_GNSS_RX_BUFFER_BYTES = 4096u; // retained from the real-device-validated GNSS parser baseline
constexpr uint32_t FT02_GNSS_STREAM_FRESH_MS = 3000u;
constexpr uint32_t FT02_GNSS_FIELD_FRESH_MS = 5000u;
constexpr uint32_t FT02_GNSS_GSV_FRESH_MS = 5000u;
// K12+ is now on a fixed, real-device-verified 115200 baud link. During cold
// acquisition, valid NMEA may arrive later than raw UART bytes. Do not churn the
// UART/parser just because a fix or checksum-valid sentence is not ready yet.
constexpr uint32_t FT02_GNSS_STARTUP_GRACE_MS = 60000u;
constexpr uint32_t FT02_GNSS_STREAM_STALL_RECOVERY_MS = 30000u;
constexpr uint32_t FT02_GNSS_RECOVERY_COOLDOWN_MS = 60000u;

// Sleep Power A5A: u-blox M10 software standby.  UBX-RXM-PMREQ v0,
// duration=0 (sleep until a configured wake source), flags.force=1 and
// wakeupSources.uartrx=1.  The checksum 0x5F/0x5B is calculated over
// class..payload according to the UBX protocol.  This is UART-only and does
// not touch the shared SDA47/SCL21 bus used by CardKB2/Compass/Battery/AW9523.
constexpr uint8_t FT02_GNSS_UBX_PMREQ_STANDBY[] = {
    0xB5, 0x62, 0x02, 0x41, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, // version + reserved0
    0x00, 0x00, 0x00, 0x00, // duration = 0
    0x04, 0x00, 0x00, 0x00, // flags: force = bit 2
    0x08, 0x00, 0x00, 0x00, // wakeupSources: UART RX = bit 3
    0x5F, 0x5B              // CK_A, CK_B
};
static_assert(sizeof(FT02_GNSS_UBX_PMREQ_STANDBY) == 24u, "UBX-RXM-PMREQ frame size");

HardwareSerial g_gnssSerial(1);
TinyGPSPlus g_gps;

// Keep the same sentence-level fallbacks already validated on real hardware.
TinyGPSCustom g_gnGgaFixQuality(g_gps, "GNGGA", 6);
TinyGPSCustom g_gpGgaFixQuality(g_gps, "GPGGA", 6);
TinyGPSCustom g_gnGgaSatUsed(g_gps, "GNGGA", 7);
TinyGPSCustom g_gpGgaSatUsed(g_gps, "GPGGA", 7);
TinyGPSCustom g_gnGgaHdop(g_gps, "GNGGA", 8);
TinyGPSCustom g_gpGgaHdop(g_gps, "GPGGA", 8);

TinyGPSCustom g_gnGsaFixType(g_gps, "GNGSA", 2);
TinyGPSCustom g_gpGsaFixType(g_gps, "GPGSA", 2);
TinyGPSCustom g_gaGsaFixType(g_gps, "GAGSA", 2);
TinyGPSCustom g_gbGsaFixType(g_gps, "GBGSA", 2);
TinyGPSCustom g_glGsaFixType(g_gps, "GLGSA", 2);
TinyGPSCustom g_gqGsaFixType(g_gps, "GQGSA", 2);

TinyGPSCustom g_gpGsvVisible(g_gps, "GPGSV", 3);
TinyGPSCustom g_gaGsvVisible(g_gps, "GAGSV", 3);
TinyGPSCustom g_gbGsvVisible(g_gps, "GBGSV", 3);
TinyGPSCustom g_glGsvVisible(g_gps, "GLGSV", 3);
TinyGPSCustom g_gqGsvVisible(g_gps, "GQGSV", 3);

FT02GnssSnapshot g_nav = {};
bool g_runtimeEnabled = true;
bool g_hardwareKnown = false;
bool g_hardwareReady = false;
uint32_t g_startedMs = 0u;
uint32_t g_lastFixMs = 0u;
uint32_t g_lastTimeSyncMs = 0u;
uint32_t g_lastUnixApplied = 0u;
uint32_t g_lastGnssByteMs = 0u;
uint32_t g_streamStartMs = 0u;
uint32_t g_serialOpenedMs = 0u;
uint32_t g_lastNmeaPassMs = 0u;
uint32_t g_lastParserPassed = 0u;
uint32_t g_lastParserFailed = 0u;
uint32_t g_lastRecoveryMs = 0u;
uint32_t g_totalBytes = 0u;
bool g_onlineAnnounced = false;
bool g_serialActive = false;
bool g_softwareStandbyRequested = false;
bool g_bootWakeProbePending = true;

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
    if(unixTime < 1600000000u || g_lastUnixApplied == unixTime) return;
    struct timeval tv = {};
    tv.tv_sec = static_cast<time_t>(unixTime);
    if(settimeofday(&tv, nullptr) == 0)
    {
        g_lastUnixApplied = unixTime;
        g_nav.systemTimeSynchronized = true;
        ++g_nav.timeSyncCount;
        g_lastTimeSyncMs = millis();
    }
}

int64_t daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const int shiftedMonth = static_cast<int>(m) + (m > 2 ? -3 : 9);
    const unsigned doy = (153u * static_cast<unsigned>(shiftedMonth) + 2u) / 5u + d - 1u;
    const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

uint32_t makeUnixUtc(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    if(year < 2020u || year > 2099u || month < 1u || month > 12u || day < 1u || day > 31u ||
       hour > 23u || minute > 59u || second > 60u) return 0u;
    const int64_t days = daysFromCivil(static_cast<int>(year), month, day);
    const int64_t sec = days * 86400LL + static_cast<int64_t>(hour) * 3600LL +
                        static_cast<int64_t>(minute) * 60LL + static_cast<int64_t>(second);
    return (sec > 0 && sec <= 0xFFFFFFFFLL) ? static_cast<uint32_t>(sec) : 0u;
}

void invalidateLiveData(bool clearCommunication)
{
    bool changed = false;
#define SET_BOOL(field, value) do { const bool _v=(value); if(g_nav.field != _v) { g_nav.field=_v; changed=true; } } while(0)
    if(clearCommunication) SET_BOOL(communicationActive, false);
    SET_BOOL(fixValid, false);
    SET_BOOL(hasPosition, false);
    SET_BOOL(altitudeValid, false);
    SET_BOOL(speedValid, false);
    SET_BOOL(moving, false);
    SET_BOOL(courseValid, false);
#undef SET_BOOL
    if(changed) markChanged();
}

TinyGPSCustom *freshestCustom(TinyGPSCustom *a, TinyGPSCustom *b, uint32_t maxAgeMs)
{
    const bool av = a && a->isValid() && a->age() <= maxAgeMs;
    const bool bv = b && b->isValid() && b->age() <= maxAgeMs;
    if(!av) return bv ? b : nullptr;
    if(!bv) return a;
    return a->age() <= b->age() ? a : b;
}

uint8_t freshCustomU8(TinyGPSCustom *c, uint32_t maxAgeMs, uint8_t fallback = 0)
{
    if(!c || !c->isValid() || c->age() > maxAgeMs || !c->value() || !c->value()[0]) return fallback;
    const long v = strtol(c->value(), nullptr, 10);
    if(v < 0) return fallback;
    if(v > 255) return 255;
    return static_cast<uint8_t>(v);
}

uint16_t customToFreshU16(TinyGPSCustom &c, uint32_t maxAgeMs)
{
    if(!c.isValid() || c.age() > maxAgeMs || !c.value() || !c.value()[0]) return 0u;
    const long v = strtol(c.value(), nullptr, 10);
    if(v <= 0) return 0u;
    if(v > 65535) return 65535u;
    return static_cast<uint16_t>(v);
}

uint8_t freshestGsaFixType()
{
    TinyGPSCustom *items[] = {&g_gnGsaFixType, &g_gpGsaFixType, &g_gaGsaFixType,
                              &g_gbGsaFixType, &g_glGsaFixType, &g_gqGsaFixType};
    TinyGPSCustom *best = nullptr;
    for(TinyGPSCustom *c : items)
    {
        if(!c->isValid() || c->age() > FT02_GNSS_FIELD_FRESH_MS) continue;
        if(!best || c->age() < best->age()) best = c;
    }
    return freshCustomU8(best, FT02_GNSS_FIELD_FRESH_MS, 0u);
}

uint8_t freshestGgaFixQuality()
{
    TinyGPSCustom *c = freshestCustom(&g_gnGgaFixQuality, &g_gpGgaFixQuality, FT02_GNSS_FIELD_FRESH_MS);
    return freshCustomU8(c, FT02_GNSS_FIELD_FRESH_MS, 0u);
}

uint8_t freshestGgaSatUsed()
{
    TinyGPSCustom *c = freshestCustom(&g_gnGgaSatUsed, &g_gpGgaSatUsed, FT02_GNSS_FIELD_FRESH_MS);
    return freshCustomU8(c, FT02_GNSS_FIELD_FRESH_MS, 0u);
}

float freshestGgaHdop()
{
    TinyGPSCustom *c = freshestCustom(&g_gnGgaHdop, &g_gpGgaHdop, FT02_GNSS_FIELD_FRESH_MS);
    if(!c || !c->isValid() || c->age() > FT02_GNSS_FIELD_FRESH_MS || !c->value() || !c->value()[0]) return 99.99f;
    const float v = strtof(c->value(), nullptr);
    return (v >= 0.0f && v < 100.0f) ? v : 99.99f;
}

bool requestSoftwareStandby()
{
    if(!g_serialActive) return g_softwareStandbyRequested;

    // Drain host RX first so the transport closes at a sentence boundary.
    while(g_gnssSerial.available() > 0) (void)g_gnssSerial.read();

    const size_t written = g_gnssSerial.write(
        FT02_GNSS_UBX_PMREQ_STANDBY,
        sizeof(FT02_GNSS_UBX_PMREQ_STANDBY)
    );
    // HardwareSerial::flush waits for UART TX completion; no timing delay is
    // used to hide a transition race.
    g_gnssSerial.flush();

    const bool sent = written == sizeof(FT02_GNSS_UBX_PMREQ_STANDBY);
    g_softwareStandbyRequested = sent;
    if(sent)
        FT02_LOGD_PRINTLN("[GNSS-PWR] standby force=1 wake=uart_rx");
    else
        FT02_LOGW("[WARN][GNSS-PWR] standby short-write=%u/%u\n",
                  static_cast<unsigned>(written),
                  static_cast<unsigned>(sizeof(FT02_GNSS_UBX_PMREQ_STANDBY)));
    return sent;
}

void closeGnssSerialForStandby()
{
    g_gnssSerial.end();
    g_serialActive = false;

    // M10 software standby wakes on either UART-RX edge.  Keep the host TX
    // line at the UART idle HIGH level while the ESP32 UART peripheral is off
    // so a floating pin cannot wake the receiver repeatedly.
    pinMode(FT02_GNSS_RX_PIN, INPUT);
    pinMode(FT02_GNSS_TX_PIN, OUTPUT);
    digitalWrite(FT02_GNSS_TX_PIN, HIGH);
}

void sendReceiverWakeEdge()
{
    // A single 0xFF byte provides a UART start-bit edge.  It is not a valid
    // NMEA/UBX frame, so an already-awake receiver safely discards it.  Sending
    // it once at boot also recovers the case where FT02 was USB-reset while
    // the independently powered GNSS receiver remained in software standby.
    const uint8_t wakeByte = 0xFFu;
    const size_t written = g_gnssSerial.write(&wakeByte, 1u);
    g_gnssSerial.flush();
    if(written == 1u)
        FT02_LOGD_PRINTLN("[GNSS-PWR] UART-RX wake edge");
    else
        FT02_LOGW_PRINTLN("[WARN][GNSS-PWR] UART-RX wake edge write failed");
    g_softwareStandbyRequested = false;
    g_bootWakeProbePending = false;
}

void beginGnssSerial()
{
    g_hardwareKnown = true;
    g_hardwareReady = false;
    g_gnssSerial.end();
    g_serialActive = false;
    delay(10);
    g_gnssSerial.setRxBufferSize(FT02_GNSS_RX_BUFFER_BYTES);
    g_gnssSerial.begin(FT02_GNSS_BAUD, SERIAL_8N1, FT02_GNSS_RX_PIN, FT02_GNSS_TX_PIN);
    g_serialActive = true;
    if(g_softwareStandbyRequested || g_bootWakeProbePending) sendReceiverWakeEdge();
    g_lastGnssByteMs = 0u;
    g_streamStartMs = 0u;
    g_serialOpenedMs = millis();
    g_lastNmeaPassMs = 0u;
    g_lastParserPassed = g_gps.passedChecksum();
    g_lastParserFailed = g_gps.failedChecksum();
    g_onlineAnnounced = false;
    FT02_LOGD("[GNSS-UART] start rx=%d tx=%d baud=%lu parser=TinyGPSPlus rxbuf=%u source=Core-UART\n",
                  FT02_GNSS_RX_PIN, FT02_GNSS_TX_PIN,
                  static_cast<unsigned long>(FT02_GNSS_BAUD),
                  static_cast<unsigned>(FT02_GNSS_RX_BUFFER_BYTES));
}

void updateParserStats(uint32_t now)
{
    const uint32_t passed = g_gps.passedChecksum();
    const uint32_t failed = g_gps.failedChecksum();
    if(passed != g_lastParserPassed)
    {
        g_lastParserPassed = passed;
        g_lastNmeaPassMs = now;
        g_hardwareReady = true;
        if(!g_onlineAnnounced)
        {
            g_onlineAnnounced = true;
            FT02_LOGI("[GNSS] ready rx=%d tx=%d baud=%lu nmea=1 bytes=%lu\n",
                          FT02_GNSS_RX_PIN, FT02_GNSS_TX_PIN,
                          static_cast<unsigned long>(FT02_GNSS_BAUD),
                          static_cast<unsigned long>(g_totalBytes));
        }
    }
    if(failed != g_lastParserFailed) g_lastParserFailed = failed;
}

void publishSnapshot(uint32_t now)
{
    const bool streamActive = g_lastNmeaPassMs != 0u &&
                              static_cast<uint32_t>(now - g_lastNmeaPassMs) < FT02_GNSS_STREAM_FRESH_MS;
    const bool wasActive = g_nav.communicationActive;

    const uint8_t gsaFix = freshestGsaFixType();
    uint8_t satUsed = 0u;
    if(g_gps.satellites.isValid() && g_gps.satellites.age() <= FT02_GNSS_FIELD_FRESH_MS)
        satUsed = static_cast<uint8_t>(min<uint32_t>(g_gps.satellites.value(), 255u));
    const uint8_t ggaSat = freshestGgaSatUsed();
    if(ggaSat > satUsed) satUsed = ggaSat;

    uint16_t satVisible = customToFreshU16(g_gpGsvVisible, FT02_GNSS_GSV_FRESH_MS) +
                          customToFreshU16(g_gaGsvVisible, FT02_GNSS_GSV_FRESH_MS) +
                          customToFreshU16(g_gbGsvVisible, FT02_GNSS_GSV_FRESH_MS) +
                          customToFreshU16(g_glGsvVisible, FT02_GNSS_GSV_FRESH_MS) +
                          customToFreshU16(g_gqGsvVisible, FT02_GNSS_GSV_FRESH_MS);
    if(satVisible < satUsed) satVisible = satUsed;
    if(satVisible > 255u) satVisible = 255u;

    const bool positionFresh = g_gps.location.isValid() && g_gps.location.age() <= FT02_GNSS_FIELD_FRESH_MS;
    const double lat = positionFresh ? g_gps.location.lat() : g_nav.latitude;
    const double lon = positionFresh ? g_gps.location.lng() : g_nav.longitude;
    const bool validRange = positionFresh && lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
    const uint8_t fixQuality = freshestGgaFixQuality();
    const bool gsaHasFix = gsaFix >= 2u && gsaFix <= 3u;
    const bool ggaHasFix = fixQuality > 0u;
    // Do not fabricate a 3D fix just because a coordinate is present. When a
    // fresh GSA sentence is available it owns the 2D/3D dimension. If GSA is
    // temporarily absent but GGA reports a valid solution, keep fixType=0 so
    // the UI can truthfully show generic "已定位" instead of "3D定位".
    const uint8_t fixType = gsaHasFix ? gsaFix : 0u;
    const bool validPos = streamActive && validRange && (gsaHasFix || ggaHasFix);

    const bool altitudeFresh = g_gps.altitude.isValid() && g_gps.altitude.age() <= FT02_GNSS_FIELD_FRESH_MS;
    const float altitude = altitudeFresh ? static_cast<float>(g_gps.altitude.meters()) : g_nav.altitudeMeters;
    const bool speedFresh = g_gps.speed.isValid() && g_gps.speed.age() <= FT02_GNSS_FIELD_FRESH_MS;
    const float speedKmh = speedFresh ? static_cast<float>(g_gps.speed.kmph()) : 0.0f;
    const bool courseFresh = g_gps.course.isValid() && g_gps.course.age() <= FT02_GNSS_FIELD_FRESH_MS;
    float course = courseFresh ? static_cast<float>(g_gps.course.deg()) : g_nav.courseDegrees;
    while(course < 0.0f) course += 360.0f;
    while(course >= 360.0f) course -= 360.0f;
    const bool moving = validPos && speedFresh && speedKmh >= 0.72f; // same ~0.2m/s threshold as the UBX path

    float hdop = 99.99f;
    if(g_gps.hdop.isValid() && g_gps.hdop.age() <= FT02_GNSS_FIELD_FRESH_MS)
        hdop = static_cast<float>(g_gps.hdop.value()) / 100.0f;
    else
        hdop = freshestGgaHdop();

    uint32_t unixTime = 0u;
    const bool timeFresh = g_gps.date.isValid() && g_gps.time.isValid() &&
                           g_gps.date.age() <= FT02_GNSS_FIELD_FRESH_MS &&
                           g_gps.time.age() <= FT02_GNSS_FIELD_FRESH_MS;
    if(timeFresh)
    {
        unixTime = makeUnixUtc(static_cast<uint16_t>(g_gps.date.year()),
                               static_cast<uint8_t>(g_gps.date.month()),
                               static_cast<uint8_t>(g_gps.date.day()),
                               static_cast<uint8_t>(g_gps.time.hour()),
                               static_cast<uint8_t>(g_gps.time.minute()),
                               static_cast<uint8_t>(g_gps.time.second()));
    }

    bool changed = false;
#define SET_FIELD(field, value) do { auto _v=(value); if(g_nav.field != _v) { g_nav.field=_v; changed=true; } } while(0)
    SET_FIELD(communicationActive, streamActive);
    SET_FIELD(fixValid, validPos);
    SET_FIELD(hasPosition, validPos);
    SET_FIELD(fixType, fixType);
    SET_FIELD(fixQuality, fixQuality);
    SET_FIELD(satellites, satUsed);
    SET_FIELD(satellitesVisible, static_cast<uint8_t>(satVisible));
    SET_FIELD(altitudeValid, validPos && altitudeFresh);
    SET_FIELD(speedValid, validPos && speedFresh);
    SET_FIELD(moving, moving);
    SET_FIELD(courseValid, moving && courseFresh);
    SET_FIELD(timeValid, timeFresh && unixTime >= 1600000000u);
    SET_FIELD(unixTime, unixTime);
    SET_FIELD(bytesReceived, g_totalBytes);
#undef SET_FIELD

    if(fabsf(g_nav.hdop - hdop) > 0.005f) { g_nav.hdop = hdop; changed = true; }
    if(validPos && (fabs(g_nav.latitude - lat) > 1e-8 || fabs(g_nav.longitude - lon) > 1e-8))
    {
        g_nav.latitude = lat;
        g_nav.longitude = lon;
        changed = true;
    }
    if(altitudeFresh && fabsf(g_nav.altitudeMeters - altitude) > 0.05f)
    {
        g_nav.altitudeMeters = altitude;
        g_nav.rawAltitudeMeters = altitude;
        changed = true;
    }
    if(speedFresh && fabsf(g_nav.speedKmh - speedKmh) > 0.05f)
    {
        g_nav.speedKmh = speedKmh;
        g_nav.rawSpeedKmh = speedKmh;
        changed = true;
    }
    if(courseFresh && fabsf(g_nav.courseDegrees - course) > 0.05f)
    {
        g_nav.courseDegrees = course;
        changed = true;
    }

    if(validPos) g_lastFixMs = now;
    if(g_nav.timeValid)
    {
        updateTimeStrings(unixTime);
        maybeSetSystemClock(unixTime);
    }
    if(changed) markChanged();

    if(!wasActive && streamActive)
    {
        FT02_LOGD("[GNSS-UART] NMEA live sats=%u visible=%u fixType=%u fix=%u pass=%lu fail=%lu\n",
                      static_cast<unsigned>(satUsed), static_cast<unsigned>(satVisible),
                      static_cast<unsigned>(fixType), validPos ? 1u : 0u,
                      static_cast<unsigned long>(g_gps.passedChecksum()),
                      static_cast<unsigned long>(g_gps.failedChecksum()));
    }
}

void recoverReceivePath(const char *reason)
{
    const uint32_t now = millis();
    if(g_lastRecoveryMs != 0u && static_cast<uint32_t>(now - g_lastRecoveryMs) < FT02_GNSS_RECOVERY_COOLDOWN_MS)
        return;
    FT02_LOGW("[WARN][GNSS] UART recovery reason=%s parser_resync=1 baud=%lu\n",
                  reason, static_cast<unsigned long>(FT02_GNSS_BAUD));
    g_lastRecoveryMs = now;
    invalidateLiveData(true);
    beginGnssSerial();
}

} // namespace

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
    g_lastFixMs = 0u;
    g_lastTimeSyncMs = 0u;
    g_lastUnixApplied = 0u;
    g_lastGnssByteMs = 0u;
    g_streamStartMs = 0u;
    g_serialOpenedMs = millis();
    g_lastNmeaPassMs = 0u;
    g_lastRecoveryMs = 0u;
    g_totalBytes = 0u;
    g_hardwareKnown = false;
    g_hardwareReady = false;
    g_runtimeEnabled = true;
    g_onlineAnnounced = false;
    g_softwareStandbyRequested = false;
    g_bootWakeProbePending = true;
    markChanged();
    beginGnssSerial();
}

void FT02_GnssPoll()
{
    if(!g_nav.started) return;
    const uint32_t now = millis();
    g_nav.startAgeMs = now - g_startedMs;
    g_nav.lastFixAgeMs = g_lastFixMs == 0u ? 0xFFFFFFFFu : now - g_lastFixMs;
    g_nav.lastTimeSyncAgeMs = g_lastTimeSyncMs == 0u ? 0xFFFFFFFFu : now - g_lastTimeSyncMs;

    if(!g_runtimeEnabled)
    {
        invalidateLiveData(true);
        return;
    }

    const uint32_t beforeBytes = g_totalBytes;
    while(g_gnssSerial.available() > 0)
    {
        const int c = g_gnssSerial.read();
        if(c < 0) break;
        g_gps.encode(static_cast<char>(c));
        ++g_totalBytes;
        g_lastGnssByteMs = millis();
    }
    if(g_totalBytes != beforeBytes && g_streamStartMs == 0u) g_streamStartMs = now;

    updateParserStats(now);
    publishSnapshot(now);

    // Recovery is deliberately byte-stream based only. A powered GNSS receiver
    // can spend a long time acquiring while still producing a healthy UART stream;
    // fix=0 or a temporary lack of checksum-valid NMEA is not a UART fault.
    const bool startupGrace = g_serialOpenedMs != 0u &&
                              static_cast<uint32_t>(now - g_serialOpenedMs) < FT02_GNSS_STARTUP_GRACE_MS;
    if(!startupGrace && g_lastGnssByteMs != 0u &&
       static_cast<uint32_t>(now - g_lastGnssByteMs) >= FT02_GNSS_STREAM_STALL_RECOVERY_MS)
    {
        recoverReceivePath("stream_stall");
        return;
    }
}

FT02GnssSnapshot FT02_GnssSnapshotCurrent()
{
    FT02GnssSnapshot out = g_nav;
    const uint32_t now = millis();
    out.startAgeMs = now - g_startedMs;
    out.lastFixAgeMs = g_lastFixMs == 0u ? 0xFFFFFFFFu : now - g_lastFixMs;
    out.lastTimeSyncAgeMs = g_lastTimeSyncMs == 0u ? 0xFFFFFFFFu : now - g_lastTimeSyncMs;
    return out;
}

bool FT02_GnssSetRuntimeEnabled(bool enabled)
{
    const bool changed = g_runtimeEnabled != enabled;
    g_runtimeEnabled = enabled;
    if(!enabled)
    {
        bool standbySent = true;
        if(g_serialActive)
        {
            standbySent = requestSoftwareStandby();
            closeGnssSerialForStandby();
            FT02_LOGD("[GNSS] OFF transport=closed standby=%s\n", standbySent ? "requested" : "not-confirmed");
        }
        g_hardwareReady = false;
        invalidateLiveData(true);
        return standbySent;
    }
    if(changed || !g_serialActive)
    {
        // Re-open the proven 115200 receive path. No receiver reconfiguration is sent.
        beginGnssSerial();
    }
    return true;
}

bool FT02_GnssRuntimeEnabled() { return g_runtimeEnabled; }
bool FT02_GnssHardwareKnown() { return g_hardwareKnown; }
bool FT02_GnssHardwareReady() { return g_hardwareReady; }
