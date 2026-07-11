#include "AudioLogger.h"

#ifndef SD_SPI_CS_PIN
#define SD_SPI_CS_PIN 12
#endif

#ifndef LORA_NSS_PIN
#define LORA_NSS_PIN 5
#endif

#ifndef LORA_RST_PIN
#define LORA_RST_PIN 3
#endif

const char* AudioLogger::AUDIO_DIR = "/lanternbox/audio";
const char* AudioLogger::INDEX_FILE = "/lanternbox/audio/index.jsonl";

static String audioBaseNameLocal(const String& path) {
  int slash = path.lastIndexOf('/');
  if (slash >= 0 && slash + 1 < path.length()) {
    return path.substring(slash + 1);
  }
  return path;
}

static String audioNormalizePathLocal(const String& path, const String& dir) {
  if (path.startsWith("/")) return path;
  return dir + "/" + path;
}

bool AudioLogger::begin() {
  if (begun) return true;

  ensureAudioDirs();

  audioQueue = xQueueCreate(QUEUE_LENGTH, sizeof(AudioChunk));
  if (audioQueue == nullptr) {
    Serial.println("[AUDIO] QUEUE FAIL");
    state = ERROR_STATE;
    return false;
  }

  micOK = initMic();

  xTaskCreatePinnedToCore(
    micTaskThunk,
    "audio_mic",
    8192,
    this,
    2,
    &micTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    sdTaskThunk,
    "audio_sd",
    8192,
    this,
    1,
    &sdTaskHandle,
    0
  );

  begun = true;
  // Do not refresh the audio list at boot.
  // SD.exists() scanning can block the boot screen on some cards.
  Serial.println("[AUDIO] begin OK");
  return micOK;
}

void AudioLogger::update() {
  // Reserved for future periodic housekeeping.
}

void AudioLogger::prepareSdBus() {
  // AudioLogger also touches SD after boot.
  // Re-assert shared SPI idle state before SD.exists/open/read/write.
  pinMode(LORA_NSS_PIN, OUTPUT);
  digitalWrite(LORA_NSS_PIN, HIGH);

  pinMode(LORA_RST_PIN, OUTPUT);
  digitalWrite(LORA_RST_PIN, HIGH);

  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);

  delayMicroseconds(50);
}

bool AudioLogger::ensureAudioDirs() {
  prepareSdBus();
  if (!SD.exists("/lanternbox")) {
    if (!SD.mkdir("/lanternbox")) return false;
  }

  if (!SD.exists(AUDIO_DIR)) {
    if (!SD.mkdir(AUDIO_DIR)) return false;
  }

  return true;
}

bool AudioLogger::initMic() {
  M5Cardputer.Speaker.end();

  if (M5Cardputer.Mic.isEnabled()) {
    M5Cardputer.Mic.end();
  }

  auto cfg = M5Cardputer.Mic.config();
  cfg.sample_rate = SAMPLE_RATE;
  cfg.left_channel = 1;
  cfg.stereo = 0;
  cfg.magnification = 32;
  cfg.noise_filter_level = 0;

  M5Cardputer.Mic.config(cfg);

  bool ok = M5Cardputer.Mic.begin();
  Serial.println(ok ? "[AUDIO] MIC OK" : "[AUDIO] MIC FAIL");
  return ok;
}

void AudioLogger::closeMic() {
  if (M5Cardputer.Mic.isEnabled()) {
    M5Cardputer.Mic.end();
  }
}

void AudioLogger::writeWavHeader(File& f, uint32_t dataBytes) {
  uint32_t riffSize = dataBytes + 36;
  uint16_t audioFormat = 1;
  uint16_t channels = 1;
  uint16_t bitsPerSample = 16;
  uint32_t sampleRate = SAMPLE_RATE;
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  uint32_t fmtSize = 16;

  f.write((uint8_t*)"RIFF", 4);
  f.write((uint8_t*)&riffSize, 4);
  f.write((uint8_t*)"WAVE", 4);

  f.write((uint8_t*)"fmt ", 4);
  f.write((uint8_t*)&fmtSize, 4);

  f.write((uint8_t*)&audioFormat, 2);
  f.write((uint8_t*)&channels, 2);
  f.write((uint8_t*)&sampleRate, 4);
  f.write((uint8_t*)&byteRate, 4);
  f.write((uint8_t*)&blockAlign, 2);
  f.write((uint8_t*)&bitsPerSample, 2);

  f.write((uint8_t*)"data", 4);
  f.write((uint8_t*)&dataBytes, 4);
}

String AudioLogger::nextAudioFilename() {
  prepareSdBus();
  for (int i = 1; i <= 999; i++) {
    char path[48];
    snprintf(path, sizeof(path), "%s/audio_%03d.wav", AUDIO_DIR, i);

    if (!SD.exists(path)) {
      return String(path);
    }
  }

  return String(AUDIO_DIR) + "/audio_999.wav";
}

String AudioLogger::latestAudioFilename() {
  prepareSdBus();
  for (int i = 999; i >= 1; i--) {
    char path[48];
    snprintf(path, sizeof(path), "%s/audio_%03d.wav", AUDIO_DIR, i);

    if (SD.exists(path)) {
      return String(path);
    }
  }

  return "";
}

String AudioLogger::fileNameOnly(const String& path) const {
  int slash = path.lastIndexOf('/');
  if (slash >= 0 && slash + 1 < path.length()) {
    return path.substring(slash + 1);
  }

  return path;
}

void AudioLogger::refreshList() {
  prepareSdBus();

  audioFileCount = 0;

  if (!SD.exists(INDEX_FILE)) {
    selectedIndex = 0;
    Serial.println("[AUDIO] no index file, list empty");
    return;
  }

  File f = SD.open(INDEX_FILE, FILE_READ);
  if (!f) {
    selectedIndex = 0;
    Serial.println("[AUDIO] index open fail");
    return;
  }

  String validFiles[64];
  int validCount = 0;

  while (f.available() && validCount < 64) {
    String line = f.readStringUntil('\n');

    int p = line.indexOf("\"file\":\"");
    if (p < 0) continue;

    p += 8;
    int e = line.indexOf("\"", p);
    if (e <= p) continue;

    String fileValue = line.substring(p, e);
    String fullPath = audioNormalizePathLocal(fileValue, String(AUDIO_DIR));

    // Critical: do not show stale index entries.
    if (SD.exists(fullPath)) {
      validFiles[validCount++] = fullPath;
    }
  }

  f.close();

  // Newest is last in index. Show newest first.
  for (int i = validCount - 1; i >= 0 && audioFileCount < MAX_AUDIO_FILES; i--) {
    audioFiles[audioFileCount++] = validFiles[i];
  }

  if (audioFileCount == 0) {
    selectedIndex = 0;
  } else if (selectedIndex >= audioFileCount) {
    selectedIndex = audioFileCount - 1;
  }

  Serial.print("[AUDIO] list loaded valid count=");
  Serial.println(audioFileCount);
}

void AudioLogger::moveSelection(int delta) {
  if (state != IDLE) return;

  if (audioFileCount == 0) {
    refreshList();
  }

  if (audioFileCount <= 0) return;

  selectedIndex += delta;

  if (selectedIndex < 0) {
    selectedIndex = audioFileCount - 1;
  }

  if (selectedIndex >= audioFileCount) {
    selectedIndex = 0;
  }
}

int AudioLogger::listCount() const {
  return audioFileCount;
}

int AudioLogger::listIndex() const {
  return selectedIndex;
}

String AudioLogger::selectedFileName() const {
  if (audioFileCount <= 0 || selectedIndex < 0 || selectedIndex >= audioFileCount) {
    return "--";
  }

  return fileNameOnly(audioFiles[selectedIndex]);
}

String AudioLogger::selectedFilePath() const {
  if (audioFileCount <= 0 || selectedIndex < 0 || selectedIndex >= audioFileCount) {
    return "";
  }

  return audioFiles[selectedIndex];
}

String AudioLogger::listFileNameAt(int index) const {
  if (audioFileCount <= 0 || index < 0 || index >= audioFileCount) {
    return "--";
  }

  return fileNameOnly(audioFiles[index]);
}

String AudioLogger::listFilePathAt(int index) const {
  if (audioFileCount <= 0 || index < 0 || index >= audioFileCount) {
    return "";
  }

  return audioFiles[index];
}

void AudioLogger::drainAudioQueue() {
  while (audioQueue && xQueueReceive(audioQueue, &drainChunk, 0) == pdTRUE) {
    // discard stale chunks before a fresh recording
  }
}

bool AudioLogger::startRecord(
  const String& sessionId,
  const AudioLoggerGnssSnapshot& gnss,
  const String& deviceDate,
  const String& deviceTime
) {
  if (!begun) {
    if (!begin()) return false;
  }

  if (!micOK) {
    micOK = initMic();
    if (!micOK) {
      state = ERROR_STATE;
      return false;
    }
  }

  if (state != IDLE) {
    Serial.println("[AUDIO] start blocked: busy");
    return false;
  }

  if (!ensureAudioDirs()) {
    Serial.println("[AUDIO] start failed: dir");
    state = ERROR_STATE;
    return false;
  }

  drainAudioQueue();

  lastAudioPath = nextAudioFilename();

  prepareSdBus();
  wavFile = SD.open(lastAudioPath, FILE_WRITE);
  if (!wavFile) {
    Serial.println("[AUDIO] WAV open fail");
    state = ERROR_STATE;
    return false;
  }

  uint8_t emptyHeader[44] = {0};
  wavFile.write(emptyHeader, 44);

  totalSamples = 0;
  dropped = 0;
  startupDiscardRemaining = STARTUP_DISCARD_CHUNKS;
  fadeInRemaining = FADE_IN_SAMPLES;

  activeSessionId = sessionId;
  activeDeviceDate = deviceDate;
  activeDeviceTime = deviceTime;
  activeGnss = gnss;

  state = RECORDING;

  Serial.print("[AUDIO] REC START ");
  Serial.println(lastAudioPath);

  return true;
}

bool AudioLogger::stopRecord() {
  if (state != RECORDING) {
    Serial.println("[AUDIO] stop blocked: not recording");
    return false;
  }

  state = SAVING;

  unsigned long deadline = millis() + 2500;

  while ((micCapturing || sdWriting || (audioQueue && uxQueueMessagesWaiting(audioQueue) > 0)) &&
         millis() < deadline) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (wavFile) {
    wavFile.seek(0);
    writeWavHeader(wavFile, totalSamples * 2);
    wavFile.close();
  }

  saveAudioIndex();
  refreshList();

  Serial.print("[AUDIO] REC SAVED samples=");
  Serial.println(totalSamples);

  state = IDLE;
  return true;
}

void AudioLogger::saveAudioIndex() {
  prepareSdBus();
  File f = SD.open(INDEX_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("[AUDIO] INDEX FAIL");
    return;
  }

  float durationSec = (float)totalSamples / (float)SAMPLE_RATE;

  f.print("{\"session_id\":\"");
  f.print(activeSessionId);
  f.print("\",\"file\":\"");
  f.print(lastAudioPath);
  f.print("\",\"samples\":");
  f.print(totalSamples);
  f.print(",\"duration_sec\":");
  f.print(durationSec, 2);
  f.print(",\"dropped_chunks\":");
  f.print(dropped);
  f.print(",\"device_date\":\"");
  f.print(activeDeviceDate);
  f.print("\",\"device_time\":\"");
  f.print(activeDeviceTime);
  f.print("\",\"gnss_fix\":");
  f.print(activeGnss.fix ? "true" : "false");
  f.print(",\"satellites\":");
  f.print(activeGnss.satellites);

  if (activeGnss.fix) {
    f.print(",\"lat\":");
    f.print(activeGnss.lat, 6);
    f.print(",\"lon\":");
    f.print(activeGnss.lon, 6);
  }

  f.print(",\"gnss_utc_time\":\"");
  f.print(activeGnss.utcTime);
  f.print("\",\"gnss_utc_date\":\"");
  f.print(activeGnss.utcDate);
  f.println("\"}");

  f.close();

  Serial.println("[AUDIO] INDEX OK");
}

void AudioLogger::applyPlaybackGain(int16_t* data, size_t samples) {
  for (size_t i = 0; i < samples; i++) {
    int32_t v = (int32_t)data[i] * playGain;
    v = v / 10;

    if (v > 32767) {
      v = 32767;
    } else if (v < -32768) {
      v = -32768;
    }

    data[i] = (int16_t)v;
  }
}

void AudioLogger::stopPlayback() {
  if (state != PLAYING) return;

  playbackStopRequested = true;
  M5Cardputer.Speaker.stop();

  Serial.println("[AUDIO] playback stop requested");
}

bool AudioLogger::playLatest() {
  String target = lastAudioPath;
  if (target.length() == 0 || !SD.exists(target)) {
    target = latestAudioFilename();
  }

  return playFile(target);
}

void AudioLogger::rewriteIndexExcluding(const String& deletedPath) {
  prepareSdBus();

  String deletedName = audioBaseNameLocal(deletedPath);
  String deletedFull = audioNormalizePathLocal(deletedPath, String(AUDIO_DIR));

  File oldFile = SD.open(INDEX_FILE, FILE_READ);
  if (!oldFile) {
    Serial.println("[AUDIO] index rewrite skipped: no index");
    return;
  }

  String keep[64];
  int keepCount = 0;
  int removedCount = 0;

  while (oldFile.available() && keepCount < 64) {
    String line = oldFile.readStringUntil('\n');
    if (line.length() == 0) continue;

    int p = line.indexOf("\"file\":\"");
    bool keepLine = true;

    if (p >= 0) {
      p += 8;
      int e = line.indexOf("\"", p);

      if (e > p) {
        String fileValue = line.substring(p, e);
        String fileName = audioBaseNameLocal(fileValue);
        String fullPath = audioNormalizePathLocal(fileValue, String(AUDIO_DIR));

        bool isDeletedTarget =
          (fileValue == deletedPath) ||
          (fullPath == deletedFull) ||
          (fileName == deletedName);

        bool existsOnSd = SD.exists(fullPath);

        // Drop exact deleted target and drop stale missing WAV entries.
        if (isDeletedTarget || !existsOnSd) {
          keepLine = false;
          removedCount++;
        }
      }
    }

    if (keepLine) {
      keep[keepCount++] = line;
    }
  }

  oldFile.close();

  prepareSdBus();
  SD.remove(INDEX_FILE);

  File newFile = SD.open(INDEX_FILE, FILE_WRITE);
  if (!newFile) {
    Serial.println("[AUDIO] index rewrite failed: open write");
    return;
  }

  for (int i = 0; i < keepCount; i++) {
    newFile.println(keep[i]);
  }

  newFile.close();

  Serial.print("[AUDIO] index rewrite keep=");
  Serial.print(keepCount);
  Serial.print(" removed=");
  Serial.println(removedCount);
}

bool AudioLogger::deleteSelected() {
  if (state != IDLE) {
    Serial.println("[AUDIO] delete blocked: busy");
    return false;
  }

  if (audioFileCount == 0) {
    refreshList();
  }

  if (audioFileCount <= 0 ||
      selectedIndex < 0 ||
      selectedIndex >= audioFileCount) {
    Serial.println("[AUDIO] delete no selection");
    return false;
  }

  String target = audioFiles[selectedIndex];
  String targetName = audioBaseNameLocal(target);
  String targetFull = audioNormalizePathLocal(target, String(AUDIO_DIR));
prepareSdBus();

  bool fileRemoved = false;

  if (SD.exists(targetFull)) {
    fileRemoved = SD.remove(targetFull);
    Serial.print("[AUDIO] wav remove ");
} else {
// Treat as cleanup success, but still remove its index record.
    fileRemoved = true;
  }

  if (!fileRemoved) {
    return false;
  }

  // This is the important part: persistently remove the index entry.
  rewriteIndexExcluding(targetFull);

  refreshList();

  if (audioFileCount == 0) {
    selectedIndex = 0;
  } else if (selectedIndex >= audioFileCount) {
    selectedIndex = audioFileCount - 1;
  }

  return true;
}

bool AudioLogger::playSelected() {
  if (audioFileCount == 0) {
    refreshList();
  }

  if (audioFileCount <= 0 || selectedIndex < 0 || selectedIndex >= audioFileCount) {
    Serial.println("[AUDIO] selected play no file");
    return false;
  }

  return playFile(audioFiles[selectedIndex]);
}

bool AudioLogger::playFile(const String& target) {
  prepareSdBus();

  if (state != IDLE) {
    Serial.println("[AUDIO] play blocked: busy");
    return false;
  }

  if (target.length() == 0 || !SD.exists(target)) {
    Serial.println("[AUDIO] PLAY no file");
    return false;
  }

  state = PLAYING;
  playbackStopRequested = false;

  Serial.print("[AUDIO] PLAY START ");
  Serial.println(target);

  prepareSdBus();
  File f = SD.open(target, FILE_READ);
  if (!f) {
    Serial.println("[AUDIO] PLAY open fail");
    state = IDLE;
    return false;
  }

  closeMic();

  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(255);

  f.seek(44);

  int16_t* current = playBufferA;
  int16_t* next = playBufferB;

  size_t currentBytes = f.read((uint8_t*)current, PLAY_CHUNK * sizeof(int16_t));
  if (currentBytes > 0) {
    applyPlaybackGain(current, currentBytes / 2);
  }

  if (currentBytes == 0) {
    f.close();
    M5Cardputer.Speaker.end();
    micOK = initMic();
    state = IDLE;
    Serial.println("[AUDIO] PLAY empty");
    return false;
  }

  while (currentBytes > 0) {
    size_t currentSamples = currentBytes / 2;

    M5Cardputer.Speaker.playRaw(
      current,
      currentSamples,
      SAMPLE_RATE,
      false,
      1,
      0
    );

    size_t nextBytes = 0;
    if (f.available()) {
      nextBytes = f.read((uint8_t*)next, PLAY_CHUNK * sizeof(int16_t));
      if (nextBytes > 0) {
        applyPlaybackGain(next, nextBytes / 2);
      }
    }

    while (M5Cardputer.Speaker.isPlaying()) {
      M5Cardputer.update();

      if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        String k = "";
        auto keys = M5Cardputer.Keyboard.keysState().word;
        for (auto c : keys) {
          k += c;
        }

        if (k.indexOf('p') >= 0 || k.indexOf('P') >= 0) {
          playbackStopRequested = true;
          M5Cardputer.Speaker.stop();
          break;
        }
      }

      if (playbackStopRequested) {
        M5Cardputer.Speaker.stop();
        break;
      }

      delay(1);
    }

    if (playbackStopRequested) {
      break;
    }

    currentBytes = nextBytes;

    int16_t* tmp = current;
    current = next;
    next = tmp;
  }

  f.close();

  M5Cardputer.Speaker.end();
  micOK = initMic();

  playbackStopRequested = false;
  state = IDLE;

  Serial.println("[AUDIO] PLAY END");
  return true;
}

void AudioLogger::gainUp() {
  int next = playGain + PLAY_GAIN_STEP_X10;
  if (next > PLAY_GAIN_MAX_X10) next = PLAY_GAIN_MAX_X10;
  playGain = next;

  Serial.print("[AUDIO] gain=");
  Serial.print(playGain / 10);
  Serial.print(".");
  Serial.println(playGain % 10);
}

void AudioLogger::gainDown() {
  int next = playGain - PLAY_GAIN_STEP_X10;
  if (next < PLAY_GAIN_MIN_X10) next = PLAY_GAIN_MIN_X10;
  playGain = next;

  Serial.print("[AUDIO] gain=");
  Serial.print(playGain / 10);
  Serial.print(".");
  Serial.println(playGain % 10);
}

bool AudioLogger::isRecording() const {
  return state == RECORDING;
}

bool AudioLogger::isSaving() const {
  return state == SAVING;
}

bool AudioLogger::isPlaying() const {
  return state == PLAYING;
}

bool AudioLogger::isBusy() const {
  return state == RECORDING || state == SAVING || state == PLAYING;
}

const char* AudioLogger::stateText() const {
  switch (state) {
    case IDLE: return "IDLE";
    case RECORDING: return "RECORDING";
    case SAVING: return "SAVING";
    case PLAYING: return "PLAYING";
    case ERROR_STATE: return "ERROR";
  }

  return "UNKNOWN";
}

String AudioLogger::lastFile() const {
  return lastAudioPath;
}

uint32_t AudioLogger::samples() const {
  return totalSamples;
}

uint32_t AudioLogger::droppedChunks() const {
  return dropped;
}

int AudioLogger::gainX10() const {
  return playGain;
}

void AudioLogger::micTaskThunk(void* arg) {
  static_cast<AudioLogger*>(arg)->micTask();
}

void AudioLogger::sdTaskThunk(void* arg) {
  static_cast<AudioLogger*>(arg)->sdTask();
}

void AudioLogger::micTask() {
  while (true) {
    if (state == RECORDING) {
      micCapturing = true;

      bool ok = M5Cardputer.Mic.record(
        micChunk.data,
        RECORD_CHUNK,
        SAMPLE_RATE
      );

      micCapturing = false;

      if (ok) {
        micChunk.samples = RECORD_CHUNK;

        if (xQueueSend(audioQueue, &micChunk, 0) != pdTRUE) {
          dropped++;
        }
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

void AudioLogger::sdTask() {
  while (true) {
    if (xQueueReceive(audioQueue, &sdChunk, portMAX_DELAY) == pdTRUE) {
      if (wavFile && (state == RECORDING || state == SAVING)) {
        sdWriting = true;

        if (startupDiscardRemaining > 0) {
          startupDiscardRemaining--;
          sdWriting = false;
          continue;
        }

        if (fadeInRemaining > 0) {
          for (size_t i = 0; i < sdChunk.samples && fadeInRemaining > 0; i++) {
            uint32_t done = FADE_IN_SAMPLES - fadeInRemaining;
            int32_t sample = sdChunk.data[i];
            sample = (sample * (int32_t)done) / (int32_t)FADE_IN_SAMPLES;
            sdChunk.data[i] = (int16_t)sample;
            fadeInRemaining--;
          }
        }

        wavFile.write((uint8_t*)sdChunk.data, sdChunk.samples * 2);
        totalSamples += sdChunk.samples;

        sdWriting = false;
      }
    }
  }
}
