#include "UiTasks.h"
#include "TaskManager.h"
#include <Arduino.h>
#include <M5Cardputer.h>

extern M5Canvas canvas;
extern TaskManager taskManager;
extern bool sdReady;

extern void useChineseFont16();
extern void useChineseFont12();
extern void useAsciiFont();
extern void returnToHomeFromModule();

static bool taskDetailMode = false;
static int taskDetailScrollLine = 0;

static bool taskIsEsc(const String& key) {
  return key == "`" || key == "[ESC]" || key == "ESC" || key == "[DEL]";
}

static bool taskIsEnter(const String& key) {
  return key == "\r" || key == "\n" || key.indexOf("ENTER") >= 0 || key == "OK" || key == "[ENTER]";
}

static bool taskIsLeft(const String& key) {
  return key == "," || key == "[LEFT]" || key == "LEFT" || key == ";" || key == "[UP]" || key == "UP";
}

static bool taskIsRight(const String& key) {
  return key == "/" || key == "[RIGHT]" || key == "RIGHT" || key == "." || key == "[DOWN]" || key == "DOWN";
}

static bool taskHasLetter(const String& key, char lower, char upper) {
  if (key == "\r" || key == "\n" || key.indexOf("ENTER") >= 0) return false;
  return key.indexOf(lower) >= 0 || key.indexOf(upper) >= 0;
}

static bool isUtf8Continuation(unsigned char c) {
  return (c & 0xC0) == 0x80;
}

static int utf8CharCount(const String& value) {
  int result = 0;
  for (int i = 0; i < value.length(); i++) {
    unsigned char c = (unsigned char)value[i];
    if (!isUtf8Continuation(c)) result++;
  }
  return result;
}

static int utf8ByteIndexForChar(const String& value, int charIndex) {
  if (charIndex <= 0) return 0;
  int chars = 0;
  for (int i = 0; i < value.length(); i++) {
    unsigned char c = (unsigned char)value[i];
    if (!isUtf8Continuation(c)) {
      if (chars >= charIndex) return i;
      chars++;
    }
  }
  return value.length();
}

static String utf8Slice(const String& value, int startChar, int maxChars) {
  if (maxChars <= 0) return "";
  int start = utf8ByteIndexForChar(value, startChar);
  int end = utf8ByteIndexForChar(value, startChar + maxChars);
  if (start < 0) start = 0;
  if (end < start) end = start;
  if (start >= value.length()) return "";
  return value.substring(start, end);
}

static String clipText(const String& value, int maxChars) {
  if (utf8CharCount(value) <= maxChars) return value;
  if (maxChars <= 1) return "~";
  return utf8Slice(value, 0, maxChars - 1) + "~";
}

static String statusShort(const String& value) {
  if (value == "in_progress") return "DOING";
  if (value == "completed") return "DONE";
  if (value == "blocked") return "BLOCK";
  return "TODO";
}

static String statusChinese(const String& value) {
  if (value == "in_progress") return "进行中";
  if (value == "completed") return "已完成";
  if (value == "blocked") return "受阻";
  return "待处理";
}

static uint16_t statusColor(const String& value) {
  if (value == "completed") return GREEN;
  if (value == "blocked") return ORANGE;
  if (value == "in_progress") return CYAN;
  return LIGHTGREY;
}

static void taskHeader(const String& title) {
  canvas.fillRect(0, 0, canvas.width(), 22, BLACK);

  useChineseFont16();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(8, 4);
  canvas.print(title);

  useAsciiFont();
  canvas.setTextColor(sdReady ? GREEN : DARKGREY, BLACK);
  canvas.setCursor(150, 5);
  canvas.print("SD");

  canvas.setTextColor(LIGHTGREY, BLACK);
  canvas.setCursor(176, 5);
  canvas.print(taskManager.taskCount());
  canvas.print(" TASK");
}

static void taskFooter(const String& text) {
  canvas.drawLine(0, 112, canvas.width(), 112, WHITE);
  useAsciiFont();
  canvas.setTextColor(WHITE, BLACK);
  canvas.setCursor(6, 116);
  canvas.print(text);
}

static void printChinese16Bold(const String& text, int x, int y, uint16_t color, uint16_t bg) {
  useChineseFont16();
  canvas.setTextColor(color, bg);
  canvas.setCursor(x, y);
  canvas.print(text);
  canvas.setCursor(x + 1, y);
  canvas.print(text);
}

static void drawSummaryStrip() {
  const TaskStats& st = taskManager.stats();
  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(8, 25);
  canvas.print("TODO "); canvas.print(st.pending);
  canvas.print("  DOING "); canvas.print(st.inProgress);
  canvas.print("  DONE "); canvas.print(st.completed);
  canvas.print("  REP "); canvas.print(st.reports);
}

static void drawTaskRow(int row, const FtTaskItem* task, bool selected) {
  int y = 40 + row * 23;

  // Do not draw an empty card for a missing task. When the Core removes a
  // completed task and the list shrinks from 3 to 2 items, the third placeholder
  // rectangle looked like a ghost task. Clear the slot and leave it blank.
  if (task == nullptr) {
    canvas.fillRect(6, y, 228, 20, BLACK);
    return;
  }

  uint16_t fill = selected ? 0x2104 : BLACK;
  uint16_t border = selected ? GREEN : DARKGREY;
  canvas.fillRoundRect(6, y, 228, 20, 4, fill);
  canvas.drawRoundRect(6, y, 228, 20, 4, border);

  useAsciiFont();
  canvas.setTextColor(statusColor(task->status), fill);
  canvas.setCursor(12, y + 4);
  canvas.print(statusShort(task->status));

  useChineseFont12();
  canvas.setTextColor(WHITE, fill);
  canvas.setCursor(62, y + 4);
  canvas.print(clipText(task->title, 13));

  useAsciiFont();
  canvas.setTextColor(DARKGREY, fill);
  canvas.setCursor(184, y + 4);
  canvas.print("R");
  canvas.print(task->revision);
}

static void drawTaskList() {
  taskHeader("任务");
  drawSummaryStrip();

  int total = taskManager.taskCount();
  if (total <= 0) {
    useChineseFont16();
    canvas.setTextColor(LIGHTGREY, BLACK);
    canvas.setCursor(38, 52);
    canvas.print("暂无任务");

    useChineseFont12();
    canvas.setTextColor(DARKGREY, BLACK);
    canvas.setCursor(26, 78);
    canvas.print("请从 Core 同步任务列表");

    taskFooter("R Refresh | ESC Home");
    return;
  }

  int selected = taskManager.selectedIndex();
  int start = selected - 1;
  if (start < 0) start = 0;
  if (start > total - 3) start = total - 3;
  if (start < 0) start = 0;

  for (int i = 0; i < 3; i++) {
    int index = start + i;
    drawTaskRow(i, taskManager.taskAt(index), index == selected);
  }

  useAsciiFont();
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(166, 25);
  canvas.print(selected + 1);
  canvas.print("/");
  canvas.print(total);

  taskFooter("< > Select | Enter Detail | S/D/B");
}

static void printWrappedChinese(const String& text, int x, int y, int charsPerLine, int lines, int startLine, uint16_t color) {
  useChineseFont16();
  canvas.setTextColor(color, BLACK);
  String source = text;
  if (source.length() == 0) source = "暂无描述";

  int startChar = startLine * charsPerLine;
  for (int i = 0; i < lines; i++) {
    String part = utf8Slice(source, startChar + i * charsPerLine, charsPerLine);
    if (part.length() == 0) break;
    canvas.setCursor(x, y + i * 17);
    canvas.print(part);
  }
}

static void drawTaskDetail() {
  taskHeader("任务详情");

  const FtTaskItem* task = taskManager.selectedTask();
  if (task == nullptr) {
    useChineseFont16();
    canvas.setTextColor(LIGHTGREY, BLACK);
    canvas.setCursor(34, 52);
    canvas.print("未选择任务");
    taskFooter("ESC Back");
    return;
  }

  useAsciiFont();
  canvas.setTextColor(statusColor(task->status), BLACK);
  canvas.setCursor(8, 26);
  canvas.print(statusShort(task->status));
  canvas.setTextColor(DARKGREY, BLACK);
  canvas.setCursor(58, 26);
  canvas.print("P ");
  canvas.print(task->priority);
  canvas.print(" R");
  canvas.print(task->revision);

  // Keep the detail page readable on the small FT-01 screen:
  // one bold title line, compact gap, and more breathing room above the footer.
  printChinese16Bold(clipText(task->title, 13), 8, 38, WHITE, BLACK);

  const int charsPerLine = 13;
  const int visibleDescLines = 2;
  int descLines = (utf8CharCount(task->description) + charsPerLine - 1) / charsPerLine;
  if (descLines < 1) descLines = 1;
  int descPages = (descLines + visibleDescLines - 1) / visibleDescLines;
  if (descPages < 1) descPages = 1;
  if (taskDetailScrollLine > descPages - 1) taskDetailScrollLine = descPages - 1;
  if (taskDetailScrollLine < 0) taskDetailScrollLine = 0;

  printWrappedChinese(task->description, 8, 60, charsPerLine, visibleDescLines, taskDetailScrollLine * visibleDescLines, LIGHTGREY);

  if (descPages > 1) {
    useAsciiFont();
    canvas.setTextColor(DARKGREY, BLACK);
    canvas.setCursor(178, 101);
    canvas.print(taskDetailScrollLine + 1);
    canvas.print("/");
    canvas.print(descPages);
    taskFooter("< > Page | S Doing | D Done | ESC");
  } else {
    taskFooter("S Doing | D Done | ESC");
  }
}

void tasksRefresh() {
  taskManager.refresh(sdReady);
}

void drawTasksScreen() {
  canvas.fillSprite(BLACK);
  if (taskDetailMode) drawTaskDetail();
  else drawTaskList();
  canvas.pushSprite(0, 0);
}

void handleTasksKey(const String& key) {
  if (taskIsEsc(key)) {
    if (taskDetailMode) {
      taskDetailMode = false;
      taskDetailScrollLine = 0;
      drawTasksScreen();
      return;
    }
    returnToHomeFromModule();
    return;
  }

  if (taskDetailMode) {
    const FtTaskItem* task = taskManager.selectedTask();
    const int charsPerLine = 13;
    const int visibleDescLines = 2;
    int maxPages = 1;
    if (task != nullptr) {
      int totalLines = (utf8CharCount(task->description) + charsPerLine - 1) / charsPerLine;
      if (totalLines < 1) totalLines = 1;
      maxPages = (totalLines + visibleDescLines - 1) / visibleDescLines;
      if (maxPages < 1) maxPages = 1;
    }

    if (taskIsLeft(key)) {
      taskDetailScrollLine--;
      if (taskDetailScrollLine < 0) taskDetailScrollLine = 0;
    } else if (taskIsRight(key)) {
      taskDetailScrollLine++;
      if (taskDetailScrollLine > maxPages - 1) taskDetailScrollLine = maxPages - 1;
    } else if (taskIsEnter(key)) {
      taskDetailMode = false;
      taskDetailScrollLine = 0;
    } else if (taskHasLetter(key, 'r', 'R')) {
      tasksRefresh();
      taskDetailScrollLine = 0;
    } else if (taskHasLetter(key, 's', 'S')) {
      taskManager.setSelectedStatus("in_progress", sdReady);
    } else if (taskHasLetter(key, 'd', 'D')) {
      taskManager.setSelectedStatus("completed", sdReady);
    } else if (taskHasLetter(key, 'b', 'B')) {
      taskManager.setSelectedStatus("blocked", sdReady);
    }

    drawTasksScreen();
    return;
  }

  if (taskIsLeft(key)) {
    taskManager.moveSelected(-1);
  } else if (taskIsRight(key)) {
    taskManager.moveSelected(1);
  } else if (taskIsEnter(key)) {
    if (taskManager.taskCount() > 0) {
      taskDetailMode = true;
      taskDetailScrollLine = 0;
    }
  } else if (taskHasLetter(key, 'r', 'R')) {
    tasksRefresh();
  } else if (taskHasLetter(key, 's', 'S')) {
    taskManager.setSelectedStatus("in_progress", sdReady);
  } else if (taskHasLetter(key, 'd', 'D')) {
    taskManager.setSelectedStatus("completed", sdReady);
  } else if (taskHasLetter(key, 'b', 'B')) {
    taskManager.setSelectedStatus("blocked", sdReady);
  }

  drawTasksScreen();
}
