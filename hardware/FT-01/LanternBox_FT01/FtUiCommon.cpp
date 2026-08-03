#include "FtUiCommon.h"

extern M5Canvas canvas;
extern bool sdReady;
extern bool gnssFix;
extern void useChineseFont16();
extern void useChineseFont12();
extern void useAsciiFont();
extern unsigned long getCurrentEpoch();
extern void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize);

namespace {
  bool containsToken(const String& key, const char* token) {
    return key.indexOf(token) >= 0;
  }
}

bool FtKey::hasLetter(const String& key, char lower, char upper) {
  if (key == "\\r" || key == "\\n" || key == "\r" || key == "\n" || containsToken(key, "ENTER")) {
    return false;
  }
  return key.indexOf(lower) >= 0 || key.indexOf(upper) >= 0;
}

bool FtKey::isEsc(const String& key, bool includeDelete) {
  if (key == "`" || key == "~" || containsToken(key, "ESC")) return true;
  return includeDelete && containsToken(key, "DEL");
}

bool FtKey::isEnter(const String& key) {
  return key == "OK" || key == "\\r" || key == "\\n" || key == "\r" || key == "\n" || containsToken(key, "ENTER");
}

bool FtKey::isLeft(const String& key) {
  return key == "," || key == "<" || containsToken(key, "LEFT");
}

bool FtKey::isRight(const String& key) {
  return key == "/" || key == ">" || containsToken(key, "RIGHT");
}

bool FtKey::isUp(const String& key) {
  return key == ";" || containsToken(key, "UP");
}

bool FtKey::isDown(const String& key) {
  return key == "." || containsToken(key, "DOWN");
}

void ftDrawHeaderBase(const String& title) {
  canvas.fillRect(0, 0, canvas.width(), 22, BLACK);
  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 4);
  canvas.print(title);
}

void ftDrawCompactTitle(const String& title) {
  canvas.fillRect(0, 0, canvas.width(), 20, BLACK);
  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(7, 4);
  canvas.print(title);
}

void ftDrawSdGnssStatus(int sdX, int gnssX) {
  useAsciiFont();
  canvas.setTextColor(sdReady ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(sdX, 5);
  canvas.print("SD");

  canvas.setTextColor(gnssFix ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(gnssX, 5);
  canvas.print("GNSS");
}

void ftDrawClockHHMM(int x) {
  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));
  timeText[5] = '\0';

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(x, 5);
  canvas.print(timeText);
}

void ftDrawFooter(const String& text, int x) {
  canvas.drawLine(0, 112, canvas.width(), 112, WHITE);
  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(x, 116);
  canvas.print(text);
}

void ftDrawLabelValueRow(int y, const String& label, const String& value, int valueX, uint16_t color, uint16_t labelColor) {
  useChineseFont12();
  canvas.setTextColor(labelColor, BLACK);
  canvas.setCursor(10, y);
  canvas.print(label);

  useAsciiFont();
  canvas.setTextColor(color, BLACK);
  canvas.setCursor(valueX, y + 1);
  canvas.print(value);
}
