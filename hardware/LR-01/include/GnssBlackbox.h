#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>

enum class GnssLogEvent : uint8_t {
  SAMPLE = 0,
  BOOT = 1,
  FIX_GAIN = 2,
  FIX_LOST = 3,
  RECOVERY_STREAM = 4,
  RECOVERY_NMEA = 5,
  RECOVERY_NO_FIX = 6,
  MANUAL_CLEAR = 7,
  RECEIVER_HOT = 8,
  RECEIVER_WARM = 9,
  RECEIVER_COLD = 10,
  CFG_BASELINE = 11,
  GNSS_SOFT_OFF = 12,
  GNSS_SOFT_ON = 13,
};

struct __attribute__((packed)) GnssLogRecord {
  uint32_t seq;
  uint32_t uptimeSec;
  uint32_t gnssBytes;
  uint32_t nmeaOk;
  uint32_t nmeaBad;
  uint32_t locAgeMs;
  uint32_t byteAgeMs;
  uint16_t satVisible;
  uint16_t hdopX100;
  uint16_t recoveries;
  uint8_t satUsed;
  uint8_t fixType;
  uint8_t flags;   // bit0=stream, bit1=fix, bit2=time_valid
  uint8_t event;
  uint8_t ggaFixQuality;
  uint8_t ggaSatUsed;
  uint8_t gsaFixType;
  uint8_t rmcStatus;
  uint8_t gnsFix;
  uint8_t gnsSatUsed;
  uint8_t locationSource;
  uint16_t receiverResets;
  uint8_t ubxFlags;       // bit0=UBX seen, bit1=MON-VER, bit2=PVT fresh, bit3=PVT fixOK, bit4=CFG valid
  uint8_t ubxFixType;
  uint8_t ubxNumSv;
  uint8_t ubxSatUsed;
  uint8_t ubxCnoMax;
  uint8_t ubxCnoGe20;
  uint8_t ubxCnoGe30;
  uint8_t ubxEphAvail;
  uint8_t cfgFixMode;
  uint8_t cfgMinSvs;
  uint8_t cfgMaxSvs;
  uint8_t cfgMinCno;
  int8_t cfgMinElev;
  uint8_t cfgNCnoThrs;
  uint8_t cfgCnoThrs;
  uint8_t cfgSigAttenComp;
  uint8_t cfgPmMode;
  uint8_t cfgPmUpdateEph;
  uint16_t cfgSignalMask;
  uint16_t cfgPDopX10;
  uint16_t cfgPAccM;
  uint16_t cfgMeasRateMs;
  uint16_t cfgNavRate;
  uint16_t cfgApplyCount;
};

class GnssBlackbox {
 public:
  static constexpr uint16_t CAPACITY = 48;

  bool begin();
  void end();
  bool clear();

  void append(const GnssLogRecord &record, bool flushNow = false);
  bool flush();
  void service(uint32_t nowMs, uint32_t flushIntervalMs);

  size_t count() const;
  bool getOldest(size_t index, GnssLogRecord &out) const;

  bool ready() const { return ready_; }
  bool persistenceHealthy() const { return persistenceHealthy_; }
  uint32_t persistFailures() const { return persistFailures_; }
  uint32_t nextSequence() const { return image_.nextSeq; }

  static const char *eventName(uint8_t event);

 private:
  static constexpr uint32_t MAGIC = 0x474E5342UL; // "GNSB"
  static constexpr uint16_t VERSION = 6;

  struct __attribute__((packed)) PersistImage {
    uint32_t magic;
    uint16_t version;
    uint16_t capacity;
    uint16_t head;
    uint16_t count;
    uint32_t nextSeq;
    uint32_t crc32;
    GnssLogRecord records[CAPACITY];
  };

  Preferences prefs_;
  PersistImage image_{};
  bool ready_ = false;
  bool dirty_ = false;
  bool persistenceHealthy_ = true;
  uint32_t persistFailures_ = 0;
  uint32_t lastFlushMs_ = 0;

  void resetImage();
  uint32_t computeCrc(const PersistImage &image) const;
};
