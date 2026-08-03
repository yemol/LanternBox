#include "UiLog.h"
#include <M5Cardputer.h>
#include <SD.h>
#include "HelpManager.h"
#include "AudioLogger.h"
#include "FtUiCommon.h"
#include "FtTextUtil.h"

extern M5Canvas canvas;

extern void useChineseFont12();
extern void useChineseFont16();
extern void useAsciiFont();
extern void returnToHomeFromModule();
extern void openLogHelp();

extern bool sdReady;
extern bool gnssFix;
extern unsigned long getCurrentEpoch();
extern void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize);
extern void updateDeviceStatus();

extern AudioLogger audioLogger;
extern String lastAction;
extern String lastWriteStatus;
extern String currentSessionId;
extern int gnssSatellites;
extern double gnssLat;
extern double gnssLon;
extern String gnssUtcTime;
extern String gnssUtcDate;
extern void ensureSessionStarted(const char* reason);
extern String currentDeviceDateText();
extern String currentDeviceTimeText();

static bool audioMetaCacheReady = false;
static String audioMetaDate[20];
static String audioMetaTime[20];
static String audioMetaDuration[20];

static bool deleteConfirmArmed = false;

static String gainText() {
  int gain = audioLogger.gainX10();
  return String(gain / 10) + "." + String(gain % 10) + "x";
}

static String durationFromPathFast(const String& path) {
  if (path.length() == 0 || !SD.exists(path)) return "--";

  File f = SD.open(path, FILE_READ);
  if (!f) return "--";

  uint32_t size = f.size();
  f.close();

  if (size <= 44) return "--";

  uint32_t seconds = ((size - 44) / 2) / 16000;

  if (seconds < 60) {
    return String(seconds) + "s";
  }

  return String(seconds / 60) + "m" + String(seconds % 60) + "s";
}

static void refreshAudioList() {
  audioLogger.refreshList();
  lastAction = "LIST REFRESH";
}

static void moveAudioSelection(int delta) {
  audioLogger.moveSelection(delta);
}

static void startAudioRecording() {
  if (!sdReady) {
    lastAction = "NO SD";
    lastWriteStatus = "AUDIO NO SD";
    Serial.println("[AUDIO] start blocked: SD not ready");
    return;
  }

  ensureSessionStarted("audio");

  AudioLoggerGnssSnapshot snap;
  snap.fix = gnssFix;
  snap.satellites = gnssSatellites;
  snap.lat = gnssLat;
  snap.lon = gnssLon;
  snap.utcTime = gnssUtcTime;
  snap.utcDate = gnssUtcDate;

  bool ok = audioLogger.startRecord(
    currentSessionId,
    snap,
    currentDeviceDateText(),
    currentDeviceTimeText()
  );
  lastAction = ok ? "AUDIO REC" : "AUDIO FAIL";
  lastWriteStatus = ok ? "AUDIO REC" : "AUDIO BUSY";
}

static void stopAudioRecording() {
  bool ok = audioLogger.stopRecord();
  lastAction = ok ? "AUDIO SAVED" : "AUDIO STOP FAIL";
  lastWriteStatus = ok ? "AUDIO SAVED" : "AUDIO FAIL";
}

static void playSelectedAudio() {
  bool ok = audioLogger.playSelected();
  lastAction = ok ? "PLAY SELECT" : "PLAY FAIL";
  lastWriteStatus = ok ? "PLAY SELECT" : "PLAY FAIL";
}

static bool deleteSelectedAudio() {
  bool ok = audioLogger.deleteSelected();
  lastAction = ok ? "AUDIO DELETE" : "DELETE FAIL";
  lastWriteStatus = ok ? "AUDIO DELETE" : "DELETE FAIL";
  return ok;
}

static void rebuildAudioMetaCache() {
  for (int i = 0; i < 20; i++) {
    audioMetaDate[i] = "--";
    audioMetaTime[i] = "--:--";
    audioMetaDuration[i] = "--";
  }

  if (!SD.exists("/lanternbox/audio/index.jsonl")) {
    audioMetaCacheReady = true;
    return;
  }

  File f = SD.open("/lanternbox/audio/index.jsonl", FILE_READ);
  if (!f) {
    audioMetaCacheReady = true;
    return;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');

    String fileName = ftJsonString(line, "file");
    if (fileName.length() == 0) continue;

    for (int i = 0; i < audioLogger.listCount() && i < 20; i++) {
      String path = audioLogger.listFilePathAt(i);

      if (path.endsWith(fileName.substring(fileName.lastIndexOf("/") + 1))) {
        audioMetaDate[i] = ftJsonString(line, "device_date");

        String t = ftJsonString(line, "device_time");
        if (t.length() >= 5) {
          audioMetaTime[i] = t.substring(0, 5);
        }

        audioMetaDuration[i] = durationFromPathFast(path);
      }
    }
  }

  f.close();

  audioMetaCacheReady = true;
}

static void invalidateAudioMetaCache() {
  audioMetaCacheReady = false;
}

static void drawTopBar() {
  updateDeviceStatus();

  char timeText[12];
  epochToTimeString(getCurrentEpoch(), timeText, sizeof(timeText));

  ftDrawCompactTitle("语音日志");
  useAsciiFont();
  canvas.setTextColor(audioLogger.isBusy() ? ORANGE : GREEN, BLACK);
  canvas.setCursor(78, 5);
  canvas.print(audioLogger.stateText());

  canvas.setTextColor(sdReady ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(140, 5);
  canvas.print("SD");

  canvas.setTextColor(gnssFix ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(170, 5);
  canvas.print("GNSS");

  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(202, 5);
  canvas.print(String(timeText).substring(0, 5));

}

static void drawLogLoading(const String& text) {
  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(GREEN, BLACK);
  canvas.setCursor(50, 50);
  canvas.print(text);

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(70, 78);
  canvas.print("PLEASE WAIT");

  canvas.pushSprite(0, 0);
}

static void drawDeleteConfirm() {
  canvas.fillSprite(BLACK);

  useChineseFont16();
  canvas.setTextColor(ORANGE, BLACK);
  canvas.setCursor(44, 42);
  canvas.print("确认删除？");

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(24, 72);
  canvas.print("再按 B 删除当前录音");

  canvas.setCursor(34, 94);
  canvas.print("其他按键取消");

  canvas.pushSprite(0, 0);
}

void drawLogScreen() {
  if (!audioMetaCacheReady) {
    rebuildAudioMetaCache();
  }

  if (deleteConfirmArmed) {
    drawDeleteConfirm();
    return;
  }

  canvas.fillSprite(BLACK);
  drawTopBar();

  int count = audioLogger.listCount();
  int selected = audioLogger.listIndex();

  int startIndex = selected - 1;
  if (startIndex < 0) startIndex = 0;

  for (int row = 0; row < 3; row++) {
    int index = startIndex + row;
    if (index >= count) break;

    int y = 30 + row * 24;
    bool active = index == selected;

    uint16_t bg = active ? WHITE : 0x03A0;
    uint16_t fg = active ? BLACK : WHITE;

    canvas.fillRoundRect(10, y, 220, 18, 3, bg);

    useAsciiFont();
    canvas.setTextColor(fg, bg);
    canvas.setCursor(15, y + 5);

    canvas.print(active ? ">" : " ");
    canvas.print(index + 1);

    // date
    canvas.setCursor(42, y + 5);
    if (audioMetaDate[index].length() >= 5) {
      canvas.print(audioMetaDate[index].substring(5));
    } else {
      canvas.print("--");
    }

    // time
    canvas.setCursor(105, y + 5);
    canvas.print(audioMetaTime[index]);

    // duration
    canvas.setCursor(170, y + 5);
    canvas.print(audioMetaDuration[index]);
  }

  canvas.drawLine(0, 106, canvas.width(), 106, WHITE);

  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);

  canvas.setCursor(8, 112);
  canvas.print("GAIN ");
  canvas.print(gainText());

  canvas.setCursor(88, 112);
  canvas.print("H Help");

  canvas.setCursor(198, 112);
  canvas.print(selected + 1);
  canvas.print("/");
  canvas.print(count);

  canvas.pushSprite(0, 0);
}

void handleLogKey(const String& key) {
  bool isDeleteKey =
      FtKey::hasLetter(key, 'b', 'B') ||
      key.indexOf("[DEL]") >= 0 ||
      key.indexOf("DEL") >= 0;

  // Delete confirmation mode:
  // Keep the confirmation page on screen until B confirms or another key cancels.
  if (deleteConfirmArmed) {
    if (isDeleteKey) {
      drawLogLoading("删除中");

      bool ok = deleteSelectedAudio();
deleteConfirmArmed = false;

      if (ok) {
        refreshAudioList();
        invalidateAudioMetaCache();
        rebuildAudioMetaCache();
      }

      drawLogScreen();
      return;
    }

    deleteConfirmArmed = false;
    drawLogScreen();
    return;
  }

  if (FtKey::isEsc(key, true)) {
    if (!audioLogger.isBusy()) {
      returnToHomeFromModule();
    }
    return;
  }

  if (FtKey::hasLetter(key, 'h', 'H')) {
    openLogHelp();
    return;
  }

  // Enter / P must be handled before R.
  // Some Enter payloads look like "\\r", which otherwise can be mistaken for R.
  if (FtKey::hasLetter(key, 'p', 'P') || FtKey::isEnter(key)) {
    if (audioLogger.isBusy()) {
      audioLogger.stopPlayback();
      drawLogScreen();
    } else {
      drawLogLoading("播放中");
      playSelectedAudio();
      drawLogScreen();
    }
    return;
  }

  if (FtKey::hasLetter(key, 'r', 'R')) {
    startAudioRecording();
    return;
  }

  if (FtKey::hasLetter(key, 's', 'S')) {
    drawLogLoading("保存中");
    stopAudioRecording();

    drawLogLoading("刷新列表");
    refreshAudioList();
    invalidateAudioMetaCache();

    drawLogLoading("整理信息");
    rebuildAudioMetaCache();

    drawLogScreen();
    return;
  }

  if (FtKey::isLeft(key)) {
    moveAudioSelection(-1);
    return;
  }

  if (FtKey::isRight(key)) {
    moveAudioSelection(1);
    return;
  }

  if (FtKey::hasLetter(key, 'a', 'A')) {
    drawLogLoading("刷新列表");
    refreshAudioList();
    invalidateAudioMetaCache();

    drawLogLoading("整理信息");
    rebuildAudioMetaCache();

    drawLogScreen();
    return;
  }

  if (isDeleteKey) {
    deleteConfirmArmed = true;
    drawDeleteConfirm();
    return;
  }

  if (FtKey::hasLetter(key, 'u', 'U')) {
    audioLogger.gainUp();
    return;
  }

  if (FtKey::hasLetter(key, 'd', 'D')) {
    audioLogger.gainDown();
    return;
  }
}

