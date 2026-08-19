#pragma once

#include <Arduino.h>
#include <Preferences.h>

class CompassCalibration {
public:
  enum class State : uint8_t {
    IDLE = 0,
    RUNNING,
    READY,
    SAVED,
    CANCELED,
    FAILED
  };

  struct Params {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;
    uint8_t quality = 0;
    bool valid = false;
  };

  CompassCalibration();

  bool begin();
  bool start();
  bool cancel();
  bool reset();
  bool save();

  void addSample(int16_t x, int16_t y, int16_t z);
  void apply(float rawX, float rawY, float rawZ,
             float &x, float &y, float &z) const;

  bool isRunning() const;
  bool isSessionActive() const;
  bool isCalibrated() const;
  State state() const;
  const char *stateName() const;

  uint8_t progress() const;
  uint8_t sessionQuality() const;
  uint8_t savedQuality() const;
  uint32_t samples() const;

  int32_t minX() const;
  int32_t maxX() const;
  int32_t minY() const;
  int32_t maxY() const;
  int32_t minZ() const;
  int32_t maxZ() const;

  const Params &params() const;

private:
  static constexpr uint32_t MAGIC = 0x514D4332UL; // "QMC2"
  static constexpr uint16_t VERSION = 1;
  static constexpr uint32_t MIN_SAMPLES_Q1 = 100;
  static constexpr uint32_t MIN_SAMPLES_Q2 = 400;
  static constexpr uint32_t MIN_SAMPLES_Q3 = 800;
  static constexpr int32_t MIN_SPAN_Q1 = 200;
  static constexpr int32_t MIN_SPAN_Q2 = 600;
  static constexpr int32_t MIN_SPAN_Q3 = 900;

  struct PersistBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t generation;
    float offsetX;
    float offsetY;
    float offsetZ;
    float scaleX;
    float scaleY;
    float scaleZ;
    uint8_t quality;
    uint8_t valid;
    uint16_t reserved;
    uint32_t crc32;
  };

  Preferences prefs;
  Params active;
  State currentState;
  uint32_t currentGeneration;
  uint8_t activeSlot;

  uint32_t sampleCount;
  int32_t sampleMinX;
  int32_t sampleMaxX;
  int32_t sampleMinY;
  int32_t sampleMaxY;
  int32_t sampleMinZ;
  int32_t sampleMaxZ;
  uint8_t octantMask;
  uint8_t progressValue;
  uint8_t qualityValue;

  void resetSessionStats();
  void evaluateSession();
  bool buildCandidate(Params &out) const;

  bool readSlot(uint8_t slot, PersistBlob &blob);
  bool writeSlot(uint8_t slot, const PersistBlob &blob);
  bool validateBlob(const PersistBlob &blob) const;
  void blobToParams(const PersistBlob &blob, Params &out) const;
  PersistBlob paramsToBlob(const Params &p, uint32_t generation) const;

  static uint32_t crc32(const uint8_t *data, size_t len);
  static uint8_t popcount8(uint8_t v);
};
