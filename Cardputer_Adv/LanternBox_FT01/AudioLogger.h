// FT-01 Audio Logger module
// Responsibility:
// - Record WAV files to SD using small buffers / RTOS queue.
// - Play selected WAV files with playback gain.
// - Maintain /lanternbox/audio/index.jsonl.
// - Expose a small UI-facing API for UiLog.
//
// Keep this module independent from screen drawing.
// UI code belongs in UiLog.*.

#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>
#include <SD.h>

struct AudioLoggerGnssSnapshot {
  bool fix = false;
  int satellites = -1;
  double lat = 0.0;
  double lon = 0.0;
  String utcTime = "--";
  String utcDate = "--";
};

class AudioLogger {
public:
  enum State {
    IDLE,
    RECORDING,
    SAVING,
    PLAYING,
    ERROR_STATE
  };

  bool begin();
  void update();

  bool startRecord(
    const String& sessionId,
    const AudioLoggerGnssSnapshot& gnss,
    const String& deviceDate,
    const String& deviceTime
  );

  bool stopRecord();
  bool playLatest();
  bool playSelected();
  bool deleteSelected();
  void stopPlayback();
  void refreshList();
  void moveSelection(int delta);
  int listCount() const;
  int listIndex() const;
  String selectedFileName() const;
  String selectedFilePath() const;
  String listFileNameAt(int index) const;
  String listFilePathAt(int index) const;

  void gainUp();
  void gainDown();

  bool isRecording() const;
  bool isSaving() const;
  bool isPlaying() const;
  bool isBusy() const;

  const char* stateText() const;
  String lastFile() const;
  uint32_t samples() const;
  uint32_t droppedChunks() const;
  int gainX10() const;

private:
  struct AudioChunk {
    int16_t data[2048];
    size_t samples;
  };

  static const uint32_t SAMPLE_RATE = 16000;
  static const size_t RECORD_CHUNK = 2048;
  static const size_t PLAY_CHUNK = 4096;

  static const int QUEUE_LENGTH = 8;
  static const int STARTUP_DISCARD_CHUNKS = 4;
  static const uint32_t FADE_IN_SAMPLES = 1600;

  static const int PLAY_GAIN_MIN_X10 = 10;
  static const int PLAY_GAIN_MAX_X10 = 80;
  static const int PLAY_GAIN_STEP_X10 = 5;

  static const char* AUDIO_DIR;
  static const char* INDEX_FILE;

  void prepareSdBus();
  bool ensureAudioDirs();
  bool initMic();
  void closeMic();
  void writeWavHeader(File& f, uint32_t dataBytes);
  String nextAudioFilename();
  String latestAudioFilename();
  bool playFile(const String& target);
  String fileNameOnly(const String& path) const;
  void drainAudioQueue();
  void saveAudioIndex();
  void rewriteIndexExcluding(const String& deletedPath);
  void applyPlaybackGain(int16_t* data, size_t samples);

  static void micTaskThunk(void* arg);
  static void sdTaskThunk(void* arg);
  void micTask();
  void sdTask();

  QueueHandle_t audioQueue = nullptr;
  TaskHandle_t micTaskHandle = nullptr;
  TaskHandle_t sdTaskHandle = nullptr;

  File wavFile;

  AudioChunk micChunk;
  AudioChunk sdChunk;
  AudioChunk drainChunk;

  int16_t playBufferA[PLAY_CHUNK];
  int16_t playBufferB[PLAY_CHUNK];

  volatile State state = IDLE;
  volatile bool sdWriting = false;
  volatile bool micCapturing = false;
  volatile bool playbackStopRequested = false;

  bool micOK = false;
  bool begun = false;

  uint32_t totalSamples = 0;
  uint32_t dropped = 0;
  int startupDiscardRemaining = 0;
  uint32_t fadeInRemaining = 0;

  int playGain = 40; // default 4.0x

  String activeSessionId = "";
  String activeDeviceDate = "";
  String activeDeviceTime = "";
  AudioLoggerGnssSnapshot activeGnss;

  String lastAudioPath = "";

  static const int MAX_AUDIO_FILES = 20;
  String audioFiles[MAX_AUDIO_FILES];
  int audioFileCount = 0;
  int selectedIndex = 0;
};
