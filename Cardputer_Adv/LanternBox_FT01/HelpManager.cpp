#include "HelpManager.h"
#include <Arduino.h>
#include <M5Cardputer.h>

extern M5Canvas canvas;

extern void useChineseFont12();
extern void useChineseFont16();
extern void useAsciiFont();

HelpType currentHelpType = HELP_HOME;

static const uint16_t HELP_CYAN = 0x07FF;
static const uint16_t HELP_RED = 0xF800;

void drawHelpTitle(const String& title) {
  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 5);
  canvas.print(title);

  canvas.drawLine(0, 24, canvas.width(), 24, WHITE);
}

void drawHelpFooter() {
  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 118);
  canvas.print("ESC / DEL 返回");
}

void showHelp(HelpType type) {
  currentHelpType = type;

  drawHelpTitle("帮助");

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);

  if (type == HELP_RECORDER) {
    canvas.setCursor(8, 32);
    canvas.print("路径记录");

    canvas.setCursor(8, 50);
    canvas.print("B 基地  A 自动记录");

    canvas.setCursor(8, 68);
    canvas.print("P 手动记录  S 停止");

    canvas.setCursor(8, 86);
    canvas.print("需要 GNSS FIX 才能记录");
  }
  else if (type == HELP_NAV_MAP) {
    canvas.setCursor(8, 32);
    canvas.setTextColor(WHITE, BLACK);
    canvas.print("地图说明");

    canvas.setTextColor(GREEN, BLACK);
    canvas.setCursor(8, 50);
    canvas.print("绿 起点");

    canvas.setTextColor(ORANGE, BLACK);
    canvas.setCursor(110, 50);
    canvas.print("橙 终点");

    canvas.setTextColor(LIGHTGREY, BLACK);
    canvas.setCursor(8, 68);
    canvas.print("灰 轨迹");

    canvas.setTextColor(HELP_CYAN, BLACK);
    canvas.setCursor(110, 68);
    canvas.print("蓝圈 基地");

    canvas.setTextColor(HELP_RED, BLACK);
    canvas.setCursor(8, 86);
    canvas.print("红圈 当前");

    canvas.setTextColor(WHITE, BLACK);
    canvas.setCursor(110, 86);
    canvas.print("白点目标");
  }
  else if (type == HELP_NAV_COMPASS) {
    canvas.setCursor(8, 32);
    canvas.setTextColor(WHITE, BLACK);
    canvas.print("指南针导航");

    canvas.setCursor(8, 50);
    canvas.print("显示目标方向");

    canvas.setCursor(8, 68);
    canvas.print("距离和方向");

    canvas.setCursor(8, 86);
    canvas.print("B 选择基地目标");
  }
  else if (type == HELP_NAVIGATION) {
    canvas.setCursor(8, 32);
    canvas.setTextColor(WHITE, BLACK);
    canvas.print("导航模块");
  }
  else if (type == HELP_DEVICE) {
    canvas.setCursor(8, 32);
    canvas.print("设备状态");

    canvas.setCursor(8, 50);
    canvas.print("查看电量 SD GNSS");
  }
  else {
    canvas.setCursor(8, 32);
    canvas.print("暂无帮助内容");
  }

  drawHelpFooter();
  canvas.pushSprite(0, 0);
}

void handleHelpManagerKey(const String& key) {
  if (key.indexOf("[DEL]") >= 0 ||
      key.indexOf("[ESC]") >= 0 ||
      key == "`") {
    return;
  }
}


void drawHelpManager() {
  showHelp(currentHelpType);
}
