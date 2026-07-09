#include "UiNavigation.h"
#include "HelpManager.h"
#include <math.h>

// ---------- Shared state from LanternBox_FT01.ino ----------
extern M5Canvas canvas;

extern bool sdReady;
extern bool gnssFix;
extern int gnssSatellites;
extern double gnssLat;
extern double gnssLon;
extern String currentSessionId;

extern double imuHeadingDeg;
extern bool imuHeadingReady;

extern void useChineseFont12();
extern void useChineseFont16();
extern void useAsciiFont();

// ---------- Local constants ----------
static const int MAX_NAV_POINTS = 40;
static const int MAX_NAV_SESSIONS = 12;
static const double EARTH_RADIUS_M = 6371000.0;
static const uint16_t NAV_BLUE = 0x001F;
static const uint16_t NAV_CYAN = 0x07FF;
static const uint16_t NAV_RED = 0xF800;

struct NavPoint {
  bool valid;
  int seq;
  double lat;
  double lon;
  int satellites;
  String mode;
  String date;
  String time;
  String session;
};

struct NavSession {
  bool valid;
  String id;
  String lastDate;
  String lastTime;
  int count;
  int lastOrder;
};

enum NavPage {
  NAV_PAGE_SESSIONS,
  NAV_PAGE_OVERVIEW,
  NAV_PAGE_MAP,
  NAV_PAGE_COMPASS,
  NAV_PAGE_HELP
};

enum NavTargetType {
  TARGET_NONE,
  TARGET_BASE,
  TARGET_POINT
};

static NavPoint navPoints[MAX_NAV_POINTS];
static NavSession navSessions[MAX_NAV_SESSIONS];
static int navPointCount = 0;
static int navSessionCount = 0;
static int navSelectedIndex = 0;
static int navSelectedSessionIndex = 0;
static bool navLoaded = false;
static String navStatus = "NOT LOADED";
static String selectedSessionId = "";

static bool baseLoaded = false;
static double baseLat = 0.0;
static double baseLon = 0.0;

static NavPage navPage = NAV_PAGE_SESSIONS;
static NavPage navPreviousPage = NAV_PAGE_SESSIONS;
static NavTargetType navTargetType = TARGET_NONE;
static int navTargetIndex = -1;

// GNSS movement heading.
// This is not a true compass. It uses movement between two valid GNSS positions.
// When the user walks several meters, we can infer the user's course over ground.
static bool moveHeadingReady = false;
static double moveHeadingDeg = 0.0;
static double moveAnchorLat = 0.0;
static double moveAnchorLon = 0.0;
static double moveLastStepMeters = 0.0;
static const double MOVE_HEADING_LOCK_METERS = 5.0;
static const double MAP_POINT_DEDUP_METERS = 2.0;

// ---------- Small parsers ----------
String extractStringField(const String& line, const String& key) {
  String pattern = "\"" + key + "\":\"";
  int start = line.indexOf(pattern);
  if (start < 0) return "";

  start += pattern.length();
  int end = line.indexOf("\"", start);
  if (end < 0) return "";

  return line.substring(start, end);
}

double extractDoubleField(const String& line, const String& key, double fallback = 0.0) {
  String pattern = "\"" + key + "\":";
  int start = line.indexOf(pattern);
  if (start < 0) return fallback;

  start += pattern.length();
  int end = line.indexOf(",", start);
  if (end < 0) end = line.indexOf("}", start);
  if (end < 0) return fallback;

  return line.substring(start, end).toDouble();
}

int extractIntField(const String& line, const String& key, int fallback = 0) {
  String pattern = "\"" + key + "\":";
  int start = line.indexOf(pattern);
  if (start < 0) return fallback;

  start += pattern.length();
  int end = line.indexOf(",", start);
  if (end < 0) end = line.indexOf("}", start);
  if (end < 0) return fallback;

  return line.substring(start, end).toInt();
}

String normalizedSessionId(const String& raw) {
  if (raw.length() == 0) return "NO_SESSION";
  return raw;
}

String dateFromGnssUtcDate(const String& raw) {
  // GNSS RMC date is DDMMYY. This is UTC date, used only as fallback for old records.
  if (raw.length() < 6) return "--";
  String day = raw.substring(0, 2);
  String month = raw.substring(2, 4);
  String year = raw.substring(4, 6);
  return "20" + year + "-" + month + "-" + day;
}

String pointDateFromLine(const String& line) {
  String localDate = extractStringField(line, "device_date");
  if (localDate.length() > 0) return localDate;

  String utcDate = extractStringField(line, "gnss_utc_date");
  if (utcDate.length() > 0) return dateFromGnssUtcDate(utcDate);

  return "--";
}

String shortText(const String& value, int maxLen) {
  if (value.length() <= maxLen) return value;
  return value.substring(0, maxLen);
}

bool parsePathPointLine(const String& line, NavPoint& point) {
  if (line.indexOf("\"type\":\"path_point\"") < 0) return false;

  double lat = extractDoubleField(line, "lat", 0.0);
  double lon = extractDoubleField(line, "lon", 0.0);

  if (lat == 0.0 && lon == 0.0) return false;

  point.valid = true;
  point.seq = extractIntField(line, "seq", 0);
  point.lat = lat;
  point.lon = lon;
  point.satellites = extractIntField(line, "satellites", -1);
  point.mode = extractStringField(line, "mode");
  point.date = pointDateFromLine(line);
  point.time = extractStringField(line, "device_time");
  point.session = normalizedSessionId(extractStringField(line, "session_id"));

  return true;
}

void resetNavPoints() {
  navPointCount = 0;
  navSelectedIndex = 0;
  for (int i = 0; i < MAX_NAV_POINTS; i++) {
    navPoints[i].valid = false;
  }
}

void pushNavPoint(const NavPoint& p) {
  if (navPointCount < MAX_NAV_POINTS) {
    navPoints[navPointCount++] = p;
    return;
  }

  for (int i = 1; i < MAX_NAV_POINTS; i++) {
    navPoints[i - 1] = navPoints[i];
  }

  navPoints[MAX_NAV_POINTS - 1] = p;
}

void resetSessions() {
  navSessionCount = 0;
  navSelectedSessionIndex = 0;
  selectedSessionId = "";

  for (int i = 0; i < MAX_NAV_SESSIONS; i++) {
    navSessions[i].valid = false;
    navSessions[i].id = "";
    navSessions[i].lastTime = "--";
    navSessions[i].count = 0;
    navSessions[i].lastOrder = -1;
  }
}

int findSessionIndex(const String& sid) {
  for (int i = 0; i < navSessionCount; i++) {
    if (navSessions[i].valid && navSessions[i].id == sid) return i;
  }
  return -1;
}

int findOldestSessionIndex() {
  if (navSessionCount <= 0) return -1;

  int oldest = 0;
  for (int i = 1; i < navSessionCount; i++) {
    if (navSessions[i].lastOrder < navSessions[oldest].lastOrder) {
      oldest = i;
    }
  }
  return oldest;
}

void upsertSession(const NavPoint& p, int order) {
  String sid = normalizedSessionId(p.session);
  int idx = findSessionIndex(sid);

  if (idx < 0) {
    if (navSessionCount < MAX_NAV_SESSIONS) {
      idx = navSessionCount++;
    } else {
      idx = findOldestSessionIndex();
    }

    if (idx < 0) return;

    navSessions[idx].valid = true;
    navSessions[idx].id = sid;
    navSessions[idx].lastDate = "--";
    navSessions[idx].lastTime = "--";
    navSessions[idx].count = 0;
  }

  navSessions[idx].count++;
  navSessions[idx].lastDate = p.date.length() > 0 ? p.date : "--";
  navSessions[idx].lastTime = p.time.length() > 0 ? p.time : "--";
  navSessions[idx].lastOrder = order;
}

void sortSessionsByUpdateDesc() {
  for (int i = 0; i < navSessionCount - 1; i++) {
    for (int j = i + 1; j < navSessionCount; j++) {
      if (navSessions[j].lastOrder > navSessions[i].lastOrder) {
        NavSession tmp = navSessions[i];
        navSessions[i] = navSessions[j];
        navSessions[j] = tmp;
      }
    }
  }
}

void loadBasePoint() {
  baseLoaded = false;

  if (!sdReady) return;
  if (!SD.exists("/lanternbox/base.json")) return;

  File file = SD.open("/lanternbox/base.json", FILE_READ);
  if (!file) return;

  String content = "";
  while (file.available()) {
    char c = file.read();
    if (content.length() < 512) content += c;
  }
  file.close();

  double lat = extractDoubleField(content, "lat", 0.0);
  double lon = extractDoubleField(content, "lon", 0.0);

  if (!(lat == 0.0 && lon == 0.0)) {
    baseLoaded = true;
    baseLat = lat;
    baseLon = lon;
  }
}

void scanSessionsFromTrackFile() {
  resetSessions();

  if (!sdReady) {
    navStatus = "NO SD";
    return;
  }

  if (!SD.exists("/lanternbox/tracks/path_points.jsonl")) {
    navStatus = "NO TRACK";
    return;
  }

  File file = SD.open("/lanternbox/tracks/path_points.jsonl", FILE_READ);
  if (!file) {
    navStatus = "OPEN FAIL";
    return;
  }

  String line = "";
  int order = 0;

  while (file.available()) {
    char c = file.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (line.length() > 0) {
        NavPoint p;
        p.valid = false;
        if (parsePathPointLine(line, p)) {
          upsertSession(p, order++);
        }
        line = "";
      }
      continue;
    }

    if (line.length() < 512) line += c;
    else line = "";
  }

  if (line.length() > 0) {
    NavPoint p;
    p.valid = false;
    if (parsePathPointLine(line, p)) {
      upsertSession(p, order++);
    }
  }

  file.close();
  sortSessionsByUpdateDesc();

  navStatus = navSessionCount > 0 ? "OK" : "EMPTY";
  if (navSessionCount > 0) {
    navSelectedSessionIndex = 0;
    selectedSessionId = navSessions[0].id;
  }
}

void loadPointsForSelectedSession() {
  resetNavPoints();

  if (!sdReady || selectedSessionId.length() == 0) return;
  if (!SD.exists("/lanternbox/tracks/path_points.jsonl")) return;

  File file = SD.open("/lanternbox/tracks/path_points.jsonl", FILE_READ);
  if (!file) {
    navStatus = "OPEN FAIL";
    return;
  }

  String line = "";

  while (file.available()) {
    char c = file.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (line.length() > 0) {
        NavPoint p;
        p.valid = false;
        if (parsePathPointLine(line, p) && p.session == selectedSessionId) {
          pushNavPoint(p);
        }
        line = "";
      }
      continue;
    }

    if (line.length() < 512) line += c;
    else line = "";
  }

  if (line.length() > 0) {
    NavPoint p;
    p.valid = false;
    if (parsePathPointLine(line, p) && p.session == selectedSessionId) {
      pushNavPoint(p);
    }
  }

  file.close();

  if (navPointCount > 0) {
    navSelectedIndex = navPointCount - 1;
    navTargetType = TARGET_POINT;
    navTargetIndex = navSelectedIndex;
  } else if (baseLoaded) {
    navTargetType = TARGET_BASE;
    navTargetIndex = -1;
  } else {
    navTargetType = TARGET_NONE;
    navTargetIndex = -1;
  }
}

void openSelectedSession() {
  if (navSessionCount <= 0) return;

  if (navSelectedSessionIndex < 0) navSelectedSessionIndex = 0;
  if (navSelectedSessionIndex >= navSessionCount) navSelectedSessionIndex = navSessionCount - 1;

  selectedSessionId = navSessions[navSelectedSessionIndex].id;
  loadPointsForSelectedSession();
  navPage = NAV_PAGE_OVERVIEW;
}

void navReloadTrackData() {
  navLoaded = false;
  navStatus = "LOAD";
  navPage = NAV_PAGE_SESSIONS;
  moveHeadingReady = false;
  moveAnchorLat = 0.0;
  moveAnchorLon = 0.0;
  moveLastStepMeters = 0.0;
  navTargetType = TARGET_NONE;
  navTargetIndex = -1;

  loadBasePoint();
  scanSessionsFromTrackFile();

  navLoaded = true;
}

// ---------- Navigation math ----------
double degToRad(double deg) {
  return deg * PI / 180.0;
}

double radToDeg(double rad) {
  return rad * 180.0 / PI;
}

double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  double p1 = degToRad(lat1);
  double p2 = degToRad(lat2);
  double dp = degToRad(lat2 - lat1);
  double dl = degToRad(lon2 - lon1);

  double a = sin(dp / 2.0) * sin(dp / 2.0) +
             cos(p1) * cos(p2) *
             sin(dl / 2.0) * sin(dl / 2.0);

  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return EARTH_RADIUS_M * c;
}

double bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  double p1 = degToRad(lat1);
  double p2 = degToRad(lat2);
  double dl = degToRad(lon2 - lon1);

  double y = sin(dl) * cos(p2);
  double x = cos(p1) * sin(p2) -
             sin(p1) * cos(p2) * cos(dl);

  double brng = radToDeg(atan2(y, x));
  while (brng < 0) brng += 360.0;
  while (brng >= 360.0) brng -= 360.0;
  return brng;
}

String directionText(double bearing) {
  const char* dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int idx = (int)((bearing + 22.5) / 45.0) % 8;
  return String(dirs[idx]);
}

double normalizeAngle360(double value) {
  while (value < 0.0) value += 360.0;
  while (value >= 360.0) value -= 360.0;
  return value;
}

double signedAngleDelta(double fromDeg, double toDeg) {
  double delta = normalizeAngle360(toDeg - fromDeg);
  if (delta > 180.0) delta -= 360.0;
  return delta;
}

String relativeDirectionCn(double delta) {
  double a = fabs(delta);

  if (a <= 22.5) return "前方";
  if (a <= 67.5) return delta > 0 ? "右前" : "左前";
  if (a <= 112.5) return delta > 0 ? "右侧" : "左侧";
  if (a <= 157.5) return delta > 0 ? "右后" : "左后";
  return "后方";
}

void updateMoveHeadingFromGnss() {
  if (!gnssFix) {
    return;
  }

  if (moveAnchorLat == 0.0 && moveAnchorLon == 0.0) {
    moveAnchorLat = gnssLat;
    moveAnchorLon = gnssLon;
    moveLastStepMeters = 0.0;
    return;
  }

  double step = distanceMeters(moveAnchorLat, moveAnchorLon, gnssLat, gnssLon);
  moveLastStepMeters = step;

  if (step >= MOVE_HEADING_LOCK_METERS) {
    moveHeadingDeg = bearingDegrees(moveAnchorLat, moveAnchorLon, gnssLat, gnssLon);
    moveHeadingReady = true;
    moveAnchorLat = gnssLat;
    moveAnchorLon = gnssLon;
    moveLastStepMeters = 0.0;
  }
}

String formatDistance(double m) {
  if (m >= 1000.0) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fkm", m / 1000.0);
    return String(buf);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%.0fm", m);
  return String(buf);
}

bool getTarget(double& lat, double& lon, String& label) {
  if (navTargetType == TARGET_BASE && baseLoaded) {
    lat = baseLat;
    lon = baseLon;
    label = "BASE";
    return true;
  }

  if (navTargetType == TARGET_POINT &&
      navTargetIndex >= 0 &&
      navTargetIndex < navPointCount &&
      navPoints[navTargetIndex].valid) {
    lat = navPoints[navTargetIndex].lat;
    lon = navPoints[navTargetIndex].lon;
    label = "P";
    label += navPoints[navTargetIndex].seq;
    return true;
  }

  return false;
}

// ---------- Drawing helpers ----------
void navHeader(const String& title) {
  canvas.fillRect(0, 0, canvas.width(), 20, BLACK);

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(7, 4);
  canvas.print(title);

  useAsciiFont();
  canvas.setTextColor(gnssFix ? GREEN : ORANGE, BLACK);
  canvas.setCursor(144, 5);
  canvas.print(gnssFix ? "FIX" : "NOFIX");

  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(190, 5);
  canvas.print("S:");
  canvas.print(gnssSatellites >= 0 ? String(gnssSatellites) : "--");

  canvas.drawLine(0, 20, canvas.width(), 20, WHITE);
}

void navFooter(const String& hint) {
  canvas.drawLine(0, 122, canvas.width(), 122, WHITE);
  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(4, 124);
  canvas.print(hint);
}

void drawSessionList() {
  navHeader("选择轨迹");

  if (!sdReady) {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(52, 58);
    canvas.print("SD不可用");
    navFooter("R刷新  ESC返回");
    return;
  }

  if (navSessionCount <= 0) {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(42, 58);
    canvas.print("暂无轨迹记录");
    navFooter("R刷新  ESC返回");
    return;
  }

  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 25);
  canvas.print("更新时间倒序");

  int maxRows = 4;
  int start = 0;
  if (navSelectedSessionIndex >= maxRows) start = navSelectedSessionIndex - maxRows + 1;

  for (int row = 0; row < maxRows; row++) {
    int idx = start + row;
    if (idx >= navSessionCount) break;

    int y = 42 + row * 19;
    bool selected = idx == navSelectedSessionIndex;

    if (selected) canvas.fillRoundRect(6, y - 2, 228, 17, 3, 0x4208);

    useAsciiFont();
    canvas.setTextColor(selected ? GREEN : WHITE, selected ? 0x4208 : BLACK);
    canvas.setCursor(10, y);
    canvas.print(selected ? ">" : " ");
    canvas.print(idx + 1);

    canvas.setCursor(28, y);
    canvas.print(shortText(navSessions[idx].id, 10));

    canvas.setCursor(108, y);
    canvas.print(shortText(navSessions[idx].lastDate, 10));

    canvas.setCursor(184, y);
    canvas.print(navSessions[idx].lastTime.length() >= 5 ? navSessions[idx].lastTime.substring(0, 5) : navSessions[idx].lastTime);

    canvas.setCursor(220, y);
    canvas.print(navSessions[idx].count);
  }

  navFooter("<>选轨迹  Enter打开");
}

void drawOverview() {
  navHeader("轨迹总览");

  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 26);
  canvas.print("当前会话");

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 42);
  canvas.print(shortText(selectedSessionId, 18));

  canvas.setCursor(8, 60);
  canvas.print("POINTS ");
  canvas.print(navPointCount);

  canvas.setCursor(8, 76);
  canvas.print("BASE   ");
  canvas.print(baseLoaded ? "YES" : "NO");

  canvas.setCursor(8, 92);
  canvas.print("TARGET ");
  double tLat, tLon;
  String label;
  if (getTarget(tLat, tLon, label)) canvas.print(label);
  else canvas.print("--");

  if (navPointCount > 0) {
    NavPoint& p = navPoints[navSelectedIndex];
    canvas.setCursor(124, 42);
    canvas.print("SEL ");
    canvas.print(navSelectedIndex + 1);
    canvas.print("/");
    canvas.print(navPointCount);

    canvas.setCursor(124, 60);
    canvas.print("SEQ ");
    canvas.print(p.seq);

    canvas.setCursor(124, 76);
    canvas.print(shortText(p.date, 10));

    canvas.setCursor(124, 92);
    canvas.print(p.time);
  }

  navFooter("L列表  M地图  N指南  B基地");
}

void drawMapPoint(double lat, double lon, double minLat, double maxLat, double minLon, double maxLon, int x, int y, int w, int h, uint16_t color, int r = 2) {
  double latRange = maxLat - minLat;
  double lonRange = maxLon - minLon;

  if (latRange == 0) latRange = 0.00001;
  if (lonRange == 0) lonRange = 0.00001;

  int px = x + (int)((lon - minLon) / lonRange * (w - 1));
  int py = y + h - 1 - (int)((lat - minLat) / latRange * (h - 1));

  if (px < x) px = x;
  if (px >= x + w) px = x + w - 1;
  if (py < y) py = y;
  if (py >= y + h) py = y + h - 1;

  canvas.fillCircle(px, py, r, color);
}

void drawMapRingPoint(double lat, double lon, double minLat, double maxLat, double minLon, double maxLon, int x, int y, int w, int h, uint16_t color, int r = 4) {
  double latRange = maxLat - minLat;
  double lonRange = maxLon - minLon;

  if (latRange == 0) latRange = 0.00001;
  if (lonRange == 0) lonRange = 0.00001;

  int px = x + (int)((lon - minLon) / lonRange * (w - 1));
  int py = y + h - 1 - (int)((lat - minLat) / latRange * (h - 1));

  if (px < x) px = x;
  if (px >= x + w) px = x + w - 1;
  if (py < y) py = y;
  if (py >= y + h) py = y + h - 1;

  canvas.drawCircle(px, py, r, color);
  canvas.drawCircle(px, py, r - 1, color);
}

bool shouldDrawPathPoint(int index, int lastDrawnIndex) {
  if (index < 0 || index >= navPointCount) return false;
  if (lastDrawnIndex < 0 || lastDrawnIndex >= navPointCount) return true;

  double d = distanceMeters(
    navPoints[lastDrawnIndex].lat,
    navPoints[lastDrawnIndex].lon,
    navPoints[index].lat,
    navPoints[index].lon
  );

  return d > MAP_POINT_DEDUP_METERS;
}

String formatCoord(double value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%.5f", value);
  return String(buf);
}

void drawRelativeMap() {
  navHeader("相对位置");

  int x = 8;
  int y = 27;
  int w = 150;
  int h = 88;

  canvas.drawRect(x, y, w, h, DARKGREY);

  if (navPointCount == 0 && !baseLoaded) {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(36, 60);
    canvas.print("暂无轨迹");
    navFooter("L列表  R刷新  ESC返回");
    return;
  }

  double minLat = 999.0, maxLat = -999.0, minLon = 999.0, maxLon = -999.0;

  for (int i = 0; i < navPointCount; i++) {
    if (!navPoints[i].valid) continue;
    if (navPoints[i].lat < minLat) minLat = navPoints[i].lat;
    if (navPoints[i].lat > maxLat) maxLat = navPoints[i].lat;
    if (navPoints[i].lon < minLon) minLon = navPoints[i].lon;
    if (navPoints[i].lon > maxLon) maxLon = navPoints[i].lon;
  }

  if (baseLoaded) {
    if (baseLat < minLat) minLat = baseLat;
    if (baseLat > maxLat) maxLat = baseLat;
    if (baseLon < minLon) minLon = baseLon;
    if (baseLon > maxLon) maxLon = baseLon;
  }

  if (gnssFix) {
    if (gnssLat < minLat) minLat = gnssLat;
    if (gnssLat > maxLat) maxLat = gnssLat;
    if (gnssLon < minLon) minLon = gnssLon;
    if (gnssLon > maxLon) maxLon = gnssLon;
  }

  double padLat = max((maxLat - minLat) * 0.12, 0.00005);
  double padLon = max((maxLon - minLon) * 0.12, 0.00005);
  minLat -= padLat;
  maxLat += padLat;
  minLon -= padLon;
  maxLon += padLon;

  int lastDrawnPathIndex = -1;
  for (int i = 0; i < navPointCount; i++) {
    if (!shouldDrawPathPoint(i, lastDrawnPathIndex)) continue;

    drawMapPoint(navPoints[i].lat, navPoints[i].lon, minLat, maxLat, minLon, maxLon, x, y, w, h, LIGHTGREY, 1);
    lastDrawnPathIndex = i;
  }

  if (navPointCount > 0) {
    drawMapPoint(navPoints[0].lat, navPoints[0].lon, minLat, maxLat, minLon, maxLon, x, y, w, h, GREEN, 3);
    drawMapPoint(navPoints[navPointCount - 1].lat, navPoints[navPointCount - 1].lon, minLat, maxLat, minLon, maxLon, x, y, w, h, ORANGE, 3);
  }

  // Reference points are rings:
  // - base: blue ring
  // - current position: red ring
  // Target is a solid white point to make "where to go" more obvious.
  if (baseLoaded) drawMapRingPoint(baseLat, baseLon, minLat, maxLat, minLon, maxLon, x, y, w, h, NAV_CYAN, 5);

  if (gnssFix) drawMapRingPoint(gnssLat, gnssLon, minLat, maxLat, minLon, maxLon, x, y, w, h, NAV_RED, 4);

  if (navPointCount > 0) {
    drawMapPoint(navPoints[navSelectedIndex].lat, navPoints[navSelectedIndex].lon, minLat, maxLat, minLon, maxLon, x, y, w, h, WHITE, 4);
  }

  if (navPointCount > 0) {
    NavPoint& sp = navPoints[navSelectedIndex];

    useChineseFont12();
    canvas.setTextColor(DARKGREY, BLACK);
    canvas.setCursor(166, 28);
    canvas.print("目标点");

    useAsciiFont();
    canvas.setTextColor(WHITE, BLACK);
    canvas.setCursor(166, 45);
    canvas.print("P");
    canvas.print(sp.seq);

    canvas.setCursor(166, 59);
    canvas.print(sp.time.length() >= 5 ? sp.time.substring(0, 5) : sp.time);

    canvas.setTextColor(LIGHTGREY, BLACK);
    canvas.setCursor(166, 78);
    canvas.print(formatCoord(sp.lat));

    canvas.setCursor(166, 92);
    canvas.print(formatCoord(sp.lon));
  }

  navFooter("<>选点 B基地 N导航 H说明");
}

void drawCompassArrow(int cx, int cy, int radius, double bearing) {
  canvas.drawCircle(cx, cy, radius, DARKGREY);
  canvas.drawCircle(cx, cy, radius - 10, DARKGREY);

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(cx - 3, cy - radius - 12);
  canvas.print("N");
  canvas.setCursor(cx + radius + 5, cy - 3);
  canvas.print("E");
  canvas.setCursor(cx - 3, cy + radius + 4);
  canvas.print("S");
  canvas.setCursor(cx - radius - 12, cy - 3);
  canvas.print("W");

  double rad = degToRad(bearing);
  int tx = cx + (int)(sin(rad) * (radius - 6));
  int ty = cy - (int)(cos(rad) * (radius - 6));

  canvas.drawLine(cx, cy, tx, ty, GREEN);
  canvas.drawLine(cx + 1, cy, tx, ty, GREEN);
  canvas.fillCircle(tx, ty, 4, GREEN);
  canvas.fillCircle(cx, cy, 3, WHITE);
}

void drawCompassNav() {
  navHeader("指南针导航");

  double tLat, tLon;
  String label;

  if (!getTarget(tLat, tLon, label)) {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(52, 58);
    canvas.print("未选择目标");
    navFooter("<>选点  B基地  L列表  M地图  H说明");
    return;
  }

  if (!gnssFix) {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(46, 58);
    canvas.print("等待当前位置");
    navFooter("<>选点  B基地  L列表  M地图  H说明");
    return;
  }

  updateMoveHeadingFromGnss();

  double dist = distanceMeters(gnssLat, gnssLon, tLat, tLon);
  double bear = bearingDegrees(gnssLat, gnssLon, tLat, tLon);

  drawCompassArrow(60, 70, 32, bear);

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(104, 30);
  canvas.print("TGT ");
  canvas.print(label);

  canvas.setCursor(104, 44);
  canvas.print("DST ");
  canvas.print(formatDistance(dist));

  canvas.setCursor(104, 58);
  canvas.print("BRG ");
  canvas.print((int)bear);
  canvas.print(" ");
  canvas.print(directionText(bear));

  if (moveHeadingReady) {
    double delta = signedAngleDelta(moveHeadingDeg, bear);

    canvas.setCursor(104, 74);
    canvas.print("MOV ");
    canvas.print((int)moveHeadingDeg);
    canvas.print(" ");
    canvas.print(directionText(moveHeadingDeg));

    useChineseFont12();
    canvas.setTextColor(GREEN, BLACK);
    canvas.setCursor(104, 90);
    canvas.print("目标在");
    canvas.print(relativeDirectionCn(delta));

    useAsciiFont();
    canvas.setTextColor(DARKGREY, BLACK);
    canvas.setCursor(104, 108);
    canvas.print("MOVE LOCK");
  } else {
    useChineseFont12();
    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(104, 78);
    canvas.print("移动5m校准");

    useAsciiFont();
    canvas.setTextColor(DARKGREY, BLACK);
    canvas.setCursor(104, 96);
    canvas.print("STEP ");
    canvas.print((int)moveLastStepMeters);
    canvas.print("m");

    canvas.setCursor(104, 108);
    canvas.print("N-UP ONLY");
  }

  navFooter("<>选点  B基地  L列表  M地图  H说明");
}

void drawNavHelp() {
  // Help is handled by HelpManager.
}

void drawNavScreen() {
  canvas.fillSprite(BLACK);

  if (!navLoaded) navReloadTrackData();

  if (navPage == NAV_PAGE_SESSIONS) drawSessionList();
  else if (navPage == NAV_PAGE_OVERVIEW) drawOverview();
  else if (navPage == NAV_PAGE_MAP) drawRelativeMap();
  else if (navPage == NAV_PAGE_COMPASS) drawCompassNav();
  else drawNavHelp();

  canvas.pushSprite(0, 0);
}

// ---------- Input ----------
bool navKeyHasLetter(const String& key, char lower, char upper) {
  return key.indexOf(lower) >= 0 || key.indexOf(upper) >= 0;
}

bool navIsLeft(const String& key) {
  return key == "," || key == "[LEFT]" || key == "LEFT";
}

bool navIsRight(const String& key) {
  return key == "/" || key == "[RIGHT]" || key == "RIGHT";
}

bool navIsEnter(const String& key) {
  return key.indexOf("[ENTER]") >= 0 ||
         key.indexOf("\n") >= 0 ||
         key.indexOf("\r") >= 0 ||
         key == "OK" ||
         key == "ENTER";
}

void selectPointDelta(int delta) {
  if (navPointCount <= 0) return;

  navSelectedIndex += delta;
  if (navSelectedIndex < 0) navSelectedIndex = navPointCount - 1;
  if (navSelectedIndex >= navPointCount) navSelectedIndex = 0;

  navTargetType = TARGET_POINT;
  navTargetIndex = navSelectedIndex;
}

void selectSessionDelta(int delta) {
  if (navSessionCount <= 0) return;

  navSelectedSessionIndex += delta;
  if (navSelectedSessionIndex < 0) navSelectedSessionIndex = navSessionCount - 1;
  if (navSelectedSessionIndex >= navSessionCount) navSelectedSessionIndex = 0;
}

extern void openNavigationHelp(HelpType type);

void openNavHelp(HelpType type) {
  openNavigationHelp(type);
}

void handleNavKey(const String& key) {
  if (navPage == NAV_PAGE_SESSIONS && navIsEnter(key)) {
    openSelectedSession();
  } else if (navKeyHasLetter(key, 'h', 'H') &&
             (navPage == NAV_PAGE_MAP || navPage == NAV_PAGE_COMPASS)) {
    if (navPage == NAV_PAGE_MAP) {
      openNavHelp(HELP_NAV_MAP);
    } else {
      openNavHelp(HELP_NAV_COMPASS);
    }
    return;
  } else if (navPage == NAV_PAGE_HELP) {
    if (navIsEnter(key)) navPage = navPreviousPage;
  } else if (navKeyHasLetter(key, 'r', 'R')) {
    navReloadTrackData();
  } else if (navKeyHasLetter(key, 'l', 'L')) {
    navPage = NAV_PAGE_SESSIONS;
  } else if (navKeyHasLetter(key, 'o', 'O')) {
    if (selectedSessionId.length() > 0 && navPointCount == 0) loadPointsForSelectedSession();
    navPage = NAV_PAGE_OVERVIEW;
  } else if (navKeyHasLetter(key, 'm', 'M')) {
    if (selectedSessionId.length() > 0 && navPointCount == 0) loadPointsForSelectedSession();
    navPage = NAV_PAGE_MAP;
  } else if (navKeyHasLetter(key, 'n', 'N')) {
    if (selectedSessionId.length() > 0 && navPointCount == 0) loadPointsForSelectedSession();
    navPage = NAV_PAGE_COMPASS;
  } else if (navKeyHasLetter(key, 'b', 'B')) {
    if (baseLoaded) {
      navTargetType = TARGET_BASE;
      navTargetIndex = -1;
      navPage = NAV_PAGE_COMPASS;
    }
  } else if (navIsEnter(key)) {
    // Enter on session list is handled first.
  } else if (navIsLeft(key)) {
    if (navPage == NAV_PAGE_SESSIONS) selectSessionDelta(-1);
    else selectPointDelta(-1);
  } else if (navIsRight(key)) {
    if (navPage == NAV_PAGE_SESSIONS) selectSessionDelta(1);
    else selectPointDelta(1);
  }

  drawNavScreen();
}
