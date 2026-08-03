#pragma once
#include <Arduino.h>

struct SyncManifestStats {
  int pathPoints;
  int fieldEvents;
  int bootLogs;
  int audioIndex;
  int audioFiles;
  int taskReports;
  uint64_t audioBytes;
};

struct SyncAudioIndexMeta {
  bool found;
  String sessionId;
  String deviceDate;
  String deviceTime;
};

class SyncManager {
public:
  void begin(const char* deviceId, const char* version);
  bool refresh(bool sdReady);
  void printUsbHello(bool sdReady, bool gnssFix, int satellites, const String& sdStatusText);
  void printManifest(bool sdReady);
  bool printRecords(const String& recordType, bool sdReady);
  bool printAudioFile(const String& filename, bool sdReady);
  bool clearSyncedRecords(const String& recordTypes, bool sdReady);
  bool deleteUploadedAudio(const String& filename, bool sdReady);

  const SyncManifestStats& stats() const { return manifestStats; }
  const String& lastStatus() const { return statusText; }
  const String& deviceId() const { return deviceIdText; }

private:
  String deviceIdText;
  String versionText;
  String syncSessionId;
  String statusText = "READY";
  SyncManifestStats manifestStats = {0, 0, 0, 0, 0, 0, 0};

  const char* recordTypeToPath(const String& recordType);
  const char* cleanupRecordTypeToPath(const String& recordType);
  bool ensureCleanupDirs();
  bool ensureDir(const char* path);
  bool truncateRecordFile(const char* path, String& errorOut);
  bool retainLastJsonlLines(const char* path, int keepLines, String& errorOut, int& retainedOut);
  int countJsonlLines(const char* path);
  int countAudioFiles(uint64_t& bytesOut);
  String makeSafeSessionId();
  SyncAudioIndexMeta findAudioIndexMeta(const String& filename);
  String buildStableAudioId(const String& filename, uint32_t size, const SyncAudioIndexMeta& meta);
};
