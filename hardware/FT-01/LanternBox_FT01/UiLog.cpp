#include "UiLog.h"
#include <M5Cardputer.h>
#include <SD.h>
#include "HelpManager.h"

extern M5Canvas canvas;

extern void useChineseFont12();
extern void useChineseFont16();
extern void useAsciiFont();
extern void returnToHomeFromModule();
extern void openLogHelp();

extern bool sdReady;
extern bool gnssFix;
extern int batteryLevel;
extern String lastWriteStatus;
extern unsigned long getCurrentEpoch();
extern void epochToTimeString(unsigned long epoch, char* buffer, size_t bufferSize);
extern void updateDeviceStatus();

extern void audioLogStart();
extern void audioLogStop();
extern void audioLogPlay();
extern void audioLogStopPlayback();
extern void audioLogGainUp();
extern void audioLogGainDown();
extern void audioLogListRefresh();
extern void audioLogListMove(int delta);
extern void audioLogListPlaySelected();
extern bool audioLogDeleteSelected();
extern String audioLogSelectedFileName();
extern String audioLogSelectedFilePath();
extern String audioLogListFileNameAt(int index);
extern String audioLogListFilePathAt(int index);
extern int audioLogListCount();
extern int audioLogListIndex();
extern const char* audioLogStateText();
extern uint32_t audioLogSamples();
extern uint32_t audioLogDropped();
extern int audioLogGainX10();
extern bool audioLogIsBusy();

static const uint16_t ROW_NORMAL = 0x03A0;
static const uint16_t ROW_ACTIVE = WHITE;

static bool audioMetaCacheReady = false;
static String audioMetaDate[20];
static String audioMetaTime[20];
static String audioMetaDuration[20];

static bool deleteConfirmArmed = false;
static unsigned long deleteConfirmMs = 0;

static bool keyHasLetterLocal(const String& key, char lower, char upper) {
  // Avoid treating textual control tokens like "\\r" as the R key.
  if (key == "\\r" || key == "\\n" || key.indexOf("ENTER") >= 0) {
    return false;
  }

  return key.indexOf(lower) >= 0 || key.indexOf(upper) >= 0;
}

static bool isEscLike(const String& key) {
  return key == "`" ||
         key == "~" ||
         key.indexOf("[ESC]") >= 0 ||
         key.indexOf("[DEL]") >= 0 ||
         key.indexOf("ESC") >= 0 ||
         key.indexOf("DEL") >= 0;
}

static bool isLeftLike(const String& key) {
  return key == "," ||
         key == "<" ||
         key.indexOf("LEFT") >= 0;
}

static bool isRightLike(const String& key) {
  return key == "/" ||
         key == ">" ||
         key.indexOf("RIGHT") >= 0;
}

static bool isEnterLike(const String& key) {
  return key.indexOf("[ENTER]") >= 0 ||
         key.indexOf("ENTER") >= 0 ||
         key == "OK" ||
         key.indexOf('\n') >= 0 ||
         key.indexOf('\r') >= 0 ||
         key == "\\n" ||
         key == "\\r";
}

static String gainText() {
  int gain = audioLogGainX10();
  return String(gain / 10) + "." + String(gain % 10) + "x";
}

static String getJsonStringValueLocal(const String& line, const String& key) {
  String marker = "\"" + key + "\":\"";
  int start = line.indexOf(marker);
  if (start < 0) return "";

  start += marker.length();
  int end = line.indexOf("\"", start);

  if (end < 0) return "";
  return line.substring(start, end);
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

    String fileName = getJsonStringValueLocal(line, "file");
    if (fileName.length() == 0) continue;

    for (int i = 0; i < audioLogListCount() && i < 20; i++) {
      String path = audioLogListFilePathAt(i);

      if (path.endsWith(fileName.substring(fileName.lastIndexOf("/") + 1))) {
        audioMetaDate[i] = getJsonStringValueLocal(line, "device_date");

        String t = getJsonStringValueLocal(line, "device_time");
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

  canvas.fillRect(0, 0, canvas.width(), 20, BLACK);

  useChineseFont12();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(7, 4);
  canvas.print("语音日志");

  useAsciiFont();
  canvas.setTextColor(audioLogIsBusy() ? ORANGE : GREEN, BLACK);
  canvas.setCursor(78, 5);
  canvas.print(audioLogStateText());

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

static void drawAudioRow(int row, int fileIndex) {
  bool selected = fileIndex == audioLogListIndex();
  int y = 28 + row * 27;
  uint16_t fill = selected ? ROW_ACTIVE : ROW_NORMAL;
  uint16_t textColor = BLACK;

  canvas.fillRoundRect(12, y, 216, 17, 3, fill);

  String timeText = "--:--";
  String duration = "--";
  String dateText = "--";

  if (fileIndex >= 0 && fileIndex < 20 && audioMetaCacheReady) {
    timeText = audioMetaTime[fileIndex];
    duration = audioMetaDuration[fileIndex];
    dateText = audioMetaDate[fileIndex];
  }

  if (dateText.length() >= 5) {
    dateText = dateText.substring(5);   // MM-DD
  }

  useAsciiFont();
  canvas.setTextSize(1);
  canvas.setTextColor(textColor, fill);

  canvas.setCursor(16, y + 3);
  canvas.print(selected ? ">" : " ");
  canvas.print(fileIndex + 1);

  canvas.setCursor(50, y + 3);
  canvas.print(dateText);

  canvas.setCursor(128, y + 3);
  canvas.print(String(timeText).substring(0, 5));

  canvas.setCursor(182, y + 3);
  canvas.print(duration);

  // light overdraw for better legibility without increasing row height
  canvas.setCursor(17, y + 3);
  canvas.print(selected ? ">" : " ");
  canvas.print(fileIndex + 1);

  canvas.setCursor(51, y + 3);
  canvas.print(dateText);

  canvas.setCursor(129, y + 3);
  canvas.print(String(timeText).substring(0, 5));

  canvas.setCursor(183, y + 3);
  canvas.print(duration);
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

  int count = audioLogListCount();
  int selected = audioLogListIndex();

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
      keyHasLetterLocal(key, 'b', 'B') ||
      key.indexOf("[DEL]") >= 0 ||
      key.indexOf("DEL") >= 0;

  // Delete confirmation mode:
  // Keep the confirmation page on screen until B confirms or another key cancels.
  if (deleteConfirmArmed) {
    if (isDeleteKey) {
      drawLogLoading("删除中");

      bool ok = audioLogDeleteSelected();
deleteConfirmArmed = false;

      if (ok) {
        audioLogListRefresh();
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

  if (isEscLike(key)) {
    if (!audioLogIsBusy()) {
      returnToHomeFromModule();
    }
    return;
  }

  if (keyHasLetterLocal(key, 'h', 'H')) {
    openLogHelp();
    return;
  }

  // Enter / P must be handled before R.
  // Some Enter payloads look like "\\r", which otherwise can be mistaken for R.
  if (keyHasLetterLocal(key, 'p', 'P') || isEnterLike(key)) {
    if (audioLogIsBusy()) {
      audioLogStopPlayback();
      drawLogScreen();
    } else {
      drawLogLoading("播放中");
      audioLogListPlaySelected();
      drawLogScreen();
    }
    return;
  }

  if (keyHasLetterLocal(key, 'r', 'R')) {
    audioLogStart();
    return;
  }

  if (keyHasLetterLocal(key, 's', 'S')) {
    drawLogLoading("保存中");
    audioLogStop();

    drawLogLoading("刷新列表");
    audioLogListRefresh();
    invalidateAudioMetaCache();

    drawLogLoading("整理信息");
    rebuildAudioMetaCache();

    drawLogScreen();
    return;
  }

  if (isLeftLike(key)) {
    audioLogListMove(-1);
    return;
  }

  if (isRightLike(key)) {
    audioLogListMove(1);
    return;
  }

  if (keyHasLetterLocal(key, 'a', 'A')) {
    drawLogLoading("刷新列表");
    audioLogListRefresh();
    invalidateAudioMetaCache();

    drawLogLoading("整理信息");
    rebuildAudioMetaCache();

    drawLogScreen();
    return;
  }

  if (isDeleteKey) {
    deleteConfirmArmed = true;
    deleteConfirmMs = millis();
    drawDeleteConfirm();
    return;
  }

  if (keyHasLetterLocal(key, 'u', 'U')) {
    audioLogGainUp();
    return;
  }

  if (keyHasLetterLocal(key, 'd', 'D')) {
    audioLogGainDown();
    return;
  }
}

