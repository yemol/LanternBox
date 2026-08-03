#include "UiSync.h"
#include "SyncManager.h"
#include <Arduino.h>
#include <M5Cardputer.h>
#include "FtUiCommon.h"
#include "FtTextUtil.h"

extern M5Canvas canvas;
extern SyncManager syncManager;

extern bool sdReady;
extern bool gnssFix;
extern int gnssSatellites;
extern String sdStatusText;
extern bool suppressGnssStatusLog;

extern void useChineseFont16();
extern void useChineseFont12();
extern void useAsciiFont();
extern void returnToHomeFromModule();
extern void openSyncHelp();

enum SyncView {
  SYNC_VIEW_TRANSPORT,
  SYNC_VIEW_DETAIL
};

static SyncView syncView = SYNC_VIEW_TRANSPORT;
static int syncTransportIndex = 0; // 0 USB, 1 WiFi, 2 BLE
static int syncDetailPage = 0;
static bool syncBusy = false;
static String syncBusyText = "";
static const int SYNC_DETAIL_PAGE_COUNT = 2;

static void syncHeader(const String& title) {
  ftDrawHeaderBase(title);
  ftDrawSdGnssStatus(138, 164);
}

static void syncFooter(const String& text) {
  ftDrawFooter(text);
}

static void row(int y, const String& label, const String& value, uint16_t color = WHITE) {
  ftDrawLabelValueRow(y, label, value, 92, color);
}

static void drawTransportCard(int x, int y, int w, int h, const String& title, const String& subtitle, bool selected, bool enabled) {
  uint16_t fill = selected ? 0x2104 : BLACK;      // dark grey, same direction as home selected card
  uint16_t border = selected ? GREEN : DARKGREY;  // selected uses green frame
  uint16_t titleColor = enabled ? WHITE : DARKGREY;
  uint16_t subColor = enabled ? LIGHTGREY : DARKGREY;

  canvas.fillRoundRect(x, y, w, h, 6, fill);
  canvas.drawRoundRect(x, y, w, h, 6, border);
  if (selected) {
    canvas.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 5, border);
  }

  useAsciiFont();
  canvas.setTextColor(titleColor, fill);
  canvas.setCursor(x + 10, y + 10);
  canvas.print(title);

  useChineseFont12();
  canvas.setTextColor(subColor, fill);
  canvas.setCursor(x + 10, y + 30);
  canvas.print(subtitle);

  if (!enabled) {
    useAsciiFont();
    canvas.setTextColor(DARKGREY, fill);
    canvas.setCursor(x + w - 44, y + 10);
    canvas.print("LOCK");
  }
}


static void drawSyncBusyScreen(const String& message) {
  canvas.fillSprite(BLACK);

  syncHeader("USB 同步");

  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(42, 46);
  canvas.print(message);

  useChineseFont12();
  canvas.setTextColor(LIGHTGREY, BLACK);
  canvas.setCursor(30, 72);
  if (message.indexOf("读取") >= 0) {
    canvas.print("正在扫描本地待同步数据");
  } else {
    canvas.print("正在读取 SD 并输出 Manifest");
  }

  useAsciiFont();
  canvas.setTextColor(ORANGE, BLACK);
  canvas.setCursor(36, 94);
  canvas.print("Please wait...");

  canvas.drawLine(0, 112, canvas.width(), 112, WHITE);
  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(38, 116);
  canvas.print("同步中 暂停按键操作");

  canvas.pushSprite(0, 0);
}


void syncShowPhase(const String& phase, const String& detail) {
  suppressGnssStatusLog = true;

  canvas.fillSprite(BLACK);
  syncHeader("USB 同步中");

  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(22, 38);
  canvas.print(phase);

  useChineseFont12();
  canvas.setTextColor(LIGHTGREY, BLACK);
  canvas.setCursor(18, 66);
  String shown = detail;
  if (shown.length() > 22) shown = shown.substring(0, 22);
  canvas.print(shown);

  useAsciiFont();
  canvas.setTextColor(ORANGE, BLACK);
  canvas.setCursor(34, 92);
  canvas.print("Core <-> FT-01");

  canvas.drawLine(0, 112, canvas.width(), 112, WHITE);
  useChineseFont12();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(32, 116);
  canvas.print("请保持在本页 不要按键");

  canvas.pushSprite(0, 0);
}

static void drawTransportSelect() {
  syncHeader("同步 选择方式");

  drawTransportCard(10, 30, 68, 70, "USB", "可用", syncTransportIndex == 0, true);
  drawTransportCard(86, 30, 68, 70, "WiFi", "待接入", syncTransportIndex == 1, false);
  drawTransportCard(162, 30, 68, 70, "BLE", "待接入", syncTransportIndex == 2, false);

  syncFooter("< > 选择 | Enter 进入 | ESC 返回");
}

static void drawDetailUpload() {
  syncHeader("USB 同步 数据");

  const SyncManifestStats& st = syncManager.stats();

  row(30, "路径点", String(st.pathPoints));
  row(46, "现场日志", String(st.fieldEvents));
  row(62, "启动日志", String(st.bootLogs));
  row(78, "录音索引", String(st.audioIndex));
  row(94, "录音文件", ftFormatBytes(st.audioBytes, 1));

  syncFooter("< > 翻页 | U Hello | M Manifest");
}

static void drawDetailTask() {
  syncHeader("USB 同步 任务");

  const SyncManifestStats& st = syncManager.stats();

  row(34, "任务下发", "PUT_TASKS");
  row(50, "回报记录", String(st.taskReports));
  row(66, "状态修改", "FT01 -> Core");
  row(82, "任务文件", "/tasks");
  row(98, "本地清理", "禁用", ORANGE);

  syncFooter("< > 翻页 | R 刷新 | H Help");
}

void drawSyncScreen() {
  suppressGnssStatusLog = true;

  if (syncBusy) {
    drawSyncBusyScreen(syncBusyText.length() > 0 ? syncBusyText : String("正在同步中..."));
    return;
  }

  canvas.fillSprite(BLACK);

  if (syncView == SYNC_VIEW_TRANSPORT) {
    drawTransportSelect();
  } else {
    if (syncDetailPage == 0) drawDetailUpload();
    else drawDetailTask();
  }

  canvas.pushSprite(0, 0);
}

void handleSyncKey(const String& key) {
  suppressGnssStatusLog = true;

  if (syncBusy) {
    return;
  }

  if (FtKey::isEsc(key, true)) {
    if (syncView == SYNC_VIEW_DETAIL) {
      syncView = SYNC_VIEW_TRANSPORT;
      drawSyncScreen();
      return;
    }

    suppressGnssStatusLog = false;
    returnToHomeFromModule();
    return;
  }

  if (syncView == SYNC_VIEW_TRANSPORT) {
    if (FtKey::isLeft(key)) {
      syncTransportIndex--;
      if (syncTransportIndex < 0) syncTransportIndex = 2;
    } else if (FtKey::isRight(key)) {
      syncTransportIndex++;
      if (syncTransportIndex > 2) syncTransportIndex = 0;
    } else if (FtKey::isEnter(key)) {
      if (syncTransportIndex == 0) {
        syncBusy = true;
        syncBusyText = "正在读取 SD...";
        drawSyncScreen();

        syncManager.refresh(sdReady);

        syncBusy = false;
        syncBusyText = "";
        syncView = SYNC_VIEW_DETAIL;
        syncDetailPage = 0;
      }
    } else if (FtKey::hasLetter(key, 'h', 'H')) {
      openSyncHelp();
      return;
    }

    drawSyncScreen();
    return;
  }

  if (FtKey::isLeft(key)) {
    syncDetailPage--;
    if (syncDetailPage < 0) syncDetailPage = SYNC_DETAIL_PAGE_COUNT - 1;
  } else if (FtKey::isRight(key)) {
    syncDetailPage++;
    if (syncDetailPage >= SYNC_DETAIL_PAGE_COUNT) syncDetailPage = 0;
  } else if (FtKey::hasLetter(key, 'r', 'R')) {
    syncManager.refresh(sdReady);
  } else if (FtKey::hasLetter(key, 'u', 'U')) {
    syncManager.printUsbHello(sdReady, gnssFix, gnssSatellites, sdStatusText);
  } else if (FtKey::hasLetter(key, 'm', 'M')) {
    syncBusy = true;
    syncBusyText = "正在同步中...";
    drawSyncScreen();

    syncManager.printManifest(sdReady);

    syncBusy = false;
    syncBusyText = "";
  } else if (FtKey::hasLetter(key, 'h', 'H')) {
    openSyncHelp();
    return;
  }

  drawSyncScreen();
}
