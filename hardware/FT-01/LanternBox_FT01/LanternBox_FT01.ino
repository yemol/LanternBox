#include <M5Cardputer.h>
#include "FtConfig.h"
#include <SPI.h>
#include <SD.h>
#include "UiRecorder.h"
#include "UiNavigation.h"
#include "HelpManager.h"
#include "UiLog.h"
#include "AudioLogger.h"
#include "SyncManager.h"
#include "UiSync.h"
#include "TaskManager.h"
#include "UiTasks.h"
#include "LoRaManager.h"
#include "UiLoRaProbe.h"
#include "FtUiCommon.h"
#include "FtTextUtil.h"
#include "FtHardware.h"
#include "FtTimeUtil.h"

/*
  LanternBox Field Terminal
  Version: v0.1.6 Nav Session List
  Device: M5Stack Cardputer Adv + LoRa/GNSS Cap

  Proven baseline:
  - SD pins: SCK=40 MISO=39 MOSI=14 CS=12
  - SD init freq: 400000
  - GNSS UART: RX15 TX13 115200
  - LoRa NSS must be pulled HIGH before SD access because LoRa and SD share SPI.

  Controls in Recorder:
  - T: storage test event
  - B: save base position, requires GNSS FIX
  - P / Enter: save path point, requires GNSS FIX
  - A / Space: toggle auto track, 30 seconds
  - R: re-init SD and GNSS
  - Esc / Del: leave recorder, stop session and auto track
*/

#define LOCAL_TIMEZONE_OFFSET_SECONDS 28800
static const char* LOCAL_TIMEZONE_TEXT = "UTC+8";

const char* VERSION = "v0.5.2e";

// Serial debug switches.
// Keep raw NMEA off by default, otherwise useful logs are buried.
static const bool DEBUG_NMEA_RAW = false;
static const bool DEBUG_GNSS_SUMMARY = true;
static const char* DEVICE_ID = "FT01-0001";

M5Canvas canvas(&M5Cardputer.Display);
HardwareSerial GNSS(1);
AudioLogger audioLogger;
SyncManager syncManager;
TaskManager taskManager;
LoRaManager loraManager;
UiLoRaProbe uiLoRaProbe;
unsigned long lastLoRaInitAttemptMs = 0;
static constexpr unsigned long LORA_INIT_RETRY_MS = 30000UL;

m5::imu_data_t imuData;
double imuHeadingDeg = 0.0;
bool imuHeadingReady = false;
unsigned long lastImuMillis = 0;

// ---------- App state ----------
enum AppScreen {
  SCREEN_HOME,
  SCREEN_RECORDER,
  SCREEN_LOG,
  SCREEN_NAV,
  SCREEN_SYNC,
  SCREEN_TASKS,
  SCREEN_DEVICE,
  SCREEN_LORA_PROBE,
  SCREEN_HELP,
  SCREEN_PLACEHOLDER
};

AppScreen currentScreen = SCREEN_HOME;
AppScreen previousScreen = SCREEN_HOME;

struct MenuItem {
  const char* titleCn;
  const char* titleEn;
  AppScreen target;
};

const MenuItem menuItems[] = {
  {"路径", "LOG",  SCREEN_RECORDER},
  {"日志", "LOGS", SCREEN_LOG},
  {"导航", "NAV",  SCREEN_NAV},
  {"任务", "TASK", SCREEN_TASKS},
  {"设置", "SET",  SCREEN_PLACEHOLDER},
  {"同步", "SYNC", SCREEN_SYNC},
  {"设备", "DEV",  SCREEN_DEVICE},
  {"通信", "LORA", SCREEN_LORA_PROBE},
  {"关于", "INFO", SCREEN_PLACEHOLDER}
};

const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
const int itemsPerPage = 4;
const int totalPages = (menuCount + itemsPerPage - 1) / itemsPerPage;

int selectedIndex = 0;
int currentPage = 0;
String lastAction = "";
String placeholderTitle = "";
String lastKeyText = "--";

bool helpJustOpened = false;
int devicePage = 0;
const int DEVICE_PAGE_COUNT = 3;

// ---------- Time ----------
unsigned long baseEpoch = 0;
unsigned long bootMillis = 0;
long timeOffsetSeconds = 0;
bool gnssTimeSynced = false;
String timeSourceText = "LOCAL";
String lastGnssSyncText = "--";

// ---------- SD ----------
bool sdReady = false;
String sdStatusText = "WAIT";
String sdMessage = "NOT TESTED";
String sdTypeText = "--";
String sdSizeText = "--";
String sdLogText = "--";
String lastWriteStatus = "READY";

// ---------- Battery ----------
int batteryLevel = -1;
int batteryVoltage = -1;
unsigned long lastStatusPoll = 0;

// ---------- GNSS ----------
String nmeaLine = "";
String gnssStatusText = "WAIT";
bool gnssNmeaSeen = false;
bool suppressGnssStatusLog = false;
bool syncCommandActive = false;
unsigned long syncQuietUntilMs = 0;
static const unsigned long TASK_RECEIVE_TIMEOUT_MS = 8000;
unsigned long taskReceiveLastActivityMs = 0;
bool gnssFix = false;
int gnssSatellites = -1;
double gnssLat = 0.0;
double gnssLon = 0.0;
String gnssUtcTime = "--";
String gnssUtcDate = "--";
unsigned long lastGnssSummaryLogMs = 0;
bool lastGnssSummaryFix = false;
String gnssLastSentence = "--";
unsigned long gnssLastNmeaMillis = 0;
unsigned long gnssLastFixMillis = 0;

bool isSyncSerialQuietMode() {
  if (suppressGnssStatusLog) return true;
  if (syncCommandActive) return true;
  if (currentScreen == SCREEN_SYNC) return true;

  unsigned long now = millis();
  if (syncQuietUntilMs != 0 && (long)(syncQuietUntilMs - now) > 0) return true;
  return false;
}

void beginSyncSerialQuiet(unsigned long holdMs = 15000) {
  unsigned long now = millis();
  syncQuietUntilMs = now + holdMs;
  suppressGnssStatusLog = true;
}

void endSyncSerialQuiet(bool keepQuiet = true) {
  syncCommandActive = false;
  suppressGnssStatusLog = false;
  if (keepQuiet) {
    syncQuietUntilMs = millis() + 15000;
  }
}

// ---------- Recorder ----------
bool baseSet = false;
double baseLat = 0.0;
double baseLon = 0.0;
int pathPointCount = 0;
int pathWriteCount = 0;
int baseWriteCount = 0;
int eventWriteCount = 0;
bool autoTrack = false;
unsigned long lastAutoTrackMillis = 0;

String currentSessionId = "";
String lastRecordTime = "--";
bool sessionActive = false;

// ---------- Forward declarations ----------
void useChineseFont16();
void useChineseFont12();
void useAsciiFont();
unsigned long getCurrentEpoch();
void updateDeviceStatus();
String formatDouble6(double value);
bool initSD();
bool ensureLanternDirs();
bool writeBootLog();
bool appendLineToFile(const char* path, const String& line);
bool appendFieldEvent(const String& eventType, const String& note);
bool writeStorageTestEvent();
void initGNSS();
void drawHomeScreen();
void drawLoRaProbeScreen();
void handleLoRaProbeKey(const String& key);
void handleDeviceKey(const String& key);
void openHelpPage(HelpType type, AppScreen returnPage);

void logGnssSummaryIfNeeded() {
  if (!DEBUG_GNSS_SUMMARY) return;
  if (isSyncSerialQuietMode()) return;

  unsigned long now = millis();

  // Log immediately on FIX/NOFIX transition, otherwise only every 10 seconds.
  bool changed = (gnssFix != lastGnssSummaryFix);
  if (!changed && now - lastGnssSummaryLogMs < 10000) {
    return;
  }

  lastGnssSummaryLogMs = now;
  lastGnssSummaryFix = gnssFix;

  Serial.print("[GNSS] ");
  Serial.print(gnssFix ? "FIX" : "NOFIX");
  Serial.print(" sats=");
  Serial.print(gnssSatellites);

  if (gnssFix) {
    Serial.print(" lat=");
    Serial.print(gnssLat, 6);
    Serial.print(" lon=");
    Serial.print(gnssLon, 6);
  }

  Serial.print(" utc=");
  Serial.print(gnssUtcTime);
  Serial.print(" date=");
  Serial.println(gnssUtcDate);
}

void readGnssStream();
void drawCurrentScreen();
void processSerialSyncCommands();
void processSerialSyncCommandLine(String line);
void markTaskReceiveActivity();
void clearTaskReceiveActivity();


String shortDeviceId() {
  String id = String(DEVICE_ID);
  if (id.length() <= 8) return id;
  return id.substring(0, 8);
}

String shortVersionText() {
  String v = String(VERSION);
  if (v.length() <= 12) return v;
  return v.substring(0, 12);
}

String uptimeText() {
  unsigned long sec = millis() / 1000UL;
  unsigned long h = sec / 3600UL;
  unsigned long m = (sec % 3600UL) / 60UL;
  unsigned long s = sec % 60UL;

  char buf[24];
  if (h > 0) snprintf(buf, sizeof(buf), "%luh%02lum", h, m);
  else snprintf(buf, sizeof(buf), "%lum%02lus", m, s);
  return String(buf);
}

void drawDeviceHeader(const String& title) {
  updateDeviceStatus();
  ftDrawHeaderBase(title);
  ftDrawSdGnssStatus(136, 162);
  ftDrawClockHHMM(204);
}

void drawDeviceRow(int y, const String& label, const String& value, uint16_t color = WHITE) {
  ftDrawLabelValueRow(y, label, value, 84, color);
}

void drawDeviceFooter() {
  ftDrawFooter("< > Page | R Refresh | H Help");

  useAsciiFont();
  canvas.setCursor(202, 116);
  canvas.print(devicePage + 1);
  canvas.print("/");
  canvas.print(DEVICE_PAGE_COUNT);
}

void drawDeviceScreen() {
  updateDeviceStatus();

  canvas.fillSprite(BLACK);

  if (devicePage == 0) {
    drawDeviceHeader("设备 核心");

    char timeText[12];
    epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

    String batt = batteryLevel >= 0 ? String(batteryLevel) + "%" : "--";
    String volt = batteryVoltage > 0 ? String(batteryVoltage) + "mV" : "--";

    drawDeviceRow(32, "设备", String("FT-01 ") + shortDeviceId(), GREEN);
    drawDeviceRow(48, "版本", String(VERSION));
    drawDeviceRow(64, "运行", uptimeText());
    drawDeviceRow(80, "时间", String(timeText) + " UTC+8");
    drawDeviceRow(96, "电源", batt + " " + volt, batteryLevel <= 20 ? ORANGE : WHITE);
  } else if (devicePage == 1) {
    drawDeviceHeader("设备 存储");

    String gnss = gnssFix ? "FIX" : (gnssNmeaSeen ? "NOFIX" : "NONE");
    String coord = gnssFix ? formatDouble6(gnssLat) + "," + formatDouble6(gnssLon) : "--";

    drawDeviceRow(32, "SD", sdStatusText + " " + sdTypeText, sdReady ? GREEN : ORANGE);
    drawDeviceRow(48, "容量", sdSizeText);
    drawDeviceRow(64, "写入", lastWriteStatus, lastWriteStatus.indexOf("OK") >= 0 || lastWriteStatus == "READY" ? GREEN : ORANGE);
    drawDeviceRow(80, "GNSS", gnss + " SAT:" + String(gnssSatellites), gnssFix ? GREEN : ORANGE);
    drawDeviceRow(96, "坐标", coord);
  } else {
    drawDeviceHeader("设备 模块");

    drawDeviceRow(32, "日志", "OK", GREEN);
    drawDeviceRow(48, "导航", "UI TEST", ORANGE);
    drawDeviceRow(64, "路径", String(pathPointCount) + " pts");
    drawDeviceRow(80, "音频", "OK", GREEN);
    drawDeviceRow(96, "LoRa", loraManager.isReady() ? "PROBE OK" : "Probe", loraManager.isReady() ? GREEN : ORANGE);
  }

  drawDeviceFooter();
  canvas.pushSprite(0, 0);
}


// ---------- Font helpers ----------
void useChineseFont16() {
  canvas.setFont(&fonts::efontCN_16);
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);
}

void useChineseFont12() {
  canvas.setFont(&fonts::efontCN_12);
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);
}

void useAsciiFont() {
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);
}

// ---------- Time helpers ----------
unsigned long getCurrentEpoch() {
  long uptimeSeconds = (millis() - bootMillis) / 1000;
  long current = (long)baseEpoch + uptimeSeconds + timeOffsetSeconds;
  if (current < 0) current = 0;
  return (unsigned long)current;
}

String currentDeviceDateText() {
  char dateText[12];
  epochToDateString(getCurrentEpoch(), dateText, sizeof(dateText));
  return String(dateText);
}

String currentDeviceTimeText() {
  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));
  return String(timeText);
}

bool syncDeviceTimeFromGnssRaw(const String& dateRaw, const String& timeRaw) {
  if (dateRaw.length() < 6 || timeRaw.length() < 6) {
    return false;
  }

  int day = dateRaw.substring(0, 2).toInt();
  int month = dateRaw.substring(2, 4).toInt();
  int year = 2000 + dateRaw.substring(4, 6).toInt();

  int hour = timeRaw.substring(0, 2).toInt();
  int minute = timeRaw.substring(2, 4).toInt();
  int second = timeRaw.substring(4, 6).toInt();

  unsigned long gnssEpoch = makeEpochFromDateTime(year, month, day, hour, minute, second);
  if (gnssEpoch == 0) {
    return false;
  }

  baseEpoch = gnssEpoch + LOCAL_TIMEZONE_OFFSET_SECONDS;
  bootMillis = millis();
  timeOffsetSeconds = 0;
  gnssTimeSynced = true;
  timeSourceText = "GNSS";
  lastGnssSyncText = currentDeviceTimeText();

  Serial.print("[TIME] GNSS sync ");
  Serial.print(gnssUtcDate);
  Serial.print(" ");
  Serial.println(gnssUtcTime);

  return true;
}

// ---------- Session helpers ----------
String makeSessionId() {
  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

  String compact = String(timeText);
  compact.replace(":", "");

  String id = String(DEVICE_ID);
  id.replace("-", "");
  return id + "-" + compact;
}

void ensureSessionStarted(const char* reason) {
  if (sessionActive && currentSessionId.length() > 0) return;

  currentSessionId = makeSessionId();
  sessionActive = true;

  String note = "session started: ";
  note += reason;
  appendFieldEvent("session_start", note);

  Serial.print("[SESSION] start ");
  Serial.println(currentSessionId);
}

void stopSession() {
  if (!sessionActive) return;

  appendFieldEvent("session_stop", "leave recorder");
  sessionActive = false;
  autoTrack = false;

  Serial.print("[SESSION] stop ");
  Serial.println(currentSessionId);
}

// ---------- SD helpers ----------
void prepareSharedSpiBusForSD() {
  FtHardware::prepareSharedSpiIdle(20000UL);
  if (!isSyncSerialQuietMode()) {
    Serial.println("[SPI] shared bus idle for SD");
  }
}

String cardTypeToText(uint8_t cardType) {
  if (cardType == CARD_NONE) return "NONE";
  if (cardType == CARD_MMC) return "MMC";
  if (cardType == CARD_SD) return "SDSC";
  if (cardType == CARD_SDHC) return "SDHC";
  return "UNKNOWN";
}

bool ensureDir(const char* path) {
  if (SD.exists(path)) return true;

  Serial.print("[SD] mkdir ");
  Serial.println(path);

  return SD.mkdir(path);
}

bool ensureLanternDirs() {
  if (!sdReady) return false;
  if (!ensureDir("/lanternbox")) return false;
  if (!ensureDir("/lanternbox/logs")) return false;
  if (!ensureDir("/lanternbox/tracks")) return false;
  if (!ensureDir("/lanternbox/sessions")) return false;
  if (!ensureDir("/lanternbox/audio")) return false;
  return true;
}

bool appendLineToFile(const char* path, const String& line) {
  if (!sdReady) {
    lastWriteStatus = "SD NOT READY";
    Serial.println("[SD] append failed: sdReady=false");
    return false;
  }

  if (!ensureLanternDirs()) {
    lastWriteStatus = "DIR FAIL";
    Serial.println("[SD] append failed: ensureLanternDirs failed");
    return false;
  }

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    lastWriteStatus = "OPEN FAIL";
    Serial.print("[SD] open append failed: ");
    Serial.println(path);
    return false;
  }

  size_t written = file.println(line);
  file.close();

  if (written == 0) {
    lastWriteStatus = "WRITE 0";
    Serial.print("[SD] write returned 0: ");
    Serial.println(path);
    return false;
  }

  lastWriteStatus = "WRITE OK";
  Serial.print("[SD] append OK: ");
  Serial.println(path);
  return true;
}

String jsonGnssFields() {
  String json = "";
  json += "\"gnss_fix\":";
  json += gnssFix ? "true" : "false";
  json += ",\"satellites\":";
  json += String(gnssSatellites);
  json += ",\"lat\":";
  json += String(gnssLat, 6);
  json += ",\"lon\":";
  json += String(gnssLon, 6);
  json += ",\"gnss_utc_time\":\"";
  json += gnssUtcTime;
  json += "\",\"gnss_utc_date\":\"";
  json += gnssUtcDate;
  json += "\",\"timezone\":\"";
  json += LOCAL_TIMEZONE_TEXT;
  json += "\"";
  return json;
}

bool writeBootLog() {
  if (!sdReady || !ensureLanternDirs()) {
    sdLogText = "NO SD";
    return false;
  }

  String line = "{";
  line += "\"type\":\"boot\",";
  line += "\"device_id\":\"";
  line += DEVICE_ID;
  line += "\",\"version\":\"";
  line += VERSION;
  line += "\",\"device_date\":\"";
  line += currentDeviceDateText();
  line += "\",\"device_time\":\"";
  line += currentDeviceTimeText();
  line += "\",\"sd\":\"ok\",";
  line += "\"card_type\":\"";
  line += sdTypeText;
  line += "\",\"card_size\":\"";
  line += sdSizeText;
  line += "\",\"gnss_uart\":\"RX15_TX13_115200\",";
  line += "\"gnss_source\":\"multi_constellation\",";
  line += "\"time_source_initial\":\"";
  line += timeSourceText;
  line += "\",\"spi_rule\":\"lora_nss_high_before_sd\"";
  line += "}";

  bool ok = appendLineToFile("/lanternbox/logs/boot.jsonl", line);
  sdLogText = ok ? "OK" : "FAIL";
  Serial.println(ok ? "[SD] boot log write OK" : "[SD] boot log write FAIL");
  return ok;
}

bool appendFieldEvent(const String& eventType, const String& note) {
  String line = "{";
  line += "\"type\":\"";
  line += eventType;
  line += "\",\"device_id\":\"";
  line += DEVICE_ID;
  line += "\",\"version\":\"";
  line += VERSION;
  line += "\",\"session_id\":\"";
  line += currentSessionId;
  line += "\",\"device_date\":\"";
  line += currentDeviceDateText();
  line += "\",\"device_time\":\"";
  line += currentDeviceTimeText();
  line += "\",";
  line += jsonGnssFields();
  line += ",\"note\":\"";
  line += note;
  line += "\"}";

  bool ok = appendLineToFile("/lanternbox/logs/field_events.jsonl", line);
  if (ok) {
    eventWriteCount++;
    lastWriteStatus = "EVENT OK";
  } else if (lastWriteStatus == "WRITE OK") {
    lastWriteStatus = "EVENT FAIL";
  }

  Serial.println(ok ? "[LOG] field event OK" : "[LOG] field event FAIL");
  return ok;
}

bool writeStorageTestEvent() {
  ensureSessionStarted("test");
  bool ok = appendFieldEvent("storage_test", "manual test write from recorder screen");
  lastAction = ok ? "TEST OK" : "TEST FAIL";
  return ok;
}

bool initSD() {
  sdReady = false;
  sdStatusText = "TEST";
  sdMessage = "INIT";
  sdTypeText = "--";
  sdSizeText = "--";
  sdLogText = "--";

  Serial.println();
  Serial.println("[SD] Stable init start");
  Serial.println("[SD] Pins: SCK=40 MISO=39 MOSI=14 CS=12");
  Serial.println("[SD] Freq: 400000");

  prepareSharedSpiBusForSD();

  SD.end();
  delay(80);
  SPI.end();
  delay(80);

  prepareSharedSpiBusForSD();

  SPI.begin(FtHardware::SD_SCK_PIN, FtHardware::SD_MISO_PIN, FtHardware::SD_MOSI_PIN, FtHardware::SD_CS_PIN);
  delay(50);

  if (!SD.begin(FtHardware::SD_CS_PIN, SPI, FtHardware::SD_INIT_FREQ)) {
    sdReady = false;
    sdStatusText = "FAIL";
    sdMessage = "BEGIN FAIL";
    Serial.println("[SD] SD.begin failed");
    return false;
  }

  uint8_t cardType = SD.cardType();
  sdTypeText = cardTypeToText(cardType);

  if (cardType == CARD_NONE) {
    sdReady = false;
    sdStatusText = "FAIL";
    sdMessage = "NO CARD";
    Serial.println("[SD] card type none");
    return false;
  }

  sdSizeText = ftFormatBytes(SD.cardSize(), 2);

  Serial.print("[SD] type: ");
  Serial.println(sdTypeText);
  Serial.print("[SD] size: ");
  Serial.println(sdSizeText);

  sdReady = true;
  sdStatusText = "OK";
  sdMessage = "READY";

  ensureLanternDirs();
  writeBootLog();

  Serial.println("[SD] Stable init success");
  return true;
}

// ---------- GNSS helpers ----------
String getCsvField(const String& line, int fieldIndex) {
  int currentField = 0;
  int start = 0;

  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line[i] == ',' || line[i] == '*') {
      if (currentField == fieldIndex) return line.substring(start, i);
      currentField++;
      start = i + 1;
    }
  }

  return "";
}

double parseNmeaCoord(const String& raw, const String& hemi) {
  if (raw.length() < 4) return 0.0;

  double val = raw.toDouble();
  int degrees = (int)(val / 100);
  double minutes = val - (degrees * 100);
  double decimal = degrees + (minutes / 60.0);

  if (hemi == "S" || hemi == "W") decimal = -decimal;
  return decimal;
}

String parseUtcTime(const String& raw) {
  if (raw.length() < 6) return "--";
  return raw.substring(0, 2) + ":" + raw.substring(2, 4) + ":" + raw.substring(4, 6);
}

String parseUtcDate(const String& raw) {
  if (raw.length() < 6) return "--";
  String dd = raw.substring(0, 2);
  String mm = raw.substring(2, 4);
  String yy = raw.substring(4, 6);
  return "20" + yy + "-" + mm + "-" + dd;
}

void parseGGA(const String& line) {
  String timeRaw = getCsvField(line, 1);
  String latRaw = getCsvField(line, 2);
  String latHemi = getCsvField(line, 3);
  String lonRaw = getCsvField(line, 4);
  String lonHemi = getCsvField(line, 5);
  String qualityRaw = getCsvField(line, 6);
  String satRaw = getCsvField(line, 7);

  gnssUtcTime = parseUtcTime(timeRaw);

  int q = qualityRaw.toInt();
  gnssFix = q > 0;
  gnssSatellites = satRaw.length() > 0 ? satRaw.toInt() : -1;

  if (gnssFix && latRaw.length() > 0 && lonRaw.length() > 0) {
    gnssLat = parseNmeaCoord(latRaw, latHemi);
    gnssLon = parseNmeaCoord(lonRaw, lonHemi);
    gnssLastFixMillis = millis();
  }

  gnssStatusText = "GGA OK";
}

void parseRMC(const String& line) {
  String timeRaw = getCsvField(line, 1);
  String statusRaw = getCsvField(line, 2);
  String latRaw = getCsvField(line, 3);
  String latHemi = getCsvField(line, 4);
  String lonRaw = getCsvField(line, 5);
  String lonHemi = getCsvField(line, 6);
  String dateRaw = getCsvField(line, 9);

  gnssUtcTime = parseUtcTime(timeRaw);
  gnssUtcDate = parseUtcDate(dateRaw);

  if (statusRaw == "A" && dateRaw.length() >= 6 && timeRaw.length() >= 6) {
    syncDeviceTimeFromGnssRaw(dateRaw, timeRaw);
  }

  if (statusRaw == "A") {
    gnssFix = true;
    if (latRaw.length() > 0 && lonRaw.length() > 0) {
      gnssLat = parseNmeaCoord(latRaw, latHemi);
      gnssLon = parseNmeaCoord(lonRaw, lonHemi);
      gnssLastFixMillis = millis();
    }
  }

  gnssStatusText = "RMC OK";
}

bool looksLikeNmea(const String& line) {
  if (!line.startsWith("$")) return false;
  if (line.indexOf("GGA") >= 0) return true;
  if (line.indexOf("RMC") >= 0) return true;
  if (line.indexOf("GLL") >= 0) return true;
  if (line.indexOf("GSA") >= 0) return true;
  if (line.indexOf("GSV") >= 0) return true;
  if (line.indexOf("VTG") >= 0) return true;
  if (line.indexOf("TXT") >= 0) return true;
  return false;
}

void handleNmeaLine(const String& line) {
  if (!looksLikeNmea(line)) return;

  gnssNmeaSeen = true;
  gnssLastNmeaMillis = millis();
  gnssLastSentence = line.substring(0, min((int)line.length(), 22));
      // raw NMEA disabled

  if (line.startsWith("$GNGGA") || line.startsWith("$GPGGA") || line.startsWith("$BDGGA")) {
    parseGGA(line);
  } else if (line.startsWith("$GNRMC") || line.startsWith("$GPRMC") || line.startsWith("$BDRMC")) {
    parseRMC(line);
  } else {
    gnssStatusText = "NMEA OK";
  }
}


// duplicate logGnssSummaryIfNeeded removed in v0.2.5a

void readGnssStream() {
  while (GNSS.available()) {
    char c = GNSS.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (nmeaLine.length() > 0) {
        handleNmeaLine(nmeaLine);
        nmeaLine = "";
      }
      continue;
    }

    if (nmeaLine.length() < 180) {
      nmeaLine += c;
    } else {
      nmeaLine = "";
    }
  }
}

void initGNSS() {
  GNSS.end();
  delay(80);

  GNSS.begin(FtHardware::GNSS_BAUD, SERIAL_8N1, FtHardware::GNSS_RX_PIN, FtHardware::GNSS_TX_PIN);
  delay(120);

  gnssStatusText = "GNSS START";
  gnssNmeaSeen = false;
  gnssFix = false;
  gnssSatellites = -1;
  gnssLat = 0.0;
  gnssLon = 0.0;
  gnssUtcTime = "--";
  gnssUtcDate = "--";
  gnssLastSentence = "--";
  gnssLastNmeaMillis = 0;
  gnssLastFixMillis = 0;

  if (!isSyncSerialQuietMode()) {
    Serial.println("[GNSS] started RX15 TX13 baud=115200");
  }
}

String formatDouble6(double value) {
  if (value == 0.0) return "--";
  char buf[24];
  snprintf(buf, sizeof(buf), "%.6f", value);
  return String(buf);
}

// ---------- Recorder ----------
bool saveBasePosition() {
  ensureSessionStarted("base");

  if (!sdReady || !ensureLanternDirs()) {
    lastAction = "NO SD";
    lastWriteStatus = "NO SD";
    return false;
  }

  if (!gnssFix) {
    appendFieldEvent("base_mark_failed", "no gnss fix");
    lastAction = "NO FIX";
    lastWriteStatus = "NO FIX";
    return false;
  }

  baseLat = gnssLat;
  baseLon = gnssLon;
  baseSet = true;

  String base = "{";
  base += "\"type\":\"base\",";
  base += "\"device_id\":\"";
  base += DEVICE_ID;
  base += "\",\"version\":\"";
  base += VERSION;
  base += "\",\"session_id\":\"";
  base += currentSessionId;
  base += "\",\"device_time\":\"";
  base += currentDeviceTimeText();
  base += "\",";
  base += jsonGnssFields();
  base += "}";

  File file = SD.open("/lanternbox/base.json", FILE_WRITE);
  if (!file) {
    lastAction = "BASE FAIL";
    lastWriteStatus = "BASE FAIL";
    return false;
  }

  size_t written = file.println(base);
  file.close();

  if (written == 0) {
    lastAction = "BASE FAIL";
    lastWriteStatus = "BASE W0";
    return false;
  }

  appendFieldEvent("base_mark", "base position saved");
  baseWriteCount++;
  lastRecordTime = currentDeviceTimeText();
  lastAction = "BASE OK";
  lastWriteStatus = "BASE OK";

  Serial.println("[REC] base saved");
  return true;
}

bool savePathPoint(const char* mode) {
  ensureSessionStarted(mode);

  if (!sdReady || !ensureLanternDirs()) {
    lastAction = "NO SD";
    lastWriteStatus = "NO SD";
    return false;
  }

  if (!gnssFix) {
    appendFieldEvent("path_point_failed", "no gnss fix");
    lastAction = "NO FIX";
    lastWriteStatus = "NO FIX";
    return false;
  }

  pathPointCount++;

  String line = "{";
  line += "\"type\":\"path_point\",";
  line += "\"mode\":\"";
  line += mode;
  line += "\",\"seq\":";
  line += String(pathPointCount);
  line += ",\"device_id\":\"";
  line += DEVICE_ID;
  line += "\",\"version\":\"";
  line += VERSION;
  line += "\",\"session_id\":\"";
  line += currentSessionId;
  line += "\",\"device_date\":\"";
  line += currentDeviceDateText();
  line += "\",\"device_time\":\"";
  line += currentDeviceTimeText();
  line += "\",";
  line += jsonGnssFields();

  if (baseSet) {
    line += ",\"base_lat\":";
    line += String(baseLat, 6);
    line += ",\"base_lon\":";
    line += String(baseLon, 6);
  }

  line += "}";

  bool ok = appendLineToFile("/lanternbox/tracks/path_points.jsonl", line);

  if (ok) {
    pathWriteCount++;
    lastRecordTime = currentDeviceTimeText();
    lastAction = "POINT OK";
    lastWriteStatus = "POINT OK";
    appendFieldEvent("path_point", mode);
  } else {
    lastAction = "POINT FAIL";
    lastWriteStatus = "POINT FAIL";
  }

  Serial.println(ok ? "[REC] path point saved" : "[REC] path point save failed");
  return ok;
}

void autoTrackTick() {
  if (!autoTrack) return;
  if (!gnssFix) return;

  unsigned long now = millis();
  if (now - lastAutoTrackMillis >= FT_AUTO_TRACK_INTERVAL_MS) {
    lastAutoTrackMillis = now;
    savePathPoint("auto");
  }
}

// ---------- Device status ----------
void updateDeviceStatus() {
  batteryLevel = M5Cardputer.Power.getBatteryLevel();
  batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
}

// ---------- Drawing ----------
void drawTopBar() {
  canvas.fillRect(0, 0, canvas.width(), 20, BLACK);

  useChineseFont12();
  canvas.setTextColor(gnssFix ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(4, 3);
  canvas.print(gnssFix ? "定位" : "搜星");

  char timeText[8];
  epochToShortTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(80, 5);
  canvas.print(timeText);

  canvas.setTextColor(sdReady ? GREEN : ORANGE, BLACK);
  canvas.setCursor(126, 5);
  canvas.print("SD:");
  canvas.print(sdReady ? "OK" : "NO");

  canvas.setTextColor(ORANGE, BLACK);
  canvas.setCursor(184, 5);
  if (batteryLevel >= 0) {
    canvas.print(batteryLevel);
    canvas.print("%");
  } else {
    canvas.print("--");
  }

  const size_t unread = loraManager.unreadMessageCount();
  if (unread > 0) {
    canvas.setTextColor(CYAN, BLACK);
    canvas.setCursor(216, 5);
    canvas.print("M");
    if (unread > 9) canvas.print("9+");
    else canvas.print((int)unread);
  }

  canvas.drawLine(0, 20, canvas.width(), 20, DARKGREY);
}

void drawTitle() {
  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(8, 24);
  canvas.print("FT-01");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(62, 30);
  canvas.print(shortDeviceId());

  canvas.setCursor(126, 30);
  canvas.print(shortVersionText());
}

void drawCard(int slot, int itemIndex, bool selected) {
  int x;
  int y;

  if (slot == 0) {
    x = 8;
    y = 48;
  } else if (slot == 1) {
    x = 126;
    y = 48;
  } else if (slot == 2) {
    x = 8;
    y = 84;
  } else {
    x = 126;
    y = 84;
  }

  int w = 106;
  int h = 30;

  uint16_t borderColor = selected ? GREEN : DARKGREY;
  uint16_t fillColor = selected ? DARKGREY : BLACK;
  uint16_t textColor = selected ? WHITE : LIGHTGREY;

  canvas.fillRoundRect(x, y, w, h, 4, fillColor);
  canvas.drawRoundRect(x, y, w, h, 4, borderColor);

  if (itemIndex >= menuCount) return;

  useChineseFont16();
  canvas.setTextColor(textColor, fillColor);
  canvas.setCursor(x + 12, y + 5);
  canvas.print(menuItems[itemIndex].titleCn);

  useAsciiFont();
  canvas.setTextColor(selected ? GREEN : DARKGREY, fillColor);
  canvas.setCursor(x + 66, y + 11);
  canvas.print(menuItems[itemIndex].titleEn);
}

void drawGrid() {
  int startIndex = currentPage * itemsPerPage;

  for (int slot = 0; slot < itemsPerPage; slot++) {
    int itemIndex = startIndex + slot;
    bool selected = (itemIndex == selectedIndex);
    drawCard(slot, itemIndex, selected);
  }
}

void drawBottomBar() {
  canvas.fillRect(0, 118, canvas.width(), 10, BLACK);

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(4, 120);
  canvas.print(currentPage + 1);
  canvas.print("/");
  canvas.print(totalPages);

  canvas.setCursor(38, 120);
  canvas.print("GNSS:");
  canvas.print(gnssFix ? "FIX" : "NO");

  canvas.setCursor(104, 120);
  canvas.print("SAT:");
  canvas.print(gnssSatellites >= 0 ? String(gnssSatellites) : "--");

  canvas.setCursor(158, 120);
  if (autoTrack) {
    canvas.setTextColor(GREEN, BLACK);
    canvas.print("AUTO ");
    canvas.print(pathWriteCount);
  } else if (sessionActive) {
    canvas.setTextColor(LIGHTGREY, BLACK);
    canvas.print("SESSION");
  } else if (lastAction.length() > 0) {
    canvas.print(lastAction);
  }
}

void drawHomeScreen() {
  canvas.fillSprite(BLACK);
  drawTopBar();
  drawTitle();
  drawGrid();
  drawBottomBar();
  canvas.pushSprite(0, 0);
}

// drawRecorderScreen() moved to UiRecorder.cpp in v0.1.0.
// The recorder page is now the first stable split module.

void drawPlaceholderScreen() {
  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(8, 8);
  canvas.print(placeholderTitle);

  useChineseFont12();
  canvas.setTextColor(LIGHTGREY, BLACK);
  canvas.setCursor(8, 42);
  canvas.print("功能将在后续版本接入");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 116);
  canvas.print("ESC Back");

  canvas.pushSprite(0, 0);
}

void drawCurrentScreen() {
  if (currentScreen == SCREEN_HOME) drawHomeScreen();
  else if (currentScreen == SCREEN_RECORDER) drawRecorderScreen();
  else if (currentScreen == SCREEN_LOG) drawLogScreen();
  else if (currentScreen == SCREEN_NAV) drawNavScreen();
  else if (currentScreen == SCREEN_SYNC) drawSyncScreen();
  else if (currentScreen == SCREEN_TASKS) drawTasksScreen();
  else if (currentScreen == SCREEN_DEVICE) drawDeviceScreen();
  else if (currentScreen == SCREEN_LORA_PROBE) drawLoRaProbeScreen();
  else if (currentScreen == SCREEN_HELP) drawHelpManager();
  else drawPlaceholderScreen();
}


// ---------- Menu movement ----------
void ensureSelectedVisible() {
  if (selectedIndex < 0) selectedIndex = menuCount - 1;
  if (selectedIndex >= menuCount) selectedIndex = 0;
  currentPage = selectedIndex / itemsPerPage;
}

void moveLeft() {
  int slot = selectedIndex % itemsPerPage;
  if (slot == 1 || slot == 3) selectedIndex--;
  else if (currentPage > 0) {
    currentPage--;
    selectedIndex = currentPage * itemsPerPage + 1;
    if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  }
}

void moveRight() {
  int slot = selectedIndex % itemsPerPage;
  if (slot == 0 || slot == 2) {
    if (selectedIndex + 1 < menuCount) selectedIndex++;
  } else if (currentPage < totalPages - 1) {
    currentPage++;
    selectedIndex = currentPage * itemsPerPage;
    if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  }
}

void moveUp() {
  int slot = selectedIndex % itemsPerPage;
  if (slot == 2 || slot == 3) selectedIndex -= 2;
  else if (currentPage > 0) {
    currentPage--;
    selectedIndex = currentPage * itemsPerPage + slot + 2;
    if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  }
}

void moveDown() {
  int slot = selectedIndex % itemsPerPage;
  if (slot == 0 || slot == 1) {
    if (selectedIndex + 2 < menuCount) selectedIndex += 2;
  } else if (currentPage < totalPages - 1) {
    currentPage++;
    selectedIndex = currentPage * itemsPerPage + (slot - 2);
    if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  }
}

// ---------- Keyboard ----------
String readKeyText() {
  if (!M5Cardputer.Keyboard.isChange()) return "";
  if (!M5Cardputer.Keyboard.isPressed()) return "";

  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

  String result = "";

  for (auto key : status.word) result += key;

  if (status.enter) result += "[ENTER]";
  if (status.del) result += "[DEL]";
  if (status.space) result += "[SPACE]";
  if (status.tab) result += "[TAB]";
  if (status.fn) result += "[FN]";
  if (status.ctrl) result += "[CTRL]";
  if (status.alt) result += "[ALT]";
  if (status.shift) result += "[SHIFT]";

  if (result.length() == 0) result = "[SPECIAL]";
  return result;
}

void confirmSelection() {
  const MenuItem& item = menuItems[selectedIndex];
  lastAction = item.titleEn;

  Serial.print("[ACTION] Enter ");
  Serial.print(item.titleCn);
  Serial.print(" / ");
  Serial.println(item.titleEn);

  currentScreen = item.target;
  switch (item.target) {
    case SCREEN_RECORDER:
      drawRecorderScreen();
      break;
    case SCREEN_LOG:
      audioLogger.refreshList();
      drawLogScreen();
      break;
    case SCREEN_NAV:
      navReloadTrackData();
      drawNavScreen();
      break;
    case SCREEN_SYNC:
      if (taskManager.isReceivingTasks()) {
        taskManager.abortReceiveTasks();
        clearTaskReceiveActivity();
      }
      drawSyncScreen();
      break;
    case SCREEN_TASKS:
      tasksRefresh();
      drawTasksScreen();
      break;
    case SCREEN_DEVICE:
      devicePage = 0;
      drawDeviceScreen();
      break;
    case SCREEN_LORA_PROBE:
      uiLoRaProbe.activate();
      break;
    default:
      placeholderTitle = item.titleCn;
      drawPlaceholderScreen();
      break;
  }
}


void drawLoRaProbeScreen() {
  uiLoRaProbe.draw();
}

void handleLoRaProbeKey(const String& key) {
  uiLoRaProbe.handleKey(key);
  if (uiLoRaProbe.wantsExit()) {
    uiLoRaProbe.clearExit();
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }
}


void openLogHelp() {
  previousScreen = SCREEN_LOG;
  currentScreen = SCREEN_HELP;
  helpJustOpened = false;
  showHelp(HELP_AUDIO);
}

void returnToHomeFromModule() {
  currentScreen = SCREEN_HOME;
  drawHomeScreen();
}

void openNavigationHelp(HelpType type) {
  previousScreen = SCREEN_NAV;
  currentScreen = SCREEN_HELP;
  showHelp(type);
}

void openSyncHelp() {
  previousScreen = SCREEN_SYNC;
  currentScreen = SCREEN_HELP;
  helpJustOpened = false;
  showHelp(HELP_SYNC);
}

void openHelpPage(HelpType type, AppScreen returnPage) {
  previousScreen = returnPage;
  currentScreen = SCREEN_HELP;
  showHelp(type);
  helpJustOpened = true;
}

void handleHomeKey(const String& key) {
  lastAction = "";

  if (key == "1") selectedIndex = currentPage * itemsPerPage + 0;
  else if (key == "2") selectedIndex = currentPage * itemsPerPage + 1;
  else if (key == "3") selectedIndex = currentPage * itemsPerPage + 2;
  else if (key == "4") selectedIndex = currentPage * itemsPerPage + 3;
  else if (FtKey::isLeft(key)) moveLeft();
  else if (FtKey::isRight(key)) moveRight();
  else if (FtKey::isUp(key)) moveUp();
  else if (FtKey::isDown(key)) moveDown();
  else if (FtKey::isEnter(key)) {
    confirmSelection();
    return;
  } else if (FtKey::hasLetter(key, 'r', 'R')) {
    initSD();
    initGNSS();
  }

  if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  ensureSelectedVisible();
  drawHomeScreen();
}

void handleDeviceKey(const String& key) {
  if (FtKey::isEsc(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  if (FtKey::isLeft(key)) {
    devicePage--;
    if (devicePage < 0) devicePage = DEVICE_PAGE_COUNT - 1;
  } else if (FtKey::isRight(key)) {
    devicePage++;
    if (devicePage >= DEVICE_PAGE_COUNT) devicePage = 0;
  } else if (FtKey::hasLetter(key, 'r', 'R')) {
    updateDeviceStatus();
    if (!sdReady) initSD();
    initGNSS();
  } else if (FtKey::hasLetter(key, 'h', 'H')) {
    openHelpPage(HELP_DEVICE, SCREEN_DEVICE);
    return;
  }

  drawDeviceScreen();
}


void handleRecorderKey(const String& key) {
  if (FtKey::isEsc(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  if (FtKey::hasLetter(key, 'h', 'H')) {
    openHelpPage(HELP_RECORDER, SCREEN_RECORDER);
    return;
  }

  bool guardedAction =
    FtKey::hasLetter(key, 'a', 'A') ||
    FtKey::hasLetter(key, 'b', 'B') ||
    FtKey::hasLetter(key, 'p', 'P') ||
    FtKey::hasLetter(key, 's', 'S') ||
    FtKey::isEnter(key) ||
    key == "[SPACE]";

  if (!gnssFix && guardedAction) {
    lastAction = "NO FIX";
    lastWriteStatus = "GNSS REQUIRED";
    Serial.println("[GUARD] recorder action blocked: GNSS FIX required");
    drawRecorderScreen();
    return;
  }

  if (FtKey::hasLetter(key, 's', 'S')) {
    stopSession();
    lastAction = "STOP";
    lastWriteStatus = "SESSION STOP";
    drawRecorderScreen();
    return;
  }

  if (FtKey::hasLetter(key, 'b', 'B')) {
    saveBasePosition();
  } else if (FtKey::hasLetter(key, 'p', 'P') || FtKey::isEnter(key)) {
    savePathPoint("manual");
  } else if (FtKey::hasLetter(key, 'a', 'A') || key == "[SPACE]") {
    autoTrack = !autoTrack;

    if (autoTrack) {
      ensureSessionStarted("auto");
    }

    lastAutoTrackMillis = millis();
    lastAction = autoTrack ? "AUTO ON" : "AUTO OFF";
    appendFieldEvent(autoTrack ? "auto_track_on" : "auto_track_off", "toggle");

    if (autoTrack && gnssFix) {
      savePathPoint("auto_start");
      lastAutoTrackMillis = millis();
    }
  } else if (FtKey::hasLetter(key, 'r', 'R')) {
    initSD();
    initGNSS();
    lastAction = "RESET";
    lastWriteStatus = sdReady ? "SD OK" : "SD FAIL";
  }

  drawRecorderScreen();
}

void handleHelpKey(const String& key) {
  // Ignore the key repeat that opened Help.
  // Otherwise pressing H can immediately close the Help screen.
  if (helpJustOpened) {
    helpJustOpened = false;
    return;
  }

  if (FtKey::isEsc(key) || key == "[DEL]" || FtKey::isEnter(key)) {
    currentScreen = previousScreen;
    drawCurrentScreen();
  }
}



// ---------- Task sync receive recovery ----------
bool isTaskSyncTopLevelCommand(const String& line) {
  return line.startsWith("PUT_TASKS_BEGIN ") ||
         line == "PUT_TASKS_ABORT" ||
         line == "PUT_TASKS_END" ||
         line.startsWith("GET_RECORDS ") ||
         line.startsWith("GET_AUDIO_FILE ") ||
         line.startsWith("CLEAR_SYNCED_RECORDS") ||
         line.startsWith("DELETE_UPLOADED_AUDIO ") ||
         line == "GET_MANIFEST" ||
         line == "HELLO" ||
         line == "GET_HELLO";
}

void markTaskReceiveActivity() {
  taskReceiveLastActivityMs = millis();
}

void clearTaskReceiveActivity() {
  taskReceiveLastActivityMs = 0;
}

void abortTaskReceiveForRecovery(const String& reason) {
  if (taskManager.isReceivingTasks()) {
    taskManager.abortReceiveTasks();
  }
  clearTaskReceiveActivity();
  Serial.print("FT01_SYNC_TASKS_ABORTED reason=");
  Serial.println(reason);
  Serial.flush();
  endSyncSerialQuiet(true);
  if (currentScreen == SCREEN_SYNC) {
    syncShowPhase("任务接收已重置", reason);
  }
}

// ---------- USB Serial Sync command input ----------
// Read-only protocol for Core-side helper.
// Supported command:
//   GET_RECORDS path_points
//   GET_RECORDS field_events
//   GET_RECORDS boot_logs
//   GET_RECORDS audio_index
//   GET_AUDIO_FILE audio_001.wav
//
// This function never clears, truncates, rotates, or deletes terminal data.
void processSerialSyncCommandLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  // During USB sync, Serial must stay protocol-clean. GNSS/key/SPI debug output is
  // muted both while a command is handled and for a short quiet window after it.
  beginSyncSerialQuiet(15000);

  if (taskManager.isReceivingTasks()) {
    syncCommandActive = true;

    if (line != "PUT_TASKS_END" && line != "PUT_TASKS_ABORT" && isTaskSyncTopLevelCommand(line)) {
      // Recovery path: the previous host-side downlink may have timed out before
      // sending PUT_TASKS_END. Do not trap the next HELLO/GET_/PUT_TASKS_BEGIN
      // command as a stale task JSON line. Abort the unfinished receive and let
      // this command fall through to the normal protocol handler below.
      taskManager.abortReceiveTasks();
      clearTaskReceiveActivity();
      Serial.println("FT01_SYNC_TASKS_ABORTED reason=new_command");
      Serial.flush();
    } else if (line == "PUT_TASKS_ABORT") {
      taskManager.abortReceiveTasks();
      clearTaskReceiveActivity();
      Serial.println("FT01_SYNC_TASKS_ABORTED reason=host_abort");
      Serial.flush();
      endSyncSerialQuiet(true);
      return;
    } else if (line == "PUT_TASKS_END") {
      // v0.4.3d: do not ACK before persistence. Core must only receive a
      // successful TASKS_ACK after /lanternbox/tasks/tasks.jsonl is really
      // written and reloaded, otherwise the terminal can show completion while
      // the task list is still empty.
      int receivedBeforeSave = taskManager.receivedReceiveCount();
      bool receiveCountOk = (taskManager.expectedReceiveCount() <= 0 || receivedBeforeSave == taskManager.expectedReceiveCount()) && !taskManager.receiveHadError();

      // IMPORTANT: keep USB CDC receive/save path display-free until the final
      // ACK is printed. E-paper refresh can block long enough for the host to
      // hit its ACK timeout.
      bool saveOk = false;
      if (receiveCountOk) {
        saveOk = taskManager.finishReceiveTasks(sdReady);
      } else {
        taskManager.abortReceiveTasks();
      }

      int storedAfterSave = taskManager.storedReceiveCount();
      bool finalOk = receiveCountOk && saveOk && storedAfterSave == receivedBeforeSave;

      Serial.print("FT01_SYNC_TASKS_ACK received=");
      Serial.print(receivedBeforeSave);
      Serial.print(" stored=");
      Serial.print(storedAfterSave);
      Serial.print(" ok=");
      Serial.print(finalOk ? "true" : "false");
      Serial.print(" stage=saved");
      if (!finalOk) {
        Serial.print(" error=");
        String err = taskManager.receiveErrorText();
        if (err.length() == 0) err = receiveCountOk ? String("save_failed") : String("count_mismatch");
        Serial.print(err);
      }
      Serial.println();
      Serial.flush();

      Serial.print("FT01_SYNC_TASKS_SAVE_DONE received=");
      Serial.print(receivedBeforeSave);
      Serial.print(" stored=");
      Serial.print(storedAfterSave);
      Serial.print(" ok=");
      Serial.print(finalOk ? "true" : "false");
      if (!finalOk && taskManager.receiveErrorText().length() > 0) {
        Serial.print(" error=");
        Serial.print(taskManager.receiveErrorText());
      }
      Serial.println();
      Serial.flush();

      if (currentScreen == SCREEN_TASKS) drawTasksScreen();
      else syncShowPhase(finalOk ? "任务保存完成" : "任务保存失败", String(storedAfterSave) + "/" + String(receivedBeforeSave));
      clearTaskReceiveActivity();
      endSyncSerialQuiet(true);
    } else {
      markTaskReceiveActivity();
      bool ok = taskManager.receiveTaskLine(line);
      // v0.4.3f: explicit per-line ACK for host-side flow control.
      // Core must wait for this ACK before sending the next task JSON line.
      // This prevents USB CDC RX overflow when downlinking JSONL to ESP32-S3.
      Serial.print("FT01_SYNC_TASK_LINE_ACK index=");
      Serial.print(taskManager.receivedReceiveCount());
      Serial.print(" ok=");
      Serial.print(ok ? "true" : "false");
      if (!ok && taskManager.receiveErrorText().length() > 0) {
        Serial.print(" error=");
        Serial.print(taskManager.receiveErrorText());
      }
      Serial.println();
      Serial.flush();
      // Keep quiet until PUT_TASKS_END arrives.
      suppressGnssStatusLog = true;
    }
    return;
  }

  if (line.startsWith("PUT_TASKS_BEGIN ")) {
    String countText = line.substring(String("PUT_TASKS_BEGIN ").length());
    countText.trim();
    int expectedCount = countText.toInt();

    syncCommandActive = true;
    bool ok = taskManager.startReceiveTasks(expectedCount, sdReady);
    if (ok) markTaskReceiveActivity();
    else clearTaskReceiveActivity();
    // Do NOT refresh the e-paper screen here. Core may already be streaming
    // JSONL task lines immediately after PUT_TASKS_BEGIN, and display refresh
    // can starve the USB receive path. Acknowledge immediately, then keep the
    // receive loop near zero-cost until PUT_TASKS_END.
    Serial.print("FT01_SYNC_TASKS_BEGIN_ACK expected=");
    Serial.print(expectedCount);
    Serial.print(" ok=");
    Serial.print(ok ? "true" : "false");
    Serial.print(" protocol=line_ack");
    if (!ok && taskManager.receiveErrorText().length() > 0) {
      Serial.print(" error=");
      Serial.print(taskManager.receiveErrorText());
    }
    Serial.println();
    // Keep sync quiet until PUT_TASKS_END or PUT_TASKS_ABORT.
    suppressGnssStatusLog = true;
    return;
  }

  if (line == "PUT_TASKS_ABORT") {
    taskManager.abortReceiveTasks();
    clearTaskReceiveActivity();
    Serial.println("FT01_SYNC_TASKS_ABORTED reason=host_abort");
    endSyncSerialQuiet(true);
    return;
  }

  syncCommandActive = true;

  if (line.startsWith("GET_RECORDS ")) {
    String recordType = line.substring(String("GET_RECORDS ").length());
    recordType.trim();

    if (recordType == "path_points") syncShowPhase("同步轨迹", "路径点 path_points");
    else if (recordType == "field_events") syncShowPhase("同步日志", "现场日志 field_events");
    else if (recordType == "boot_logs") syncShowPhase("同步日志", "启动日志 boot_logs");
    else if (recordType == "audio_index") syncShowPhase("同步录音索引", "audio_index");
    else if (recordType == "task_reports") syncShowPhase("上传任务回报", "task_reports");
    else syncShowPhase("同步记录", recordType);

    if (recordType == "task_reports") {
      taskManager.printTaskReports(sdReady);
    } else {
      syncManager.printRecords(recordType, sdReady);
    }

    syncShowPhase("记录同步完成", recordType);
    endSyncSerialQuiet(true);
    return;
  }

  if (line.startsWith("CLEAR_SYNCED_RECORDS")) {
    String recordTypes = line.substring(String("CLEAR_SYNCED_RECORDS").length());
    recordTypes.trim();

    syncShowPhase("清理本地记录", recordTypes.length() > 0 ? recordTypes : String("none"));
    bool cleanupOk = syncManager.clearSyncedRecords(recordTypes, sdReady);
    syncShowPhase(cleanupOk ? String("清理完成") : String("清理失败"), recordTypes.length() > 0 ? recordTypes : String("none"));
    endSyncSerialQuiet(true);
    return;
  }

  if (line.startsWith("DELETE_UPLOADED_AUDIO ")) {
    String filename = line.substring(String("DELETE_UPLOADED_AUDIO ").length());
    filename.trim();

    syncShowPhase("清理录音文件", filename.length() > 0 ? filename : String("none"));
    bool audioCleanupOk = syncManager.deleteUploadedAudio(filename, sdReady);
    syncShowPhase(audioCleanupOk ? String("录音清理完成") : String("录音清理失败"), filename.length() > 0 ? filename : String("none"));
    endSyncSerialQuiet(true);
    return;
  }

  if (line.startsWith("GET_AUDIO_FILE ")) {
    String filename = line.substring(String("GET_AUDIO_FILE ").length());
    filename.trim();

    syncShowPhase("同步录音文件", filename);
    syncManager.printAudioFile(filename, sdReady);
    syncShowPhase("录音文件完成", filename);
    endSyncSerialQuiet(true);
    return;
  }

  if (line == "GET_MANIFEST") {
    syncShowPhase("生成同步清单", "Manifest");
    syncManager.printManifest(sdReady);
    syncShowPhase("同步清单完成", "Manifest sent");
    endSyncSerialQuiet(true);
    return;
  }

  if (line == "HELLO" || line == "GET_HELLO") {
    syncShowPhase("USB 握手", "Hello");
    syncManager.printUsbHello(sdReady, gnssFix, gnssSatellites, sdStatusText);
    endSyncSerialQuiet(true);
    return;
  }

  syncCommandActive = false;
}

void processSerialSyncCommands() {
  static String serialCommandBuffer = "";

  if (taskManager.isReceivingTasks() && taskReceiveLastActivityMs != 0 &&
      (millis() - taskReceiveLastActivityMs) > TASK_RECEIVE_TIMEOUT_MS) {
    // If the host times out mid-downlink, the terminal must not remain in
    // task-receive mode forever. Clear any partial JSON line so the next sync
    // can start without requiring a reboot.
    serialCommandBuffer = "";
    abortTaskReceiveForRecovery("timeout");
  }

  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (serialCommandBuffer.length() > 0) {
        processSerialSyncCommandLine(serialCommandBuffer);
        serialCommandBuffer = "";
      }
    } else {
      // Keep the command buffer bounded. Task JSONL downlink can be longer than command lines.
      if (serialCommandBuffer.length() < 4096) {
        serialCommandBuffer += c;
      } else {
        serialCommandBuffer = "";
        if (taskManager.isReceivingTasks()) {
          taskManager.abortReceiveTasks();
          clearTaskReceiveActivity();
          Serial.print("FT01_SYNC_TASK_LINE_ACK index=");
          Serial.print(taskManager.receivedReceiveCount());
          Serial.println(" ok=false error=line_too_long");
          Serial.println("FT01_SYNC_TASKS_ABORTED reason=line_too_long");
          Serial.flush();
          endSyncSerialQuiet(true);
        } else {
          Serial.println("FT01_SYNC_ERROR command_too_long");
        }
      }
    }
  }
}

void handleNavKeyWrapper(const String& key) {
  if (FtKey::isEsc(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  // Navigation has its own contextual help page.
  // Do not route H to the recorder/global help screen.
  handleNavKey(key);
}

void handlePlaceholderKey(const String& key) {
  if (FtKey::isEsc(key) || key == "[DEL]" || FtKey::isEnter(key)) {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
  }
}

void handleKey(const String& key) {
  lastKeyText = key;

  if (!isSyncSerialQuietMode()) {
    Serial.print("[KEY] ");
    Serial.println(key);
  }

  if (currentScreen == SCREEN_HOME) handleHomeKey(key);
  else if (currentScreen == SCREEN_RECORDER) handleRecorderKey(key);
  else if (currentScreen == SCREEN_LOG) handleLogKey(key);
  else if (currentScreen == SCREEN_NAV) handleNavKeyWrapper(key);
  else if (currentScreen == SCREEN_SYNC) handleSyncKey(key);
  else if (currentScreen == SCREEN_TASKS) handleTasksKey(key);
  else if (currentScreen == SCREEN_DEVICE) handleDeviceKey(key);
  else if (currentScreen == SCREEN_LORA_PROBE) handleLoRaProbeKey(key);
  else if (currentScreen == SCREEN_HELP) handleHelpKey(key);
  else handlePlaceholderKey(key);
}

// ---------- Setup / loop ----------
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  Serial.begin(115200);
  delay(500);

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(128);
  M5Cardputer.Display.setTextWrap(false);

  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setTextWrap(false);

  baseEpoch = makeEpochFromCompileTime();
  bootMillis = millis();

  updateDeviceStatus();
  prepareSharedSpiBusForSD();

  Serial.println();
  Serial.println("[BOOT] LanternBox Field Terminal");
  Serial.print("[BOOT] Device: ");
  Serial.println(DEVICE_ID);
  Serial.print("[BOOT] Version: ");
  Serial.println(VERSION);
  Serial.println("[BOOT] Nav Session List v0.1.6");

  canvas.fillSprite(BLACK);
  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(8, 8);
  canvas.print("壳中灯启动");
  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 36);
  canvas.print("Init SD/GNSS...");
  canvas.pushSprite(0, 0);

  initSD();
  initGNSS();
  audioLogger.begin();
  syncManager.begin(DEVICE_ID, VERSION);
  uiLoRaProbe.begin(&canvas, &loraManager);
  lastLoRaInitAttemptMs = millis();
  if (loraManager.begin()) {
    Serial.println("LORA_BACKGROUND_BOOT_READY");
    loraManager.sendMeshtasticNodeInfo(true);
    loraManager.startListening();
  } else {
    Serial.println("LORA_BACKGROUND_BOOT_FAIL");
  }
  taskManager.begin(DEVICE_ID);
  taskManager.refresh(sdReady);

  drawHomeScreen();
}


void updateIMUHeading() {
  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  unsigned long now = millis();
  if (lastImuMillis == 0) {
    lastImuMillis = now;
    return;
  }

  double dt = (now - lastImuMillis) / 1000.0;
  lastImuMillis = now;

  // BMI270 gyro z is used for short-term heading assist.
  // It is not an absolute compass and will drift over time.
  imuHeadingDeg += imuData.gyro.z * dt;

  while (imuHeadingDeg < 0) imuHeadingDeg += 360.0;
  while (imuHeadingDeg >= 360) imuHeadingDeg -= 360.0;

  if (fabs(imuData.gyro.z) > 0.5) {
    imuHeadingReady = true;
  }
}

void serviceLoRaBackground() {
  const unsigned long now = millis();

  if (!loraManager.isReady()) {
    if (lastLoRaInitAttemptMs == 0 || now - lastLoRaInitAttemptMs >= LORA_INIT_RETRY_MS) {
      lastLoRaInitAttemptMs = now;
      if (loraManager.begin()) {
        Serial.println("LORA_BACKGROUND_READY");
        loraManager.sendMeshtasticNodeInfo(true);
        loraManager.startListening();
      } else {
        Serial.println("LORA_BACKGROUND_INIT_RETRY");
      }
    }
    return;
  }

  // pollReceive() returns immediately unless the radio IRQ reports a packet.
  // This keeps radio reception alive on every screen without blocking keys.
  const bool received = loraManager.pollReceive();
  if (received && currentScreen != SCREEN_LORA_PROBE) {
    Serial.print("LORA_BACKGROUND_RX unread=");
    Serial.println((int)loraManager.unreadMessageCount());

    // Refresh the home status indicator immediately. Other pages keep their
    // own drawing cadence and the message remains queued for the inbox.
    if (currentScreen == SCREEN_HOME) drawHomeScreen();
  }
}

void loop() {
  M5Cardputer.update();

  readGnssStream();
  logGnssSummaryIfNeeded();
  processSerialSyncCommands();
  audioLogger.update();
  updateIMUHeading();
  autoTrackTick();
  serviceLoRaBackground();

  String key = readKeyText();
  if (key.length() > 0) {
    handleKey(key);
  }

  if (currentScreen == SCREEN_LORA_PROBE) {
    uiLoRaProbe.tick();
  }

  unsigned long now = millis();

  if (now - lastStatusPoll >= 5000) {
    lastStatusPoll = now;
    updateDeviceStatus();
  }

  if (gnssNmeaSeen && now - gnssLastNmeaMillis > 5000) {
    gnssStatusText = "SIGNAL LOST";
  }

  static unsigned long lastRefresh = 0;
  if (now - lastRefresh >= 1000) {
    lastRefresh = now;
    drawCurrentScreen();
  }

  delay(10);
}
