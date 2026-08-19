#include "CompassCalibration.h"

#include <math.h>
#include <string.h>
#include <stddef.h>

CompassCalibration::CompassCalibration()
  : currentState(State::IDLE),
    currentGeneration(0),
    activeSlot(0),
    sampleCount(0),
    sampleMinX(0), sampleMaxX(0),
    sampleMinY(0), sampleMaxY(0),
    sampleMinZ(0), sampleMaxZ(0),
    octantMask(0), progressValue(0), qualityValue(0) {}

bool CompassCalibration::begin() {
  active = Params{};
  currentGeneration = 0;
  activeSlot = 0;
  currentState = State::IDLE;
  resetSessionStats();

  if (!prefs.begin("qmc_cal", false)) {
    return false;
  }

  PersistBlob a{}, b{};
  bool va = readSlot(0, a) && validateBlob(a);
  bool vb = readSlot(1, b) && validateBlob(b);

  if (!va && !vb) return true;

  const PersistBlob *best = nullptr;
  uint8_t slot = 0;
  if (va && vb) {
    if ((int32_t)(b.generation - a.generation) > 0) {
      best = &b;
      slot = 1;
    } else {
      best = &a;
      slot = 0;
    }
  } else if (va) {
    best = &a;
    slot = 0;
  } else {
    best = &b;
    slot = 1;
  }

  blobToParams(*best, active);
  currentGeneration = best->generation;
  activeSlot = slot;
  return true;
}

bool CompassCalibration::start() {
  if (currentState == State::RUNNING || currentState == State::READY) {
    return false;
  }
  resetSessionStats();
  currentState = State::RUNNING;
  return true;
}

bool CompassCalibration::cancel() {
  if (currentState != State::RUNNING && currentState != State::READY) {
    return false;
  }
  // Keep the session statistics only for the immediate CANCELED status report.
  // They are never applied and are reset on the next START.
  currentState = State::CANCELED;
  return true;
}

bool CompassCalibration::reset() {
  bool ok0 = prefs.remove("cal0");
  bool ok1 = prefs.remove("cal1");
  // Preferences::remove returns false when a key does not exist, so verify by readback.
  PersistBlob tmp{};
  bool remains0 = readSlot(0, tmp);
  bool remains1 = readSlot(1, tmp);
  if (remains0 || remains1) return false;

  active = Params{};
  currentGeneration = 0;
  activeSlot = 0;
  resetSessionStats();
  currentState = State::IDLE;
  (void)ok0;
  (void)ok1;
  return true;
}

bool CompassCalibration::save() {
  if (currentState != State::RUNNING && currentState != State::READY) {
    return false;
  }

  evaluateSession();
  if (qualityValue < 2) return false;

  Params candidate;
  if (!buildCandidate(candidate)) {
    currentState = State::FAILED;
    return false;
  }

  const uint8_t targetSlot = active.valid ? (uint8_t)(1U - activeSlot) : 0;
  const uint32_t nextGeneration = currentGeneration + 1U;
  PersistBlob blob = paramsToBlob(candidate, nextGeneration);

  if (!writeSlot(targetSlot, blob)) {
    currentState = State::FAILED;
    return false;
  }

  PersistBlob verify{};
  if (!readSlot(targetSlot, verify) || !validateBlob(verify) ||
      verify.generation != nextGeneration) {
    currentState = State::FAILED;
    return false;
  }

  // Old slot is deliberately kept as a rollback copy. The newest valid generation wins.
  active = candidate;
  currentGeneration = nextGeneration;
  activeSlot = targetSlot;
  currentState = State::SAVED;
  progressValue = 100;
  return true;
}

void CompassCalibration::addSample(int16_t x, int16_t y, int16_t z) {
  if (currentState != State::RUNNING && currentState != State::READY) return;

  if (sampleCount == 0) {
    sampleMinX = sampleMaxX = x;
    sampleMinY = sampleMaxY = y;
    sampleMinZ = sampleMaxZ = z;
  } else {
    if (x < sampleMinX) sampleMinX = x;
    if (x > sampleMaxX) sampleMaxX = x;
    if (y < sampleMinY) sampleMinY = y;
    if (y > sampleMaxY) sampleMaxY = y;
    if (z < sampleMinZ) sampleMinZ = z;
    if (z > sampleMaxZ) sampleMaxZ = z;
  }
  sampleCount++;

  // Spatial coverage estimate: after a minimal settling period, map each sample
  // into one of 8 octants around the current running hard-iron midpoint.
  if (sampleCount >= 40) {
    const int32_t cx = (sampleMaxX + sampleMinX) / 2;
    const int32_t cy = (sampleMaxY + sampleMinY) / 2;
    const int32_t cz = (sampleMaxZ + sampleMinZ) / 2;
    uint8_t oct = 0;
    if ((int32_t)x >= cx) oct |= 1;
    if ((int32_t)y >= cy) oct |= 2;
    if ((int32_t)z >= cz) oct |= 4;
    octantMask |= (uint8_t)(1U << oct);
  }

  evaluateSession();
}

void CompassCalibration::apply(float rawX, float rawY, float rawZ,
                               float &x, float &y, float &z) const {
  if (!active.valid) {
    x = rawX;
    y = rawY;
    z = rawZ;
    return;
  }
  x = (rawX - active.offsetX) * active.scaleX;
  y = (rawY - active.offsetY) * active.scaleY;
  z = (rawZ - active.offsetZ) * active.scaleZ;
}

bool CompassCalibration::isRunning() const {
  return currentState == State::RUNNING || currentState == State::READY;
}

bool CompassCalibration::isSessionActive() const { return isRunning(); }
bool CompassCalibration::isCalibrated() const { return active.valid; }
CompassCalibration::State CompassCalibration::state() const { return currentState; }

const char *CompassCalibration::stateName() const {
  switch (currentState) {
    case State::IDLE: return "IDLE";
    case State::RUNNING: return "RUNNING";
    case State::READY: return "READY";
    case State::SAVED: return "SAVED";
    case State::CANCELED: return "CANCELED";
    case State::FAILED: return "FAILED";
    default: return "FAILED";
  }
}

uint8_t CompassCalibration::progress() const { return progressValue; }
uint8_t CompassCalibration::sessionQuality() const { return qualityValue; }
uint8_t CompassCalibration::savedQuality() const { return active.valid ? active.quality : 0; }
uint32_t CompassCalibration::samples() const { return sampleCount; }
int32_t CompassCalibration::minX() const { return sampleCount ? sampleMinX : 0; }
int32_t CompassCalibration::maxX() const { return sampleCount ? sampleMaxX : 0; }
int32_t CompassCalibration::minY() const { return sampleCount ? sampleMinY : 0; }
int32_t CompassCalibration::maxY() const { return sampleCount ? sampleMaxY : 0; }
int32_t CompassCalibration::minZ() const { return sampleCount ? sampleMinZ : 0; }
int32_t CompassCalibration::maxZ() const { return sampleCount ? sampleMaxZ : 0; }
const CompassCalibration::Params &CompassCalibration::params() const { return active; }

void CompassCalibration::resetSessionStats() {
  sampleCount = 0;
  sampleMinX = sampleMaxX = 0;
  sampleMinY = sampleMaxY = 0;
  sampleMinZ = sampleMaxZ = 0;
  octantMask = 0;
  progressValue = 0;
  qualityValue = 0;
}

void CompassCalibration::evaluateSession() {
  if (sampleCount == 0) {
    progressValue = 0;
    qualityValue = 0;
    if (currentState == State::READY) currentState = State::RUNNING;
    return;
  }

  const int32_t sx = sampleMaxX - sampleMinX;
  const int32_t sy = sampleMaxY - sampleMinY;
  const int32_t sz = sampleMaxZ - sampleMinZ;
  const int32_t minSpan = min(sx, min(sy, sz));
  const int32_t maxSpan = max(sx, max(sy, sz));
  const float balance = maxSpan > 0 ? (float)minSpan / (float)maxSpan : 0.0f;
  const uint8_t octants = popcount8(octantMask);

  if (sampleCount < MIN_SAMPLES_Q1 || minSpan < MIN_SPAN_Q1) {
    qualityValue = 0;
  } else if (sampleCount < MIN_SAMPLES_Q2 || minSpan < MIN_SPAN_Q2 ||
             balance < 0.25f || octants < 3) {
    qualityValue = 1;
  } else if (sampleCount < MIN_SAMPLES_Q3 || minSpan < MIN_SPAN_Q3 ||
             balance < 0.50f || octants < 6) {
    qualityValue = 2;
  } else {
    qualityValue = 3;
  }

  const float sampleScore = min(1.0f, (float)sampleCount / 600.0f) * 30.0f;
  const float rangeScore = (
      min(1.0f, (float)sx / 1200.0f) +
      min(1.0f, (float)sy / 1200.0f) +
      min(1.0f, (float)sz / 1200.0f)) / 3.0f * 40.0f;
  const float balanceScore = min(1.0f, balance / 0.60f) * 15.0f;
  const float coverageScore = ((float)octants / 8.0f) * 15.0f;

  int p = (int)lroundf(sampleScore + rangeScore + balanceScore + coverageScore);
  if (qualityValue >= 2 && sampleCount >= MIN_SAMPLES_Q2 && minSpan >= MIN_SPAN_Q2) {
    p = 100;
    currentState = State::READY;
  } else {
    if (p > 99) p = 99;
    if (currentState == State::READY) currentState = State::RUNNING;
  }
  if (p < 0) p = 0;
  progressValue = (uint8_t)p;
}

bool CompassCalibration::buildCandidate(Params &out) const {
  if (sampleCount < MIN_SAMPLES_Q2 || qualityValue < 2) return false;

  const float rangeX = (float)(sampleMaxX - sampleMinX) * 0.5f;
  const float rangeY = (float)(sampleMaxY - sampleMinY) * 0.5f;
  const float rangeZ = (float)(sampleMaxZ - sampleMinZ) * 0.5f;
  if (rangeX <= 1.0f || rangeY <= 1.0f || rangeZ <= 1.0f) return false;

  const float avgRange = (rangeX + rangeY + rangeZ) / 3.0f;

  out.offsetX = ((float)sampleMaxX + (float)sampleMinX) * 0.5f;
  out.offsetY = ((float)sampleMaxY + (float)sampleMinY) * 0.5f;
  out.offsetZ = ((float)sampleMaxZ + (float)sampleMinZ) * 0.5f;
  out.scaleX = avgRange / rangeX;
  out.scaleY = avgRange / rangeY;
  out.scaleZ = avgRange / rangeZ;
  out.quality = qualityValue;
  out.valid = true;

  // Reject implausibly unbalanced datasets instead of poisoning a valid old calibration.
  if (!isfinite(out.scaleX) || !isfinite(out.scaleY) || !isfinite(out.scaleZ)) return false;
  if (out.scaleX < 0.25f || out.scaleX > 4.0f ||
      out.scaleY < 0.25f || out.scaleY > 4.0f ||
      out.scaleZ < 0.25f || out.scaleZ > 4.0f) return false;
  return true;
}

bool CompassCalibration::readSlot(uint8_t slot, PersistBlob &blob) {
  const char *key = slot == 0 ? "cal0" : "cal1";
  size_t n = prefs.getBytesLength(key);
  if (n != sizeof(PersistBlob)) return false;
  return prefs.getBytes(key, &blob, sizeof(blob)) == sizeof(blob);
}

bool CompassCalibration::writeSlot(uint8_t slot, const PersistBlob &blob) {
  const char *key = slot == 0 ? "cal0" : "cal1";
  return prefs.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
}

bool CompassCalibration::validateBlob(const PersistBlob &blob) const {
  if (blob.magic != MAGIC || blob.version != VERSION || blob.size != sizeof(PersistBlob)) return false;
  if (blob.valid != 1 || blob.quality < 2 || blob.quality > 3) return false;
  if (!isfinite(blob.offsetX) || !isfinite(blob.offsetY) || !isfinite(blob.offsetZ) ||
      !isfinite(blob.scaleX) || !isfinite(blob.scaleY) || !isfinite(blob.scaleZ)) return false;
  if (blob.scaleX < 0.25f || blob.scaleX > 4.0f ||
      blob.scaleY < 0.25f || blob.scaleY > 4.0f ||
      blob.scaleZ < 0.25f || blob.scaleZ > 4.0f) return false;

  PersistBlob copy = blob;
  uint32_t expected = copy.crc32;
  copy.crc32 = 0;
  return crc32(reinterpret_cast<const uint8_t *>(&copy), sizeof(copy)) == expected;
}

void CompassCalibration::blobToParams(const PersistBlob &blob, Params &out) const {
  out.offsetX = blob.offsetX;
  out.offsetY = blob.offsetY;
  out.offsetZ = blob.offsetZ;
  out.scaleX = blob.scaleX;
  out.scaleY = blob.scaleY;
  out.scaleZ = blob.scaleZ;
  out.quality = blob.quality;
  out.valid = blob.valid == 1;
}

CompassCalibration::PersistBlob CompassCalibration::paramsToBlob(const Params &p, uint32_t generation) const {
  PersistBlob blob{};
  blob.magic = MAGIC;
  blob.version = VERSION;
  blob.size = sizeof(PersistBlob);
  blob.generation = generation;
  blob.offsetX = p.offsetX;
  blob.offsetY = p.offsetY;
  blob.offsetZ = p.offsetZ;
  blob.scaleX = p.scaleX;
  blob.scaleY = p.scaleY;
  blob.scaleZ = p.scaleZ;
  blob.quality = p.quality;
  blob.valid = p.valid ? 1 : 0;
  blob.reserved = 0;
  blob.crc32 = 0;
  blob.crc32 = crc32(reinterpret_cast<const uint8_t *>(&blob), sizeof(blob));
  return blob;
}

uint32_t CompassCalibration::crc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; ++b) {
      const uint32_t mask = -(crc & 1UL);
      crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
  }
  return ~crc;
}

uint8_t CompassCalibration::popcount8(uint8_t v) {
  uint8_t n = 0;
  while (v) {
    n += (v & 1U);
    v >>= 1U;
  }
  return n;
}
