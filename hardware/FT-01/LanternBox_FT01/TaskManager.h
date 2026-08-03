#pragma once
#include <Arduino.h>

struct FtTaskItem {
  bool valid;
  String taskId;
  String title;
  String description;
  String status;
  String priority;
  String updatedAt;
  int revision;
};

struct TaskStats {
  int total;
  int pending;
  int inProgress;
  int completed;
  int blocked;
  int reports;
};

class TaskManager {
public:
  void begin(const char* deviceId);
  bool refresh(bool sdReady);
  bool ensureTaskDirs(bool sdReady);

  int taskCount() const { return count; }
  int selectedIndex() const { return selected; }
  void moveSelected(int delta);
  const FtTaskItem* selectedTask() const;
  const FtTaskItem* taskAt(int index) const;
  const TaskStats& stats() const { return taskStats; }
  const String& lastStatus() const { return statusText; }

  bool setSelectedStatus(const String& newStatus, bool sdReady);
  bool appendStatusReport(const FtTaskItem& task, const String& newStatus, const String& note, bool sdReady);

  bool startReceiveTasks(int expectedCount, bool sdReady);
  bool receiveTaskLine(const String& line);
  bool finishReceiveTasks(bool sdReady);
  bool abortReceiveTasks();
  bool isReceivingTasks() const { return receivingTasks; }
  int expectedReceiveCount() const { return receiveExpected; }
  int receivedReceiveCount() const { return receiveReceived; }
  int storedReceiveCount() const { return receiveStored; }
  bool receiveHadError() const { return receiveHadErrorFlag; }
  const String& receiveErrorText() const { return receiveError; }

  bool printTaskReports(bool sdReady);
  int countTaskReports(bool sdReady);

private:
  static const int MAX_TASKS = 24;
  FtTaskItem tasks[MAX_TASKS];
  int count = 0;
  int selected = 0;
  String deviceIdText = "FT01-0001";
  String statusText = "READY";
  TaskStats taskStats = {0, 0, 0, 0, 0, 0};

  bool receivingTasks = false;
  int receiveExpected = 0;
  int receiveReceived = 0;
  int receiveStored = 0;
  bool receiveHadErrorFlag = false;
  String receiveError = "";
  String receiveBuffer[MAX_TASKS];

  String normalizeStatus(const String& value);
  void clearTasks();
  void applyReportOverlay(bool sdReady);
  void updateStats(bool sdReady);
};
