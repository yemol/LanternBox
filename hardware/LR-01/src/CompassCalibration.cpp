#include "CompassCalibration.h"

#include <math.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

CompassCalibration::CompassCalibration()
  : currentState(State::IDLE),
    currentGeneration(0),
    activeSlot(0),
    sampleCount(0),
    sampleMinX(0), sampleMaxX(0),
    sampleMinY(0), sampleMaxY(0),
    sampleMinZ(0), sampleMaxZ(0),
    quadrantMask(0),
    progressValue(0), qualityValue(0),
    storedCount(0), storedWrite(0) {}

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
  currentState = State::CANCELED;
  return true;
}

bool CompassCalibration::reset() {
  bool ok0 = prefs.remove("cal0");
  bool ok1 = prefs.remove("cal1");
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

  storedX[storedWrite] = x;
  storedY[storedWrite] = y;
  storedWrite = (uint16_t)((storedWrite + 1U) % SAMPLE_STORE_MAX);
  if (storedCount < SAMPLE_STORE_MAX) storedCount++;

  // Heading is strictly horizontal on this hardware. Track X/Y coverage only;
  // Z remains useful for diagnostics but does not affect calibration quality.
  if (sampleCount >= 40) {
    const int32_t cx = (sampleMaxX + sampleMinX) / 2;
    const int32_t cy = (sampleMaxY + sampleMinY) / 2;
    uint8_t quadrant = 0;
    if ((int32_t)x >= cx) quadrant |= 1;
    if ((int32_t)y >= cy) quadrant |= 2;
    quadrantMask |= (uint8_t)(1U << quadrant);
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
  const float dx = rawX - active.offsetX;
  const float dy = rawY - active.offsetY;
  x = active.matrixXX * dx + active.matrixXY * dy;
  y = active.matrixXY * dx + active.matrixYY * dy;
  z = rawZ;
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
  quadrantMask = 0;
  progressValue = 0;
  qualityValue = 0;
  storedCount = 0;
  storedWrite = 0;
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
  const int32_t minSpan = min(sx, sy);
  const int32_t maxSpan = max(sx, sy);
  const float balance = maxSpan > 0 ? (float)minSpan / (float)maxSpan : 0.0f;
  const uint8_t quadrants = popcount8(quadrantMask);

  if (sampleCount < MIN_SAMPLES_Q1 || minSpan < MIN_SPAN_Q1) {
    qualityValue = 0;
  } else if (sampleCount < MIN_SAMPLES_Q2 || minSpan < MIN_SPAN_Q2 ||
             balance < 0.25f || quadrants < 3) {
    qualityValue = 1;
  } else if (sampleCount < MIN_SAMPLES_Q3 || minSpan < MIN_SPAN_Q3 ||
             balance < 0.50f || quadrants < 4) {
    qualityValue = 2;
  } else {
    qualityValue = 3;
  }

  const float sampleScore = min(1.0f, (float)sampleCount / 600.0f) * 30.0f;
  const float rangeScore = (
      min(1.0f, (float)sx / 1200.0f) +
      min(1.0f, (float)sy / 1200.0f)) / 2.0f * 40.0f;
  const float balanceScore = min(1.0f, balance / 0.60f) * 15.0f;
  const float coverageScore = ((float)quadrants / 4.0f) * 15.0f;

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

bool CompassCalibration::solve5(double a[5][6], double out[5]) {
  for (int col = 0; col < 5; ++col) {
    int pivot = col;
    double best = fabs(a[col][col]);
    for (int row = col + 1; row < 5; ++row) {
      const double v = fabs(a[row][col]);
      if (v > best) {
        best = v;
        pivot = row;
      }
    }
    if (!isfinite(best) || best < 1e-10) return false;
    if (pivot != col) {
      for (int j = col; j < 6; ++j) {
        const double t = a[col][j];
        a[col][j] = a[pivot][j];
        a[pivot][j] = t;
      }
    }

    const double div = a[col][col];
    for (int j = col; j < 6; ++j) a[col][j] /= div;

    for (int row = 0; row < 5; ++row) {
      if (row == col) continue;
      const double factor = a[row][col];
      if (fabs(factor) < 1e-18) continue;
      for (int j = col; j < 6; ++j) {
        a[row][j] -= factor * a[col][j];
      }
    }
  }

  for (int i = 0; i < 5; ++i) {
    out[i] = a[i][5];
    if (!isfinite(out[i])) return false;
  }
  return true;
}

bool CompassCalibration::buildCandidate(Params &out) const {
  if (sampleCount < MIN_SAMPLES_Q2 || qualityValue < 2 || storedCount < MIN_SAMPLES_Q2) {
    return false;
  }

  // Robust provisional center. Averaging a small group of opposite extrema is
  // much less sensitive than using one min/max sample, while retaining the
  // central-symmetry property of a horizontal magnetic ellipse.
  uint8_t k = (uint8_t)(storedCount / 40U); // ~2.5 % of retained samples
  if (k < 12U) k = 12U;
  if (k > EXTREME_AVG_MAX) k = EXTREME_AVG_MAX;
  if (storedCount < (uint16_t)(k * 2U)) return false;

  int16_t lowX[EXTREME_AVG_MAX], highX[EXTREME_AVG_MAX];
  int16_t lowY[EXTREME_AVG_MAX], highY[EXTREME_AVG_MAX];
  for (uint8_t i = 0; i < k; ++i) {
    lowX[i] = INT16_MAX; lowY[i] = INT16_MAX;
    highX[i] = INT16_MIN; highY[i] = INT16_MIN;
  }

  auto pushLow = [k](int16_t *arr, int16_t v) {
    uint8_t idx = 0;
    for (uint8_t i = 1; i < k; ++i) if (arr[i] > arr[idx]) idx = i;
    if (v < arr[idx]) arr[idx] = v;
  };
  auto pushHigh = [k](int16_t *arr, int16_t v) {
    uint8_t idx = 0;
    for (uint8_t i = 1; i < k; ++i) if (arr[i] < arr[idx]) idx = i;
    if (v > arr[idx]) arr[idx] = v;
  };

  for (uint16_t i = 0; i < storedCount; ++i) {
    pushLow(lowX, storedX[i]); pushHigh(highX, storedX[i]);
    pushLow(lowY, storedY[i]); pushHigh(highY, storedY[i]);
  }

  double loX = 0.0, hiX = 0.0, loY = 0.0, hiY = 0.0;
  for (uint8_t i = 0; i < k; ++i) {
    loX += lowX[i]; hiX += highX[i];
    loY += lowY[i]; hiY += highY[i];
  }
  loX /= k; hiX /= k; loY /= k; hiY /= k;
  const double c0x = 0.5 * (loX + hiX);
  const double c0y = 0.5 * (loY + hiY);
  const double spanX = hiX - loX;
  const double spanY = hiY - loY;
  const double normScale = 0.5 * max(spanX, spanY);
  if (!isfinite(normScale) || normScale < 100.0) return false;

  // Equal-angle binning removes dwell-time bias: a manual sweep that pauses at
  // one heading must not receive more statistical weight than the rest of the
  // circle. Each occupied 10-degree sector contributes exactly one point.
  double binX[ANGLE_BIN_COUNT] = {0};
  double binY[ANGLE_BIN_COUNT] = {0};
  uint16_t binN[ANGLE_BIN_COUNT] = {0};
  const double twoPi = 2.0 * PI;
  for (uint16_t i = 0; i < storedCount; ++i) {
    const double dx = (double)storedX[i] - c0x;
    const double dy = (double)storedY[i] - c0y;
    if (dx * dx + dy * dy < 400.0) continue;
    double a = atan2(dy, dx);
    if (a < 0.0) a += twoPi;
    int b = (int)floor(a * (double)ANGLE_BIN_COUNT / twoPi);
    if (b < 0) b = 0;
    if (b >= ANGLE_BIN_COUNT) b = ANGLE_BIN_COUNT - 1;
    binX[b] += storedX[i];
    binY[b] += storedY[i];
    if (binN[b] < 65535U) binN[b]++;
  }

  uint8_t occupied = 0;
  double normal[5][6] = {{0}};
  for (uint8_t b = 0; b < ANGLE_BIN_COUNT; ++b) {
    if (binN[b] == 0) continue;
    occupied++;
    const double x = (binX[b] / binN[b] - c0x) / normScale;
    const double y = (binY[b] / binN[b] - c0y) / normScale;
    const double row[5] = {x * x, x * y, y * y, x, y};
    for (int r = 0; r < 5; ++r) {
      for (int c = 0; c < 5; ++c) normal[r][c] += row[r] * row[c];
      normal[r][5] += row[r]; // RHS is 1.0
    }
  }
  if (occupied < MIN_OCCUPIED_BINS) return false;

  double p[5] = {0};
  if (!solve5(normal, p)) return false;

  // General centered ellipse:
  //   A x^2 + B xy + C y^2 + D x + E y = 1
  // Q = [[A, B/2], [B/2, C]]. The fitted linear terms refine the hard-iron
  // center instead of trusting raw extrema alone.
  const double q00 = p[0];
  const double q01 = 0.5 * p[1];
  const double q11 = p[2];
  const double detQ = q00 * q11 - q01 * q01;
  if (!isfinite(q00) || !isfinite(q01) || !isfinite(q11) || detQ <= 1e-8) return false;
  if (q00 <= 0.0 || q11 <= 0.0) return false;

  const double inv00 = q11 / detQ;
  const double inv01 = -q01 / detQ;
  const double inv11 = q00 / detQ;
  const double dcx = -0.5 * (inv00 * p[3] + inv01 * p[4]);
  const double dcy = -0.5 * (inv01 * p[3] + inv11 * p[4]);
  if (!isfinite(dcx) || !isfinite(dcy) || fabs(dcx) > 1.5 || fabs(dcy) > 1.5) return false;

  const double cqcx = dcx * (q00 * dcx + q01 * dcy) +
                      dcy * (q01 * dcx + q11 * dcy);
  const double kappa = 1.0 + cqcx;
  if (!isfinite(kappa) || kappa <= 1e-6) return false;

  const double n00 = q00 / kappa;
  const double n01 = q01 / kappa;
  const double n11 = q11 / kappa;
  const double trace = n00 + n11;
  const double diff = n00 - n11;
  const double disc = sqrt(diff * diff + 4.0 * n01 * n01);
  const double lambda1 = 0.5 * (trace + disc);
  const double lambda2 = 0.5 * (trace - disc);
  if (!isfinite(lambda1) || !isfinite(lambda2) || lambda1 <= 1e-8 || lambda2 <= 1e-8) {
    return false;
  }
  const double condition = lambda1 / lambda2;
  if (!isfinite(condition) || condition > 25.0) return false;

  const double theta = 0.5 * atan2(2.0 * n01, diff);
  const double ct = cos(theta);
  const double st = sin(theta);
  const double s1 = sqrt(lambda1);
  const double s2 = sqrt(lambda2);

  double m00 = ct * ct * s1 + st * st * s2;
  double m01 = ct * st * (s1 - s2);
  double m11 = st * st * s1 + ct * ct * s2;

  // Heading is invariant to a common positive gain. Normalize determinant to
  // one so stored coefficients stay near unity and preserve the existing
  // runtime magnitude/validation behavior.
  const double detM = m00 * m11 - m01 * m01;
  if (!isfinite(detM) || detM <= 1e-10) return false;
  const double gain = 1.0 / sqrt(detM);
  m00 *= gain;
  m01 *= gain;
  m11 *= gain;

  const double centerX = c0x + dcx * normScale;
  const double centerY = c0y + dcy * normScale;
  if (!isfinite(centerX) || !isfinite(centerY)) return false;
  if (centerX < loX - spanX || centerX > hiX + spanX ||
      centerY < loY - spanY || centerY > hiY + spanY) return false;

  const double finalDet = m00 * m11 - m01 * m01;
  if (!isfinite(m00) || !isfinite(m01) || !isfinite(m11) || finalDet <= 0.0) return false;
  if (m00 < 0.20 || m00 > 5.0 || m11 < 0.20 || m11 > 5.0 || fabs(m01) > 3.0) {
    return false;
  }

  out.offsetX = (float)centerX;
  out.offsetY = (float)centerY;
  out.offsetZ = 0.0f;
  out.matrixXX = (float)m00;
  out.matrixXY = (float)m01;
  out.matrixYY = (float)m11;
  out.scaleZ = 1.0f;
  out.quality = qualityValue;
  out.valid = true;
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
      !isfinite(blob.matrixXX) || !isfinite(blob.matrixXY) || !isfinite(blob.matrixYY) ||
      !isfinite(blob.scaleZ)) return false;
  const float det = blob.matrixXX * blob.matrixYY - blob.matrixXY * blob.matrixXY;
  if (blob.matrixXX < 0.20f || blob.matrixXX > 5.0f ||
      blob.matrixYY < 0.20f || blob.matrixYY > 5.0f ||
      fabsf(blob.matrixXY) > 3.0f || det <= 0.0f ||
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
  out.matrixXX = blob.matrixXX;
  out.matrixXY = blob.matrixXY;
  out.matrixYY = blob.matrixYY;
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
  blob.matrixXX = p.matrixXX;
  blob.matrixXY = p.matrixXY;
  blob.matrixYY = p.matrixYY;
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
