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
    // Symmetric 2D soft-iron correction matrix. Heading is calculated from
    // the corrected horizontal X/Y vector; Z remains diagnostic only.
    float matrixXX = 1.0f;
    float matrixXY = 0.0f;
    float matrixYY = 1.0f;
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
  // v4 = geometric 2D ellipse fit with robust center + equal-angle weighting.
  // v1/v2/v3 slots are intentionally invalidated because their center/shape
  // estimators can bias heading after a manual calibration sweep.
  static constexpr uint16_t VERSION = 4;
  static constexpr uint32_t MIN_SAMPLES_Q1 = 100;
  static constexpr uint32_t MIN_SAMPLES_Q2 = 400;
  static constexpr uint32_t MIN_SAMPLES_Q3 = 800;
  static constexpr int32_t MIN_SPAN_Q1 = 200;
  static constexpr int32_t MIN_SPAN_Q2 = 600;
  static constexpr int32_t MIN_SPAN_Q3 = 900;

  static constexpr uint16_t SAMPLE_STORE_MAX = 1600;
  static constexpr uint8_t ANGLE_BIN_COUNT = 36;
  static constexpr uint8_t MIN_OCCUPIED_BINS = 24;
  static constexpr uint8_t EXTREME_AVG_MAX = 32;

  struct PersistBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t generation;
    float offsetX;
    float offsetY;
    float offsetZ;
    float matrixXX;
    float matrixXY;
    float matrixYY;
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
  uint8_t quadrantMask;
  uint8_t progressValue;
  uint8_t qualityValue;

  // A bounded ring keeps enough of the latest horizontal sweep to fit the
  // geometry at SAVE time without making calibration quality depend on how
  // long the user paused at any particular direction.
  int16_t storedX[SAMPLE_STORE_MAX];
  int16_t storedY[SAMPLE_STORE_MAX];
  uint16_t storedCount;
  uint16_t storedWrite;

  void resetSessionStats();
  void evaluateSession();
  bool buildCandidate(Params &out) const;

  bool readSlot(uint8_t slot, PersistBlob &blob);
  bool writeSlot(uint8_t slot, const PersistBlob &blob);
  bool validateBlob(const PersistBlob &blob) const;
  void blobToParams(const PersistBlob &blob, Params &out) const;
  PersistBlob paramsToBlob(const Params &p, uint32_t generation) const;

  static bool solve5(double a[5][6], double out[5]);
  static uint32_t crc32(const uint8_t *data, size_t len);
  static uint8_t popcount8(uint8_t v);
};
