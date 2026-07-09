#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "UiRecorder.h"
#include "UiNavigation.h"
#include "HelpManager.h"
#include "UiLog.h"

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

#define SD_SPI_SCK_PIN   40
#define SD_SPI_MISO_PIN  39
#define SD_SPI_MOSI_PIN  14
#define SD_SPI_CS_PIN    12
#define SD_INIT_FREQ     400000

#define LORA_NSS_PIN     5
#define LORA_RST_PIN     3
#define LORA_IRQ_PIN     4
#define LORA_BUSY_PIN    6

#define GNSS_RX_PIN      15
#define GNSS_TX_PIN      13
#define GNSS_BAUD        115200

#define LOCAL_TIMEZONE_OFFSET_SECONDS 28800
static const char* LOCAL_TIMEZONE_TEXT = "UTC+8";

const char* VERSION = "v0.1.9i";
static const char* DEVICE_ID = "FT-01";

M5Canvas canvas(&M5Cardputer.Display);
HardwareSerial GNSS(1);

m5::imu_data_t imuData;
double imuHeadingDeg = 0.0;
bool imuHeadingReady = false;
unsigned long lastImuMillis = 0;

// ---------- App state ----------
enum AppScreen {
  SCREEN_HOME,
  SCREEN_STATUS,
  SCREEN_RECORDER,
  SCREEN_LOG,
  SCREEN_NAV,
  SCREEN_HELP,
  SCREEN_PLACEHOLDER
};

AppScreen currentScreen = SCREEN_HOME;
AppScreen previousScreen = SCREEN_HOME;

struct MenuItem {
  const char* titleCn;
  const char* titleEn;
};

MenuItem menuItems[] = {
  {"路径", "LOG"},
  {"日志", "LOGS"},
  {"状态", "STAT"},
  {"同步", "SYNC"},
  {"设置", "SET"},
  {"导航", "NAV"},
  {"设备", "DEV"},
  {"关于", "INFO"}
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
bool gnssFix = false;
int gnssSatellites = -1;
double gnssLat = 0.0;
double gnssLon = 0.0;
String gnssUtcTime = "--";
String gnssUtcDate = "--";
String gnssLastSentence = "--";
unsigned long gnssLastNmeaMillis = 0;
unsigned long gnssLastFixMillis = 0;

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
const unsigned long autoTrackIntervalMs = 30000;

String currentSessionId = "";
String lastRecordTime = "--";
bool sessionActive = false;

// ---------- Forward declarations ----------
bool initSD();
bool ensureLanternDirs();
bool writeBootLog();
bool appendLineToFile(const char* path, const String& line);
bool appendFieldEvent(const String& eventType, const String& note);
bool writeStorageTestEvent();
void initGNSS();
void readGnssStream();
void drawCurrentScreen();
void drawHomeScreen();
void drawStatusScreen();
void drawRecorderScreen();
void drawHelpScreen();
void drawHelpManager();
void drawLogScreen();
void handleLogKey(const String& key);
void openHelpPage(HelpType type, AppScreen returnPage);
void updateDeviceStatus();

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
int monthNameToNumber(const char* month) {
  if (strcmp(month, "Jan") == 0) return 1;
  if (strcmp(month, "Feb") == 0) return 2;
  if (strcmp(month, "Mar") == 0) return 3;
  if (strcmp(month, "Apr") == 0) return 4;
  if (strcmp(month, "May") == 0) return 5;
  if (strcmp(month, "Jun") == 0) return 6;
  if (strcmp(month, "Jul") == 0) return 7;
  if (strcmp(month, "Aug") == 0) return 8;
  if (strcmp(month, "Sep") == 0) return 9;
  if (strcmp(month, "Oct") == 0) return 10;
  if (strcmp(month, "Nov") == 0) return 11;
  if (strcmp(month, "Dec") == 0) return 12;
  return 1;
}

bool isLeapYear(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return year % 4 == 0;
}

unsigned long daysBeforeMonth(int year, int month) {
  const int daysInMonth[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
  };

  unsigned long days = 0;
  for (int m = 1; m < month; m++) {
    days += daysInMonth[m - 1];
    if (m == 2 && isLeapYear(year)) days += 1;
  }

  return days;
}

unsigned long makeEpochFromCompileTime() {
  char monthStr[4];
  int day;
  int year;
  int hour;
  int minute;
  int second;

  sscanf(__DATE__, "%3s %d %d", monthStr, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  int month = monthNameToNumber(monthStr);

  unsigned long days = 0;
  for (int y = 1970; y < year; y++) {
    days += isLeapYear(y) ? 366 : 365;
  }

  days += daysBeforeMonth(year, month);
  days += day - 1;

  return days * 86400UL + hour * 3600UL + minute * 60UL + second;
}

unsigned long getCurrentEpoch() {
  long uptimeSeconds = (millis() - bootMillis) / 1000;
  long current = (long)baseEpoch + uptimeSeconds + timeOffsetSeconds;
  if (current < 0) current = 0;
  return (unsigned long)current;
}

void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long secondsInDay = epoch % 86400UL;
  int hour = secondsInDay / 3600;
  int minute = (secondsInDay % 3600) / 60;
  int second = secondsInDay % 60;
  snprintf(buffer, bufferSize, "%02d:%02d:%02d", hour, minute, second);
}

void epochToShortTimeString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long secondsInDay = epoch % 86400UL;
  int hour = secondsInDay / 3600;
  int minute = (secondsInDay % 3600) / 60;
  snprintf(buffer, bufferSize, "%02d:%02d", hour, minute);
}

int daysInMonth(int year, int month) {
  switch (month) {
    case 1: return 31;
    case 2: return isLeapYear(year) ? 29 : 28;
    case 3: return 31;
    case 4: return 30;
    case 5: return 31;
    case 6: return 30;
    case 7: return 31;
    case 8: return 31;
    case 9: return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 30;
  }
}

void epochToDateString(unsigned long epoch, char* buffer, size_t bufferSize) {
  unsigned long days = epoch / 86400UL;
  int year = 1970;

  while (true) {
    int daysInYear = isLeapYear(year) ? 366 : 365;
    if (days < (unsigned long)daysInYear) break;
    days -= daysInYear;
    year++;
  }

  int month = 1;
  while (month <= 12) {
    int dim = daysInMonth(year, month);
    if (days < (unsigned long)dim) break;
    days -= dim;
    month++;
  }

  int day = (int)days + 1;
  snprintf(buffer, bufferSize, "%04d-%02d-%02d", year, month, day);
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

unsigned long makeEpochFromDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
    return 0;
  }

  unsigned long days = 0;
  for (int y = 1970; y < year; y++) {
    days += isLeapYear(y) ? 366 : 365;
  }

  days += daysBeforeMonth(year, month);
  days += day - 1;

  return days * 86400UL + hour * 3600UL + minute * 60UL + second;
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
  pinMode(LORA_NSS_PIN, OUTPUT);
  digitalWrite(LORA_NSS_PIN, HIGH);

  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);

  pinMode(LORA_RST_PIN, OUTPUT);
  digitalWrite(LORA_RST_PIN, HIGH);

  pinMode(LORA_IRQ_PIN, INPUT);
  pinMode(LORA_BUSY_PIN, INPUT);

  delay(20);
  Serial.println("[SPI] shared bus prepared: LoRa NSS high, SD CS high");
}

String formatBytes(uint64_t bytes) {
  double value = (double)bytes;

  if (value >= 1024.0 * 1024.0 * 1024.0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%.2fGB", value / 1024.0 / 1024.0 / 1024.0);
    return String(buf);
  }

  if (value >= 1024.0 * 1024.0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%.2fMB", value / 1024.0 / 1024.0);
    return String(buf);
  }

  if (value >= 1024.0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%.2fKB", value / 1024.0);
    return String(buf);
  }

  return String((unsigned long)bytes) + "B";
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

  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  delay(50);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_INIT_FREQ)) {
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

  sdSizeText = formatBytes(SD.cardSize());

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

  Serial.print("[NMEA] ");
  Serial.println(line);

  if (line.startsWith("$GNGGA") || line.startsWith("$GPGGA") || line.startsWith("$BDGGA")) {
    parseGGA(line);
  } else if (line.startsWith("$GNRMC") || line.startsWith("$GPRMC") || line.startsWith("$BDRMC")) {
    parseRMC(line);
  } else {
    gnssStatusText = "NMEA OK";
  }
}

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

  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
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

  Serial.println("[GNSS] started RX15 TX13 baud=115200");
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
  if (now - lastAutoTrackMillis >= autoTrackIntervalMs) {
    lastAutoTrackMillis = now;
    savePathPoint("auto");
  }
}

// ---------- Device status ----------
void updateDeviceStatus() {
  batteryLevel = M5Cardputer.Power.getBatteryLevel();
  batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
}

String recorderModeText() {
  if (autoTrack) return "AUTO";
  if (sessionActive) return "SESSION";
  return "IDLE";
}

String baseStateText() {
  return baseSet ? "YES" : "NO";
}

String shortSessionText() {
  if (currentSessionId.length() == 0) return "--";
  return currentSessionId;
}

String gnssStateText() {
  if (gnssFix) return "FIX";
  if (gnssNmeaSeen && millis() - gnssLastNmeaMillis > 5000) return "LOST";
  if (gnssNmeaSeen) return "SEARCH";
  return "WAIT";
}

// ---------- Drawing ----------
void drawRowAscii(int y, const char* labelCn, const String& value, uint16_t color = WHITE) {
  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, y);
  canvas.print(labelCn);

  useAsciiFont();
  canvas.setTextColor(color, BLACK);
  canvas.setCursor(66, y + 2);
  canvas.print(value);
}

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

  canvas.drawLine(0, 20, canvas.width(), 20, DARKGREY);
}

void drawTitle() {
  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(8, 24);
  canvas.print("壳中灯");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(78, 30);
  canvas.print(DEVICE_ID);

  canvas.setCursor(148, 30);
  canvas.print(VERSION);
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

void drawStatusScreen() {
  updateDeviceStatus();

  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(8, 4);
  canvas.print("壳中灯 状态");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(144, 10);
  canvas.print(DEVICE_ID);

  canvas.drawLine(0, 25, canvas.width(), 25, DARKGREY);

  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

  String batteryText = batteryLevel >= 0 ? String(batteryLevel) + "%" : "--";

  drawRowAscii(30, "版本", VERSION);
  drawRowAscii(44, "时间", String(timeText));
  drawRowAscii(58, "电量", batteryText, batteryLevel <= 20 ? ORANGE : WHITE);
  drawRowAscii(72, "SD", sdStatusText + " " + sdTypeText, sdReady ? GREEN : ORANGE);
  drawRowAscii(86, "GNSS", gnssFix ? "FIX" : gnssStatusText, gnssFix ? GREEN : ORANGE);
  drawRowAscii(100, "卫星", gnssSatellites >= 0 ? String(gnssSatellites) : "--");
  drawRowAscii(114, "日志", sdLogText, sdLogText == "OK" ? GREEN : ORANGE);

  canvas.pushSprite(0, 0);
}



String displaySessionId(int maxLen) {
  if (currentSessionId.length() == 0) return "--";
  if (currentSessionId.length() <= maxLen) return currentSessionId;
  return currentSessionId.substring(0, maxLen);
}

String autoNextText() {
  if (!autoTrack) return "--";

  unsigned long now = millis();
  unsigned long elapsed = now - lastAutoTrackMillis;

  if (elapsed >= autoTrackIntervalMs) {
    return "00:00";
  }

  unsigned long remain = (autoTrackIntervalMs - elapsed) / 1000;
  char buf[8];
  snprintf(buf, sizeof(buf), "00:%02lu", remain);
  return String(buf);
}

String baseDisplayText() {
  return baseSet ? "SET" : "NO";
}


String displaySessionIdForCard() {
  if (!sessionActive || currentSessionId.length() == 0) {
    return "尚未开启";
  }

  if (currentSessionId.length() <= 12) {
    return currentSessionId;
  }

  return currentSessionId.substring(0, 12);
}

String pointsDisplayText() {
  return String(pathWriteCount);
}

String satelliteDisplayText() {
  if (!gnssFix) return "NO FIX";
  if (gnssSatellites < 0) return "--";
  return String(gnssSatellites);
}

String countdownDisplayText() {
  if (!autoTrack) return "--:--";
  return autoNextText();
}

void drawBatteryIcon(int x, int y, int percent) {
  canvas.drawRoundRect(x, y + 3, 14, 17, 2, WHITE);
  canvas.fillRect(x + 4, y, 6, 3, WHITE);

  int h = 0;
  if (percent >= 0) {
    h = map(percent, 0, 100, 0, 13);
    if (h < 0) h = 0;
    if (h > 13) h = 13;
  }

  if (h > 0) {
    canvas.fillRect(x + 3, y + 17 - h, 8, h, WHITE);
  }
}

void drawSdIconSmall(int x, int y, bool ok) {
  uint16_t c = ok ? GREEN : DARKGREY;

  canvas.fillRoundRect(x + 2, y + 2, 16, 20, 2, c);
  canvas.fillTriangle(x + 2, y + 2, x + 8, y + 2, x + 2, y + 8, BLACK);
  canvas.fillRect(x + 8, y + 5, 2, 5, BLACK);
  canvas.fillRect(x + 12, y + 5, 2, 5, BLACK);
  canvas.fillRect(x + 16, y + 5, 2, 5, BLACK);
}

void drawAutoIconSmall(int x, int y, bool on) {
  uint16_t c = on ? GREEN : DARKGREY;

  canvas.drawRoundRect(x + 2, y + 3, 18, 20, 2, c);
  canvas.fillRect(x, y + 7, 4, 3, c);
  canvas.fillRect(x, y + 14, 4, 3, c);
  canvas.drawLine(x + 8, y + 9, x + 17, y + 9, c);
  canvas.drawLine(x + 8, y + 15, x + 17, y + 15, c);
}

void drawTagIcon(int x, int y) {
  uint16_t c = DARKGREY;
  canvas.fillRoundRect(x + 6, y + 4, 19, 14, 3, c);
  canvas.fillTriangle(x + 6, y + 4, x + 0, y + 12, x + 6, y + 18, c);
  canvas.fillCircle(x + 6, y + 13, 2, WHITE);
}

void drawNavIcon(int x, int y) {
  uint16_t c = DARKGREY;
  canvas.fillTriangle(x + 10, y + 2, x + 25, y + 27, x + 4, y + 18, c);
}

void drawStopwatchIcon(int x, int y) {
  uint16_t c = DARKGREY;
  canvas.fillCircle(x + 14, y + 16, 12, c);
  canvas.fillRoundRect(x + 10, y + 1, 8, 3, 1, c);
  canvas.drawLine(x + 14, y + 16, x + 14, y + 8, WHITE);
  canvas.fillCircle(x + 14, y + 16, 2, WHITE);
}

void drawSatelliteIcon(int x, int y) {
  uint16_t c = DARKGREY;
  canvas.fillCircle(x + 19, y + 6, 4, c);
  canvas.fillRect(x + 8, y + 10, 8, 8, c);
  canvas.drawLine(x + 8, y + 10, x + 3, y + 5, c);
  canvas.drawLine(x + 16, y + 18, x + 22, y + 24, c);
  canvas.drawCircle(x + 8, y + 20, 10, c);
  canvas.fillRect(x + 8, y + 10, 18, 20, WHITE);
}

void drawPinIcon(int x, int y) {
  canvas.fillCircle(x + 8, y + 7, 8, WHITE);
  canvas.fillTriangle(x, y + 10, x + 16, y + 10, x + 8, y + 24, WHITE);
  canvas.fillCircle(x + 8, y + 7, 3, BLACK);
}

void drawClockIcon(int x, int y) {
  canvas.drawRoundRect(x, y, 22, 22, 4, WHITE);
  canvas.drawCircle(x + 11, y + 11, 8, WHITE);
  canvas.drawLine(x + 11, y + 11, x + 11, y + 6, WHITE);
  canvas.drawLine(x + 11, y + 11, x + 16, y + 11, WHITE);
}

void drawInfoCard(int x, int y, int w, int h, const String& title, const String& value, int iconType, bool valueGreen = false) {
  canvas.fillRect(x, y, w, h, WHITE);

  if (iconType == 1) drawTagIcon(x + 8, y + 5);
  else if (iconType == 2) drawNavIcon(x + 9, y + 3);
  else if (iconType == 3) drawStopwatchIcon(x + 8, y + 3);
  else if (iconType == 4) drawSatelliteIcon(x + 8, y + 3);

  if (title == "SESSION" || title == "POINTS") {
    useAsciiFont();
    canvas.setTextColor(BLACK, WHITE);
    canvas.setCursor(x + 39, y + 8);
    canvas.print(title);
  } else {
    useChineseFont12();
    canvas.setTextColor(BLACK, WHITE);
    canvas.setCursor(x + 39, y + 7);
    canvas.print(title);
  }

  if (title == "SESSION") {
    useChineseFont12();
    canvas.setTextColor(valueGreen ? GREEN : BLACK, WHITE);
    canvas.setCursor(x + 39, y + 22);
    canvas.print(value);
  } else {
    useAsciiFont();
    canvas.setTextColor(valueGreen ? GREEN : BLACK, WHITE);
    canvas.setCursor(x + 47, y + 23);
    canvas.print(value);
  }
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
  else if (currentScreen == SCREEN_STATUS) drawStatusScreen();
  else if (currentScreen == SCREEN_RECORDER) drawRecorderScreen();
  else if (currentScreen == SCREEN_LOG) drawLogScreen();
  else if (currentScreen == SCREEN_NAV) drawNavScreen();
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

bool keyHasLetter(const String& key, char lower, char upper) {
  return key.indexOf(lower) >= 0 || key.indexOf(upper) >= 0;
}

bool isEscKey(const String& key) {
  return key == "`" || key == "~" || key == "[ESC]" || key == "ESC";
}

bool isLeftKey(const String& key) {
  return key == "," || key == "[LEFT]" || key == "LEFT";
}

bool isRightKey(const String& key) {
  return key == "/" || key == "[RIGHT]" || key == "RIGHT";
}

bool isUpKey(const String& key) {
  return key == ";" || key == "[UP]" || key == "UP";
}

bool isDownKey(const String& key) {
  return key == "." || key == "[DOWN]" || key == "DOWN";
}

bool isEnterKey(const String& key) {
  return key == "[ENTER]" || key == "OK";
}

void confirmSelection() {
  lastAction = menuItems[selectedIndex].titleEn;

  Serial.print("[ACTION] Enter ");
  Serial.print(menuItems[selectedIndex].titleCn);
  Serial.print(" / ");
  Serial.println(menuItems[selectedIndex].titleEn);

  if (strcmp(menuItems[selectedIndex].titleEn, "LOG") == 0) {
    currentScreen = SCREEN_RECORDER;
    drawRecorderScreen();
    return;
  }

  if (strcmp(menuItems[selectedIndex].titleEn, "STAT") == 0) {
    currentScreen = SCREEN_STATUS;
    drawStatusScreen();
    return;
  }

  if (strcmp(menuItems[selectedIndex].titleEn, "LOGS") == 0) {
    currentScreen = SCREEN_LOG;
    drawLogScreen();
    return;
  }

  if (strcmp(menuItems[selectedIndex].titleEn, "NAV") == 0) {
    currentScreen = SCREEN_NAV;
    navReloadTrackData();
    drawNavScreen();
    return;
  }

  placeholderTitle = menuItems[selectedIndex].titleCn;
  currentScreen = SCREEN_PLACEHOLDER;
  drawPlaceholderScreen();
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
  else if (isLeftKey(key)) moveLeft();
  else if (isRightKey(key)) moveRight();
  else if (isUpKey(key)) moveUp();
  else if (isDownKey(key)) moveDown();
  else if (isEnterKey(key)) {
    confirmSelection();
    return;
  } else if (keyHasLetter(key, 'r', 'R')) {
    initSD();
    initGNSS();
  }

  if (selectedIndex >= menuCount) selectedIndex = menuCount - 1;
  ensureSelectedVisible();
  drawHomeScreen();
}

void handleStatusKey(const String& key) {
  if (isEscKey(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  if (isLeftKey(key)) timeOffsetSeconds -= 60;
  else if (isRightKey(key)) timeOffsetSeconds += 60;
  else if (isUpKey(key)) timeOffsetSeconds += 10;
  else if (isDownKey(key)) timeOffsetSeconds -= 10;
  else if (keyHasLetter(key, 'r', 'R')) {
    initSD();
    initGNSS();
  }

  Serial.print("[TIME] offset seconds=");
  Serial.println(timeOffsetSeconds);

  drawStatusScreen();
}

void handleRecorderKey(const String& key) {
  if (isEscKey(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  if (keyHasLetter(key, 'h', 'H')) {
    openHelpPage(HELP_RECORDER, SCREEN_RECORDER);
    return;
  }

  bool guardedAction =
    keyHasLetter(key, 'a', 'A') ||
    keyHasLetter(key, 'b', 'B') ||
    keyHasLetter(key, 'p', 'P') ||
    keyHasLetter(key, 's', 'S') ||
    isEnterKey(key) ||
    key == "[SPACE]";

  if (!gnssFix && guardedAction) {
    lastAction = "NO FIX";
    lastWriteStatus = "GNSS REQUIRED";
    Serial.println("[GUARD] recorder action blocked: GNSS FIX required");
    drawRecorderScreen();
    return;
  }

  if (keyHasLetter(key, 's', 'S')) {
    stopSession();
    lastAction = "STOP";
    lastWriteStatus = "SESSION STOP";
    drawRecorderScreen();
    return;
  }

  if (keyHasLetter(key, 'b', 'B')) {
    saveBasePosition();
  } else if (keyHasLetter(key, 'p', 'P') || isEnterKey(key)) {
    savePathPoint("manual");
  } else if (keyHasLetter(key, 'a', 'A') || key == "[SPACE]") {
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
  } else if (keyHasLetter(key, 'r', 'R')) {
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

  if (isEscKey(key) || key == "[DEL]" || isEnterKey(key)) {
    currentScreen = previousScreen;
    drawCurrentScreen();
  }
}

void handleNavKeyWrapper(const String& key) {
  if (isEscKey(key) || key == "[DEL]") {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }

  // Navigation has its own contextual help page.
  // Do not route H to the recorder/global help screen.
  handleNavKey(key);
}

void handlePlaceholderKey(const String& key) {
  if (isEscKey(key) || key == "[DEL]" || isEnterKey(key)) {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
  }
}

void handleKey(const String& key) {
  lastKeyText = key;

  Serial.print("[KEY] ");
  Serial.println(key);

  if (currentScreen == SCREEN_HOME) handleHomeKey(key);
  else if (currentScreen == SCREEN_STATUS) handleStatusKey(key);
  else if (currentScreen == SCREEN_RECORDER) handleRecorderKey(key);
  else if (currentScreen == SCREEN_LOG) handleLogKey(key);
  else if (currentScreen == SCREEN_NAV) handleNavKeyWrapper(key);
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

void loop() {
  M5Cardputer.update();

  readGnssStream();
  updateIMUHeading();
  autoTrackTick();

  String key = readKeyText();
  if (key.length() > 0) {
    handleKey(key);
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
