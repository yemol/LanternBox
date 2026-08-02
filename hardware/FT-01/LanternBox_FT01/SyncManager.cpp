#include "SyncManager.h"
#include <Arduino.h>
#include <SD.h>

extern void prepareSharedSpiBusForSD();
extern String currentDeviceDateText();
extern String currentDeviceTimeText();

static const char* PATH_POINTS_FILE = "/lanternbox/tracks/path_points.jsonl";
static const char* FIELD_EVENTS_FILE = "/lanternbox/logs/field_events.jsonl";
static const char* BOOT_LOG_FILE = "/lanternbox/logs/boot.jsonl";
static const char* AUDIO_INDEX_FILE = "/lanternbox/audio/index.jsonl";
static const char* AUDIO_DIR = "/lanternbox/audio";
static const char* TASK_REPORTS_FILE = "/lanternbox/tasks/task_reports.jsonl";


static bool isRealAudioWavFile(const String& filename) {
  if (filename.length() == 0) return false;
  if (filename.startsWith(".")) return false;
  if (filename.startsWith("._")) return false;
  if (filename.indexOf("/.") >= 0) return false;

  String lower = filename;
  lower.toLowerCase();
  if (!lower.endsWith(".wav")) return false;

  // First-phase recorder files use audio_###.wav. This excludes macOS resource fork files
  // and unrelated wav files accidentally copied to the SD card.
  if (!lower.startsWith("audio_")) return false;

  return true;
}

void SyncManager::begin(const char* deviceId, const char* version) {
  deviceIdText = String(deviceId);
  versionText = String(version);
  syncSessionId = makeSafeSessionId();
  statusText = "READY";
}

String SyncManager::makeSafeSessionId() {
  String s = deviceIdText;
  s.replace(" ", "_");
  s.replace(":", "_");
  s.replace("/", "_");
  s += "-";
  String date = currentDeviceDateText();
  String time = currentDeviceTimeText();
  date.replace("-", "");
  time.replace(":", "");
  s += date;
  s += "-";
  s += time;
  return s;
}

String SyncManager::jsonEscape(const String& value) {
  String out = "";
  for (int i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else out += c;
  }
  return out;
}

String SyncManager::baseNameFromPath(const String& path) {
  int idx = path.lastIndexOf('/');
  if (idx < 0) return path;
  return path.substring(idx + 1);
}


const char* SyncManager::recordTypeToPath(const String& recordType) {
  if (recordType == "path_points") return PATH_POINTS_FILE;
  if (recordType == "field_events") return FIELD_EVENTS_FILE;
  if (recordType == "boot_logs") return BOOT_LOG_FILE;
  if (recordType == "audio_index") return AUDIO_INDEX_FILE;
  if (recordType == "task_reports") return TASK_REPORTS_FILE;
  return nullptr;
}

const char* SyncManager::cleanupRecordTypeToPath(const String& recordType) {
  // Local cleanup is intentionally narrower than GET_RECORDS.
  // Do not clear tasks.jsonl, audio_index.jsonl, audio WAV files, base.json, or sessions here.
  // audio_index must stay paired with audio/*.wav; uploaded audio uses DELETE_UPLOADED_AUDIO.
  // boot_logs uses retainLastJsonlLines() instead of truncateRecordFile().
  if (recordType == "path_points") return PATH_POINTS_FILE;
  if (recordType == "field_events") return FIELD_EVENTS_FILE;
  if (recordType == "task_reports") return TASK_REPORTS_FILE;
  return nullptr;
}

bool SyncManager::ensureDir(const char* path) {
  if (SD.exists(path)) return true;
  return SD.mkdir(path);
}

bool SyncManager::ensureCleanupDirs() {
  if (!ensureDir("/lanternbox")) return false;
  if (!ensureDir("/lanternbox/tracks")) return false;
  if (!ensureDir("/lanternbox/logs")) return false;
  if (!ensureDir("/lanternbox/tasks")) return false;
  if (!ensureDir("/lanternbox/audio")) return false;
  return true;
}

bool SyncManager::truncateRecordFile(const char* path, String& errorOut) {
  errorOut = "";
  if (path == nullptr) {
    errorOut = "bad_path";
    return false;
  }

  // Remove and recreate as an empty JSONL file. This avoids append-mode
  // surprises on SD implementations and never touches directories.
  if (SD.exists(path)) {
    if (!SD.remove(path)) {
      errorOut = "remove_failed";
      return false;
    }
  }

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    errorOut = "open_failed";
    return false;
  }
  f.flush();
  f.close();
  return true;
}



bool SyncManager::retainLastJsonlLines(const char* path, int keepLines, String& errorOut, int& retainedOut) {
  errorOut = "";
  retainedOut = 0;

  if (path == nullptr) {
    errorOut = "bad_path";
    return false;
  }

  if (keepLines <= 0) {
    return truncateRecordFile(path, errorOut);
  }

  // This helper is currently used for boot_logs, where we intentionally keep
  // a tiny black-box tail instead of deleting all diagnostics. Keep the memory
  // footprint bounded on ESP32-S3 by capping the retained window.
  const int MAX_RETAINED_LINES = 20;
  if (keepLines > MAX_RETAINED_LINES) keepLines = MAX_RETAINED_LINES;

  // Missing boot log is not a cleanup failure. Create an empty JSONL file so
  // future appends still have a normal target.
  if (!SD.exists(path)) {
    File empty = SD.open(path, FILE_WRITE);
    if (!empty) {
      errorOut = "open_failed";
      return false;
    }
    empty.flush();
    empty.close();
    retainedOut = 0;
    return true;
  }

  File in = SD.open(path, FILE_READ);
  if (!in) {
    errorOut = "open_failed";
    return false;
  }

  String retained[MAX_RETAINED_LINES];
  int totalLines = 0;

  while (in.available()) {
    String line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    retained[totalLines % keepLines] = line;
    totalLines++;
    delay(1);
  }

  in.close();

  retainedOut = totalLines < keepLines ? totalLines : keepLines;
  if (totalLines <= keepLines) {
    return true;
  }

  if (SD.exists(path)) {
    if (!SD.remove(path)) {
      errorOut = "remove_failed";
      return false;
    }
  }

  File out = SD.open(path, FILE_WRITE);
  if (!out) {
    errorOut = "open_failed";
    return false;
  }

  int startIndex = totalLines % keepLines;
  for (int i = 0; i < retainedOut; i++) {
    int idx = (startIndex + i) % keepLines;
    out.println(retained[idx]);
    delay(1);
  }

  out.flush();
  out.close();
  return true;
}


bool SyncManager::isSafeAudioDeleteFilename(const String& filename) {
  if (!isRealAudioWavFile(filename)) return false;
  if (filename.indexOf('/') >= 0) return false;
  if (filename.indexOf('\\') >= 0) return false;
  if (filename.indexOf("..") >= 0) return false;
  if (filename.length() > 64) return false;
  return true;
}

bool SyncManager::rewriteAudioIndexWithoutFile(const String& filename, int& removedOut, String& errorOut) {
  removedOut = 0;
  errorOut = "";

  if (!ensureDir("/lanternbox")) {
    errorOut = "mkdir_lanternbox_failed";
    return false;
  }
  if (!ensureDir("/lanternbox/audio")) {
    errorOut = "mkdir_audio_failed";
    return false;
  }

  if (!SD.exists(AUDIO_INDEX_FILE)) {
    File empty = SD.open(AUDIO_INDEX_FILE, FILE_WRITE);
    if (!empty) {
      errorOut = "open_index_failed";
      return false;
    }
    empty.flush();
    empty.close();
    return true;
  }

  File in = SD.open(AUDIO_INDEX_FILE, FILE_READ);
  if (!in) {
    errorOut = "open_index_failed";
    return false;
  }

  const char* TEMP_INDEX_FILE = "/lanternbox/audio/index.tmp";
  if (SD.exists(TEMP_INDEX_FILE)) {
    SD.remove(TEMP_INDEX_FILE);
  }

  File out = SD.open(TEMP_INDEX_FILE, FILE_WRITE);
  if (!out) {
    in.close();
    errorOut = "open_temp_failed";
    return false;
  }

  while (in.available()) {
    String line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    String lineFilename = extractJsonStringValue(line, "filename");
    if (lineFilename.length() == 0) {
      lineFilename = baseNameFromPath(extractJsonStringValue(line, "path"));
    }
    if (lineFilename.length() == 0) {
      lineFilename = baseNameFromPath(extractJsonStringValue(line, "file"));
    }

    if (lineFilename == filename) {
      removedOut++;
    } else {
      out.println(line);
    }
    delay(1);
  }

  in.close();
  out.flush();
  out.close();

  if (SD.exists(AUDIO_INDEX_FILE)) {
    if (!SD.remove(AUDIO_INDEX_FILE)) {
      SD.remove(TEMP_INDEX_FILE);
      errorOut = "remove_index_failed";
      return false;
    }
  }

  if (!SD.rename(TEMP_INDEX_FILE, AUDIO_INDEX_FILE)) {
    errorOut = "rename_temp_failed";
    return false;
  }

  return true;
}

bool SyncManager::deleteUploadedAudio(const String& filenameInput, bool sdReady) {
  String filename = filenameInput;
  filename.trim();

  bool wavDeleted = false;
  bool wavMissing = false;
  bool indexRemoved = false;
  int indexRemovedCount = 0;
  String errorText = "";

  if (!sdReady) {
    Serial.print("FT01_SYNC_AUDIO_DELETE_ACK file=");
    Serial.print(filename.length() > 0 ? filename : String("unknown"));
    Serial.println(" wav_deleted=false index_removed=false ok=false error=sd_not_ready");
    statusText = "AUDIO CLEAN FAIL";
    return false;
  }

  prepareSharedSpiBusForSD();

  if (!isSafeAudioDeleteFilename(filename)) {
    Serial.print("FT01_SYNC_AUDIO_DELETE_ACK file=");
    Serial.print(filename.length() > 0 ? filename : String("unknown"));
    Serial.println(" wav_deleted=false index_removed=false ok=false error=bad_filename");
    statusText = "AUDIO CLEAN FAIL";
    return false;
  }

  if (!ensureDir("/lanternbox") || !ensureDir("/lanternbox/audio")) {
    Serial.print("FT01_SYNC_AUDIO_DELETE_ACK file=");
    Serial.print(filename);
    Serial.println(" wav_deleted=false index_removed=false ok=false error=mkdir_failed");
    statusText = "AUDIO CLEAN FAIL";
    return false;
  }

  String wavPath = String(AUDIO_DIR) + "/" + filename;
  if (SD.exists(wavPath.c_str())) {
    wavDeleted = SD.remove(wavPath.c_str());
    if (!wavDeleted) {
      errorText = "wav_remove_failed";
    }
  } else {
    wavMissing = true;
  }

  bool indexOk = false;
  if (errorText.length() == 0) {
    indexOk = rewriteAudioIndexWithoutFile(filename, indexRemovedCount, errorText);
    indexRemoved = indexRemovedCount > 0;
  }

  bool ok = (wavDeleted || wavMissing) && indexOk;

  Serial.print("FT01_SYNC_AUDIO_DELETE_ACK file=");
  Serial.print(filename);
  Serial.print(" wav_deleted=");
  Serial.print(wavDeleted ? "true" : "false");
  if (wavMissing) {
    Serial.print(" wav_missing=true");
  }
  Serial.print(" index_removed=");
  Serial.print(indexRemoved ? "true" : "false");
  Serial.print(" index_removed_count=");
  Serial.print(indexRemovedCount);
  Serial.print(" ok=");
  Serial.print(ok ? "true" : "false");
  if (!ok) {
    Serial.print(" error=");
    Serial.print(errorText.length() > 0 ? errorText : String("unknown"));
  }
  Serial.println();
  Serial.flush();

  refresh(sdReady);
  statusText = ok ? "AUDIO CLEAN OK" : "AUDIO CLEAN FAIL";
  return ok;
}

String SyncManager::extractJsonStringValue(const String& line, const String& key) {
  String needle = "\"" + key + "\"";
  int keyPos = line.indexOf(needle);
  if (keyPos < 0) return "";

  int colon = line.indexOf(':', keyPos + needle.length());
  if (colon < 0) return "";

  int firstQuote = line.indexOf('"', colon + 1);
  if (firstQuote < 0) return "";

  int secondQuote = firstQuote + 1;
  bool escaped = false;

  while (secondQuote < line.length()) {
    char c = line[secondQuote];
    if (c == '\\' && !escaped) {
      escaped = true;
    } else {
      if (c == '"' && !escaped) break;
      escaped = false;
    }
    secondQuote++;
  }

  if (secondQuote >= line.length()) return "";
  return line.substring(firstQuote + 1, secondQuote);
}

SyncAudioIndexMeta SyncManager::findAudioIndexMeta(const String& filename) {
  SyncAudioIndexMeta meta;
  meta.found = false;
  meta.sessionId = "unknown";
  meta.deviceDate = "";
  meta.deviceTime = "";
  if (!SD.exists(AUDIO_INDEX_FILE)) return meta;

  File f = SD.open(AUDIO_INDEX_FILE, FILE_READ);
  if (!f) return meta;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    String lineFilename = extractJsonStringValue(line, "filename");
    if (lineFilename.length() == 0) {
      lineFilename = baseNameFromPath(extractJsonStringValue(line, "path"));
    }
    if (lineFilename.length() == 0) {
      lineFilename = baseNameFromPath(extractJsonStringValue(line, "file"));
    }

    if (lineFilename == filename) {
      meta.found = true;

      String sessionId = extractJsonStringValue(line, "session_id");
      if (sessionId.length() > 0) meta.sessionId = sessionId;

      String deviceDate = extractJsonStringValue(line, "device_date");
      if (deviceDate.length() > 0) meta.deviceDate = deviceDate;

      String deviceTime = extractJsonStringValue(line, "device_time");
      if (deviceTime.length() > 0) meta.deviceTime = deviceTime;
      break;
    }
  }

  f.close();
  return meta;
}

String SyncManager::buildStableAudioId(const String& filename, uint32_t size, const SyncAudioIndexMeta& meta) {
  String stablePart = meta.sessionId;

  if (stablePart.length() == 0 || stablePart == "unknown") {
    if (meta.deviceDate.length() > 0 && meta.deviceTime.length() > 0) {
      stablePart = meta.deviceDate + "T" + meta.deviceTime;
      stablePart.replace(":", "");
      stablePart.replace("-", "");
      stablePart.replace(" ", "_");
    }
  }

  if (stablePart.length() == 0 || stablePart == "unknown") {
    stablePart = "unknown";
  }

  stablePart.replace(" ", "_");
  stablePart.replace("/", "_");
  stablePart.replace("\\", "_");
  stablePart.replace(":", "_");

  return deviceIdText + ":audio:" + stablePart + ":" + filename + ":" + String(size);
}

int SyncManager::countJsonlLines(const char* path) {
  if (!SD.exists(path)) return 0;

  File f = SD.open(path, FILE_READ);
  if (!f) return 0;

  int count = 0;
  bool hasData = false;

  while (f.available()) {
    char c = (char)f.read();
    if (c == '\n') {
      count++;
      hasData = false;
    } else if (c != '\r' && c != ' ' && c != '\t') {
      hasData = true;
    }
  }

  if (hasData) count++;
  f.close();
  return count;
}

int SyncManager::countAudioFiles(uint64_t& bytesOut) {
  bytesOut = 0;

  File root = SD.open(AUDIO_DIR);
  if (!root) return 0;

  int count = 0;
  File file = root.openNextFile();

  while (file) {
    if (!file.isDirectory()) {
      String name = baseNameFromPath(String(file.name()));
      if (isRealAudioWavFile(name)) {
        count++;
        bytesOut += file.size();
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
  return count;
}

bool SyncManager::refresh(bool sdReady) {
  syncSessionId = makeSafeSessionId();

  if (!sdReady) {
    manifestStats = {0, 0, 0, 0, 0, 0, 0};
    statusText = "NO SD";
    return false;
  }

  prepareSharedSpiBusForSD();

  manifestStats.pathPoints = countJsonlLines(PATH_POINTS_FILE);
  manifestStats.fieldEvents = countJsonlLines(FIELD_EVENTS_FILE);
  manifestStats.bootLogs = countJsonlLines(BOOT_LOG_FILE);
  manifestStats.audioIndex = countJsonlLines(AUDIO_INDEX_FILE);
  manifestStats.audioFiles = countAudioFiles(manifestStats.audioBytes);
  manifestStats.taskReports = countJsonlLines(TASK_REPORTS_FILE);

  statusText = "REFRESHED";
  return true;
}

void SyncManager::printUsbHello(bool sdReady, bool gnssFix, int satellites, const String& sdStatusText) {
  Serial.println();
  Serial.println("FT01_SYNC_HELLO");
  Serial.print("DEVICE_ID ");
  Serial.println(deviceIdText);
  Serial.print("VERSION ");
  Serial.println(versionText);
  Serial.println("TRANSPORT usb_serial");
  Serial.print("SYNC_SESSION ");
  Serial.println(syncSessionId);
  Serial.print("SD ");
  Serial.println(sdReady ? sdStatusText : "NO_SD");
  Serial.print("GNSS ");
  Serial.println(gnssFix ? "FIX" : "NOFIX");
  Serial.print("SATELLITES ");
  Serial.println(satellites);
  Serial.println("SYNC_V 0.1");
  Serial.println("END");
  Serial.println();

  statusText = "HELLO SENT";
}

void SyncManager::printManifest(bool sdReady) {
  refresh(sdReady);

  Serial.println();
  Serial.println("FT01_SYNC_MANIFEST_BEGIN");
  Serial.println("{");
  Serial.print("  \"device_id\":\"");
  Serial.print(jsonEscape(deviceIdText));
  Serial.println("\",");
  Serial.print("  \"firmware_version\":\"");
  Serial.print(jsonEscape(versionText));
  Serial.println("\",");
  Serial.print("  \"sync_session_id\":\"");
  Serial.print(jsonEscape(syncSessionId));
  Serial.println("\",");
  Serial.println("  \"transport\":\"usb_serial\",");
  Serial.println("  \"items\":{");
  Serial.print("    \"path_points\":{\"count\":");
  Serial.print(manifestStats.pathPoints);
  Serial.println("},");
  Serial.print("    \"field_events\":{\"count\":");
  Serial.print(manifestStats.fieldEvents);
  Serial.println("},");
  Serial.print("    \"boot_logs\":{\"count\":");
  Serial.print(manifestStats.bootLogs);
  Serial.println("},");
  Serial.print("    \"audio_index\":{\"count\":");
  Serial.print(manifestStats.audioIndex);
  Serial.println("},");

  Serial.println("    \"audio_files\":[");

  if (sdReady) {
    File root = SD.open(AUDIO_DIR);
    if (root) {
      bool first = true;
      File file = root.openNextFile();

      while (file) {
        if (!file.isDirectory()) {
          String filename = baseNameFromPath(String(file.name()));

          if (isRealAudioWavFile(filename)) {
            if (!first) Serial.println(",");
            first = false;

            uint32_t fileSize = (uint32_t)file.size();
            SyncAudioIndexMeta meta = findAudioIndexMeta(filename);
            String audioId = buildStableAudioId(filename, fileSize, meta);

            String sessionId = meta.sessionId;
            String deviceDate = meta.deviceDate.length() > 0 ? meta.deviceDate : currentDeviceDateText();
            String deviceTime = meta.deviceTime.length() > 0 ? meta.deviceTime : currentDeviceTimeText();

            Serial.print("      {\"audio_id\":\"");
            Serial.print(jsonEscape(audioId));
            Serial.print("\",\"filename\":\"");
            Serial.print(jsonEscape(filename));
            Serial.print("\",\"path\":\"/lanternbox/audio/");
            Serial.print(jsonEscape(filename));
            Serial.print("\",\"size\":");
            Serial.print(fileSize);
            Serial.print(",\"session_id\":\"");
            Serial.print(jsonEscape(sessionId));
            Serial.print("\"");
            Serial.print(",\"device_date\":\"");
            Serial.print(jsonEscape(deviceDate));
            Serial.print("\",\"device_time\":\"");
            Serial.print(jsonEscape(deviceTime));
            Serial.print("\"");
            Serial.print("}");
          }
        }

        file.close();
        file = root.openNextFile();
      }

      root.close();
    }
  }

  Serial.println();
  Serial.println("    ]");
  Serial.println("  }");
  Serial.println("}");
  Serial.println("FT01_SYNC_MANIFEST_END");
  Serial.println();

  statusText = "MANIFEST SENT";
}


bool SyncManager::printRecords(const String& recordType, bool sdReady) {
  if (!sdReady) {
    Serial.print("FT01_SYNC_RECORDS_ERROR ");
    Serial.print(recordType);
    Serial.println(" sd_not_ready");
    return false;
  }

  const char* path = recordTypeToPath(recordType);
  if (path == nullptr) {
    Serial.print("FT01_SYNC_RECORDS_ERROR ");
    Serial.print(recordType);
    Serial.println(" unsupported_record_type");
    return false;
  }

  prepareSharedSpiBusForSD();

  Serial.print("FT01_SYNC_RECORDS_BEGIN ");
  Serial.println(recordType);

  if (SD.exists(path)) {
    File f = SD.open(path, FILE_READ);
    if (f) {
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();

        // Keep dump as JSONL only. Empty lines are skipped.
        if (line.length() > 0) {
          Serial.println(line);
        }

        // Yield lightly to keep the device responsive during larger dumps.
        delay(1);
      }

      f.close();
    } else {
      Serial.print("{\"sync_dump_error\":\"open_failed\",\"record_type\":\"");
      Serial.print(jsonEscape(recordType));
      Serial.print("\",\"path\":\"");
      Serial.print(jsonEscape(String(path)));
      Serial.println("\"}");
    }
  }

  Serial.print("FT01_SYNC_RECORDS_END ");
  Serial.println(recordType);

  statusText = "RECORDS SENT";
  return true;
}


bool SyncManager::clearSyncedRecords(const String& recordTypes, bool sdReady) {
  int requested = 0;
  int cleared = 0;
  int failed = 0;

  Serial.println("FT01_SYNC_CLEANUP_BEGIN");

  if (!sdReady) {
    Serial.println("FT01_SYNC_CLEANUP_ITEM name=all cleared=false ok=false error=sd_not_ready");
    Serial.println("FT01_SYNC_CLEANUP_ACK ok=false cleared=0 failed=1");
    Serial.println("FT01_SYNC_CLEANUP_END");
    statusText = "CLEANUP FAIL";
    return false;
  }

  prepareSharedSpiBusForSD();

  if (!ensureCleanupDirs()) {
    Serial.println("FT01_SYNC_CLEANUP_ITEM name=all cleared=false ok=false error=mkdir_failed");
    Serial.println("FT01_SYNC_CLEANUP_ACK ok=false cleared=0 failed=1");
    Serial.println("FT01_SYNC_CLEANUP_END");
    statusText = "CLEANUP FAIL";
    return false;
  }

  String rest = recordTypes;
  rest.trim();

  while (rest.length() > 0) {
    int space = rest.indexOf(' ');
    String name;
    if (space < 0) {
      name = rest;
      rest = "";
    } else {
      name = rest.substring(0, space);
      rest = rest.substring(space + 1);
      rest.trim();
    }

    name.trim();
    if (name.length() == 0) continue;

    requested++;
    const char* path = cleanupRecordTypeToPath(name);
    String errorText = "";
    bool ok = false;
    int retained = -1;

    if (name == "boot_logs") {
      ok = retainLastJsonlLines(BOOT_LOG_FILE, 20, errorText, retained);
    } else if (name == "audio_index") {
      errorText = "audio_index_cleanup_disabled";
      ok = false;
    } else if (path == nullptr) {
      errorText = "unsupported_record_type";
      ok = false;
    } else {
      ok = truncateRecordFile(path, errorText);
    }

    Serial.print("FT01_SYNC_CLEANUP_ITEM name=");
    Serial.print(name);
    Serial.print(" cleared=");
    Serial.print(ok ? "true" : "false");
    Serial.print(" ok=");
    Serial.print(ok ? "true" : "false");
    if (retained >= 0) {
      Serial.print(" retained=");
      Serial.print(retained);
    }
    if (!ok) {
      Serial.print(" error=");
      Serial.print(errorText.length() > 0 ? errorText : String("unknown"));
      failed++;
    } else {
      cleared++;
    }
    Serial.println();
    Serial.flush();
    delay(2);
  }

  bool finalOk = requested > 0 && failed == 0;
  Serial.print("FT01_SYNC_CLEANUP_ACK ok=");
  Serial.print(finalOk ? "true" : "false");
  Serial.print(" cleared=");
  Serial.print(cleared);
  Serial.print(" failed=");
  Serial.println(failed);
  Serial.println("FT01_SYNC_CLEANUP_END");
  Serial.flush();

  refresh(sdReady);
  statusText = finalOk ? "CLEANUP OK" : "CLEANUP FAIL";
  return finalOk;
}


static int base64EncodeChunk(const uint8_t* input, int inputLen, char* output) {
  static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int out = 0;

  for (int i = 0; i < inputLen; i += 3) {
    uint32_t value = ((uint32_t)input[i]) << 16;
    bool hasSecond = (i + 1) < inputLen;
    bool hasThird = (i + 2) < inputLen;

    if (hasSecond) value |= ((uint32_t)input[i + 1]) << 8;
    if (hasThird) value |= input[i + 2];

    output[out++] = table[(value >> 18) & 0x3F];
    output[out++] = table[(value >> 12) & 0x3F];
    output[out++] = hasSecond ? table[(value >> 6) & 0x3F] : '=';
    output[out++] = hasThird ? table[value & 0x3F] : '=';
  }

  output[out] = '\0';
  return out;
}

bool SyncManager::printAudioFile(const String& requestedFilename, bool sdReady) {
  String filename = baseNameFromPath(requestedFilename);
  filename.trim();

  if (!sdReady) {
    Serial.print("FT01_SYNC_AUDIO_ERROR ");
    Serial.print(filename.length() ? filename : "unknown");
    Serial.println(" sd_not_ready");
    return false;
  }

  if (!isRealAudioWavFile(filename)) {
    Serial.print("FT01_SYNC_AUDIO_ERROR ");
    Serial.print(filename.length() ? filename : "unknown");
    Serial.println(" invalid_filename");
    return false;
  }

  prepareSharedSpiBusForSD();

  String path = String(AUDIO_DIR) + "/" + filename;
  if (!SD.exists(path.c_str())) {
    Serial.print("FT01_SYNC_AUDIO_ERROR ");
    Serial.print(filename);
    Serial.println(" not_found");
    return false;
  }

  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) {
    Serial.print("FT01_SYNC_AUDIO_ERROR ");
    Serial.print(filename);
    Serial.println(" open_failed");
    return false;
  }

  uint32_t fileSize = (uint32_t)f.size();
  Serial.print("FT01_SYNC_AUDIO_BEGIN ");
  Serial.print(filename);
  Serial.print(" ");
  Serial.println(fileSize);

  // Keep USB CDC text chunks short and paced. Larger base64 lines can be
  // truncated by host-side line reads on some macOS / ESP32-S3 USB stacks.
  // Read against the known file size instead of File.available(), because
  // available() can occasionally report end early while streaming from SD.
  static const int RAW_CHUNK_SIZE = 192;
  static uint8_t rawChunk[RAW_CHUNK_SIZE];
  static char encodedChunk[((RAW_CHUNK_SIZE + 2) / 3) * 4 + 1];

  uint32_t sentBytes = 0;
  int emptyReadRetries = 0;

  while (sentBytes < fileSize) {
    uint32_t remaining = fileSize - sentBytes;
    int wanted = remaining > RAW_CHUNK_SIZE ? RAW_CHUNK_SIZE : (int)remaining;
    int readLen = f.read(rawChunk, wanted);

    if (readLen <= 0) {
      emptyReadRetries += 1;
      if (emptyReadRetries > 8) {
        f.close();
        Serial.print("FT01_SYNC_AUDIO_ERROR ");
        Serial.print(filename);
        Serial.print(" short_read ");
        Serial.print(sentBytes);
        Serial.print("/");
        Serial.println(fileSize);
        return false;
      }
      delay(8);
      continue;
    }

    emptyReadRetries = 0;
    sentBytes += (uint32_t)readLen;

    int encodedLen = base64EncodeChunk(rawChunk, readLen, encodedChunk);
    Serial.write((const uint8_t*)encodedChunk, encodedLen);
    Serial.println();
    Serial.flush();
    delay(4);
  }

  f.close();

  Serial.print("FT01_SYNC_AUDIO_END ");
  Serial.print(filename);
  Serial.print(" ");
  Serial.println(sentBytes);

  statusText = "AUDIO SENT";
  return true;
}
