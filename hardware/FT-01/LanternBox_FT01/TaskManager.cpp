#include "TaskManager.h"
#include <Arduino.h>
#include <SD.h>
#include "FtTextUtil.h"

extern void prepareSharedSpiBusForSD();
extern String currentDeviceDateText();
extern String currentDeviceTimeText();

static const char* TASK_DIR = "/lanternbox/tasks";
static const char* TASKS_FILE = "/lanternbox/tasks/tasks.jsonl";
static const char* TASKS_TMP_FILE = "/lanternbox/tasks/tasks.tmp";
static const char* TASK_REPORTS_FILE = "/lanternbox/tasks/task_reports.jsonl";

void TaskManager::begin(const char* deviceId) {
  deviceIdText = String(deviceId);
  statusText = "READY";
  clearTasks();
}

void TaskManager::clearTasks() {
  for (int i = 0; i < MAX_TASKS; i++) {
    tasks[i].valid = false;
    tasks[i].taskId = "";
    tasks[i].title = "";
    tasks[i].description = "";
    tasks[i].status = "";
    tasks[i].priority = "";
    tasks[i].updatedAt = "";
    tasks[i].revision = 0;
  }
  count = 0;
  selected = 0;
}

String TaskManager::normalizeStatus(const String& value) {
  String s = value;
  s.trim();
  s.toLowerCase();
  if (s == "doing") return "in_progress";
  if (s == "done") return "completed";
  if (s == "assigned") return "pending";
  if (s == "blocked") return "blocked";
  if (s == "in_progress") return "in_progress";
  if (s == "completed") return "completed";
  if (s == "pending") return "pending";
  return s.length() ? s : "pending";
}

bool TaskManager::ensureTaskDirs(bool sdReady) {
  if (!sdReady) {
    statusText = "NO SD";
    return false;
  }
  prepareSharedSpiBusForSD();
  if (!SD.exists("/lanternbox")) SD.mkdir("/lanternbox");
  if (!SD.exists(TASK_DIR)) SD.mkdir(TASK_DIR);
  return SD.exists(TASK_DIR);
}

bool TaskManager::refresh(bool sdReady) {
  clearTasks();

  if (!ensureTaskDirs(sdReady)) {
    updateStats(sdReady);
    return false;
  }

  if (!SD.exists(TASKS_FILE)) {
    statusText = "NO TASKS";
    updateStats(sdReady);
    return true;
  }

  File f = SD.open(TASKS_FILE, FILE_READ);
  if (!f) {
    statusText = "OPEN FAIL";
    updateStats(sdReady);
    return false;
  }

  while (f.available() && count < MAX_TASKS) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || !line.startsWith("{")) continue;

    String taskId = ftJsonString(line, "task_id");
    if (taskId.length() == 0) taskId = ftJsonString(line, "id");
    if (taskId.length() == 0) continue;

    FtTaskItem& item = tasks[count];
    item.valid = true;
    item.taskId = taskId;
    item.title = ftJsonString(line, "title");
    item.description = ftJsonString(line, "description");
    item.status = normalizeStatus(ftJsonString(line, "status"));
    item.priority = ftJsonString(line, "priority");
    item.updatedAt = ftJsonString(line, "updated_at");
    item.revision = ftJsonInt(line, "revision", 0);

    if (item.title.length() == 0) item.title = item.taskId;
    if (item.priority.length() == 0) item.priority = "normal";
    count++;
  }

  f.close();
  applyReportOverlay(sdReady);
  updateStats(sdReady);
  statusText = "LOADED";
  return true;
}

void TaskManager::applyReportOverlay(bool sdReady) {
  if (!sdReady || !SD.exists(TASK_REPORTS_FILE)) return;

  File f = SD.open(TASK_REPORTS_FILE, FILE_READ);
  if (!f) return;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || !line.startsWith("{")) continue;
    String taskId = ftJsonString(line, "task_id");
    String status = normalizeStatus(ftJsonString(line, "status"));
    if (taskId.length() == 0 || status.length() == 0) continue;

    for (int i = 0; i < count; i++) {
      if (tasks[i].valid && tasks[i].taskId == taskId) {
        tasks[i].status = status;
        break;
      }
    }
  }

  f.close();
}

void TaskManager::updateStats(bool sdReady) {
  taskStats = {0, 0, 0, 0, 0, 0};
  taskStats.total = count;
  for (int i = 0; i < count; i++) {
    String status = normalizeStatus(tasks[i].status);
    if (status == "in_progress") taskStats.inProgress++;
    else if (status == "completed") taskStats.completed++;
    else if (status == "blocked") taskStats.blocked++;
    else taskStats.pending++;
  }
  taskStats.reports = countTaskReports(sdReady);
}

int TaskManager::countTaskReports(bool sdReady) {
  if (!sdReady || !SD.exists(TASK_REPORTS_FILE)) return 0;
  File f = SD.open(TASK_REPORTS_FILE, FILE_READ);
  if (!f) return 0;
  int result = 0;
  bool hasData = false;
  while (f.available()) {
    char c = (char)f.read();
    if (c == '\n') {
      result++;
      hasData = false;
    } else if (c != '\r' && c != ' ' && c != '\t') {
      hasData = true;
    }
  }
  if (hasData) result++;
  f.close();
  return result;
}

void TaskManager::moveSelected(int delta) {
  if (count <= 0) {
    selected = 0;
    return;
  }
  selected += delta;
  while (selected < 0) selected += count;
  while (selected >= count) selected -= count;
}

const FtTaskItem* TaskManager::taskAt(int index) const {
  if (index < 0 || index >= count) return nullptr;
  if (!tasks[index].valid) return nullptr;
  return &tasks[index];
}

const FtTaskItem* TaskManager::selectedTask() const {
  return taskAt(selected);
}

bool TaskManager::appendStatusReport(const FtTaskItem& task, const String& newStatus, const String& note, bool sdReady) {
  if (!ensureTaskDirs(sdReady)) return false;
  if (!task.valid || task.taskId.length() == 0) {
    statusText = "NO TASK";
    return false;
  }

  String date = currentDeviceDateText();
  String time = currentDeviceTimeText();
  String compactTime = time;
  compactTime.replace(":", "");
  String reportId = deviceIdText + ":task_report:" + task.taskId + ":" + date + "-" + compactTime + ":" + normalizeStatus(newStatus) + ":" + String(millis());

  File f = SD.open(TASK_REPORTS_FILE, FILE_APPEND);
  if (!f) {
    statusText = "REPORT FAIL";
    return false;
  }

  f.print("{\"report_id\":\"");
  f.print(ftJsonEscape(reportId));
  f.print("\",\"device_id\":\"");
  f.print(ftJsonEscape(deviceIdText));
  f.print("\",\"task_id\":\"");
  f.print(ftJsonEscape(task.taskId));
  f.print("\",\"status\":\"");
  f.print(ftJsonEscape(normalizeStatus(newStatus)));
  f.print("\",\"note\":\"");
  f.print(ftJsonEscape(note));
  f.print("\",\"device_date\":\"");
  f.print(ftJsonEscape(date));
  f.print("\",\"device_time\":\"");
  f.print(ftJsonEscape(time));
  f.print("\",\"source\":\"ft01\"}");
  f.println();
  f.close();

  statusText = "REPORT OK";
  return true;
}

bool TaskManager::setSelectedStatus(const String& newStatus, bool sdReady) {
  const FtTaskItem* current = selectedTask();
  if (current == nullptr) {
    statusText = "NO TASK";
    return false;
  }

  FtTaskItem copy = *current;
  bool ok = appendStatusReport(copy, newStatus, "", sdReady);
  if (ok) {
    tasks[selected].status = normalizeStatus(newStatus);
    updateStats(sdReady);
  }
  return ok;
}

bool TaskManager::startReceiveTasks(int expectedCount, bool sdReady) {
  if (!ensureTaskDirs(sdReady)) {
    receiveHadErrorFlag = true;
    receiveError = "sd_not_ready";
    return false;
  }
  if (receivingTasks) abortReceiveTasks();

  for (int i = 0; i < MAX_TASKS; i++) receiveBuffer[i] = "";
  receiveExpected = expectedCount;
  receiveReceived = 0;
  receiveStored = 0;
  receiveHadErrorFlag = false;
  receiveError = "";
  receivingTasks = true;
  statusText = "TASK RX";
  return true;
}

bool TaskManager::receiveTaskLine(const String& rawLine) {
  if (!receivingTasks) return false;

  String line = rawLine;
  line.trim();
  if (line.length() == 0) return true;

  if (!line.startsWith("{")) {
    statusText = "TASK RX BAD";
    receiveHadErrorFlag = true;
    receiveError = "bad_json_line";
    return false;
  }

  if (receiveReceived >= MAX_TASKS) {
    statusText = "TASK RX FULL";
    receiveHadErrorFlag = true;
    receiveError = "too_many_tasks";
    return false;
  }

  // Buffer task JSON in RAM first. Writing SD per incoming serial line can block
  // long enough for USB CDC input to drop following lines, including PUT_TASKS_END.
  receiveBuffer[receiveReceived] = line;
  receiveReceived++;
  statusText = "TASK RX " + String(receiveReceived) + "/" + String(receiveExpected);
  return true;
}

bool TaskManager::finishReceiveTasks(bool sdReady) {
  if (!receivingTasks) {
    receiveHadErrorFlag = true;
    receiveError = "not_receiving";
    return false;
  }
  receivingTasks = false;

  if (!ensureTaskDirs(sdReady)) {
    receiveHadErrorFlag = true;
    receiveError = "sd_not_ready";
    return false;
  }

  SD.remove(TASKS_TMP_FILE);
  File tmp = SD.open(TASKS_TMP_FILE, FILE_WRITE);
  if (!tmp) {
    statusText = "TASK SAVE FAIL";
    receiveHadErrorFlag = true;
    receiveError = "tmp_open_fail";
    return false;
  }

  receiveStored = 0;
  for (int i = 0; i < receiveReceived && i < MAX_TASKS; i++) {
    if (receiveBuffer[i].length() == 0) continue;
    tmp.println(receiveBuffer[i]);
    receiveStored++;
  }
  tmp.flush();
  tmp.close();

  if (SD.exists(TASKS_FILE)) SD.remove(TASKS_FILE);

  bool renamed = SD.rename(TASKS_TMP_FILE, TASKS_FILE);
  if (!renamed) {
    // Fallback copy for SD implementations where rename is unavailable.
    File src = SD.open(TASKS_TMP_FILE, FILE_READ);
    File dst = SD.open(TASKS_FILE, FILE_WRITE);
    if (!src || !dst) {
      if (src) src.close();
      if (dst) dst.close();
      statusText = "TASK SAVE FAIL";
      receiveHadErrorFlag = true;
      receiveError = "rename_copy_fail";
      return false;
    }
    while (src.available()) dst.write(src.read());
    src.close();
    dst.flush();
    dst.close();
    SD.remove(TASKS_TMP_FILE);
  }

  bool countMatches = (receiveExpected <= 0 || receiveReceived == receiveExpected) && (receiveStored == receiveReceived);
  if (receiveHadErrorFlag) countMatches = false;
  refresh(sdReady);
  statusText = countMatches ? "TASKS STORED" : "TASK COUNT BAD";
  if (!countMatches && receiveError.length() == 0) receiveError = "count_mismatch";
  return countMatches;
}

bool TaskManager::abortReceiveTasks() {
  receivingTasks = false;
  receiveExpected = 0;
  receiveReceived = 0;
  receiveStored = 0;
  receiveHadErrorFlag = false;
  receiveError = "";
  for (int i = 0; i < MAX_TASKS; i++) receiveBuffer[i] = "";
  SD.remove(TASKS_TMP_FILE);
  statusText = "TASK RX ABORT";
  return true;
}

bool TaskManager::printTaskReports(bool sdReady) {
  if (!sdReady) {
    Serial.println("FT01_SYNC_RECORDS_ERROR task_reports sd_not_ready");
    return false;
  }
  prepareSharedSpiBusForSD();
  Serial.println("FT01_SYNC_RECORDS_BEGIN task_reports");
  if (SD.exists(TASK_REPORTS_FILE)) {
    File f = SD.open(TASK_REPORTS_FILE, FILE_READ);
    if (f) {
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) Serial.println(line);
        delay(1);
      }
      f.close();
    }
  }
  Serial.println("FT01_SYNC_RECORDS_END task_reports");
  statusText = "REPORTS SENT";
  return true;
}
