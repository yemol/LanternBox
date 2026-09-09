#include "GnssBlackbox.h"

#include <string.h>
#include <stddef.h>

static_assert(sizeof(GnssLogRecord) == 77, "GnssLogRecord layout changed");

void GnssBlackbox::resetImage() {
  memset(&image_, 0, sizeof(image_));
  image_.magic = MAGIC;
  image_.version = VERSION;
  image_.capacity = CAPACITY;
  image_.head = 0;
  image_.count = 0;
  image_.nextSeq = 1;
  image_.crc32 = 0;
}

uint32_t GnssBlackbox::computeCrc(const PersistImage &image) const {
  // PersistImage is several KiB. Never copy it onto the Arduino loopTask stack:
  // the default loop stack is only about 8 KiB on this target, and the old
  // implementation could trip the stack canary during boot/flush. Compute the
  // CRC directly from the member image while logically treating the crc32 field
  // itself as four zero bytes, which is byte-for-byte equivalent to hashing a
  // temporary copy with copy.crc32 = 0.
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&image);
  const size_t crcOff = offsetof(PersistImage, crc32);

  uint32_t crc = 0;
  crc = ~crc;
  for (size_t i = 0; i < sizeof(PersistImage); ++i) {
    uint8_t b = bytes[i];
    if (i >= crcOff && i < crcOff + sizeof(image.crc32)) b = 0;
    crc ^= b;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint32_t mask = -(crc & 1U);
      crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
  }
  return ~crc;
}

bool GnssBlackbox::begin() {
  resetImage();
  ready_ = prefs_.begin("gnsslog", false);
  if (!ready_) {
    persistenceHealthy_ = false;
    persistFailures_++;
    return false;
  }

  const size_t storedLen = prefs_.getBytesLength("ring");
  if (storedLen == sizeof(PersistImage)) {
    // Read directly into the class-owned image buffer. A local PersistImage here
    // would consume several KiB of loopTask stack and can immediately trip the
    // ESP32-S3 stack canary. Validate in-place, then either keep it or reset.
    const size_t readLen = prefs_.getBytes("ring", &image_, sizeof(image_));
    const bool valid =
        readLen == sizeof(image_) &&
        image_.magic == MAGIC &&
        image_.version == VERSION &&
        image_.capacity == CAPACITY &&
        image_.head < CAPACITY &&
        image_.count <= CAPACITY &&
        image_.nextSeq != 0 &&
        image_.crc32 == computeCrc(image_);
    if (!valid) {
      // Remove an invalid/legacy snapshot before the next write. NVS updates
      // are copy-on-write; keeping a large invalid blob around can leave too
      // little transient space for the replacement snapshot.
      prefs_.remove("ring");
      resetImage();
      dirty_ = true;
    }
  } else if (storedLen != 0) {
    // Schema/capacity changed. Drop only this namespace key, not other NVS
    // users, then start a fresh compact ring.
    prefs_.remove("ring");
    resetImage();
    dirty_ = true;
  }

  lastFlushMs_ = millis();
  return true;
}

void GnssBlackbox::end() {
  if (ready_) {
    if (dirty_) flush();
    prefs_.end();
  }
  ready_ = false;
}

bool GnssBlackbox::clear() {
  resetImage();
  dirty_ = true;
  return flush();
}

void GnssBlackbox::append(const GnssLogRecord &record, bool flushNow) {
  GnssLogRecord r = record;
  r.seq = image_.nextSeq++;
  if (image_.nextSeq == 0) image_.nextSeq = 1;

  image_.records[image_.head] = r;
  image_.head = (uint16_t)((image_.head + 1U) % CAPACITY);
  if (image_.count < CAPACITY) image_.count++;
  dirty_ = true;

  if (flushNow) flush();
}

bool GnssBlackbox::flush() {
  if (!ready_) {
    persistenceHealthy_ = false;
    persistFailures_++;
    return false;
  }
  if (!dirty_) {
    lastFlushMs_ = millis();
    return true;
  }

  image_.crc32 = 0;
  image_.crc32 = computeCrc(image_);
  size_t written = prefs_.putBytes("ring", &image_, sizeof(image_));
  if (written != sizeof(image_)) {
    // One recovery attempt: remove the previous blob first so the NVS
    // copy-on-write allocator does not need room for old+new snapshots at
    // the same time. This namespace contains only the GNSS ring key.
    prefs_.remove("ring");
    written = prefs_.putBytes("ring", &image_, sizeof(image_));
  }
  if (written != sizeof(image_)) {
    persistenceHealthy_ = false;
    persistFailures_++;
    return false;
  }

  dirty_ = false;
  persistenceHealthy_ = true;
  lastFlushMs_ = millis();
  return true;
}

void GnssBlackbox::service(uint32_t nowMs, uint32_t flushIntervalMs) {
  if (!dirty_ || !ready_) return;
  if (lastFlushMs_ == 0 || nowMs - lastFlushMs_ >= flushIntervalMs) flush();
}

size_t GnssBlackbox::count() const {
  return image_.count;
}

bool GnssBlackbox::getOldest(size_t index, GnssLogRecord &out) const {
  if (index >= image_.count) return false;
  const uint16_t oldest = (uint16_t)((image_.head + CAPACITY - image_.count) % CAPACITY);
  const uint16_t slot = (uint16_t)((oldest + index) % CAPACITY);
  out = image_.records[slot];
  return true;
}

const char *GnssBlackbox::eventName(uint8_t event) {
  switch ((GnssLogEvent)event) {
    case GnssLogEvent::BOOT: return "BOOT";
    case GnssLogEvent::FIX_GAIN: return "FIX_GAIN";
    case GnssLogEvent::FIX_LOST: return "FIX_LOST";
    case GnssLogEvent::RECOVERY_STREAM: return "RECOVERY_STREAM";
    case GnssLogEvent::RECOVERY_NMEA: return "RECOVERY_NMEA";
    case GnssLogEvent::RECOVERY_NO_FIX: return "RECOVERY_NO_FIX";
    case GnssLogEvent::MANUAL_CLEAR: return "MANUAL_CLEAR";
    case GnssLogEvent::RECEIVER_HOT: return "RECEIVER_HOT";
    case GnssLogEvent::RECEIVER_WARM: return "RECEIVER_WARM";
    case GnssLogEvent::RECEIVER_COLD: return "RECEIVER_COLD";
    case GnssLogEvent::CFG_BASELINE: return "CFG_BASELINE";
    case GnssLogEvent::GNSS_SOFT_OFF: return "GNSS_SOFT_OFF";
    case GnssLogEvent::GNSS_SOFT_ON: return "GNSS_SOFT_ON";
    case GnssLogEvent::SAMPLE:
    default: return "SAMPLE";
  }
}
