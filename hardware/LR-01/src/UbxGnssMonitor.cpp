#include "UbxGnssMonitor.h"

#include <string.h>

uint16_t UbxGnssMonitor::le16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t UbxGnssMonitor::le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int32_t UbxGnssMonitor::leI32(const uint8_t *p) {
  return (int32_t)le32(p);
}

size_t UbxGnssMonitor::cfgValueSize(uint32_t key) {
  switch ((key >> 28) & 0x0F) {
    case 1: return 1; // L
    case 2: return 1; // U1/I1/E1/X1
    case 3: return 2; // U2/I2/E2/X2
    case 4: return 4; // U4/I4/E4/X4/R4
    case 5: return 8; // U8/I8/R8
    default: return 0;
  }
}

void UbxGnssMonitor::checksum(uint8_t v) {
  ckA_ = (uint8_t)(ckA_ + v);
  ckB_ = (uint8_t)(ckB_ + ckA_);
}

void UbxGnssMonitor::reset() {
  state_ = State::SYNC1;
  cls_ = id_ = 0;
  len_ = pos_ = 0;
  ckA_ = ckB_ = rxCkA_ = 0;
  overflow_ = false;
}

void UbxGnssMonitor::feed(uint8_t b, uint32_t nowMs) {
  switch (state_) {
    case State::SYNC1:
      if (b == 0xB5) state_ = State::SYNC2;
      break;
    case State::SYNC2:
      if (b == 0x62) state_ = State::CLS;
      else state_ = (b == 0xB5) ? State::SYNC2 : State::SYNC1;
      break;
    case State::CLS:
      cls_ = b; ckA_ = ckB_ = 0; checksum(b); state_ = State::ID; break;
    case State::ID:
      id_ = b; checksum(b); state_ = State::LEN1; break;
    case State::LEN1:
      len_ = b; checksum(b); state_ = State::LEN2; break;
    case State::LEN2:
      len_ |= (uint16_t)b << 8; checksum(b); pos_ = 0; overflow_ = len_ > MAX_PAYLOAD;
      state_ = len_ ? State::PAYLOAD : State::CKA;
      break;
    case State::PAYLOAD:
      if (!overflow_ && pos_ < MAX_PAYLOAD) payload_[pos_] = b;
      checksum(b);
      pos_++;
      if (pos_ >= len_) state_ = State::CKA;
      break;
    case State::CKA:
      rxCkA_ = b; state_ = State::CKB; break;
    case State::CKB:
      if (!overflow_ && rxCkA_ == ckA_ && b == ckB_) {
        protocolSeen_ = true;
        framesOk_++;
        lastFrameMs_ = nowMs;
        handleFrame(nowMs);
      } else {
        framesBad_++;
      }
      reset();
      break;
  }
}

void UbxGnssMonitor::handleFrame(uint32_t nowMs) {
  if (cls_ == 0x0A && id_ == 0x04) handleMonVer();
  else if (cls_ == 0x01 && id_ == 0x07) handlePvt(nowMs);
  else if (cls_ == 0x01 && id_ == 0x35) handleSat(nowMs);
  else if (cls_ == 0x06 && id_ == 0x8B) handleValGet(nowMs);
  else if (cls_ == 0x05 && id_ == 0x01) ackCount_++;
  else if (cls_ == 0x05 && id_ == 0x00) nakCount_++;
}

void UbxGnssMonitor::handleMonVer() {
  if (len_ < 40) return;
  memset(swVersion_, 0, sizeof(swVersion_));
  memset(hwVersion_, 0, sizeof(hwVersion_));
  memset(protVersion_, 0, sizeof(protVersion_));
  memcpy(swVersion_, payload_, 30);
  memcpy(hwVersion_, payload_ + 30, 10);
  for (uint16_t off = 40; off + 30 <= len_; off += 30) {
    char ext[31]{};
    memcpy(ext, payload_ + off, 30);
    const char *p = strstr(ext, "PROTVER=");
    if (p) {
      strncpy(protVersion_, p + 8, sizeof(protVersion_) - 1);
      break;
    }
  }
  monVerSeen_ = true;
}

void UbxGnssMonitor::handlePvt(uint32_t nowMs) {
  if (len_ < 92) return;
  pvt_.fixType = payload_[20];
  const uint8_t flags = payload_[21];
  const uint16_t flags3 = le16(payload_ + 78);
  pvt_.fixOk = (flags & 0x01U) != 0 && (flags3 & 0x0001U) == 0;
  pvt_.psmState = (flags >> 2) & 0x07U;
  pvt_.numSv = payload_[23];
  pvt_.lonE7 = leI32(payload_ + 24);
  pvt_.latE7 = leI32(payload_ + 28);
  pvt_.hMslMm = leI32(payload_ + 36);
  pvt_.hAccMm = le32(payload_ + 40);
  pvt_.pDopX100 = le16(payload_ + 76);
  pvt_.updatedMs = nowMs;
  pvt_.valid = true;
}

void UbxGnssMonitor::handleSat(uint32_t nowMs) {
  if (len_ < 8) return;
  const uint8_t n = payload_[5];
  if ((uint32_t)8 + (uint32_t)n * 12U > len_) return;
  UbxSatSnapshot s{};
  s.valid = true;
  s.updatedMs = nowMs;
  s.numSv = n;
  for (uint8_t i = 0; i < n; ++i) {
    const uint16_t off = (uint16_t)(8 + i * 12);
    const uint8_t cno = payload_[off + 2];
    const uint32_t flags = le32(payload_ + off + 8);
    const uint8_t quality = flags & 0x07U;
    if (cno > s.cnoMax) s.cnoMax = cno;
    if (cno >= 20) s.cnoGe20++;
    if (cno >= 25) s.cnoGe25++;
    if (cno >= 30) s.cnoGe30++;
    if (quality >= 4) s.codeLocked++;
    if (flags & (1UL << 3)) s.used++;
    if (flags & (1UL << 11)) s.ephAvail++;
  }
  sat_ = s;
}

void UbxGnssMonitor::handleValGet(uint32_t nowMs) {
  if (len_ < 4 || payload_[0] != 0x01) return;
  uint16_t off = 4;
  bool any = false;
  while (off + 4 <= len_) {
    const uint32_t key = le32(payload_ + off);
    off += 4;
    const size_t size = cfgValueSize(key);
    if (size == 0 || off + size > len_) break;
    uint32_t v = 0;
    for (size_t i = 0; i < size && i < 4; ++i) v |= (uint32_t)payload_[off + i] << (8 * i);
    switch (key) {
      case 0x20110011UL: cfg_.fixMode = (uint8_t)v; any = true; break;
      case 0x10110013UL: cfg_.iniFix3d = (uint8_t)v; any = true; break;
      case 0x20110021UL: cfg_.dynModel = (uint8_t)v; any = true; break;
      case 0x201100A1UL: cfg_.minSvs = (uint8_t)v; any = true; break;
      case 0x201100A2UL: cfg_.maxSvs = (uint8_t)v; any = true; break;
      case 0x201100A3UL: cfg_.minCno = (uint8_t)v; any = true; break;
      case 0x201100A4UL: cfg_.minElev = (int8_t)(uint8_t)v; any = true; break;
      case 0x201100AAUL: cfg_.nCnoThrs = (uint8_t)v; any = true; break;
      case 0x201100ABUL: cfg_.cnoThrs = (uint8_t)v; any = true; break;
      case 0x201100D6UL: cfg_.sigAttenComp = (uint8_t)v; any = true; break;
      case 0x20D00001UL: cfg_.pmOperateMode = (uint8_t)v; any = true; break;
      case 0x10D0000AUL: cfg_.pmUpdateEph = (uint8_t)v; any = true; break;
      case 0x1031001FUL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 0)) | ((v ? 1U : 0U) << 0)); any = true; break;
      case 0x10310001UL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 1)) | ((v ? 1U : 0U) << 1)); any = true; break;
      case 0x10310021UL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 2)) | ((v ? 1U : 0U) << 2)); any = true; break;
      case 0x10310007UL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 3)) | ((v ? 1U : 0U) << 3)); any = true; break;
      case 0x10310022UL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 4)) | ((v ? 1U : 0U) << 4)); any = true; break;
      case 0x1031000DUL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 5)) | ((v ? 1U : 0U) << 5)); any = true; break;
      case 0x1031000FUL: cfg_.signalMask = (uint16_t)((cfg_.signalMask & ~(1U << 6)) | ((v ? 1U : 0U) << 6)); any = true; break;
      case 0x301100B1UL: cfg_.pDopX10 = (uint16_t)v; any = true; break;
      case 0x301100B3UL: cfg_.pAccM = (uint16_t)v; any = true; break;
      case 0x30210001UL: cfg_.measRateMs = (uint16_t)v; any = true; break;
      case 0x30210002UL: cfg_.navRate = (uint16_t)v; any = true; break;
      default: break;
    }
    off += size;
  }
  if (any) {
    cfg_.valid = true;
    cfg_.updatedMs = nowMs;
  }
}
