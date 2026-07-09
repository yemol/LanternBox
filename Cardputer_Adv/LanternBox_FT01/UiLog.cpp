#include "UiLog.h"
#include <M5Cardputer.h>

extern M5Canvas canvas;

extern void useChineseFont12();
extern void useChineseFont16();
extern void useAsciiFont();
extern void drawHomeScreen();
extern void returnToHomeFromModule();

void drawLogScreen() {
  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 8);
  canvas.print("日志");

  canvas.drawLine(0, 28, canvas.width(), 28, WHITE);

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(20, 55);
  canvas.print("暂无日志");

  canvas.setCursor(20, 75);
  canvas.print("录音模块准备中");

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 120);
  canvas.print("ESC 返回");

  canvas.pushSprite(0, 0);
}

void handleLogKey(const String& key) {
  // Log page exit handling.
  // Cardputer may report ESC as special key text depending on firmware version.
  if (key == "`" ||
      key == "~" ||
      key.indexOf("[ESC]") >= 0 ||
      key.indexOf("[DEL]") >= 0 ||
      key.indexOf("ESC") >= 0 ||
      key.indexOf("DEL") >= 0) {
    returnToHomeFromModule();
  }
}
