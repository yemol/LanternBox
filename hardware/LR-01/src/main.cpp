#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <esp_heap_caps.h>

#include "FtHardware.h"
#include "FtConfig.h"
#include "LoRaManager.h"
#include "CompassCalibration.h"

HardwareSerial GnssSerial(1);
HardwareSerial HostSerial(2);
TinyGPSPlus gps;

// GSA fix type + GSV visible satellite counters.
TinyGPSCustom gsaFixType(gps, "GNGSA", 2);
TinyGPSCustom gpGsvVisible(gps, "GPGSV", 3);
TinyGPSCustom gaGsvVisible(gps, "GAGSV", 3);
TinyGPSCustom gbGsvVisible(gps, "GBGSV", 3);
TinyGPSCustom glGsvVisible(gps, "GLGSV", 3);
TinyGPSCustom gqGsvVisible(gps, "GQGSV", 3);

LoRaManager lora;
CompassCalibration compassCal;

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

struct PendingDelivery {
  bool used = false;
  uint32_t coreId = 0;
  uint32_t meshPacketId = 0;
  uint32_t node = 0;
  uint32_t startedMs = 0;
};

NavState nav;
PendingDelivery pending[8];

String hostLine;
bool hostDiscardUntilLf = false;

uint32_t lastGnssByteMs = 0;
uint32_t gnssBytes = 0;
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

static void hostSend(const String &s) {
  HostSerial.println(s);
  Serial.print("[HOST TX] ");
  Serial.println(s);
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
  float h = atan2f(y, x) * 180.0f / PI;
  if (h < 0) h += 360.0f;
  headingX10 = (uint16_t)lroundf(h * 10.0f);

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

static uint16_t customToU16(TinyGPSCustom &c) {
  if (!c.isValid()) return 0;
  long v = atol(c.value());
  if (v < 0) v = 0;
  if (v > 65535) v = 65535;
  return (uint16_t)v;
}

static void pollGnss() {
  while (GnssSerial.available()) {
    int c = GnssSerial.read();
    if (c < 0) break;
    gps.encode((char)c);
    gnssBytes++;
    lastGnssByteMs = millis();
  }

  nav.gnssStream = gnssBytes > 0 && millis() - lastGnssByteMs < 3000;

  if (gps.location.isUpdated()) {
    nav.fix = gps.location.isValid();
    if (nav.fix) {
      nav.latE7 = (int32_t)llround(gps.location.lat() * 10000000.0);
      nav.lonE7 = (int32_t)llround(gps.location.lng() * 10000000.0);
    }
  }
  if (gps.location.age() > 5000) nav.fix = false;

  if (gsaFixType.isUpdated() && gsaFixType.isValid()) {
    int ft = atoi(gsaFixType.value());
    nav.fixType = (uint8_t)constrain(ft, 0, 3);
  } else if (nav.fix && nav.fixType == 0) {
    nav.fixType = 3;
  }
  if (!nav.fix) nav.fixType = 0;

  if (gps.altitude.isUpdated() && gps.altitude.isValid())
    nav.altDm = (int16_t)lround(gps.altitude.meters() * 10.0);

  if (gps.satellites.isUpdated() && gps.satellites.isValid())
    nav.satUsed = (uint8_t)min<uint32_t>(gps.satellites.value(), 255);

  if (gps.hdop.isUpdated() && gps.hdop.isValid())
    nav.hdopX100 = (uint16_t)min<uint32_t>(gps.hdop.value(), 65535);

  if (gps.speed.isUpdated() && gps.speed.isValid()) {
    double cmps = gps.speed.mps() * 100.0;
    if (cmps < 0) cmps = 0;
    if (cmps > 65535) cmps = 65535;
    nav.speedCms = (uint16_t)lround(cmps);
  }

  nav.satVisible =
      customToU16(gpGsvVisible) +
      customToU16(gaGsvVisible) +
      customToU16(gbGsvVisible) +
      customToU16(glGsvVisible) +
      customToU16(gqGsvVisible);
  if (nav.satVisible < nav.satUsed) nav.satVisible = nav.satUsed;

  if (gps.date.isValid() && gps.time.isValid() &&
      gps.date.age() < 5000 && gps.time.age() < 5000) {
    nav.unixTime = utcToUnix(
      gps.date.year(), gps.date.month(), gps.date.day(),
      gps.time.hour(), gps.time.minute(), gps.time.second()
    );
    nav.timeValid = nav.unixTime != 0;
  } else {
    nav.timeValid = false;
  }
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
  hostSend(s);
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

  Serial.print("[HOST RX] ");
  Serial.println(line);

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
        Serial.print("[USB CMD] ");
        Serial.println(usbLine);
        handleHostCommand(usbLine);
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
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("FT02-LR01 Meshtastic Native A2");
  Serial.println("Host Protocol 2 / FT01 native mesh core");
  Serial.println("==============================================");

  HostSerial.begin(FtHardware::HOST_BAUD, SERIAL_8N1,
                   FtHardware::HOST_RX_PIN, FtHardware::HOST_TX_PIN);

  GnssSerial.begin(FtHardware::GNSS_BAUD, SERIAL_8N1,
                   FtHardware::GNSS_RX_PIN, FtHardware::GNSS_TX_PIN);

  Wire.begin(FtHardware::I2C_SDA_PIN, FtHardware::I2C_SCL_PIN);
  delay(100);
  nav.compassReady = qmcBegin();
  bool compassCalStoreOk = compassCal.begin();

  SPI.begin(FtHardware::LORA_SCK_PIN, FtHardware::LORA_MISO_PIN,
            FtHardware::LORA_MOSI_PIN, FtHardware::LORA_NSS_PIN);

  bool loraOk = lora.begin();

  Serial.print("[COMPASS] ready=");
  Serial.println(nav.compassReady ? 1 : 0);
  Serial.print("[LORA] native ready=");
  Serial.println(loraOk ? 1 : 0);
  Serial.print("[COMPASS CAL] nvs=");
  Serial.print(compassCalStoreOk ? "1" : "0");
  Serial.print(" calibrated=");
  Serial.print(compassCal.isCalibrated() ? "1" : "0");
  Serial.print(" quality=");
  Serial.println(compassCal.savedQuality());

  hostSend("LR01_BOOT version=MESH_NATIVE_A2 protocol=2 node=FT02 id=!4c423002");
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
