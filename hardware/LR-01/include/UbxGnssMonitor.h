#pragma once

#include <Arduino.h>
#include <stdint.h>

struct UbxPvtSnapshot {
  bool valid = false;
  uint32_t updatedMs = 0;
  uint8_t fixType = 0;
  bool fixOk = false;
  uint8_t numSv = 0;
  int32_t lonE7 = 0;
  int32_t latE7 = 0;
  int32_t hMslMm = 0;
  uint32_t hAccMm = 0;
  uint16_t pDopX100 = 9999;
  uint8_t psmState = 0;
};

struct UbxSatSnapshot {
  bool valid = false;
  uint32_t updatedMs = 0;
  uint8_t numSv = 0;
  uint8_t used = 0;
  uint8_t cnoMax = 0;
  uint8_t cnoGe20 = 0;
  uint8_t cnoGe25 = 0;
  uint8_t cnoGe30 = 0;
  uint8_t codeLocked = 0;
  uint8_t ephAvail = 0;
};

struct UbxConfigSnapshot {
  bool valid = false;
  uint32_t updatedMs = 0;
  uint8_t fixMode = 0xFF;
  uint8_t iniFix3d = 0xFF;
  uint8_t dynModel = 0xFF;
  uint8_t minSvs = 0xFF;
  uint8_t maxSvs = 0xFF;
  uint8_t minCno = 0xFF;
  int8_t minElev = 127;
  uint8_t nCnoThrs = 0xFF;
  uint8_t cnoThrs = 0xFF;
  uint8_t sigAttenComp = 0xFF;
  uint8_t pmOperateMode = 0xFF;
  uint8_t pmUpdateEph = 0xFF;
  uint16_t signalMask = 0; // GPS/GAL/BDS master+signal enable bits
  uint16_t pDopX10 = 0xFFFF;
  uint16_t pAccM = 0xFFFF;
  uint16_t measRateMs = 0xFFFF;
  uint16_t navRate = 0xFFFF;
};

class UbxGnssMonitor {
 public:
  void feed(uint8_t byte, uint32_t nowMs);

  const UbxPvtSnapshot &pvt() const { return pvt_; }
  const UbxSatSnapshot &sat() const { return sat_; }
  const UbxConfigSnapshot &cfg() const { return cfg_; }

  bool protocolSeen() const { return protocolSeen_; }
  bool monVerSeen() const { return monVerSeen_; }
  const char *swVersion() const { return swVersion_; }
  const char *hwVersion() const { return hwVersion_; }
  const char *protVersion() const { return protVersion_; }
  uint32_t framesOk() const { return framesOk_; }
  uint32_t framesBad() const { return framesBad_; }
  uint32_t lastFrameMs() const { return lastFrameMs_; }
  uint32_t ackCount() const { return ackCount_; }
  uint32_t nakCount() const { return nakCount_; }

 private:
  static constexpr uint16_t MAX_PAYLOAD = 1024;
  enum class State : uint8_t { SYNC1, SYNC2, CLS, ID, LEN1, LEN2, PAYLOAD, CKA, CKB };

  State state_ = State::SYNC1;
  uint8_t cls_ = 0;
  uint8_t id_ = 0;
  uint16_t len_ = 0;
  uint16_t pos_ = 0;
  uint8_t ckA_ = 0;
  uint8_t ckB_ = 0;
  uint8_t rxCkA_ = 0;
  uint8_t payload_[MAX_PAYLOAD]{};
  bool overflow_ = false;

  bool protocolSeen_ = false;
  bool monVerSeen_ = false;
  char swVersion_[31]{};
  char hwVersion_[11]{};
  char protVersion_[16]{};
  uint32_t framesOk_ = 0;
  uint32_t framesBad_ = 0;
  uint32_t lastFrameMs_ = 0;
  uint32_t ackCount_ = 0;
  uint32_t nakCount_ = 0;
  UbxPvtSnapshot pvt_{};
  UbxSatSnapshot sat_{};
  UbxConfigSnapshot cfg_{};

  void checksum(uint8_t v);
  void reset();
  void handleFrame(uint32_t nowMs);
  void handleMonVer();
  void handlePvt(uint32_t nowMs);
  void handleSat(uint32_t nowMs);
  void handleValGet(uint32_t nowMs);
  static uint16_t le16(const uint8_t *p);
  static uint32_t le32(const uint8_t *p);
  static int32_t leI32(const uint8_t *p);
  static size_t cfgValueSize(uint32_t key);
};
