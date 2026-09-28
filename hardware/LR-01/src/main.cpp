#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <esp_heap_caps.h>
#include <string.h>

#include "FtHardware.h"
#include "FtConfig.h"
#include "LoRaManager.h"
#include "CompassCalibration.h"
#include "GnssBlackbox.h"
#include "UbxGnssMonitor.h"

HardwareSerial GnssSerial(1);
HardwareSerial HostSerial(2);
TinyGPSPlus gps;

// Raw NMEA diagnostics/fallbacks. TinyGPSPlus is the primary parser, while
// these exact-field listeners let LR01 distinguish receiver-side no-fix from
// parser-side issues and provide a standards-based GGA/RMC location fallback.
TinyGPSCustom gnGgaFixQuality(gps, "GNGGA", 6);
TinyGPSCustom gpGgaFixQuality(gps, "GPGGA", 6);
TinyGPSCustom gnGgaSatUsed(gps, "GNGGA", 7);
TinyGPSCustom gpGgaSatUsed(gps, "GPGGA", 7);
TinyGPSCustom gnGgaHdop(gps, "GNGGA", 8);
TinyGPSCustom gpGgaHdop(gps, "GPGGA", 8);
TinyGPSCustom gnGgaLat(gps, "GNGGA", 2);
TinyGPSCustom gpGgaLat(gps, "GPGGA", 2);
TinyGPSCustom gnGgaNs(gps, "GNGGA", 3);
TinyGPSCustom gpGgaNs(gps, "GPGGA", 3);
TinyGPSCustom gnGgaLon(gps, "GNGGA", 4);
TinyGPSCustom gpGgaLon(gps, "GPGGA", 4);
TinyGPSCustom gnGgaEw(gps, "GNGGA", 5);
TinyGPSCustom gpGgaEw(gps, "GPGGA", 5);

TinyGPSCustom gnRmcStatus(gps, "GNRMC", 2);
TinyGPSCustom gpRmcStatus(gps, "GPRMC", 2);
TinyGPSCustom gnRmcLat(gps, "GNRMC", 3);
TinyGPSCustom gpRmcLat(gps, "GPRMC", 3);
TinyGPSCustom gnRmcNs(gps, "GNRMC", 4);
TinyGPSCustom gpRmcNs(gps, "GPRMC", 4);
TinyGPSCustom gnRmcLon(gps, "GNRMC", 5);
TinyGPSCustom gpRmcLon(gps, "GPRMC", 5);
TinyGPSCustom gnRmcEw(gps, "GNRMC", 6);
TinyGPSCustom gpRmcEw(gps, "GPRMC", 6);

TinyGPSCustom gnGnsMode(gps, "GNGNS", 6);
TinyGPSCustom gpGnsMode(gps, "GPGNS", 6);
TinyGPSCustom gnGnsSatUsed(gps, "GNGNS", 7);
TinyGPSCustom gpGnsSatUsed(gps, "GPGNS", 7);
TinyGPSCustom gnGnsHdop(gps, "GNGNS", 8);
TinyGPSCustom gpGnsHdop(gps, "GPGNS", 8);
TinyGPSCustom gnGnsLat(gps, "GNGNS", 2);
TinyGPSCustom gpGnsLat(gps, "GPGNS", 2);
TinyGPSCustom gnGnsNs(gps, "GNGNS", 3);
TinyGPSCustom gpGnsNs(gps, "GPGNS", 3);
TinyGPSCustom gnGnsLon(gps, "GNGNS", 4);
TinyGPSCustom gpGnsLon(gps, "GPGNS", 4);
TinyGPSCustom gnGnsEw(gps, "GNGNS", 5);
TinyGPSCustom gpGnsEw(gps, "GPGNS", 5);

TinyGPSCustom gnGsaFixType(gps, "GNGSA", 2);
TinyGPSCustom gpGsaFixType(gps, "GPGSA", 2);
TinyGPSCustom gaGsaFixType(gps, "GAGSA", 2);
TinyGPSCustom gbGsaFixType(gps, "GBGSA", 2);
TinyGPSCustom glGsaFixType(gps, "GLGSA", 2);
TinyGPSCustom gqGsaFixType(gps, "GQGSA", 2);

TinyGPSCustom gpGsvVisible(gps, "GPGSV", 3);
TinyGPSCustom gaGsvVisible(gps, "GAGSV", 3);
TinyGPSCustom gbGsvVisible(gps, "GBGSV", 3);
TinyGPSCustom glGsvVisible(gps, "GLGSV", 3);
TinyGPSCustom gqGsvVisible(gps, "GQGSV", 3);

LoRaManager lora;
CompassCalibration compassCal;
GnssBlackbox gnssBlackbox;
UbxGnssMonitor ubxGnss;

static constexpr size_t HOST_LINE_MAX = 256;
static constexpr size_t USER_TEXT_MAX = 120;
static constexpr uint32_t DELIVERY_TIMEOUT_MS = 15000;
static constexpr uint32_t NODE_ONLINE_MS = 300000;

struct NavState {
  bool gnssStream = false;
  bool fix = false;
  uint8_t fixType = 0;
  int32_t latE7 = 0;
  int32_t lonE7 = 0;
  int16_t altDm = 0;
  uint8_t satUsed = 0;
  uint16_t satVisible = 0;
  uint16_t hdopX100 = 9999;
  uint16_t speedCms = 0;
  uint32_t unixTime = 0;
  bool timeValid = false;
  uint16_t headingX10 = 0;
  bool compassReady = false;
  uint8_t compassQuality = 0;
};

struct GnssDiagState {
  uint8_t ggaFixQuality = 0;
  uint8_t ggaSatUsed = 0;
  uint16_t ggaHdopX100 = 9999;
  uint8_t gsaFixType = 0;
  uint8_t rmcStatus = 0;      // 0=unknown, 1=V/invalid, 2=A/active
  uint8_t gnsFix = 0;         // 0=unknown/no-fix, 1=GNS posMode reports a solution
  uint8_t gnsSatUsed = 0;
  uint16_t gnsHdopX100 = 9999;
  uint8_t locationSource = 0; // 0=none, 1=TinyGPSPlus, 2=GGA, 3=RMC, 4=GNS, 5=UBX-PVT
};

struct PendingDelivery {
  bool used = false;
  uint32_t coreId = 0;
  uint32_t meshPacketId = 0;
  uint32_t node = 0;
  uint32_t startedMs = 0;
};

NavState nav;
GnssDiagState gnssDiag;
PendingDelivery pending[8];

String hostLine;
bool hostDiscardUntilLf = false;

uint32_t lastGnssByteMs = 0;
uint32_t gnssStreamStartMs = 0;
uint32_t gnssBytes = 0;
size_t gnssRxBufferBytes = 0;
uint32_t gnssPassedChecksumTotal = 0;
uint32_t gnssFailedChecksumTotal = 0;
uint32_t lastParserPassedChecksum = 0;
uint32_t lastParserFailedChecksum = 0;
uint32_t lastNmeaPassMs = 0;
uint32_t lastLocationCommitMs = 0;

// Software GNSS standby does not cut VCC. While disabled, LR01 must not poll or
// write the GNSS UART because UART RX is also the receiver wake source.
bool gnssSoftwareEnabled = true;
uint32_t gnssSoftwareStateChangedMs = 0;
uint32_t outdoorNoFixSinceMs = 0;
uint32_t lastGnssRecoveryMs = 0;
uint32_t gnssRecoveryCount = 0;
uint32_t gnssReceiverResetCount = 0;
uint8_t gnssReceiverRecoveryStage = 0; // manual status / 3=automatic A4 cold sent
uint32_t gnssReceiverStageSinceMs = 0;
uint32_t gnssReceiverResetGraceUntilMs = 0;
// A4 Production receiver-acquisition watchdog state.
uint32_t gnssA4StrongEvidenceMs = 0;
uint32_t gnssA4EvidenceLastMs = 0;
uint32_t gnssA4LastProgressMs = 0;
uint32_t gnssA4PostColdLockoutUntilMs = 0;
uint32_t gnssA4LastAutoColdMs = 0;
uint8_t gnssA4BestEph = 0;
uint8_t gnssA4BestUsed = 0;
uint32_t gnssLogLastSampleMs = 0;
uint32_t gnssLogDetailedUntilMs = 0;
uint32_t gnssLogNoFixSinceMs = 0;
bool gnssLogFixInitialized = false;
bool gnssLogLastFix = false;
uint32_t lastUbxMonVerPollMs = 0;
uint32_t lastUbxPvtPollMs = 0;
uint32_t lastUbxSatPollMs = 0;
uint32_t lastUbxCfgPollMs = 0;
uint32_t gnssCfgBaselineApplyAtMs = 0;
uint32_t gnssCfgBaselineApplyCount = 0;
uint32_t lastNavTx = 0;
uint32_t lastSysTx = 0;
uint32_t lastMeshNodeInfoMs = 0;
uint32_t lastMessageRevision = 0;
uint32_t lastDeliveryRevision = 0;
uint32_t uartErrorCount = 0;
uint32_t radioResetCount = 0;
uint32_t lastCompassCalStateTx = 0;
String usbLine;
bool usbDiscardUntilLf = false;
bool usbCommandActive = false;

static void hostSend(const String &s) {
  // Host UART is authoritative. USB mirrors only explicit console-command
  // responses, keeping normal production operation quiet.
  HostSerial.println(s);
  if (usbCommandActive) Serial.println(s);
}

static void hostError(uint8_t code, const char *msg) {
  String s = "LR01_ERR code=";
  s += String(code);
  s += " message=\"";
  s += msg;
  s += "\"";
  hostSend(s);
}

static bool parseUint32(const String &s, uint32_t &out) {
  if (!s.length()) return false;
  char *end = nullptr;
  unsigned long v = strtoul(s.c_str(), &end, 10);
  if (!end || *end != '\0') return false;
  out = (uint32_t)v;
  return true;
}

static uint32_t parseNodeHex(String s) {
  s.trim();
  if (s.startsWith("!")) s.remove(0, 1);
  if (s.startsWith("0x") || s.startsWith("0X")) s.remove(0, 2);
  if (s.length() != 8) return 0;
  for (size_t i = 0; i < (size_t)s.length(); ++i) {
    if (!isxdigit((unsigned char)s[(int)i])) return 0;
  }
  return (uint32_t)strtoul(s.c_str(), nullptr, 16);
}

static bool validUtf8Length(const String &s) {
  return s.length() > 0 && (size_t)s.length() <= USER_TEXT_MAX;
}

static bool qmcWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(FtHardware::QMC_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool qmcBegin() {
  for (int i = 0; i < 5; ++i) {
    Wire.beginTransmission(FtHardware::QMC_ADDR);
    if (Wire.endTransmission() == 0) {
      qmcWrite(0x0B, 0x01);
      delay(10);
      if (qmcWrite(0x09, 0x1D)) {
        delay(20);
        return true;
      }
    }
    delay(100);
  }
  return false;
}

static bool qmcReadRaw(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(FtHardware::QMC_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)FtHardware::QMC_ADDR, 6) != 6) return false;

  uint8_t xl = Wire.read(), xh = Wire.read();
  uint8_t yl = Wire.read(), yh = Wire.read();
  uint8_t zl = Wire.read(), zh = Wire.read();

  x = (int16_t)((xh << 8) | xl);
  y = (int16_t)((yh << 8) | yl);
  z = (int16_t)((zh << 8) | zl);
  return true;
}

static bool qmcComputeHeading(int16_t rawX, int16_t rawY, int16_t rawZ,
                              uint16_t &headingX10, uint8_t &quality) {
  float x = 0.0f, y = 0.0f, z = 0.0f;
  compassCal.apply((float)rawX, (float)rawY, (float)rawZ, x, y, z);

  // All three axes are calibrated. This hardware has no accelerometer for tilt
  // compensation, so magnetic azimuth is calculated from corrected X/Y while
  // corrected Z participates in calibration quality and future tilt support.
  float h = atan2f(-y, x) * 180.0f / PI;
  if (h < 0.0f) h += 360.0f;

  // Body-frame transform is intentionally applied AFTER magnetic calibration.
  // Bench validation showed the installed horizontal frame has opposite yaw
  // handedness, so corrected Y is inverted in atan2 above. The remaining fixed
  // device-front alignment is applied as a yaw offset here. Host protocol fields
  // and saved hard/soft-iron calibration semantics remain unchanged.
  h += FT_COMPASS_INSTALL_YAW_OFFSET_DEG;
  while (h >= 360.0f) h -= 360.0f;
  while (h < 0.0f) h += 360.0f;
  headingX10 = (uint16_t)lroundf(h * 10.0f);
  if (headingX10 >= 3600U) headingX10 -= 3600U;

  const float mag = sqrtf(x * x + y * y + z * z);
  if (!isfinite(mag) || mag < 50.0f || mag > 20000.0f) {
    quality = 0;
    return true;
  }

  if (compassCal.isCalibrated()) {
    quality = compassCal.savedQuality();
  } else {
    quality = 1; // sensor is live, but heading is not calibrated
  }
  return true;
}

static bool leapYear(int y) {
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static uint32_t utcToUnix(int y, int mo, int d, int hh, int mm, int ss) {
  static const uint16_t beforeMonth[] =
    {0,0,31,59,90,120,151,181,212,243,273,304,334};
  if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0;
  uint32_t days = 0;
  for (int yr = 1970; yr < y; ++yr) days += leapYear(yr) ? 366 : 365;
  days += beforeMonth[mo];
  if (mo > 2 && leapYear(y)) days++;
  days += (uint32_t)(d - 1);
  return days * 86400UL + (uint32_t)hh * 3600UL + (uint32_t)mm * 60UL + (uint32_t)ss;
}

static void appendGnssLog(GnssLogEvent event, bool flushNow);
static void resetGnssLocalAcquisitionState(uint32_t now);

static uint16_t customToFreshU16(TinyGPSCustom &c, uint32_t maxAgeMs) {
  if (!c.isValid() || c.age() > maxAgeMs) return 0;
  long v = atol(c.value());
  if (v < 0) v = 0;
  if (v > 65535) v = 65535;
  return (uint16_t)v;
}

static TinyGPSCustom *freshestCustom(TinyGPSCustom *a, TinyGPSCustom *b, uint32_t maxAgeMs) {
  const bool av = a && a->isValid() && a->age() <= maxAgeMs;
  const bool bv = b && b->isValid() && b->age() <= maxAgeMs;
  if (!av) return bv ? b : nullptr;
  if (!bv) return a;
  return a->age() <= b->age() ? a : b;
}

static uint8_t freshCustomU8(TinyGPSCustom *c, uint32_t maxAgeMs, uint8_t fallback = 0) {
  if (!c || !c->isValid() || c->age() > maxAgeMs) return fallback;
  long v = atol(c->value());
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

static uint16_t freshCustomDecimalX100(TinyGPSCustom *c, uint32_t maxAgeMs, uint16_t fallback = 9999) {
  if (!c || !c->isValid() || c->age() > maxAgeMs) return fallback;
  double v = atof(c->value());
  if (!isfinite(v) || v < 0.0) return fallback;
  v *= 100.0;
  if (v > 65535.0) v = 65535.0;
  return (uint16_t)lround(v);
}

static uint8_t freshestGsaFixType() {
  TinyGPSCustom *items[] = {&gnGsaFixType, &gpGsaFixType, &gaGsaFixType,
                            &gbGsaFixType, &glGsaFixType, &gqGsaFixType};
  TinyGPSCustom *best = nullptr;
  for (TinyGPSCustom *c : items) {
    if (!c->isValid() || c->age() > FT_GNSS_FIELD_FRESH_MS) continue;
    if (!best || c->age() < best->age()) best = c;
  }
  return freshCustomU8(best, FT_GNSS_FIELD_FRESH_MS, 0);
}

static uint8_t freshestRmcStatus() {
  TinyGPSCustom *c = freshestCustom(&gnRmcStatus, &gpRmcStatus, FT_GNSS_FIELD_FRESH_MS);
  if (!c || !c->value()[0]) return 0;
  if (c->value()[0] == 'A') return 2;
  if (c->value()[0] == 'V') return 1;
  return 0;
}

static bool parseNmeaCoordinate(const char *raw, const char *hemisphere,
                                bool latitude, int32_t &outE7) {
  if (!raw || !*raw || !hemisphere || !*hemisphere) return false;
  char *end = nullptr;
  const double dm = strtod(raw, &end);
  if (end == raw || !isfinite(dm) || dm <= 0.0) return false;
  const int degreesPart = (int)(dm / 100.0);
  const double minutes = dm - (double)degreesPart * 100.0;
  if (minutes < 0.0 || minutes >= 60.0) return false;
  const int maxDegrees = latitude ? 90 : 180;
  if (degreesPart < 0 || degreesPart > maxDegrees) return false;
  double decimalDegrees = degreesPart + minutes / 60.0;
  const char h = hemisphere[0];
  if (h == 'S' || h == 'W') decimalDegrees = -decimalDegrees;
  else if (h != 'N' && h != 'E') return false;
  const double scaled = decimalDegrees * 10000000.0;
  if (scaled < -2147483648.0 || scaled > 2147483647.0) return false;
  outE7 = (int32_t)llround(scaled);
  return true;
}

static bool customPairFresh(TinyGPSCustom *a, TinyGPSCustom *b, uint32_t maxAgeMs) {
  return a && b && a->isValid() && b->isValid() &&
         a->age() <= maxAgeMs && b->age() <= maxAgeMs;
}

static bool fallbackLocationFromGga(int32_t &latE7, int32_t &lonE7) {
  TinyGPSCustom *q = freshestCustom(&gnGgaFixQuality, &gpGgaFixQuality, FT_GNSS_FIELD_FRESH_MS);
  if (!q || freshCustomU8(q, FT_GNSS_FIELD_FRESH_MS, 0) == 0) return false;
  const bool useGn = (q == &gnGgaFixQuality);
  TinyGPSCustom *lat = useGn ? &gnGgaLat : &gpGgaLat;
  TinyGPSCustom *ns  = useGn ? &gnGgaNs  : &gpGgaNs;
  TinyGPSCustom *lon = useGn ? &gnGgaLon : &gpGgaLon;
  TinyGPSCustom *ew  = useGn ? &gnGgaEw  : &gpGgaEw;
  if (!customPairFresh(lat, ns, FT_GNSS_FIELD_FRESH_MS) ||
      !customPairFresh(lon, ew, FT_GNSS_FIELD_FRESH_MS)) return false;
  return parseNmeaCoordinate(lat->value(), ns->value(), true, latE7) &&
         parseNmeaCoordinate(lon->value(), ew->value(), false, lonE7);
}

static bool fallbackLocationFromRmc(int32_t &latE7, int32_t &lonE7) {
  TinyGPSCustom *st = freshestCustom(&gnRmcStatus, &gpRmcStatus, FT_GNSS_FIELD_FRESH_MS);
  if (!st || st->value()[0] != 'A') return false;
  const bool useGn = (st == &gnRmcStatus);
  TinyGPSCustom *lat = useGn ? &gnRmcLat : &gpRmcLat;
  TinyGPSCustom *ns  = useGn ? &gnRmcNs  : &gpRmcNs;
  TinyGPSCustom *lon = useGn ? &gnRmcLon : &gpRmcLon;
  TinyGPSCustom *ew  = useGn ? &gnRmcEw  : &gpRmcEw;
  if (!customPairFresh(lat, ns, FT_GNSS_FIELD_FRESH_MS) ||
      !customPairFresh(lon, ew, FT_GNSS_FIELD_FRESH_MS)) return false;
  return parseNmeaCoordinate(lat->value(), ns->value(), true, latE7) &&
         parseNmeaCoordinate(lon->value(), ew->value(), false, lonE7);
}

static uint8_t freshestGnsFix() {
  TinyGPSCustom *mode = freshestCustom(&gnGnsMode, &gpGnsMode, FT_GNSS_FIELD_FRESH_MS);
  if (!mode || !mode->value()[0]) return 0;
  for (const char *p = mode->value(); *p; ++p) {
    if (*p != 'N') return 1;
  }
  return 0;
}

static bool fallbackLocationFromGns(int32_t &latE7, int32_t &lonE7) {
  TinyGPSCustom *mode = freshestCustom(&gnGnsMode, &gpGnsMode, FT_GNSS_FIELD_FRESH_MS);
  if (!mode || freshestGnsFix() == 0) return false;
  const bool useGn = (mode == &gnGnsMode);
  TinyGPSCustom *lat = useGn ? &gnGnsLat : &gpGnsLat;
  TinyGPSCustom *ns  = useGn ? &gnGnsNs  : &gpGnsNs;
  TinyGPSCustom *lon = useGn ? &gnGnsLon : &gpGnsLon;
  TinyGPSCustom *ew  = useGn ? &gnGnsEw  : &gpGnsEw;
  if (!customPairFresh(lat, ns, FT_GNSS_FIELD_FRESH_MS) ||
      !customPairFresh(lon, ew, FT_GNSS_FIELD_FRESH_MS)) return false;
  return parseNmeaCoordinate(lat->value(), ns->value(), true, latE7) &&
         parseNmeaCoordinate(lon->value(), ew->value(), false, lonE7);
}

static void sendUbxFrame(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t length) {
  uint8_t ckA = 0, ckB = 0;
  auto checksumByte = [&](uint8_t v) {
    ckA = (uint8_t)(ckA + v);
    ckB = (uint8_t)(ckB + ckA);
  };
  const uint8_t lenLo = (uint8_t)(length & 0xFF);
  const uint8_t lenHi = (uint8_t)(length >> 8);
  checksumByte(cls); checksumByte(id); checksumByte(lenLo); checksumByte(lenHi);
  for (uint16_t i = 0; i < length; ++i) checksumByte(payload[i]);

  GnssSerial.write((uint8_t)0xB5); GnssSerial.write((uint8_t)0x62);
  GnssSerial.write(cls); GnssSerial.write(id);
  GnssSerial.write(lenLo); GnssSerial.write(lenHi);
  if (length) GnssSerial.write(payload, length);
  GnssSerial.write(ckA); GnssSerial.write(ckB);
  GnssSerial.flush();
}


static void appendLe16(uint8_t *buf, size_t &pos, uint16_t v) {
  buf[pos++] = (uint8_t)(v & 0xFFU);
  buf[pos++] = (uint8_t)(v >> 8);
}

static void appendLe32(uint8_t *buf, size_t &pos, uint32_t v) {
  buf[pos++] = (uint8_t)(v & 0xFFU);
  buf[pos++] = (uint8_t)((v >> 8) & 0xFFU);
  buf[pos++] = (uint8_t)((v >> 16) & 0xFFU);
  buf[pos++] = (uint8_t)((v >> 24) & 0xFFU);
}

static void appendCfgU1(uint8_t *buf, size_t &pos, uint32_t key, uint8_t value) {
  appendLe32(buf, pos, key);
  buf[pos++] = value;
}

static void appendCfgU2(uint8_t *buf, size_t &pos, uint32_t key, uint16_t value) {
  appendLe32(buf, pos, key);
  appendLe16(buf, pos, value);
}

static void requestUbxMonitorVersion() {
  sendUbxFrame(0x0A, 0x04, nullptr, 0); // UBX-MON-VER
}

static void requestUbxPvt() {
  sendUbxFrame(0x01, 0x07, nullptr, 0); // UBX-NAV-PVT
}

static void requestUbxSat() {
  sendUbxFrame(0x01, 0x35, nullptr, 0); // UBX-NAV-SAT
}

static void sendCfgValGetKeys(const uint32_t *keys, size_t count) {
  if (!keys || count == 0 || count > 64) return;
  uint8_t payload[4 + 64 * 4]{};
  size_t pos = 0;
  payload[pos++] = 0x00; // poll request version
  payload[pos++] = 0x00; // RAM layer
  payload[pos++] = 0x00; payload[pos++] = 0x00; // position
  for (size_t i = 0; i < count; ++i) appendLe32(payload, pos, keys[i]);
  sendUbxFrame(0x06, 0x8B, payload, (uint16_t)pos); // UBX-CFG-VALGET
}

static void requestGnssConfigSnapshot() {
  // Split into independent requests. UBX-CFG-VALGET NAKs the request if even
  // one key is unknown, so grouping keeps the core NAV evidence available on
  // receiver firmware variants that may omit an optional PM/signal item.
  static const uint32_t navKeys[] = {
    0x20110011UL, // CFG-NAVSPG-FIXMODE
    0x10110013UL, // CFG-NAVSPG-INIFIX3D
    0x20110021UL, // CFG-NAVSPG-DYNMODEL
    0x201100A1UL, // CFG-NAVSPG-INFIL_MINSVS
    0x201100A2UL, // CFG-NAVSPG-INFIL_MAXSVS
    0x201100A3UL, // CFG-NAVSPG-INFIL_MINCNO
    0x201100A4UL, // CFG-NAVSPG-INFIL_MINELEV
    0x201100AAUL, // CFG-NAVSPG-INFIL_NCNOTHRS
    0x201100ABUL, // CFG-NAVSPG-INFIL_CNOTHRS
    0x301100B1UL, // CFG-NAVSPG-OUTFIL_PDOP
    0x301100B3UL, // CFG-NAVSPG-OUTFIL_PACC
    0x201100D6UL, // CFG-NAVSPG-SIGATTCOMP
    0x30210001UL, // CFG-RATE-MEAS
    0x30210002UL  // CFG-RATE-NAV
  };
  static const uint32_t pmKeys[] = {
    0x20D00001UL, // CFG-PM-OPERATEMODE
    0x10D0000AUL  // CFG-PM-UPDATEEPH
  };
  static const uint32_t signalKeys[] = {
    0x1031001FUL, // CFG-SIGNAL-GPS_ENA
    0x10310001UL, // CFG-SIGNAL-GPS_L1CA_ENA
    0x10310021UL, // CFG-SIGNAL-GAL_ENA
    0x10310007UL, // CFG-SIGNAL-GAL_E1_ENA
    0x10310022UL, // CFG-SIGNAL-BDS_ENA
    0x1031000DUL, // CFG-SIGNAL-BDS_B1_ENA
    0x1031000FUL  // CFG-SIGNAL-BDS_B1C_ENA
  };
  sendCfgValGetKeys(navKeys, sizeof(navKeys) / sizeof(navKeys[0]));
  sendCfgValGetKeys(pmKeys, sizeof(pmKeys) / sizeof(pmKeys[0]));
  sendCfgValGetKeys(signalKeys, sizeof(signalKeys) / sizeof(signalKeys[0]));
}

static void applyGnssAcquisitionBaselineRam() {
  // Apply only to RAM. Nothing is written to BBR/Flash. Core NAVSPG settings
  // and PM settings are sent separately so an optional key cannot reject the
  // entire acquisition baseline.
  uint8_t navPayload[96]{};
  size_t pos = 0;
  navPayload[pos++] = 0x00; // UBX-CFG-VALSET version 0
  navPayload[pos++] = 0x01; // RAM layer only
  navPayload[pos++] = 0x00; navPayload[pos++] = 0x00;
  appendCfgU1(navPayload, pos, 0x20110011UL, 3);   // FIXMODE=AUTO
  appendCfgU1(navPayload, pos, 0x10110013UL, 0);   // INIFIX3D=false
  appendCfgU1(navPayload, pos, 0x20110021UL, 0);   // DYNMODEL=PORTABLE
  appendCfgU1(navPayload, pos, 0x201100A1UL, 3);   // MINSVS=3
  appendCfgU1(navPayload, pos, 0x201100A2UL, 32);  // MAXSVS=32
  appendCfgU1(navPayload, pos, 0x201100A3UL, 6);   // MINCNO=6 dBHz
  appendCfgU1(navPayload, pos, 0x201100A4UL, 5);   // MINELEV=5 deg
  appendCfgU1(navPayload, pos, 0x201100AAUL, 0);   // no extra C/N0 gate
  appendCfgU1(navPayload, pos, 0x201100ABUL, 0);   // no extra C/N0 threshold
  appendCfgU2(navPayload, pos, 0x301100B1UL, 250); // PDOP mask 25.0
  appendCfgU2(navPayload, pos, 0x301100B3UL, 100); // PACC mask 100 m
  appendCfgU1(navPayload, pos, 0x201100D6UL, 255); // signal attenuation compensation=AUTO
  sendUbxFrame(0x06, 0x8A, navPayload, (uint16_t)pos);

  uint8_t pmPayload[20]{};
  pos = 0;
  pmPayload[pos++] = 0x00;
  pmPayload[pos++] = 0x01; // RAM only
  pmPayload[pos++] = 0x00; pmPayload[pos++] = 0x00;
  appendCfgU1(pmPayload, pos, 0x20D00001UL, 0); // power mode=FULL
  appendCfgU1(pmPayload, pos, 0x10D0000AUL, 1); // keep ephemeris updated
  sendUbxFrame(0x06, 0x8A, pmPayload, (uint16_t)pos);

  gnssCfgBaselineApplyCount++;
  appendGnssLog(GnssLogEvent::CFG_BASELINE, true);
  if (FT_VERBOSE_DIAGNOSTICS) Serial.println("[GNSS CFG] RAM acquisition baseline applied");
}

static void sendGnssSoftwareStandbyRequest() {
  // u-blox M10 UBX-RXM-PMREQ (0x02 0x41), version 0.
  // duration=0: remain in software standby until an enabled wake source fires.
  // flags: backup(bit1) + force(bit2) = 0x00000006.
  // wakeupSources: UART RX edge(bit3) = 0x00000008.
  uint8_t payload[16]{};
  payload[0] = 0x00; // version
  // payload[1..3] reserved
  // duration [4..7] = 0
  payload[8] = 0x06; // backup + force
  // flags [9..11] = 0
  payload[12] = 0x08; // UART RX wake source
  // wakeupSources [13..15] = 0
  sendUbxFrame(0x02, 0x41, payload, sizeof(payload));
}

static void setGnssSoftwareEnabled(bool enabled) {
  if (enabled == gnssSoftwareEnabled) return;
  const uint32_t now = millis();

  if (!enabled) {
    // Save the transition before the receiver goes quiet. Do not end the UART:
    // UART RX on the receiver is the wake source for software standby.
    appendGnssLog(GnssLogEvent::GNSS_SOFT_OFF, true);
    sendGnssSoftwareStandbyRequest();
    delay(25); // allow the PMREQ frame to leave the UART before suppressing polls

    gnssSoftwareEnabled = false;
    gnssSoftwareStateChangedMs = now;
    resetGnssLocalAcquisitionState(now);
    nav.timeValid = false;
    nav.unixTime = 0;
    lastUbxPvtPollMs = lastUbxSatPollMs = lastUbxMonVerPollMs = lastUbxCfgPollMs = 0;
    gnssCfgBaselineApplyAtMs = 0;
    Serial.println("[GNSS POWER] software_standby=1");
    return;
  }

  // An edge on M10 UART RX wakes software standby. The first byte is only a
  // wake stimulus and is intentionally not a valid NMEA/UBX frame. The parser
  // will resynchronize on the first subsequent valid frame.
  gnssSoftwareEnabled = true; // prevent background logic from treating wake as OFF
  gnssSoftwareStateChangedMs = now;
  GnssSerial.write((uint8_t)0xFF);
  GnssSerial.flush();
  delay(120);

  resetGnssLocalAcquisitionState(millis());
  lastUbxPvtPollMs = lastUbxSatPollMs = lastUbxMonVerPollMs = lastUbxCfgPollMs = 0;
  gnssCfgBaselineApplyAtMs = millis() + 1200UL; // standby clears RAM config
  appendGnssLog(GnssLogEvent::GNSS_SOFT_ON, true);
  requestUbxMonitorVersion();
  Serial.println("[GNSS POWER] software_standby=0 wake=UART_RX");
}

static void sendGnssSoftwarePowerState() {
  String s = "GNSS_POWER_STATE enabled=";
  s += gnssSoftwareEnabled ? "1" : "0";
  s += " mode=";
  s += gnssSoftwareEnabled ? "ACTIVE" : "SOFTWARE_STANDBY";
  s += " stream=";
  s += nav.gnssStream ? "1" : "0";
  s += " fix=";
  s += nav.fix ? "1" : "0";
  s += " changed_ms=" + String(gnssSoftwareStateChangedMs);
  hostSend(s);
}

static void serviceUbxDiagnostics(uint32_t now) {
  if (lastUbxPvtPollMs == 0 || now - lastUbxPvtPollMs >= 1000UL) {
    lastUbxPvtPollMs = now;
    requestUbxPvt();
  }
  if (lastUbxSatPollMs == 0 || now - lastUbxSatPollMs >= 5000UL) {
    lastUbxSatPollMs = now;
    requestUbxSat();
  }
  const uint32_t monInterval = ubxGnss.monVerSeen() ? 300000UL : 15000UL;
  if (lastUbxMonVerPollMs == 0 || now - lastUbxMonVerPollMs >= monInterval) {
    lastUbxMonVerPollMs = now;
    requestUbxMonitorVersion();
  }
  if (lastUbxCfgPollMs == 0 || now - lastUbxCfgPollMs >= 30000UL) {
    lastUbxCfgPollMs = now;
    requestGnssConfigSnapshot();
  }
  if (gnssCfgBaselineApplyAtMs != 0 && (int32_t)(now - gnssCfgBaselineApplyAtMs) >= 0) {
    gnssCfgBaselineApplyAtMs = 0;
    applyGnssAcquisitionBaselineRam();
    lastUbxCfgPollMs = now - 29000UL; // re-read RAM config shortly after apply
  }
}

static GnssLogEvent receiverEventForStage(uint8_t stage) {
  if (stage == 1) return GnssLogEvent::RECEIVER_HOT;
  if (stage == 2) return GnssLogEvent::RECEIVER_WARM;
  return GnssLogEvent::RECEIVER_COLD;
}

static const char *receiverStageName(uint8_t stage) {
  switch (stage) {
    case 1: return "hot";
    case 2: return "warm";
    case 3: return "cold";
    default: return "none";
  }
}

static void resetGnssA4ProgressState(uint32_t now) {
  gnssA4StrongEvidenceMs = 0;
  gnssA4EvidenceLastMs = now;
  gnssA4LastProgressMs = now;
  gnssA4BestEph = 0;
  gnssA4BestUsed = 0;
}

static void resetGnssLocalAcquisitionState(uint32_t now) {
  lastGnssByteMs = 0;
  gnssStreamStartMs = 0;
  lastNmeaPassMs = 0;
  outdoorNoFixSinceMs = 0;
  lastLocationCommitMs = 0;
  nav.gnssStream = false;
  nav.fix = false;
  nav.fixType = 0;
  nav.satUsed = 0;
  nav.satVisible = 0;
  nav.hdopX100 = 9999;
  resetGnssA4ProgressState(now);
}

static void issueGnssReceiverReset(uint8_t stage, const char *reason, bool manual = false) {
  if (stage < 1 || stage > 3) return;
  const uint32_t now = millis();
  const uint16_t navBbrMask = stage == 1 ? 0x0000U : (stage == 2 ? 0x0001U : 0xFFFFU);
  const uint8_t resetMode = 0x01; // controlled software reset; preserve stored config layers
  const uint8_t payload[4] = {
    (uint8_t)(navBbrMask & 0xFF), (uint8_t)(navBbrMask >> 8),
    resetMode,
    0x00
  };

  Serial.print("[GNSS RECEIVER RESET] stage=");
  Serial.print(receiverStageName(stage));
  Serial.print(" reason=");
  Serial.print(reason ? reason : "manual");
  Serial.print(" manual=");
  Serial.println(manual ? 1 : 0);

  gnssReceiverResetCount++;
  gnssReceiverRecoveryStage = stage;
  gnssReceiverStageSinceMs = now;
  appendGnssLog(receiverEventForStage(stage), true);
  sendUbxFrame(0x06, 0x04, payload, sizeof(payload)); // UBX-CFG-RST

  // CFG-RST is not reliably ACKed. Keep the LR01 UART/parser alive so the first
  // returning NMEA/UBX frame resynchronizes naturally.
  gnssReceiverResetGraceUntilMs = now + FT_GNSS_RECEIVER_RESET_GRACE_MS;
  gnssCfgBaselineApplyAtMs = now + 3000UL;
  lastUbxPvtPollMs = lastUbxSatPollMs = lastUbxMonVerPollMs = lastUbxCfgPollMs = 0;
  resetGnssLocalAcquisitionState(now);

  // A4 automatic cold recovery is intentionally one-shot for a long window.
  // The field test needed ~5 minutes after cold reset to build ephemeris and
  // reach a stable 3D fix, so never disturb that reacquisition period.
  if (!manual && stage == 3) {
    gnssA4LastAutoColdMs = now;
    gnssA4PostColdLockoutUntilMs = now + FT_GNSS_A4_POST_COLD_LOCKOUT_MS;
  }
}

static bool receiverClaimsCurrentFix(uint32_t now) {
  const UbxPvtSnapshot &up = ubxGnss.pvt();
  const bool ubxFix = up.valid &&
                      now - up.updatedMs <= FT_GNSS_FIELD_FRESH_MS &&
                      up.fixOk && up.fixType >= 2 && up.fixType <= 4;
  return nav.fix ||
         gnssDiag.ggaFixQuality > 0 ||
         gnssDiag.rmcStatus == 2 ||
         gnssDiag.gnsFix != 0 ||
         gnssDiag.gsaFixType >= 2 ||
         ubxFix;
}

static void serviceGnssReceiverRecovery(uint32_t now, bool nmeaHealthy) {
  if (gnssReceiverResetGraceUntilMs != 0 &&
      (int32_t)(gnssReceiverResetGraceUntilMs - now) > 0) return;

  if (receiverClaimsCurrentFix(now)) {
    resetGnssA4ProgressState(now);
    gnssReceiverRecoveryStage = 0;
    return;
  }

  // Never diagnose a receiver-side acquisition lock from a sick transport.
  if (!nav.gnssStream || !nmeaHealthy) {
    resetGnssA4ProgressState(now);
    return;
  }

  const UbxSatSnapshot &us = ubxGnss.sat();
  const bool satFresh = us.valid && now - us.updatedMs <= FT_GNSS_GSV_FRESH_MS;
  if (!satFresh) {
    resetGnssA4ProgressState(now);
    return;
  }

  if (gnssA4EvidenceLastMs == 0) gnssA4EvidenceLastMs = now;
  uint32_t dt = now - gnssA4EvidenceLastMs;
  gnssA4EvidenceLastMs = now;
  if (dt > 1000UL) dt = 1000UL;

  const bool strongSky = us.numSv >= FT_GNSS_A4_MIN_VISIBLE_SV &&
                         us.cnoMax >= FT_GNSS_A4_STRONG_CNO_MAX &&
                         us.cnoGe20 >= FT_GNSS_A4_MIN_CNO20_SV;
  const bool weakSky = us.numSv >= FT_GNSS_A4_WEAK_VISIBLE_SV &&
                       us.cnoMax >= FT_GNSS_A4_WEAK_CNO_MAX &&
                       us.cnoGe20 >= FT_GNSS_A4_WEAK_CNO20_SV;

  // Field logs show normal outdoor C/N0 can oscillate while walking. Use a
  // leaky weighted evidence bucket instead of requiring an unrealistically
  // perfect uninterrupted sky window.
  if (strongSky) {
    gnssA4StrongEvidenceMs = min<uint32_t>(FT_GNSS_A4_EVIDENCE_CAP_MS,
                                            gnssA4StrongEvidenceMs + dt);
  } else if (weakSky) {
    gnssA4StrongEvidenceMs = min<uint32_t>(FT_GNSS_A4_EVIDENCE_CAP_MS,
                                            gnssA4StrongEvidenceMs + dt / 4U);
  } else {
    const uint32_t decay = dt * 2U;
    gnssA4StrongEvidenceMs = gnssA4StrongEvidenceMs > decay
        ? gnssA4StrongEvidenceMs - decay : 0;
  }

  // Any increase in ephemeris or used-SV is genuine acquisition progress. Give
  // the M10 time to finish instead of resetting it while it is improving.
  if (us.ephAvail > gnssA4BestEph || us.used > gnssA4BestUsed) {
    gnssA4BestEph = max<uint8_t>(gnssA4BestEph, us.ephAvail);
    gnssA4BestUsed = max<uint8_t>(gnssA4BestUsed, us.used);
    gnssA4LastProgressMs = now;
  }

  // Once any SV is being used, the receiver is demonstrably acquiring. Do not
  // reset it; keep only enough evidence to diagnose a later genuine stall.
  if (us.used > 0) {
    gnssA4StrongEvidenceMs = 0;
    return;
  }

  if (gnssA4PostColdLockoutUntilMs != 0 &&
      (int32_t)(gnssA4PostColdLockoutUntilMs - now) > 0) return;

  if (gnssA4LastAutoColdMs != 0 &&
      now - gnssA4LastAutoColdMs < FT_GNSS_A4_COLD_RETRY_MS) return;

  if (gnssA4StrongEvidenceMs < FT_GNSS_A4_EVIDENCE_TRIGGER_MS) return;
  if (gnssA4LastProgressMs == 0 ||
      now - gnssA4LastProgressMs < FT_GNSS_A4_NO_PROGRESS_MS) return;

  issueGnssReceiverReset(3, "a4_strong_sky_no_acquisition_progress", false);
}

static void beginGnssSerial() {
  // HardwareSerial requires the RX buffer size to be set before begin().
  gnssRxBufferBytes = GnssSerial.setRxBufferSize(FT_GNSS_RX_BUFFER_BYTES);
  GnssSerial.begin(FtHardware::GNSS_BAUD, SERIAL_8N1,
                   FtHardware::GNSS_RX_PIN, FtHardware::GNSS_TX_PIN);
}

static GnssLogEvent recoveryEventForReason(const char *reason) {
  if (strcmp(reason, "stream_stall") == 0) return GnssLogEvent::RECOVERY_STREAM;
  if (strcmp(reason, "nmea_stall") == 0) return GnssLogEvent::RECOVERY_NMEA;
  return GnssLogEvent::RECOVERY_NO_FIX;
}

static void appendGnssLog(GnssLogEvent event, bool flushNow) {
  const uint32_t now = millis();
  GnssLogRecord r{};
  r.uptimeSec = now / 1000UL;
  r.gnssBytes = gnssBytes;
  r.nmeaOk = gnssPassedChecksumTotal;
  r.nmeaBad = gnssFailedChecksumTotal;
  r.locAgeMs = lastLocationCommitMs == 0 ? 0xFFFFFFFFUL : now - lastLocationCommitMs;
  r.byteAgeMs = lastGnssByteMs == 0 ? 0xFFFFFFFFUL : now - lastGnssByteMs;
  r.satVisible = nav.satVisible;
  r.hdopX100 = nav.hdopX100;
  r.recoveries = (uint16_t)min<uint32_t>(gnssRecoveryCount, 65535UL);
  r.satUsed = nav.satUsed;
  r.fixType = nav.fixType;
  r.flags = (nav.gnssStream ? 0x01 : 0x00) |
            (nav.fix ? 0x02 : 0x00) |
            (nav.timeValid ? 0x04 : 0x00);
  r.event = (uint8_t)event;
  r.ggaFixQuality = gnssDiag.ggaFixQuality;
  r.ggaSatUsed = gnssDiag.ggaSatUsed;
  r.gsaFixType = gnssDiag.gsaFixType;
  r.rmcStatus = gnssDiag.rmcStatus;
  r.gnsFix = gnssDiag.gnsFix;
  r.gnsSatUsed = gnssDiag.gnsSatUsed;
  r.locationSource = gnssDiag.locationSource;
  r.receiverResets = (uint16_t)min<uint32_t>(gnssReceiverResetCount, 65535UL);

  const UbxPvtSnapshot &up = ubxGnss.pvt();
  const UbxSatSnapshot &us = ubxGnss.sat();
  const UbxConfigSnapshot &uc = ubxGnss.cfg();
  const bool pvtFresh = up.valid && now - up.updatedMs <= FT_GNSS_FIELD_FRESH_MS;
  r.ubxFlags = (ubxGnss.protocolSeen() ? 0x01 : 0x00) |
               (ubxGnss.monVerSeen() ? 0x02 : 0x00) |
               (pvtFresh ? 0x04 : 0x00) |
               (pvtFresh && up.fixOk ? 0x08 : 0x00) |
               (uc.valid ? 0x10 : 0x00);
  r.ubxFixType = pvtFresh ? up.fixType : 0;
  r.ubxNumSv = pvtFresh ? up.numSv : 0;
  const bool satFresh = us.valid && now - us.updatedMs <= FT_GNSS_GSV_FRESH_MS;
  r.ubxSatUsed = satFresh ? us.used : 0;
  r.ubxCnoMax = satFresh ? us.cnoMax : 0;
  r.ubxCnoGe20 = satFresh ? us.cnoGe20 : 0;
  r.ubxCnoGe30 = satFresh ? us.cnoGe30 : 0;
  r.ubxEphAvail = satFresh ? us.ephAvail : 0;
  r.cfgFixMode = uc.valid ? uc.fixMode : 0xFF;
  r.cfgMinSvs = uc.valid ? uc.minSvs : 0xFF;
  r.cfgMaxSvs = uc.valid ? uc.maxSvs : 0xFF;
  r.cfgMinCno = uc.valid ? uc.minCno : 0xFF;
  r.cfgMinElev = uc.valid ? uc.minElev : 127;
  r.cfgNCnoThrs = uc.valid ? uc.nCnoThrs : 0xFF;
  r.cfgCnoThrs = uc.valid ? uc.cnoThrs : 0xFF;
  r.cfgSigAttenComp = uc.valid ? uc.sigAttenComp : 0xFF;
  r.cfgPmMode = uc.valid ? uc.pmOperateMode : 0xFF;
  r.cfgPmUpdateEph = uc.valid ? uc.pmUpdateEph : 0xFF;
  r.cfgSignalMask = uc.valid ? uc.signalMask : 0;
  r.cfgPDopX10 = uc.valid ? uc.pDopX10 : 0xFFFF;
  r.cfgPAccM = uc.valid ? uc.pAccM : 0xFFFF;
  r.cfgMeasRateMs = uc.valid ? uc.measRateMs : 0xFFFF;
  r.cfgNavRate = uc.valid ? uc.navRate : 0xFFFF;
  r.cfgApplyCount = (uint16_t)min<uint32_t>(gnssCfgBaselineApplyCount, 65535UL);
  gnssBlackbox.append(r, flushNow);
}

static void serviceGnssBlackbox(uint32_t now) {
  if (!gnssLogFixInitialized) {
    gnssLogFixInitialized = true;
    gnssLogLastFix = nav.fix;
  } else if (nav.fix != gnssLogLastFix) {
    gnssLogLastFix = nav.fix;
    gnssLogDetailedUntilMs = now + FT_GNSS_LOG_TRANSITION_DETAIL_MS;
    appendGnssLog(nav.fix ? GnssLogEvent::FIX_GAIN : GnssLogEvent::FIX_LOST, true);
    gnssLogLastSampleMs = now;
  }

  if (!nav.fix) {
    if (gnssLogNoFixSinceMs == 0) gnssLogNoFixSinceMs = now;
  } else {
    gnssLogNoFixSinceMs = 0;
  }

  const bool transitionDetail =
      gnssLogDetailedUntilMs != 0 &&
      (int32_t)(gnssLogDetailedUntilMs - now) > 0;
  const bool highRateNoFix = !nav.fix &&
      gnssLogNoFixSinceMs != 0 &&
      now - gnssLogNoFixSinceMs < FT_GNSS_LOG_HIGH_RATE_NOFIX_MS;

  uint32_t sampleInterval = FT_GNSS_LOG_STABLE_SAMPLE_MS;
  uint32_t flushInterval = FT_GNSS_LOG_STABLE_FLUSH_MS;
  if (transitionDetail || highRateNoFix) {
    sampleInterval = FT_GNSS_LOG_ACQUIRE_SAMPLE_MS;
    flushInterval = FT_GNSS_LOG_ACQUIRE_FLUSH_MS;
  } else if (!nav.fix) {
    sampleInterval = FT_GNSS_LOG_LONG_NOFIX_SAMPLE_MS;
    flushInterval = FT_GNSS_LOG_LONG_NOFIX_FLUSH_MS;
  }

  if (gnssLogLastSampleMs == 0 || now - gnssLogLastSampleMs >= sampleInterval) {
    appendGnssLog(GnssLogEvent::SAMPLE, false);
    gnssLogLastSampleMs = now;
  }

  gnssBlackbox.service(now, flushInterval);
}

static void recoverGnssReceivePath(const char *reason) {
  const uint32_t now = millis();
  if (lastGnssRecoveryMs != 0 &&
      now - lastGnssRecoveryMs < FT_GNSS_RECOVERY_COOLDOWN_MS) {
    return;
  }

  Serial.print("[GNSS RECOVERY] reason=");
  Serial.print(reason);
  Serial.println(" parser_resync=1");

  // Capture the pre-recovery evidence before touching the UART path.
  gnssRecoveryCount++;
  appendGnssLog(recoveryEventForReason(reason), true);

  GnssSerial.end();
  delay(10);
  // TinyGPSPlus resynchronizes at the next '$' sentence start. We deliberately
  // keep the registered TinyGPSCustom objects intact instead of reconstructing
  // the parser object and its internal custom-field linked list.
  beginGnssSerial();

  lastGnssByteMs = 0;
  gnssStreamStartMs = 0;
  lastNmeaPassMs = 0;
  outdoorNoFixSinceMs = 0;
  nav.gnssStream = false;
  nav.fix = false;
  nav.fixType = 0;
  nav.satUsed = 0;
  nav.satVisible = 0;
  nav.hdopX100 = 9999;
  lastGnssRecoveryMs = now;
}

static void updateGnssParserStats(uint32_t now) {
  const uint32_t passed = gps.passedChecksum();
  const uint32_t failed = gps.failedChecksum();

  if (passed != lastParserPassedChecksum) {
    const uint32_t delta = passed - lastParserPassedChecksum;
    gnssPassedChecksumTotal += delta;
    lastParserPassedChecksum = passed;
    lastNmeaPassMs = now;
  }

  if (failed != lastParserFailedChecksum) {
    const uint32_t delta = failed - lastParserFailedChecksum;
    gnssFailedChecksumTotal += delta;
    lastParserFailedChecksum = failed;
  }
}

static void pollGnss() {
  if (!gnssSoftwareEnabled) {
    // Critical: any UART TX activity can wake M10 software standby. While OFF,
    // do not poll UBX, do not run recovery, and do not generate blackbox samples.
    return;
  }

  const uint32_t beforeBytes = gnssBytes;
  while (GnssSerial.available()) {
    int c = GnssSerial.read();
    if (c < 0) break;
    const uint32_t byteNow = millis();
    ubxGnss.feed((uint8_t)c, byteNow);
    gps.encode((char)c);
    gnssBytes++;
    lastGnssByteMs = millis();
  }

  const uint32_t now = millis();
  if (gnssBytes != beforeBytes && gnssStreamStartMs == 0) gnssStreamStartMs = now;
  updateGnssParserStats(now);

  nav.gnssStream = gnssBytes > 0 &&
                   lastGnssByteMs != 0 &&
                   now - lastGnssByteMs < FT_GNSS_STREAM_FRESH_MS;

  TinyGPSCustom *ggaQ = freshestCustom(&gnGgaFixQuality, &gpGgaFixQuality, FT_GNSS_FIELD_FRESH_MS);
  TinyGPSCustom *ggaSat = freshestCustom(&gnGgaSatUsed, &gpGgaSatUsed, FT_GNSS_FIELD_FRESH_MS);
  TinyGPSCustom *ggaHdop = freshestCustom(&gnGgaHdop, &gpGgaHdop, FT_GNSS_FIELD_FRESH_MS);
  gnssDiag.ggaFixQuality = freshCustomU8(ggaQ, FT_GNSS_FIELD_FRESH_MS, 0);
  gnssDiag.ggaSatUsed = freshCustomU8(ggaSat, FT_GNSS_FIELD_FRESH_MS, 0);
  gnssDiag.ggaHdopX100 = freshCustomDecimalX100(ggaHdop, FT_GNSS_FIELD_FRESH_MS, 9999);
  gnssDiag.gsaFixType = freshestGsaFixType();
  gnssDiag.rmcStatus = freshestRmcStatus();
  gnssDiag.gnsFix = freshestGnsFix();
  TinyGPSCustom *gnsSat = freshestCustom(&gnGnsSatUsed, &gpGnsSatUsed, FT_GNSS_FIELD_FRESH_MS);
  TinyGPSCustom *gnsHdop = freshestCustom(&gnGnsHdop, &gpGnsHdop, FT_GNSS_FIELD_FRESH_MS);
  gnssDiag.gnsSatUsed = freshCustomU8(gnsSat, FT_GNSS_FIELD_FRESH_MS, 0);
  gnssDiag.gnsHdopX100 = freshCustomDecimalX100(gnsHdop, FT_GNSS_FIELD_FRESH_MS, 9999);
  gnssDiag.locationSource = 0;

  bool haveFreshLocation = false;
  if (gps.location.isValid() && gps.location.age() <= FT_GNSS_FIELD_FRESH_MS) {
    if (gps.location.isUpdated()) lastLocationCommitMs = now;
    nav.latE7 = (int32_t)llround(gps.location.lat() * 10000000.0);
    nav.lonE7 = (int32_t)llround(gps.location.lng() * 10000000.0);
    haveFreshLocation = true;
    gnssDiag.locationSource = 1;
  } else {
    int32_t fallbackLat = 0, fallbackLon = 0;
    if (fallbackLocationFromGga(fallbackLat, fallbackLon)) {
      nav.latE7 = fallbackLat;
      nav.lonE7 = fallbackLon;
      lastLocationCommitMs = now;
      haveFreshLocation = true;
      gnssDiag.locationSource = 2;
    } else if (fallbackLocationFromRmc(fallbackLat, fallbackLon)) {
      nav.latE7 = fallbackLat;
      nav.lonE7 = fallbackLon;
      lastLocationCommitMs = now;
      haveFreshLocation = true;
      gnssDiag.locationSource = 3;
    } else if (fallbackLocationFromGns(fallbackLat, fallbackLon)) {
      nav.latE7 = fallbackLat;
      nav.lonE7 = fallbackLon;
      lastLocationCommitMs = now;
      haveFreshLocation = true;
      gnssDiag.locationSource = 4;
    }
  }

  const UbxPvtSnapshot &ubxPvt = ubxGnss.pvt();
  const bool ubxPvtFresh = ubxPvt.valid && now - ubxPvt.updatedMs <= FT_GNSS_FIELD_FRESH_MS;
  if (!haveFreshLocation && ubxPvtFresh && ubxPvt.fixOk &&
      ubxPvt.fixType >= 2 && ubxPvt.fixType <= 4) {
    nav.latE7 = ubxPvt.latE7;
    nav.lonE7 = ubxPvt.lonE7;
    int32_t ubxAltDm = ubxPvt.hMslMm / 100;
    if (ubxAltDm < -32768) ubxAltDm = -32768;
    if (ubxAltDm > 32767) ubxAltDm = 32767;
    nav.altDm = (int16_t)ubxAltDm;
    lastLocationCommitMs = now;
    haveFreshLocation = true;
    gnssDiag.locationSource = 5;
  }
  nav.fix = haveFreshLocation;

  nav.fixType = ubxPvtFresh && ubxPvt.fixOk ? ubxPvt.fixType : gnssDiag.gsaFixType;
  if (nav.fix && nav.fixType == 0) nav.fixType = 3;
  if (!nav.fix) nav.fixType = 0;

  if (gps.altitude.isValid() && gps.altitude.age() <= FT_GNSS_FIELD_FRESH_MS)
    nav.altDm = (int16_t)lround(gps.altitude.meters() * 10.0);

  if (gps.satellites.isValid() && gps.satellites.age() <= FT_GNSS_FIELD_FRESH_MS)
    nav.satUsed = (uint8_t)min<uint32_t>(gps.satellites.value(), 255);
  else
    nav.satUsed = gnssDiag.ggaSatUsed;
  if (gnssDiag.ggaSatUsed > nav.satUsed) nav.satUsed = gnssDiag.ggaSatUsed;
  if (gnssDiag.gnsSatUsed > nav.satUsed) nav.satUsed = gnssDiag.gnsSatUsed;
  if (ubxPvtFresh && ubxPvt.numSv > nav.satUsed) nav.satUsed = ubxPvt.numSv;
  const UbxSatSnapshot &ubxSat = ubxGnss.sat();
  const bool ubxSatFresh = ubxSat.valid && now - ubxSat.updatedMs <= FT_GNSS_GSV_FRESH_MS;
  if (ubxSatFresh && ubxSat.used > nav.satUsed) nav.satUsed = ubxSat.used;

  if (gps.hdop.isValid() && gps.hdop.age() <= FT_GNSS_FIELD_FRESH_MS)
    nav.hdopX100 = (uint16_t)min<uint32_t>(gps.hdop.value(), 65535);
  else {
    nav.hdopX100 = gnssDiag.ggaHdopX100;
    if (gnssDiag.gnsHdopX100 < nav.hdopX100) nav.hdopX100 = gnssDiag.gnsHdopX100;
  }

  if (gps.speed.isValid() && gps.speed.age() <= FT_GNSS_FIELD_FRESH_MS) {
    double cmps = gps.speed.mps() * 100.0;
    if (cmps < 0) cmps = 0;
    if (cmps > 65535) cmps = 65535;
    nav.speedCms = (uint16_t)lround(cmps);
  } else if (!nav.fix) {
    nav.speedCms = 0;
  }

  nav.satVisible =
      customToFreshU16(gpGsvVisible, FT_GNSS_GSV_FRESH_MS) +
      customToFreshU16(gaGsvVisible, FT_GNSS_GSV_FRESH_MS) +
      customToFreshU16(gbGsvVisible, FT_GNSS_GSV_FRESH_MS) +
      customToFreshU16(glGsvVisible, FT_GNSS_GSV_FRESH_MS) +
      customToFreshU16(gqGsvVisible, FT_GNSS_GSV_FRESH_MS);
  if (ubxSatFresh && ubxSat.numSv > nav.satVisible) nav.satVisible = ubxSat.numSv;
  if (nav.satVisible < nav.satUsed) nav.satVisible = nav.satUsed;

  if (gps.date.isValid() && gps.time.isValid() &&
      gps.date.age() < FT_GNSS_FIELD_FRESH_MS &&
      gps.time.age() < FT_GNSS_FIELD_FRESH_MS) {
    nav.unixTime = utcToUnix(
      gps.date.year(), gps.date.month(), gps.date.day(),
      gps.time.hour(), gps.time.minute(), gps.time.second()
    );
    nav.timeValid = nav.unixTime != 0;
  } else {
    nav.timeValid = false;
  }

  // Poll the receiver's native UBX interface and maintain a RAM-only safe
  // acquisition baseline. Replies are parsed on subsequent loop iterations.
  serviceUbxDiagnostics(now);

  // Record transitions/snapshots after all current GNSS fields have been refreshed.
  serviceGnssBlackbox(now);

  const bool receiverResetGrace = gnssReceiverResetGraceUntilMs != 0 &&
                                  (int32_t)(gnssReceiverResetGraceUntilMs - now) > 0;

  // Recovery level 1: GNSS byte stream stopped. Restart only the GNSS UART path.
  if (!receiverResetGrace && gnssBytes > 0 && lastGnssByteMs != 0 &&
      now - lastGnssByteMs >= FT_GNSS_STREAM_STALL_RECOVERY_MS) {
    recoverGnssReceivePath("stream_stall");
    return;
  }

  // Recovery level 2: bytes are flowing but TinyGPSPlus has not accepted a
  // checksum-valid NMEA sentence for too long. This is the signature expected
  // after UART overrun/corruption; restart RX so TinyGPSPlus can resync at '$'.
  if (!receiverResetGrace && nav.gnssStream && gnssStreamStartMs != 0) {
    const bool neverPassed = lastNmeaPassMs == 0 &&
                             now - gnssStreamStartMs >= FT_GNSS_NO_NMEA_RECOVERY_MS;
    const bool stoppedPassing = lastNmeaPassMs != 0 &&
                                now - lastNmeaPassMs >= FT_GNSS_NO_NMEA_RECOVERY_MS;
    if (neverPassed || stoppedPassing) {
      recoverGnssReceivePath("nmea_stall");
      return;
    }
  }

  // Recovery level 3 is A4 Production receiver-side recovery. Healthy NMEA/UBX
  // plus sustained strong sky but no ephemeris/used-SV progress triggers one cold
  // receiver restart, followed by a long undisturbed reacquisition window.
  const bool nmeaHealthy = lastNmeaPassMs != 0 &&
                           now - lastNmeaPassMs < FT_GNSS_STREAM_FRESH_MS;
  serviceGnssReceiverRecovery(now, nmeaHealthy);
}

static void sendCompassCalState() {
  String s = "COMPASS_CAL_STATE state=";
  s += compassCal.stateName();
  s += " progress=" + String(compassCal.progress());
  s += " quality=" + String(compassCal.sessionQuality());
  s += " samples=" + String(compassCal.samples());
  s += " calibrated=";
  s += compassCal.isCalibrated() ? "1" : "0";
  s += " min_x=" + String(compassCal.minX());
  s += " max_x=" + String(compassCal.maxX());
  s += " min_y=" + String(compassCal.minY());
  s += " max_y=" + String(compassCal.maxY());
  s += " min_z=" + String(compassCal.minZ());
  s += " max_z=" + String(compassCal.maxZ());
  if (compassCal.isCalibrated()) {
    const CompassCalibration::Params &cp = compassCal.params();
    s += " off_x=" + String(cp.offsetX, 2);
    s += " off_y=" + String(cp.offsetY, 2);
    s += " m_xx=" + String(cp.matrixXX, 6);
    s += " m_xy=" + String(cp.matrixXY, 6);
    s += " m_yy=" + String(cp.matrixYY, 6);
  }
  hostSend(s);
}

static void sendNavState() {
  String s = "NAV_STATE fix=";
  s += nav.fix ? "1" : "0";
  s += " fix_type=" + String(nav.fixType);
  s += " lat=" + String(nav.latE7);
  s += " lon=" + String(nav.lonE7);
  s += " alt=" + String(nav.altDm);
  s += " sat=" + String(nav.satUsed); // A1 compatibility
  s += " sat_used=" + String(nav.satUsed);
  s += " sat_visible=" + String(nav.satVisible);
  s += " hdop=" + String(nav.hdopX100);
  s += " speed=" + String(nav.speedCms);
  s += " unix=" + String(nav.unixTime);
  s += " time_valid=";
  s += nav.timeValid ? "1" : "0";
  s += " heading=" + String(nav.headingX10);
  s += " compass=";
  s += nav.compassReady ? "1" : "0";
  s += " compass_q=" + String(nav.compassQuality);
  hostSend(s);
}

static void sendRadioState() {
  String s = "RADIO_STATE ready=";
  s += lora.isReady() ? "1" : "0";
  s += " profile=" + lora.currentProfileName();
  s += " freq=" + String(lora.currentFreqMhz(), 3);
  s += " rx=" + String(lora.rxCount());
  s += " nodes=" + String(lora.networkNodeCount());
  s += " pki=" + String(lora.pkiPeerCount());
  s += " dup=" + String(lora.duplicateDropCount());
  s += " tx_queue=0";
  hostSend(s);
}

static void sendSystemState() {
  String s = "SYSTEM_STATE gnss=";
  s += nav.gnssStream ? "1" : "0";
  s += " compass=";
  s += nav.compassReady ? "1" : "0";
  s += " lora=";
  s += lora.isReady() ? "1" : "0";
  s += " uptime=" + String(millis() / 1000);
  s += " gnss_bytes=" + String(gnssBytes);
  s += " heap=" + String(ESP.getFreeHeap());
  s += " psram=" + String(ESP.getFreePsram());
  s += " rx_errors=" + String(lora.rxErrorCount());
  s += " uart_errors=" + String(uartErrorCount);
  s += " radio_resets=" + String(radioResetCount);
  s += " gnss_nmea_ok=" + String(gnssPassedChecksumTotal);
  s += " gnss_nmea_bad=" + String(gnssFailedChecksumTotal);
  s += " gnss_recoveries=" + String(gnssRecoveryCount);
  s += " gnss_rxbuf=" + String((unsigned long)gnssRxBufferBytes);
  const uint32_t locAge = lastLocationCommitMs == 0 ? 0xFFFFFFFFUL : millis() - lastLocationCommitMs;
  s += " gnss_loc_age=" + String(locAge);
  s += " gnss_log_count=" + String((unsigned long)gnssBlackbox.count());
  s += " gnss_log_persist=";
  s += gnssBlackbox.persistenceHealthy() ? "1" : "0";
  s += " gnss_log_fail=" + String(gnssBlackbox.persistFailures());
  hostSend(s);
}

static void sendGnssLogStatus() {
  String s = "GNSS_LOG_STATUS ready=";
  s += gnssBlackbox.ready() ? "1" : "0";
  s += " persist=";
  s += gnssBlackbox.persistenceHealthy() ? "1" : "0";
  s += " count=" + String((unsigned long)gnssBlackbox.count());
  s += " capacity=" + String((unsigned long)GnssBlackbox::CAPACITY);
  s += " next_seq=" + String(gnssBlackbox.nextSequence());
  s += " failures=" + String(gnssBlackbox.persistFailures());
  hostSend(s);
}

static void dumpGnssLog() {
  const size_t n = gnssBlackbox.count();
  hostSend("GNSS_LOG_BEGIN count=" + String((unsigned long)n));
  for (size_t i = 0; i < n; ++i) {
    GnssLogRecord r{};
    if (!gnssBlackbox.getOldest(i, r)) continue;
    String s = "GNSS_LOG seq=" + String(r.seq);
    s += " up=" + String(r.uptimeSec);
    s += " event=";
    s += GnssBlackbox::eventName(r.event);
    s += " stream=" + String((r.flags & 0x01) ? 1 : 0);
    s += " fix=" + String((r.flags & 0x02) ? 1 : 0);
    s += " fix_type=" + String(r.fixType);
    s += " sat_used=" + String(r.satUsed);
    s += " sat_visible=" + String(r.satVisible);
    s += " hdop=" + String(r.hdopX100);
    s += " bytes=" + String(r.gnssBytes);
    s += " nmea_ok=" + String(r.nmeaOk);
    s += " nmea_bad=" + String(r.nmeaBad);
    s += " loc_age=" + String(r.locAgeMs);
    s += " byte_age=" + String(r.byteAgeMs);
    s += " recoveries=" + String(r.recoveries);
    s += " rr=" + String(r.receiverResets);
    s += " gq=" + String(r.ggaFixQuality);
    s += " gs=" + String(r.ggaSatUsed);
    s += " gf=" + String(r.gsaFixType);
    s += " r=" + String(r.rmcStatus);
    s += " nf=" + String(r.gnsFix);
    s += " ns=" + String(r.gnsSatUsed);
    s += " ls=" + String(r.locationSource);
    s += " ub=" + String(r.ubxFlags);
    s += " uf=" + String(r.ubxFixType);
    s += " un=" + String(r.ubxNumSv);
    s += " uu=" + String(r.ubxSatUsed);
    s += " cm=" + String(r.ubxCnoMax);
    s += " c20=" + String(r.ubxCnoGe20);
    s += " c30=" + String(r.ubxCnoGe30);
    s += " ep=" + String(r.ubxEphAvail);
    s += " cf=" + String(r.cfgFixMode);
    s += " ms=" + String(r.cfgMinSvs);
    s += " mx=" + String(r.cfgMaxSvs);
    s += " mc=" + String(r.cfgMinCno);
    s += " me=" + String(r.cfgMinElev);
    s += " nc=" + String(r.cfgNCnoThrs);
    s += " ct=" + String(r.cfgCnoThrs);
    s += " sa=" + String(r.cfgSigAttenComp);
    s += " pm=" + String(r.cfgPmMode);
    s += " ue=" + String(r.cfgPmUpdateEph);
    s += " sm=" + String(r.cfgSignalMask, HEX);
    s += " pd=" + String(r.cfgPDopX10);
    s += " pa=" + String(r.cfgPAccM);
    s += " mr=" + String(r.cfgMeasRateMs);
    s += " nr=" + String(r.cfgNavRate);
    s += " ca=" + String(r.cfgApplyCount);
    hostSend(s);
    if ((i & 0x0F) == 0x0F) delay(1);
  }
  hostSend("GNSS_LOG_END count=" + String((unsigned long)n));
}

static String formatGnssLogRecord(const GnssLogRecord &r) {
  String s = "GNSS_LOG seq=" + String(r.seq);
  s += " up=" + String(r.uptimeSec);
  s += " event=";
  s += GnssBlackbox::eventName(r.event);
  s += " stream=" + String((r.flags & 0x01) ? 1 : 0);
  s += " fix=" + String((r.flags & 0x02) ? 1 : 0);
  s += " fix_type=" + String(r.fixType);
  s += " sat_used=" + String(r.satUsed);
  s += " sat_visible=" + String(r.satVisible);
  s += " hdop=" + String(r.hdopX100);
  s += " bytes=" + String(r.gnssBytes);
  s += " nmea_ok=" + String(r.nmeaOk);
  s += " nmea_bad=" + String(r.nmeaBad);
  s += " loc_age=" + String(r.locAgeMs);
  s += " byte_age=" + String(r.byteAgeMs);
  s += " recoveries=" + String(r.recoveries);
  s += " rr=" + String(r.receiverResets);
  s += " gq=" + String(r.ggaFixQuality);
  s += " gs=" + String(r.ggaSatUsed);
  s += " gf=" + String(r.gsaFixType);
  s += " r=" + String(r.rmcStatus);
  s += " nf=" + String(r.gnsFix);
  s += " ns=" + String(r.gnsSatUsed);
  s += " ls=" + String(r.locationSource);
  s += " ub=" + String(r.ubxFlags);
  s += " uf=" + String(r.ubxFixType);
  s += " un=" + String(r.ubxNumSv);
  s += " uu=" + String(r.ubxSatUsed);
  s += " cm=" + String(r.ubxCnoMax);
  s += " c20=" + String(r.ubxCnoGe20);
  s += " c30=" + String(r.ubxCnoGe30);
  s += " ep=" + String(r.ubxEphAvail);
  s += " cf=" + String(r.cfgFixMode);
  s += " ms=" + String(r.cfgMinSvs);
  s += " mx=" + String(r.cfgMaxSvs);
  s += " mc=" + String(r.cfgMinCno);
  s += " me=" + String(r.cfgMinElev);
  s += " nc=" + String(r.cfgNCnoThrs);
  s += " ct=" + String(r.cfgCnoThrs);
  s += " sa=" + String(r.cfgSigAttenComp);
  s += " pm=" + String(r.cfgPmMode);
  s += " ue=" + String(r.cfgPmUpdateEph);
  s += " sm=" + String(r.cfgSignalMask, HEX);
  s += " pd=" + String(r.cfgPDopX10);
  s += " pa=" + String(r.cfgPAccM);
  s += " mr=" + String(r.cfgMeasRateMs);
  s += " nr=" + String(r.cfgNavRate);
  s += " ca=" + String(r.cfgApplyCount);
  return s;
}

static void sendGnssRecoveryStatus() {
  String s = "GNSS_RECOVERY_STATUS stage=" + String(gnssReceiverRecoveryStage);
  s += " stage_name=";
  s += receiverStageName(gnssReceiverRecoveryStage);
  s += " receiver_resets=" + String(gnssReceiverResetCount);
  s += " rx_recoveries=" + String(gnssRecoveryCount);
  s += " a4_evidence_ms=" + String(gnssA4StrongEvidenceMs);
  s += " a4_progress_age_ms=" + String(gnssA4LastProgressMs == 0 ? 0 : millis() - gnssA4LastProgressMs);
  s += " a4_last_cold_age_ms=" + String(gnssA4LastAutoColdMs == 0 ? 0 : millis() - gnssA4LastAutoColdMs);
  s += " a4_best_eph=" + String(gnssA4BestEph);
  s += " a4_best_used=" + String(gnssA4BestUsed);
  s += " a4_lockout_ms=" + String((gnssA4PostColdLockoutUntilMs != 0 && (int32_t)(gnssA4PostColdLockoutUntilMs - millis()) > 0) ? (gnssA4PostColdLockoutUntilMs - millis()) : 0);
  s += " gga_q=" + String(gnssDiag.ggaFixQuality);
  s += " gga_sat=" + String(gnssDiag.ggaSatUsed);
  s += " gsa_fix=" + String(gnssDiag.gsaFixType);
  s += " rmc=" + String(gnssDiag.rmcStatus);
  s += " gns_fix=" + String(gnssDiag.gnsFix);
  s += " gns_sat=" + String(gnssDiag.gnsSatUsed);
  s += " loc_src=" + String(gnssDiag.locationSource);
  hostSend(s);
}

static void sendGnssUbxStatus() {
  const uint32_t now = millis();
  const UbxPvtSnapshot &p = ubxGnss.pvt();
  const UbxSatSnapshot &sat = ubxGnss.sat();
  const UbxConfigSnapshot &cfg = ubxGnss.cfg();
  String s = "GNSS_UBX_STATUS seen=" + String(ubxGnss.protocolSeen() ? 1 : 0);
  s += " monver=" + String(ubxGnss.monVerSeen() ? 1 : 0);
  s += " frames_ok=" + String(ubxGnss.framesOk());
  s += " frames_bad=" + String(ubxGnss.framesBad());
  s += " ack=" + String(ubxGnss.ackCount());
  s += " nak=" + String(ubxGnss.nakCount());
  s += " pvt_fresh=" + String(p.valid && now - p.updatedMs <= FT_GNSS_FIELD_FRESH_MS ? 1 : 0);
  s += " pvt_fix=" + String(p.fixOk ? 1 : 0);
  s += " pvt_type=" + String(p.fixType);
  s += " pvt_sv=" + String(p.numSv);
  s += " hacc_mm=" + String(p.hAccMm);
  s += " pdop=" + String(p.pDopX100);
  s += " sat_fresh=" + String(sat.valid && now - sat.updatedMs <= FT_GNSS_GSV_FRESH_MS ? 1 : 0);
  s += " sat_n=" + String(sat.numSv);
  s += " used=" + String(sat.used);
  s += " cno_max=" + String(sat.cnoMax);
  s += " c20=" + String(sat.cnoGe20);
  s += " c25=" + String(sat.cnoGe25);
  s += " c30=" + String(sat.cnoGe30);
  s += " lock=" + String(sat.codeLocked);
  s += " eph=" + String(sat.ephAvail);
  s += " cfg=" + String(cfg.valid ? 1 : 0);
  s += " fixmode=" + String(cfg.fixMode);
  s += " ini3d=" + String(cfg.iniFix3d);
  s += " dyn=" + String(cfg.dynModel);
  s += " minsv=" + String(cfg.minSvs);
  s += " maxsv=" + String(cfg.maxSvs);
  s += " mincno=" + String(cfg.minCno);
  s += " elev=" + String(cfg.minElev);
  s += " ncno=" + String(cfg.nCnoThrs);
  s += " cnoth=" + String(cfg.cnoThrs);
  s += " sigatt=" + String(cfg.sigAttenComp);
  s += " pm=" + String(cfg.pmOperateMode);
  s += " upeph=" + String(cfg.pmUpdateEph);
  s += " sigmask=" + String(cfg.signalMask, HEX);
  s += " meas=" + String(cfg.measRateMs);
  s += " navrate=" + String(cfg.navRate);
  s += " cfg_apply=" + String(gnssCfgBaselineApplyCount);
  hostSend(s);

  if (ubxGnss.monVerSeen()) {
    String v = "GNSS_UBX_VERSION sw=\"";
    v += ubxGnss.swVersion();
    v += "\" hw=\"";
    v += ubxGnss.hwVersion();
    v += "\" prot=\"";
    v += ubxGnss.protVersion();
    v += "\"";
    hostSend(v);
  }
}

static void forceUbxDiagnosticPoll() {
  requestUbxMonitorVersion();
  requestUbxPvt();
  requestUbxSat();
  requestGnssConfigSnapshot();
  hostSend("GNSS_UBX_POLL ok=1");
}

static void emitNodeList() {
  size_t count = lora.networkNodeCount();
  for (size_t i = 0; i < count; ++i) {
    LoRaNodeView n;
    if (!lora.getNetworkNode(i, n) || !n.valid) continue;

    String s = "MESH_NODE id=!";
    char hexbuf[9];
    snprintf(hexbuf, sizeof(hexbuf), "%08lX", (unsigned long)n.node);
    s += hexbuf;
    s += " long=\"" + n.longName + "\"";
    s += " short=\"" + n.shortName + "\"";
    s += " online=";
    s += (n.ageMs != 0xFFFFFFFFUL && n.ageMs <= NODE_ONLINE_MS) ? "1" : "0";
    s += " hops=" + String((int)n.hops);
    s += " rssi=" + String(n.lastRssi, 1);
    s += " snr=" + String(n.lastSnr, 1);
    s += " pki=";
    s += n.pkiAvailable ? "1" : "0";
    s += " last=";
    s += n.ageMs == 0xFFFFFFFFUL ? "4294967295" : String(n.ageMs / 1000);
    hostSend(s);
  }
  hostSend("MESH_NODE_END count=" + String(count));
}

static PendingDelivery* allocPending(uint32_t coreId, uint32_t meshId, uint32_t node) {
  for (auto &p : pending) {
    if (!p.used) {
      p.used = true;
      p.coreId = coreId;
      p.meshPacketId = meshId;
      p.node = node;
      p.startedMs = millis();
      return &p;
    }
  }
  return nullptr;
}

static PendingDelivery* findPendingByMesh(uint32_t meshId) {
  for (auto &p : pending) if (p.used && p.meshPacketId == meshId) return &p;
  return nullptr;
}

static size_t pendingCount() {
  size_t n = 0;
  for (auto &p : pending) if (p.used) n++;
  return n;
}

static bool parseIdPrefix(String &rest, uint32_t &id) {
  rest.trim();
  if (!rest.startsWith("id=")) return false;
  int sp = rest.indexOf(' ');
  String idPart = sp < 0 ? rest.substring(3) : rest.substring(3, sp);
  if (!parseUint32(idPart, id)) return false;
  rest = sp < 0 ? "" : rest.substring(sp + 1);
  rest.trim();
  return true;
}

static void doBroadcast(uint32_t coreId, const String &msg, bool a2) {
  if (!lora.isReady()) {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=radio_not_ready");
    else hostSend("MESH_TX_RESULT type=TEXT ok=0");
    return;
  }
  if (!validUtf8Length(msg)) {
    if (a2) {
      hostSend("MESH_TX_RESULT id=" + String(coreId) + " type=TEXT ok=0 reason=payload_too_large");
      hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=payload_too_large");
    } else hostSend("MESH_TX_RESULT type=TEXT ok=0");
    return;
  }

  if (a2) hostSend("MESH_TX_ACCEPTED id=" + String(coreId));
  bool ok = lora.sendMeshtasticText(msg);
  if (ok) {
    if (a2) {
      hostSend("MESH_TX_SENT id=" + String(coreId));
      hostSend("MESH_TX_RESULT id=" + String(coreId) + " type=TEXT ok=1");
    } else hostSend("MESH_TX_RESULT type=TEXT ok=1");
  } else {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=encode_or_radio_failed");
    else hostSend("MESH_TX_RESULT type=TEXT ok=0");
  }
}

static void doPrivate(uint32_t coreId, uint32_t node, const String &msg, bool a2) {
  if (!lora.isReady()) {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=radio_not_ready");
    else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
    return;
  }
  if (node == 0 || node == 0xFFFFFFFFUL || node == 0x4C423002UL) {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=invalid_node");
    else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
    return;
  }
  if (!lora.nodeSupportsPrivateMessage(node)) {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=no_pki_key");
    else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
    return;
  }
  if (!validUtf8Length(msg)) {
    if (a2) {
      hostSend("MESH_TX_RESULT id=" + String(coreId) + " type=PRIVATE ok=0 reason=payload_too_large");
      hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=payload_too_large");
    } else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
    return;
  }
  if (pendingCount() >= 8) {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=queue_full");
    else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
    return;
  }

  if (a2) hostSend("MESH_TX_ACCEPTED id=" + String(coreId));
  bool ok = lora.sendMeshtasticPrivateText(node, msg);
  if (ok) {
    if (a2) {
      uint32_t meshId = lora.lastTxPacketId();
      if (!allocPending(coreId, meshId, node)) {
        hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=queue_full");
        return;
      }
      hostSend("MESH_TX_SENT id=" + String(coreId));
      hostSend("MESH_TX_RESULT id=" + String(coreId) + " type=PRIVATE ok=1");
    } else hostSend("MESH_TX_RESULT type=PRIVATE ok=1");
  } else {
    if (a2) hostSend("MESH_TX_FAILED id=" + String(coreId) + " reason=encode_or_radio_failed");
    else hostSend("MESH_TX_RESULT type=PRIVATE ok=0");
  }
}

static void handleHostCommand(String line) {
  line.trim();
  if (!line.length()) return;

  if (line.startsWith("CORE_PING")) {
    hostSend("LR01_PONG" + line.substring(String("CORE_PING").length()));
    return;
  }

  if (line.startsWith("CORE_STATUS?")) {
    String rest = line.substring(String("CORE_STATUS?").length());
    rest.trim();
    uint32_t id = 0;
    bool hasId = parseIdPrefix(rest, id);
    sendSystemState();
    sendNavState();
    sendRadioState();
    hostSend(hasId ? ("STATUS_END id=" + String(id)) : "STATUS_END");
    return;
  }

  if (line == "GNSS_POWER?") {
    sendGnssSoftwarePowerState();
    return;
  }

  if (line == "GNSS_SOFT_OFF") {
    setGnssSoftwareEnabled(false);
    hostSend("GNSS_POWER_SET enabled=0 mode=SOFTWARE_STANDBY ok=1");
    return;
  }

  if (line == "GNSS_SOFT_ON") {
    setGnssSoftwareEnabled(true);
    hostSend("GNSS_POWER_SET enabled=1 mode=ACTIVE ok=1");
    return;
  }

  if (line == "GNSS_LOG_STATUS?") {
    sendGnssLogStatus();
    return;
  }

  if (line == "GNSS_LOG?") {
    // Persist the newest RAM records first, then stream oldest -> newest.
    gnssBlackbox.flush();
    dumpGnssLog();
    return;
  }

  if (line == "GNSS_LOG_CLEAR") {
    const bool ok = gnssBlackbox.clear();
    gnssLogLastSampleMs = 0;
    hostSend(String("GNSS_LOG_CLEARED ok=") + (ok ? "1" : "0"));
    return;
  }

  if (line == "GNSS_LOG_FLUSH") {
    const bool ok = gnssBlackbox.flush();
    hostSend(String("GNSS_LOG_FLUSHED ok=") + (ok ? "1" : "0"));
    return;
  }

  if (line == "GNSS_UBX_STATUS?") {
    sendGnssUbxStatus();
    return;
  }

  if (line == "GNSS_UBX_POLL") {
    forceUbxDiagnosticPoll();
    return;
  }

  if (line == "GNSS_CFG_BASELINE") {
    applyGnssAcquisitionBaselineRam();
    requestGnssConfigSnapshot();
    hostSend("GNSS_CFG_BASELINE ok=1 layer=RAM");
    return;
  }

  if (line == "GNSS_RECOVERY_STATUS?") {
    sendGnssRecoveryStatus();
    return;
  }

  if (line == "GNSS_RECEIVER_HOT") {
    issueGnssReceiverReset(1, "manual", true);
    hostSend("GNSS_RECEIVER_RESET stage=hot ok=1");
    return;
  }

  if (line == "GNSS_RECEIVER_WARM") {
    issueGnssReceiverReset(2, "manual", true);
    hostSend("GNSS_RECEIVER_RESET stage=warm ok=1");
    return;
  }

  if (line == "GNSS_RECEIVER_COLD") {
    issueGnssReceiverReset(3, "manual", true);
    hostSend("GNSS_RECEIVER_RESET stage=cold ok=1");
    return;
  }

  if (line == "COMPASS_CAL_STATUS?") {
    sendCompassCalState();
    return;
  }

  if (line == "COMPASS_CAL_START") {
    if (!nav.compassReady) {
      hostError(25, "compass_not_ready");
      return;
    }
    if (!compassCal.start()) {
      hostError(20, "compass_cal_busy");
      return;
    }
    sendCompassCalState();
    return;
  }

  if (line == "COMPASS_CAL_SAVE") {
    if (!compassCal.isSessionActive()) {
      hostError(21, "compass_cal_not_running");
      return;
    }
    if (compassCal.sessionQuality() < 2) {
      hostError(22, "compass_cal_quality_low");
      sendCompassCalState();
      return;
    }
    if (!compassCal.save()) {
      hostError(23, "compass_cal_save_failed");
      sendCompassCalState();
      return;
    }
    nav.compassQuality = compassCal.savedQuality();
    sendCompassCalState();
    return;
  }

  if (line == "COMPASS_CAL_CANCEL") {
    if (!compassCal.cancel()) {
      hostError(21, "compass_cal_not_running");
      return;
    }
    nav.compassQuality = compassCal.isCalibrated() ? compassCal.savedQuality() : 1;
    sendCompassCalState();
    return;
  }

  if (line == "COMPASS_CAL_RESET") {
    if (compassCal.isSessionActive()) {
      hostError(20, "compass_cal_busy");
      return;
    }
    if (!compassCal.reset()) {
      hostError(24, "compass_cal_reset_failed");
      return;
    }
    nav.compassQuality = nav.compassReady ? 1 : 0;
    sendCompassCalState();
    return;
  }

  if (line == "MESH_NODEINFO") {
    bool ok = lora.sendMeshtasticNodeInfo(true);
    hostSend(String("MESH_TX_RESULT type=NODEINFO ok=") + (ok ? "1" : "0"));
    return;
  }

  if (line == "MESH_NODES?") {
    emitNodeList();
    return;
  }

  if (line.startsWith("MESH_TX ")) {
    String rest = line.substring(8);
    uint32_t id = 0;
    bool a2 = parseIdPrefix(rest, id);
    doBroadcast(id, rest, a2);
    return;
  }

  if (line.startsWith("MESH_PRIVATE ")) {
    String rest = line.substring(13);
    uint32_t id = 0;
    bool a2 = parseIdPrefix(rest, id);

    int sp = rest.indexOf(' ');
    if (sp <= 0) {
      if (a2) hostSend("MESH_TX_FAILED id=" + String(id) + " reason=invalid_node");
      else hostError(2, "invalid_argument");
      return;
    }
    uint32_t node = parseNodeHex(rest.substring(0, sp));
    String msg = rest.substring(sp + 1);
    doPrivate(id, node, msg, a2);
    return;
  }

  hostError(1, "unknown_command");
}

static void pollHost() {
  while (HostSerial.available()) {
    int v = HostSerial.read();
    if (v < 0) break;
    char c = (char)v;

    if (hostDiscardUntilLf) {
      if (c == '\n') hostDiscardUntilLf = false;
      continue;
    }

    if (c == '\r') continue;
    if (c == '\n') {
      handleHostCommand(hostLine);
      hostLine = "";
      continue;
    }

    if (hostLine.length() >= HOST_LINE_MAX) {
      hostLine = "";
      hostDiscardUntilLf = true;
      uartErrorCount++;
      hostError(8, "line_too_long");
      continue;
    }

    if ((uint8_t)c >= 0x20 || ((uint8_t)c & 0x80)) {
      hostLine += c;
    } else {
      uartErrorCount++;
    }
  }
}

static void pollUsbConsole() {
  while (Serial.available()) {
    int v = Serial.read();
    if (v < 0) break;
    char c = (char)v;

    if (usbDiscardUntilLf) {
      if (c == '\n') usbDiscardUntilLf = false;
      continue;
    }
    if (c == '\r') continue;
    if (c == '\n') {
      if (usbLine.length()) {
        usbCommandActive = true;
        handleHostCommand(usbLine);
        usbCommandActive = false;
      }
      usbLine = "";
      continue;
    }
    if (usbLine.length() >= HOST_LINE_MAX) {
      usbLine = "";
      usbDiscardUntilLf = true;
      Serial.println("[USB ERR] line_too_long");
      continue;
    }
    if ((uint8_t)c >= 0x20 || ((uint8_t)c & 0x80)) usbLine += c;
  }
}

static void publishMeshEvents() {
  uint32_t rev = lora.messageRevision();
  if (rev == lastMessageRevision) return;
  lastMessageRevision = rev;

  LoRaFrameView f;
  if (!lora.getMessageFrame(0, f) || !f.valid) return;

  String s = "MESH_RX id=" + String(f.id);
  s += " from=" + lora.formatMeshNodeShort(f.from);
  s += " to=" + lora.formatMeshNodeShort(f.to);
  s += " name=\"" + lora.nodeDisplayName(f.from) + "\"";
  s += " kind=" + f.kind;
  s += " rssi=" + String(f.rssi, 1);
  s += " snr=" + String(f.snr, 1);
  s += " text=\"" + f.text + "\"";
  hostSend(s);
}

static void pollDelivery() {
  uint32_t rev = lora.deliveryRevision();
  if (rev != lastDeliveryRevision) {
    lastDeliveryRevision = rev;
    uint32_t requestId = lora.lastDeliveryRequestId();
    PendingDelivery *p = findPendingByMesh(requestId);
    if (p) {
      String s = "MESH_DELIVERY id=" + String(p->coreId);
      s += " node=" + lora.formatMeshNodeShort(p->node);
      s += " status=";
      s += lora.lastDeliveryAck() ? "ACK" : "TIMEOUT";
      hostSend(s);
      p->used = false;
    }
  }

  uint32_t now = millis();
  for (auto &p : pending) {
    if (p.used && now - p.startedMs >= DELIVERY_TIMEOUT_MS) {
      String s = "MESH_DELIVERY id=" + String(p.coreId);
      s += " node=" + lora.formatMeshNodeShort(p.node);
      s += " status=TIMEOUT";
      hostSend(s);
      p.used = false;
    }
  }
}

void setup() {
  // Wireless Stick Lite V3 CP2102 is wired to ESP32-S3 UART0:
  // CP2102 TXD -> GPIO44 (U0RXD), CP2102 RXD <- GPIO43 (U0TXD).
  // Pin explicitly instead of relying on the selected board variant defaults.
  Serial.setRxBufferSize(1024);
  Serial.begin(115200, SERIAL_8N1, 44, 43);
  delay(800);

  Serial.println();
  Serial.println("[LR01] A4.4 Compass Axis Fix RC2 / Host Protocol 2");

  HostSerial.begin(FtHardware::HOST_BAUD, SERIAL_8N1,
                   FtHardware::HOST_RX_PIN, FtHardware::HOST_TX_PIN);

  const bool gnssLogReady = gnssBlackbox.begin();
  Serial.print("[GNSS BLACKBOX] ready=");
  Serial.print(gnssLogReady ? 1 : 0);
  Serial.print(" persist=");
  Serial.print(gnssBlackbox.persistenceHealthy() ? 1 : 0);
  Serial.print(" records=");
  Serial.println((unsigned long)gnssBlackbox.count());
  // Production keeps the blackbox, but never auto-dumps it at boot.
  // Use GNSS_LOG? explicitly when forensic data is needed.
  appendGnssLog(GnssLogEvent::BOOT, true);

  beginGnssSerial();
  // A3: normalize only the volatile RAM acquisition/filter/power settings.
  // Vendor BBR/Flash settings are preserved.
  gnssCfgBaselineApplyAtMs = millis() + 1500UL;

  Wire.begin(FtHardware::I2C_SDA_PIN, FtHardware::I2C_SCL_PIN);
  delay(100);
  nav.compassReady = qmcBegin();
  bool compassCalStoreOk = compassCal.begin();

  SPI.begin(FtHardware::LORA_SCK_PIN, FtHardware::LORA_MISO_PIN,
            FtHardware::LORA_MOSI_PIN, FtHardware::LORA_NSS_PIN);

  bool loraOk = lora.begin();

  Serial.print("[LR01 READY] gnss_log=");
  Serial.print(gnssLogReady ? 1 : 0);
  Serial.print(" compass=");
  Serial.print(nav.compassReady ? 1 : 0);
  Serial.print(" lora=");
  Serial.print(loraOk ? 1 : 0);
  Serial.print(" cal_nvs=");
  Serial.println(compassCalStoreOk ? 1 : 0);

  hostSend("LR01_BOOT version=MESH_NATIVE_A2_GNSS_A4_4_COMPASS_AXIS_RC2 protocol=2 node=FT02 id=!4c423002");
  hostSend("LR01_READY protocol=2");
  sendCompassCalState();

  if (loraOk) {
    delay(300);
    lora.sendMeshtasticNodeInfo(true);
    lastMeshNodeInfoMs = millis();
  }
}

void loop() {
  pollHost();
  pollUsbConsole();
  pollGnss();

  static uint32_t lastCompass = 0;
  if (millis() - lastCompass >= 100) {
    lastCompass = millis();
    int16_t rawX = 0, rawY = 0, rawZ = 0;
    if (qmcReadRaw(rawX, rawY, rawZ)) {
      nav.compassReady = true;
      compassCal.addSample(rawX, rawY, rawZ);

      uint16_t h = 0;
      uint8_t q = 0;
      qmcComputeHeading(rawX, rawY, rawZ, h, q);
      nav.headingX10 = h;
      nav.compassQuality = q;
    } else {
      nav.compassReady = false;
      nav.compassQuality = 0;
    }
  }

  if (compassCal.isSessionActive() && millis() - lastCompassCalStateTx >= 1000) {
    lastCompassCalStateTx = millis();
    sendCompassCalState();
  }

  if (lora.isReady()) {
    lora.pollReceive();
    publishMeshEvents();
    pollDelivery();
  }

  uint32_t now = millis();

  if (now - lastNavTx >= 1000) {
    lastNavTx = now;
    sendNavState();
    sendRadioState();
  }

  if (now - lastSysTx >= 5000) {
    lastSysTx = now;
    sendSystemState();
  }

  if (lora.isReady() && now - lastMeshNodeInfoMs >= 180000) {
    lastMeshNodeInfoMs = now;
    lora.sendMeshtasticNodeInfo(false);
  }

  delay(1);
}
