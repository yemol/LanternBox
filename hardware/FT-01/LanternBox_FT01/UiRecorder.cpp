#include "UiRecorder.h"

static const uint16_t CARD_GREEN = 0x8EEC;  // readable soft green

// ---------- Shared state from LanternBox_FT01.ino ----------
extern const char* VERSION;
extern M5Canvas canvas;

extern bool autoTrack;
extern bool sdReady;
extern bool gnssFix;
extern bool sessionActive;

extern int batteryLevel;
extern int gnssSatellites;
extern int pathWriteCount;

extern double gnssLat;
extern double gnssLon;

extern String currentSessionId;

extern unsigned long lastAutoTrackMillis;
extern const unsigned long autoTrackIntervalMs;


// ---------- Shared helpers from LanternBox_FT01.ino ----------
extern void useChineseFont12();
extern void useAsciiFont();
extern void updateDeviceStatus();
extern unsigned long getCurrentEpoch();
extern void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize);
extern String autoNextText();

void drawTextCard(int x, int y, int w, int h, const String& title, const String& value, bool highlight = false) {
  canvas.fillRoundRect(x, y, w, h, 3, CARD_GREEN);

  useChineseFont12();
  canvas.setTextColor(BLACK, CARD_GREEN);
  canvas.setCursor(x + 6, y + 5);
  canvas.print(title);

  if (value.length() > 8) {
    useChineseFont12();
  } else {
    useAsciiFont();
  }

  canvas.setTextColor(BLACK, CARD_GREEN);
  canvas.setCursor(x + 6, y + 20);
  canvas.print(value);

  useAsciiFont();
}

String displaySessionIdCompact() {
  if (!sessionActive || currentSessionId.length() == 0) {
    return "";
  }

  if (currentSessionId.length() <= 13) {
    return currentSessionId;
  }

  return currentSessionId.substring(0, 13);
}

String satelliteDisplayCompact() {
  if (!gnssFix) return "NO FIX";
  if (gnssSatellites < 0) return "--";
  return String(gnssSatellites);
}

String countdownDisplayCompact() {
  if (!autoTrack) return "--:--";
  return autoNextText();
}


void drawRecorderScreen() {
  canvas.fillSprite(BLACK);
  updateDeviceStatus();

  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

  // ===== Compact Header =====
  canvas.fillRect(0, 0, canvas.width(), 20, BLACK);

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(7, 4);
  canvas.print("路径记录");

  useAsciiFont();
  canvas.setTextColor(autoTrack ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(112, 5);
  canvas.print("AUTO");

  canvas.setTextColor(sdReady ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(154, 5);
  canvas.print("SD");

  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(178, 5);
  canvas.print("|");

  canvas.setTextColor(batteryLevel <= 20 ? ORANGE : WHITE, BLACK);
  canvas.setCursor(191, 5);
  if (batteryLevel >= 0) {
    canvas.print(batteryLevel);
    canvas.print("%");
  } else {
    canvas.print("--");
  }

  canvas.drawLine(0, 20, canvas.width(), 20, WHITE);

  // ===== Main Cards =====
  const int cardW = 102;
  const int cardH = 33;
  const int leftX = 10;
  const int rightX = 128;
  const int topY = 29;
  const int bottomY = 68;

  drawTextCard(leftX, topY, cardW, cardH, "会话", displaySessionIdCompact(), sessionActive);
  drawTextCard(rightX, topY, cardW, cardH, "路径点", String(pathWriteCount), pathWriteCount > 0);
  drawTextCard(leftX, bottomY, cardW, cardH, "倒计时", countdownDisplayCompact(), autoTrack);
  drawTextCard(rightX, bottomY, cardW, cardH, "卫星", satelliteDisplayCompact(), gnssFix);

  // ===== Shortcut row =====
  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(10, 108);
  if (gnssFix) {
    canvas.print("B:基地  A:自动  S:停止  P:路径");
  } else {
    canvas.print("等待定位：A/B/P/S禁用  H:帮助");
  }

  // ===== Bottom status bar =====
  canvas.drawLine(0, 122, canvas.width(), 122, WHITE);

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(4, 125);

  if (gnssFix) {
    canvas.print(String(gnssLat, 5));
    canvas.print("/");
    canvas.print(String(gnssLon, 5));
  } else {
    canvas.print("-- / --");
  }

  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(178, 125);
  canvas.print(timeText);

  canvas.pushSprite(0, 0);
}


void drawHelpScreen() {
  canvas.fillSprite(BLACK);

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 5);
  canvas.print("帮助 / HELP");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(86, 6);
  canvas.print(VERSION);

  canvas.drawLine(0, 20, canvas.width(), 20, WHITE);

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);

  canvas.setCursor(10, 28);
  canvas.print("A  AUTO ON/OFF   FIX");

  canvas.setCursor(10, 43);
  canvas.print("B  SAVE BASE     FIX");

  canvas.setCursor(10, 58);
  canvas.print("P  SAVE POINT    FIX");

  canvas.setCursor(10, 73);
  canvas.print("S  STOP SESSION  FIX");

  canvas.setCursor(10, 88);
  canvas.print("R  RESET SD/GNSS");

  canvas.setCursor(10, 103);
  canvas.print("H  CLOSE HELP");

  canvas.setCursor(10, 118);
  canvas.print("ESC BACK, REC KEEPS RUNNING");

  canvas.setTextColor(autoTrack ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(174, 28);
  canvas.print(autoTrack ? "AUTO" : "IDLE");

  canvas.setTextColor(sdReady ? GREEN : ORANGE, BLACK);
  canvas.setCursor(174, 43);
  canvas.print(sdReady ? "SD OK" : "NO SD");

  canvas.pushSprite(0, 0);
}

