#include "LoRaManager.h"
#include "FtHardware.h"
#include "FtConfig.h"
#include <string.h>
#include <stdio.h>
#include <mbedtls/aes.h>
#include <esp_system.h>
#include <Preferences.h>
#include "Ft01Secrets.h"

static const float LORA_PROBE_BW_KHZ = 250.0;
static const uint8_t LORA_PROBE_SF = 11;
static const uint8_t LORA_PROBE_CR = 5;
static const int8_t LORA_PROBE_POWER_DBM = 10;
static const uint16_t LORA_PROBE_PREAMBLE = 8;


// v0.5.2d fresh-GNSS attachment for broadcast and PKI direct messages.
// CN35 locked build: AES-256 channel TX plus persistent X25519 PKI identity.
// Do not publish this source package; it contains the current channel key.
static const uint32_t MESH_NATIVE_NODE_NUM = 0x4C423001UL; // !4c423001, FT01
static const uint32_t MESH_NATIVE_BROADCAST = 0xFFFFFFFFUL;
static const uint8_t MESH_NATIVE_HOP_LIMIT = 3;
static const uint8_t MESH_NATIVE_HOP_START = 3;
static const size_t MESH_NATIVE_HEADER_LEN = 16;
static const size_t MESH_NATIVE_FRAME_MAX = 240;
static const uint8_t MESH_PORT_TEXT_MESSAGE_APP = 1;
static const uint8_t MESH_PORT_NODEINFO_APP = 4;
static const uint8_t MESH_PORT_ROUTING_APP = 5;
static const uint8_t MESH_FLAG_WANT_ACK = 0x08;
static volatile bool gLoRaPacketReceived = false;
static void onLoRaPacketReceived() { gLoRaPacketReceived = true; }

static bool appendByte(uint8_t* out, size_t& pos, size_t maxLen, uint8_t value) {
  if (pos >= maxLen) return false;
  out[pos++] = value;
  return true;
}

static bool appendVarintRaw(uint8_t* out, size_t& pos, size_t maxLen, uint32_t value) {
  while (value >= 0x80) {
    if (!appendByte(out, pos, maxLen, (uint8_t)((value & 0x7F) | 0x80))) return false;
    value >>= 7;
  }
  return appendByte(out, pos, maxLen, (uint8_t)value);
}

static bool appendTag(uint8_t* out, size_t& pos, size_t maxLen, uint8_t fieldNo, uint8_t wireType) {
  return appendVarintRaw(out, pos, maxLen, ((uint32_t)fieldNo << 3) | wireType);
}

static bool appendVarintField(uint8_t* out, size_t& pos, size_t maxLen, uint8_t fieldNo, uint32_t value) {
  return appendTag(out, pos, maxLen, fieldNo, 0) && appendVarintRaw(out, pos, maxLen, value);
}

static bool appendFixed32Field(uint8_t* out, size_t& pos, size_t maxLen, uint8_t fieldNo, uint32_t value) {
  if (!appendTag(out, pos, maxLen, fieldNo, 5) || pos + 4 > maxLen) return false;
  out[pos++] = (uint8_t)(value & 0xFF);
  out[pos++] = (uint8_t)((value >> 8) & 0xFF);
  out[pos++] = (uint8_t)((value >> 16) & 0xFF);
  out[pos++] = (uint8_t)((value >> 24) & 0xFF);
  return true;
}

static bool appendBytesField(uint8_t* out, size_t& pos, size_t maxLen, uint8_t fieldNo, const uint8_t* data, size_t len) {
  if (!appendTag(out, pos, maxLen, fieldNo, 2)) return false;
  if (!appendVarintRaw(out, pos, maxLen, (uint32_t)len)) return false;
  if (pos + len > maxLen) return false;
  memcpy(out + pos, data, len);
  pos += len;
  return true;
}

static bool appendStringField(uint8_t* out, size_t& pos, size_t maxLen, uint8_t fieldNo, const String& text) {
  return appendBytesField(out, pos, maxLen, fieldNo, (const uint8_t*)text.c_str(), text.length());
}

static bool readVarintRaw(const uint8_t* data, size_t len, size_t& pos, uint32_t& value) {
  value = 0;
  uint8_t shift = 0;
  while (pos < len && shift <= 28) {
    uint8_t b = data[pos++];
    value |= ((uint32_t)(b & 0x7F)) << shift;
    if ((b & 0x80) == 0) return true;
    shift += 7;
  }
  return false;
}

static void writeLe32To(uint8_t* out, uint32_t value) {
  out[0] = (uint8_t)(value & 0xFF);
  out[1] = (uint8_t)((value >> 8) & 0xFF);
  out[2] = (uint8_t)((value >> 16) & 0xFF);
  out[3] = (uint8_t)((value >> 24) & 0xFF);
}

static void writeLe64To(uint8_t* out, uint64_t value) {
  for (uint8_t i = 0; i < 8; i++) out[i] = (uint8_t)((value >> (8 * i)) & 0xFF);
}

static bool aesCtrCrypt(uint8_t* data, size_t len, uint32_t fromNode, uint32_t packetId) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  int rc = mbedtls_aes_setkey_enc(&ctx, FT01_MESH_AES_KEY, 256);
  if (rc != 0) {
    mbedtls_aes_free(&ctx);
    return false;
  }

  uint8_t nonceCounter[16];
  memset(nonceCounter, 0, sizeof(nonceCounter));
  writeLe64To(nonceCounter + 0, (uint64_t)packetId);
  writeLe32To(nonceCounter + 8, fromNode);
  writeLe32To(nonceCounter + 12, 0); // extraNonce, only used for PKI packets

  uint8_t streamBlock[16];
  memset(streamBlock, 0, sizeof(streamBlock));
  size_t ncOff = 0;
  rc = mbedtls_aes_crypt_ctr(&ctx, len, &ncOff, nonceCounter, streamBlock, data, data);
  mbedtls_aes_free(&ctx);
  return rc == 0;
}

static bool buildDataProto(uint8_t portnum,
                           const uint8_t* payload,
                           size_t payloadLen,
                           bool wantResponse,
                           uint32_t requestId,
                           uint8_t* out,
                           size_t maxLen,
                           size_t& outLen) {
  outLen = 0;
  if (!appendVarintField(out, outLen, maxLen, 1, portnum)) return false;
  if (!appendBytesField(out, outLen, maxLen, 2, payload, payloadLen)) return false;
  if (wantResponse && !appendVarintField(out, outLen, maxLen, 3, 1)) return false;
  if (requestId != 0 && !appendFixed32Field(out, outLen, maxLen, 6, requestId)) return false;
  return true;
}

static bool buildUserProto(const uint8_t publicKey[32], uint8_t* out, size_t maxLen, size_t& outLen) {
  outLen = 0;
  if (!publicKey) return false;
  if (!appendStringField(out, outLen, maxLen, 1, String("!4c423001"))) return false;
  if (!appendStringField(out, outLen, maxLen, 2, String("FT01"))) return false;
  if (!appendStringField(out, outLen, maxLen, 3, String("FT01"))) return false;
  if (!appendVarintField(out, outLen, maxLen, 5, 112)) return false; // M5STACK_CARDPUTER_ADV
  if (!appendBytesField(out, outLen, maxLen, 8, publicKey, 32)) return false;
  // Send the optional field explicitly so clients clear the old cached true value.
  if (!appendVarintField(out, outLen, maxLen, 9, 0)) return false; // is_unmessagable=false
  return true;
}

static bool isZeroKey(const uint8_t key[32]) {
  uint8_t combined = 0;
  for (size_t i = 0; i < 32; i++) combined |= key[i];
  return combined == 0;
}

struct ParsedUserInfo {
  bool hasPublicKey;
  uint8_t publicKey[32];
  String id;
  String longName;
  String shortName;
  uint32_t nodeNumber;
};

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static uint32_t parseNodeNumberFromUserId(const String& id) {
  if (id.length() != 9 || id[0] != '!') return 0;
  uint32_t value = 0;
  for (int i = 1; i < 9; i++) {
    int nibble = hexNibble(id[i]);
    if (nibble < 0) return 0;
    value = (value << 4) | (uint32_t)nibble;
  }
  return value;
}

static String protoString(const uint8_t* data, size_t len, size_t maxBytes) {
  String value;
  if (!data) return value;
  size_t n = len < maxBytes ? len : maxBytes;
  value.reserve(n);
  for (size_t i = 0; i < n; i++) {
    uint8_t c = data[i];
    if (c == '\r' || c == '\n' || c == '\t' || c == 0) continue;
    if (c >= 0x20) value += (char)c;
  }
  return value;
}

static bool parseUserInfo(const uint8_t* data, size_t len, ParsedUserInfo& user) {
  user.hasPublicKey = false;
  memset(user.publicKey, 0, sizeof(user.publicKey));
  user.id = "";
  user.longName = "";
  user.shortName = "";
  user.nodeNumber = 0;
  if (!data) return false;

  size_t pos = 0;
  while (pos < len) {
    uint32_t tag = 0;
    if (!readVarintRaw(data, len, pos, tag)) return false;
    uint32_t fieldNo = tag >> 3;
    uint32_t wireType = tag & 0x07;
    if (wireType == 2) {
      uint32_t fieldLen = 0;
      if (!readVarintRaw(data, len, pos, fieldLen) || pos + fieldLen > len) return false;
      if (fieldNo == 1) user.id = protoString(data + pos, fieldLen, 9);
      else if (fieldNo == 2) user.longName = protoString(data + pos, fieldLen, 24);
      else if (fieldNo == 3) user.shortName = protoString(data + pos, fieldLen, 12);
      else if (fieldNo == 8 && fieldLen == 32) {
        memcpy(user.publicKey, data + pos, 32);
        user.hasPublicKey = !isZeroKey(user.publicKey);
      }
      pos += fieldLen;
    } else if (wireType == 0) {
      uint32_t ignored = 0;
      if (!readVarintRaw(data, len, pos, ignored)) return false;
    } else if (wireType == 1) {
      if (pos + 8 > len) return false;
      pos += 8;
    } else if (wireType == 5) {
      if (pos + 4 > len) return false;
      pos += 4;
    } else {
      return false;
    }
  }
  user.nodeNumber = parseNodeNumberFromUserId(user.id);
  return user.hasPublicKey || user.id.length() > 0 || user.longName.length() > 0 || user.shortName.length() > 0;
}

static bool parseDataProto(const uint8_t* data,
                           size_t len,
                           uint32_t& portnum,
                           const uint8_t*& payload,
                           size_t& payloadLen,
                           bool& wantResponse) {
  portnum = 0;
  payload = nullptr;
  payloadLen = 0;
  wantResponse = false;
  size_t pos = 0;
  while (pos < len) {
    uint32_t tag = 0;
    if (!readVarintRaw(data, len, pos, tag)) return false;
    uint32_t fieldNo = tag >> 3;
    uint32_t wireType = tag & 0x07;
    if (fieldNo == 1 && wireType == 0) {
      uint32_t v = 0;
      if (!readVarintRaw(data, len, pos, v)) return false;
      portnum = v;
    } else if (fieldNo == 2 && wireType == 2) {
      uint32_t l = 0;
      if (!readVarintRaw(data, len, pos, l)) return false;
      if (pos + l > len) return false;
      payload = data + pos;
      payloadLen = l;
      pos += l;
    } else if (fieldNo == 3 && wireType == 0) {
      uint32_t value = 0;
      if (!readVarintRaw(data, len, pos, value)) return false;
      wantResponse = value != 0;
    } else {
      if (wireType == 0) {
        uint32_t ignored = 0;
        if (!readVarintRaw(data, len, pos, ignored)) return false;
      } else if (wireType == 2) {
        uint32_t l = 0;
        if (!readVarintRaw(data, len, pos, l)) return false;
        if (pos + l > len) return false;
        pos += l;
      } else if (wireType == 5) {
        if (pos + 4 > len) return false;
        pos += 4;
      } else if (wireType == 1) {
        if (pos + 8 > len) return false;
        pos += 8;
      } else {
        return false;
      }
    }
  }
  return portnum != 0;
}

struct LoRaProbeProfile {
  const char* label;
  float freqMhz;
  uint8_t syncWord;
};

// Meshtastic CN region uses 470-510 MHz. The user-provided Channel URL
// decodes to manual BW250/SF11/CR5, Region CN, txPower19, hopLimit3 and
// rx-boost enabled. Meshtastic chooses a frequency slot; for default
// LongFast/CN field logs commonly show slot 35 at 478.875 MHz. This probe
// therefore starts at CN35 and includes nearby 250 kHz slots.
static const LoRaProbeProfile LORA_PROBE_PROFILES[] = {
  {"CN35", 478.875f, 0x2B},
};
static const size_t LORA_PROBE_PROFILE_COUNT = sizeof(LORA_PROBE_PROFILES) / sizeof(LORA_PROBE_PROFILES[0]);

LoRaManager::LoRaManager()
  : radio(new Module(FtHardware::LORA_NSS_PIN, FtHardware::LORA_IRQ_PIN, FtHardware::LORA_RST_PIN, FtHardware::LORA_BUSY_PIN)),
    ready(false),
    listening(false),
    seq(0),
    rxOkCounter(0),
    rxTimeoutCounter(0),
    rxFailCounter(0),
    pkiDecryptOkCounter(0),
    pkiDecryptFailCounter(0),
    ackSentCounter(0),
    ackFailCounter(0),
    duplicateDropCounter(0),
    unreadMessageCounter(0),
    messageRevisionCounter(0),
    lastRxMillis(0),
    lastAutomaticNodeInfoRequestMs(0),
    statusText("WAIT"),
    payloadText(""),
    payloadHexText(""),
    frameKindText("--"),
    rawFrameLen(0),
    meshTo(0),
    meshFrom(0),
    meshId(0),
    meshFlags(0),
    meshChannel(0),
    meshPayloadLen(-1),
    meshIsBroadcast(false),
    lbxType(""),
    lbxDevice(""),
    lbxText(""),
    lbxSeq(0),
    meshDuplicatePacket(false),
    nodeInfoReplyPending(false),
    ackReplyPending(false),
    ackReplyTo(0),
    ackRequestId(0),
    ackReplyReliable(false),
    recentPacketCount(0),
    uniqueMeshSenderCount(0),
    inboxWriteIndex(0),
    inboxStored(0),
    pendingPkiReplaceIndex(0),
    nodeNameCount(0),
    payloadBytes(0),
    rssi(0.0f),
    snr(0.0f),
    profileIndex(0) {
  for (size_t i = 0; i < MESH_RECENT_PACKET_COUNT; i++) {
    recentPacketFrom[i] = 0;
    recentPacketIds[i] = 0;
  }
  for (size_t i = 0; i < MESH_SENDER_MAX; i++) uniqueMeshSenders[i] = 0;
  for (size_t i = 0; i < MESH_INBOX_MAX; i++) {
    inbox[i].valid = false;
    inbox[i].hex = "";
    inbox[i].kind = "";
    inbox[i].text = "";
    inbox[i].lbxType = "";
    inbox[i].lbxDevice = "";
    inbox[i].lbxSeq = 0;
  }
  for (size_t i = 0; i < PKI_PENDING_MAX; i++) {
    pendingPki[i].valid = false;
    pendingPki[i].encryptedLen = 0;
  }
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    nodeNames[i].node = 0;
    nodeNames[i].longName[0] = '\0';
    nodeNames[i].shortName[0] = '\0';
    nodeLastSeenMs[i] = 0;
  }
}

bool LoRaManager::begin() {
  if (!pki.begin()) {
    ready = false;
    statusText = "PKI INIT FAIL";
    Serial.println("MESH_PKI_INIT_FAIL");
    return false;
  }
  Serial.println("MESH_PKI_IDENTITY_READY persistent=true");
  loadNodeNames();
  FtHardware::prepareSharedSpiIdle(10000UL);

  const LoRaProbeProfile& profile = LORA_PROBE_PROFILES[profileIndex];

  Serial.print("LORA_PROBE_INIT_BEGIN profile=");
  Serial.print(profile.label);
  Serial.print(" freq=");
  Serial.print(profile.freqMhz, 3);
  Serial.print(" bw=");
  Serial.print((int)LORA_PROBE_BW_KHZ);
  Serial.print(" sf=");
  Serial.print(LORA_PROBE_SF);
  Serial.print(" cr=");
  Serial.print(LORA_PROBE_CR);
  Serial.print(" sync=0x");
  Serial.println(profile.syncWord, HEX);

  int16_t state = radio.begin(
    profile.freqMhz,
    LORA_PROBE_BW_KHZ,
    LORA_PROBE_SF,
    LORA_PROBE_CR,
    profile.syncWord,
    LORA_PROBE_POWER_DBM,
    LORA_PROBE_PREAMBLE
  );

  if (state != RADIOLIB_ERR_NONE) {
    ready = false;
    statusText = "INIT FAIL";
    Serial.print("LORA_PROBE_INIT_FAIL code=");
    Serial.println(state);
    return false;
  }

  // Many SX1262 boards route the RF switch through DIO2. If this board does not
  // require it, RadioLib still keeps the radio usable. This is a probe build, so
  // avoid hard-failing on RF switch configuration.
  radio.setDio2AsRfSwitch(true);

  ready = true;
  listening = false;
  radio.setPacketReceivedAction(onLoRaPacketReceived);
  statusText = "INIT OK";
  Serial.println("LORA_PROBE_INIT_OK");
  return startListening();
}

bool LoRaManager::isReady() const {
  return ready;
}

bool LoRaManager::startListening() {
  if (!ready && !begin()) return false;
  FtHardware::prepareSharedSpiIdle(0);
  gLoRaPacketReceived = false;
  int16_t state = radio.startReceive();
  digitalWrite(FtHardware::LORA_NSS_PIN, HIGH);
  listening = state == RADIOLIB_ERR_NONE;
  if (!listening) {
    statusText = "RX START FAIL";
    rxFailCounter++;
    Serial.print("LORA_RX_START_FAIL code=");
    Serial.println(state);
  }
  return listening;
}

bool LoRaManager::isListening() const {
  return listening;
}

uint32_t LoRaManager::ackSentCount() const { return ackSentCounter; }
uint32_t LoRaManager::ackFailCount() const { return ackFailCounter; }
uint32_t LoRaManager::duplicateDropCount() const { return duplicateDropCounter; }

size_t LoRaManager::unreadMessageCount() const {
  return unreadMessageCounter;
}

uint32_t LoRaManager::messageRevision() const {
  return messageRevisionCounter;
}

void LoRaManager::markMessagesRead() {
  unreadMessageCounter = 0;
}

bool LoRaManager::pkiReady() const {
  return pki.isReady();
}

uint8_t LoRaManager::pkiPeerCount() const {
  return pki.peerCount();
}

uint32_t LoRaManager::pkiDecryptOkCount() const {
  return pkiDecryptOkCounter;
}

uint32_t LoRaManager::pkiDecryptFailCount() const {
  return pkiDecryptFailCounter;
}

uint8_t LoRaManager::pkiPendingCount() const {
  uint8_t count = 0;
  for (size_t i = 0; i < PKI_PENDING_MAX; i++) if (pendingPki[i].valid) count++;
  return count;
}

void LoRaManager::cycleProfile() {
  profileIndex = (profileIndex + 1) % LORA_PROBE_PROFILE_COUNT;
  ready = false;
  listening = false;
  statusText = String("PROFILE ") + currentProfileName();
  Serial.print("LORA_PROBE_PROFILE profile=");
  Serial.print(currentProfileName());
  Serial.print(" freq=");
  Serial.print(currentFreqMhz(), 3);
  Serial.print(" sync=0x");
  Serial.println(LORA_PROBE_PROFILES[profileIndex].syncWord, HEX);
}

float LoRaManager::currentFreqMhz() const {
  return LORA_PROBE_PROFILES[profileIndex].freqMhz;
}

String LoRaManager::currentProfileName() const {
  return String(LORA_PROBE_PROFILES[profileIndex].label);
}

bool LoRaManager::sendMeshtasticNodeInfo(bool requestReplies) {
  uint8_t userProto[128];
  size_t userLen = 0;
  if (!pki.isReady() || !buildUserProto(pki.publicKey(), userProto, sizeof(userProto), userLen)) {
    statusText = "MESH NI BUILD FAIL";
    Serial.println("MESH_NATIVE_TX_FAIL type=NODEINFO error=user_proto");
    return false;
  }
  return sendMeshtasticData(MESH_PORT_NODEINFO_APP, userProto, userLen, "NODEINFO", requestReplies);
}

bool LoRaManager::sendMeshtasticText(const String& text) {
  String clean = sanitizeText(text, FT_MESH_TEXT_PAYLOAD_MAX_BYTES);
  if (clean.length() == 0) {
    statusText = "MESH EMPTY";
    Serial.println("MESH_TX_REJECT empty");
    return false;
  }
  return sendMeshtasticData(MESH_PORT_TEXT_MESSAGE_APP, (const uint8_t*)clean.c_str(), clean.length(), "TEXT", false);
}

bool LoRaManager::sendMeshtasticPrivateText(uint32_t toNode, const String& text) {
  String clean = sanitizeText(text, FT_MESH_TEXT_PAYLOAD_MAX_BYTES);
  if (clean.length() == 0) {
    statusText = "PKI EMPTY";
    Serial.println("MESH_PKI_TX_REJECT empty");
    return false;
  }
  if (toNode == 0 || toNode == MESH_NATIVE_BROADCAST || toNode == MESH_NATIVE_NODE_NUM) {
    statusText = "PKI BAD NODE";
    Serial.println("MESH_PKI_TX_REJECT invalid_node");
    return false;
  }
  if (!ready && !begin()) return false;
  const uint8_t* peerPublicKey = pki.peerPublicKey(toNode);
  if (!peerPublicKey) {
    statusText = "PKI KEY MISSING";
    Serial.print("MESH_PKI_TX_REJECT key_missing to=");
    Serial.println(formatHex32(toNode));
    return false;
  }

  uint8_t dataProto[180];
  size_t dataLen = 0;
  if (!buildDataProto(MESH_PORT_TEXT_MESSAGE_APP,
                      (const uint8_t*)clean.c_str(),
                      clean.length(),
                      false,
                      0,
                      dataProto,
                      sizeof(dataProto),
                      dataLen)) {
    statusText = "PKI DATA FAIL";
    return false;
  }

  seq++;
  uint32_t packetId = esp_random();
  if (packetId == 0) packetId = 0x50000000UL | (seq & 0x00FFFFFFUL);

  uint8_t encrypted[MESH_NATIVE_FRAME_MAX - MESH_NATIVE_HEADER_LEN];
  size_t encryptedLen = 0;
  if (!pki.encryptForPeer(peerPublicKey,
                          packetId,
                          MESH_NATIVE_NODE_NUM,
                          dataProto,
                          dataLen,
                          encrypted,
                          sizeof(encrypted),
                          encryptedLen)) {
    statusText = "PKI ENCRYPT FAIL";
    Serial.print("MESH_PKI_TX_FAIL error=encrypt to=");
    Serial.println(formatHex32(toNode));
    return false;
  }

  return transmitMeshFrame(toNode, 0, encrypted, encryptedLen, "PKI_TEXT", true, packetId);
}

bool LoRaManager::sendMeshtasticData(uint8_t portnum,
                                      const uint8_t* appPayload,
                                      size_t appLen,
                                      const char* typeName,
                                      bool wantResponse) {
  return sendChannelPacket(MESH_NATIVE_BROADCAST,
                           portnum,
                           appPayload,
                           appLen,
                           typeName,
                           wantResponse,
                           false,
                           0);
}

bool LoRaManager::sendChannelPacket(uint32_t toNode,
                                    uint8_t portnum,
                                    const uint8_t* appPayload,
                                    size_t appLen,
                                    const char* typeName,
                                    bool wantResponse,
                                    bool wantAck,
                                    uint32_t requestId) {
  if (!ready && !begin()) return false;

  seq++;
  uint32_t packetId = esp_random();
  if (packetId == 0) packetId = 0x40000000UL | (seq & 0x00FFFFFFUL);

  uint8_t dataProto[180];
  size_t dataLen = 0;
  if (!buildDataProto(portnum, appPayload, appLen, wantResponse, requestId, dataProto, sizeof(dataProto), dataLen)) {
    statusText = "MESH DATA FAIL";
    Serial.print("MESH_NATIVE_TX_FAIL type=");
    Serial.print(typeName);
    Serial.println(" error=data_proto");
    startListening();
    return false;
  }

  if (!aesCtrCrypt(dataProto, dataLen, MESH_NATIVE_NODE_NUM, packetId)) {
    statusText = "MESH CRYPTO FAIL";
    Serial.print("MESH_NATIVE_TX_FAIL type=");
    Serial.print(typeName);
    Serial.println(" error=aes_ctr");
    startListening();
    return false;
  }

  return transmitMeshFrame(toNode, FT01_MESH_CHANNEL_HASH, dataProto, dataLen, typeName, wantAck, packetId);
}

bool LoRaManager::transmitMeshFrame(uint32_t toNode,
                                    uint8_t channel,
                                    const uint8_t* payload,
                                    size_t payloadLen,
                                    const char* typeName,
                                    bool wantAck,
                                    uint32_t packetId) {
  if (!ready && !begin()) return false;
  if (!payload || payloadLen == 0 || MESH_NATIVE_HEADER_LEN + payloadLen > MESH_NATIVE_FRAME_MAX) {
    statusText = "MESH TOO LARGE";
    Serial.print("MESH_NATIVE_TX_FAIL type=");
    Serial.print(typeName);
    Serial.println(" error=too_large");
    return false;
  }

  FtHardware::prepareSharedSpiIdle(0);
  listening = false;
  gLoRaPacketReceived = false;

  uint8_t frame[MESH_NATIVE_FRAME_MAX];
  writeLe32To(frame + 0, toNode);
  writeLe32To(frame + 4, MESH_NATIVE_NODE_NUM);
  writeLe32To(frame + 8, packetId);
  frame[12] = (uint8_t)((MESH_NATIVE_HOP_START << 5) |
                        (wantAck ? MESH_FLAG_WANT_ACK : 0) |
                        (MESH_NATIVE_HOP_LIMIT & 0x07));
  frame[13] = channel;
  frame[14] = 0x00;
  frame[15] = (uint8_t)(MESH_NATIVE_NODE_NUM & 0xFF);
  memcpy(frame + MESH_NATIVE_HEADER_LEN, payload, payloadLen);
  size_t frameLen = MESH_NATIVE_HEADER_LEN + payloadLen;

  Serial.print("MESH_NATIVE_TX_BEGIN encrypted=true type=");
  Serial.print(typeName);
  Serial.print(" id=");
  Serial.print(formatHex32(packetId));
  Serial.print(" to=");
  Serial.print(formatHex32(toNode));
  Serial.print(" channel=");
  Serial.print(formatHex8(channel));
  Serial.print(" want_ack=");
  Serial.print(wantAck ? "true" : "false");
  Serial.print(" len=");
  Serial.println((int)frameLen);

  int16_t state = radio.transmit(frame, frameLen);
  digitalWrite(FtHardware::LORA_NSS_PIN, HIGH);
  bool txOk = state == RADIOLIB_ERR_NONE;

  if (!txOk) {
    statusText = "MESH TX FAIL";
    Serial.print("MESH_NATIVE_TX_FAIL type=");
    Serial.print(typeName);
    Serial.print(" code=");
    Serial.println(state);
  } else {
    statusText = String(typeName) + " TX OK";
    Serial.print("MESH_NATIVE_TX_OK type=");
    Serial.print(typeName);
    Serial.print(" id=");
    Serial.println(formatHex32(packetId));
  }

  startListening();
  return txOk;
}

bool LoRaManager::sendRoutingAck(uint32_t toNode, uint32_t requestId, bool reliableAck) {
  // Routing.error_reason = NONE.  Oneof presence is encoded explicitly as
  // field 3, enum value 0, matching Meshtastic routing ACK packets.
  const uint8_t routingOk[] = {0x18, 0x00};
  bool ok = sendChannelPacket(toNode,
                              MESH_PORT_ROUTING_APP,
                              routingOk,
                              sizeof(routingOk),
                              "ACK",
                              false,
                              reliableAck,
                              requestId);
  if (ok) {
    ackSentCounter++;
    Serial.print("MESH_ACK_TX_OK to=");
    Serial.print(formatHex32(toNode));
    Serial.print(" request_id=");
    Serial.println(formatHex32(requestId));
  } else {
    ackFailCounter++;
    Serial.print("MESH_ACK_TX_FAIL to=");
    Serial.print(formatHex32(toNode));
    Serial.print(" request_id=");
    Serial.println(formatHex32(requestId));
  }
  return ok;
}

bool LoRaManager::processReceivedFrame(const uint8_t* data, size_t len) {
  if (!data || len == 0 || len > RAW_FRAME_MAX) return false;

  rawFrameLen = len;
  memcpy(rawFrame, data, rawFrameLen);
  payloadText = "";
  payloadHexText = toHexPreview(rawFrame, rawFrameLen, RAW_FRAME_MAX);
  frameKindText = classifyFrame(rawFrame, rawFrameLen);
  nodeInfoReplyPending = false;
  ackReplyPending = false;
  payloadBytes = (int)rawFrameLen;

  bool isLbx = parseLbxFrame(rawFrame, rawFrameLen);
  if (!isLbx) {
    parseMeshHeader(rawFrame, rawFrameLen);
    tryDecodeMeshtasticPayload();
  }

  const uint32_t nowMs = millis();
  const bool unknownMessageSender = !isLbx && isMessageKind(frameKindText) &&
                                    meshFrom != 0 && meshFrom != MESH_NATIVE_NODE_NUM &&
                                    !hasKnownNodeName(meshFrom);
  const bool requestUnknownName = unknownMessageSender &&
                                  (lastAutomaticNodeInfoRequestMs == 0 ||
                                   nowMs - lastAutomaticNodeInfoRequestMs >= 60000UL);

  rememberFrameSummary();
  statusText = "RX OK";
  rxOkCounter++;
  lastRxMillis = millis();

  Serial.print("LORA_RX_OK profile=");
  Serial.print(currentProfileName());
  Serial.print(" len=");
  Serial.print(payloadBytes);
  Serial.print(" rssi=");
  Serial.print(rssi, 1);
  Serial.print(" snr=");
  Serial.print(snr, 1);
  Serial.print(" kind=");
  Serial.println(frameKindText);

  const bool replyNodeInfo = nodeInfoReplyPending;
  const bool replyAck = ackReplyPending;
  const uint32_t replyTo = ackReplyTo;
  const uint32_t replyRequestId = ackRequestId;
  const bool reliableAck = ackReplyReliable;
  nodeInfoReplyPending = false;
  ackReplyPending = false;

  if (replyAck) sendRoutingAck(replyTo, replyRequestId, reliableAck);
  if (replyNodeInfo) {
    Serial.println("MESH_NODEINFO_REPLY requested=true");
    sendMeshtasticNodeInfo(false);
  }
  if (requestUnknownName) {
    lastAutomaticNodeInfoRequestMs = millis();
    Serial.print("MESH_NODEINFO_AUTO_REQUEST unknown_from=");
    Serial.println(formatHex32(meshFrom));
    sendMeshtasticNodeInfo(true);
  }
  return true;
}

bool LoRaManager::pollReceive() {
  if (!ready && !begin()) return false;
  if (!listening && !startListening()) return false;
  if (!gLoRaPacketReceived) return false;

  gLoRaPacketReceived = false;
  listening = false;
  FtHardware::prepareSharedSpiIdle(0);

  size_t packetLen = radio.getPacketLength();
  if (packetLen == 0 || packetLen > RAW_FRAME_MAX) {
    rxFailCounter++;
    statusText = "RX LENGTH FAIL";
    Serial.print("LORA_RX_LENGTH_FAIL len=");
    Serial.println((int)packetLen);
    startListening();
    return false;
  }

  uint8_t frameBuf[RAW_FRAME_MAX];
  int16_t state = radio.readData(frameBuf, packetLen);
  digitalWrite(FtHardware::LORA_NSS_PIN, HIGH);
  if (state != RADIOLIB_ERR_NONE) {
    rxFailCounter++;
    statusText = "RX READ FAIL";
    Serial.print("LORA_RX_READ_FAIL code=");
    Serial.println(state);
    startListening();
    return false;
  }

  rssi = radio.getRSSI();
  snr = radio.getSNR();
  bool processed = processReceivedFrame(frameBuf, packetLen);
  if (!listening) startListening();
  return processed;
}

bool LoRaManager::receiveProbeOnce(uint32_t timeoutMs) {
  if (!startListening()) return false;
  unsigned long started = millis();
  while (millis() - started < timeoutMs) {
    if (pollReceive()) return true;
    delay(1);
  }
  rxTimeoutCounter++;
  statusText = "RX TIMEOUT";
  return false;
}

String LoRaManager::classifyFrame(const uint8_t* data, size_t len) const {
  if (len == 0) return "EMPTY";
  if (len >= 4 && data[0] == 'L' && data[1] == 'B' && data[2] == 'X' && (data[3] == '_' || data[3] == '1')) return "LBX_TEXT";
  bool mostlyPrintable = true;
  size_t n = len;
  if (n > 32) n = 32;
  for (size_t i = 0; i < n; i++) {
    uint8_t b = data[i];
    if (b < 0x20 || b > 0x7E) {
      mostlyPrintable = false;
      break;
    }
  }
  if (mostlyPrintable) return "TEXT";
  return "MESH_RAW";
}

uint32_t LoRaManager::readLe32(const uint8_t* data) const {
  return ((uint32_t)data[0]) |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

String LoRaManager::formatHex32(uint32_t value) const {
  char buf[11];
  snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)value);
  return String(buf);
}

String LoRaManager::formatHex8(uint8_t value) const {
  char buf[5];
  snprintf(buf, sizeof(buf), "0x%02X", value);
  return String(buf);
}

String LoRaManager::formatNodeShort(uint32_t value) const {
  char buf[9];
  snprintf(buf, sizeof(buf), "%08lX", (unsigned long)value);
  return String(buf);
}

void LoRaManager::rememberMeshPacket(uint32_t from, uint32_t packetId) {
  meshDuplicatePacket = false;
  if (from != 0 && packetId != 0) {
    for (size_t i = 0; i < recentPacketCount; i++) {
      if (recentPacketFrom[i] == from && recentPacketIds[i] == packetId) {
        meshDuplicatePacket = true;
        break;
      }
    }
  }

  if (!meshDuplicatePacket && from != 0 && packetId != 0) {
    if (recentPacketCount < MESH_RECENT_PACKET_COUNT) {
      recentPacketFrom[recentPacketCount] = from;
      recentPacketIds[recentPacketCount] = packetId;
      recentPacketCount++;
    } else {
      for (size_t i = MESH_RECENT_PACKET_COUNT - 1; i > 0; i--) {
        recentPacketFrom[i] = recentPacketFrom[i - 1];
        recentPacketIds[i] = recentPacketIds[i - 1];
      }
      recentPacketFrom[0] = from;
      recentPacketIds[0] = packetId;
    }
  }

  if (from != 0) {
    bool knownSender = false;
    for (size_t i = 0; i < uniqueMeshSenderCount; i++) {
      if (uniqueMeshSenders[i] == from) {
        knownSender = true;
        break;
      }
    }
    if (!knownSender && uniqueMeshSenderCount < MESH_SENDER_MAX) {
      uniqueMeshSenders[uniqueMeshSenderCount++] = from;
    }
  }
}

void LoRaManager::parseMeshHeader(const uint8_t* data, size_t len) {
  meshTo = 0;
  meshFrom = 0;
  meshId = 0;
  meshFlags = 0;
  meshChannel = 0;
  meshPayloadLen = -1;
  meshIsBroadcast = false;
  meshDuplicatePacket = false;
  nodeInfoReplyPending = false;
  ackReplyPending = false;
  ackReplyTo = 0;
  ackRequestId = 0;
  ackReplyReliable = false;
  lbxType = "";
  lbxDevice = "";
  lbxText = "";
  lbxSeq = 0;
  if (len >= MESH_NATIVE_HEADER_LEN) {
    meshTo = readLe32(data + 0);
    meshFrom = readLe32(data + 4);
    meshId = readLe32(data + 8);
    meshFlags = data[12];
    meshChannel = data[13];
    meshPayloadLen = (int)len - (int)MESH_NATIVE_HEADER_LEN;
    meshIsBroadcast = (meshTo == 0xFFFFFFFFUL);
    rememberMeshPacket(meshFrom, meshId);
    rememberNodeSeen(meshFrom);
  }
}

bool LoRaManager::tryDecodeMeshtasticPayload() {
  if (rawFrameLen < MESH_NATIVE_HEADER_LEN || meshPayloadLen <= 0) return false;

  const size_t encryptedLen = rawFrameLen - MESH_NATIVE_HEADER_LEN;
  if (encryptedLen > PKI_PENDING_PAYLOAD_MAX) return false;
  uint8_t plain[PKI_PENDING_PAYLOAD_MAX];
  size_t plainLen = encryptedLen;
  bool pkiEncrypted = false;

  if (meshChannel == FT01_MESH_CHANNEL_HASH) {
    memcpy(plain, rawFrame + MESH_NATIVE_HEADER_LEN, encryptedLen);
    if (!aesCtrCrypt(plain, encryptedLen, meshFrom, meshId)) return false;
  } else if (meshChannel == 0 && meshTo == MESH_NATIVE_NODE_NUM && meshFrom != 0 && meshFrom != MESH_NATIVE_NODE_NUM) {
    const uint8_t* peerPublicKey = pki.peerPublicKey(meshFrom);
    if (!peerPublicKey) {
      rememberPendingPki(rawFrame + MESH_NATIVE_HEADER_LEN, encryptedLen);
      frameKindText = "MESH_PKI_WAIT";
      payloadText = "等待发送方公钥";
      Serial.print("MESH_PKI_DECODE_WAIT from=");
      Serial.print(formatHex32(meshFrom));
      Serial.print(" id=");
      Serial.print(formatHex32(meshId));
      Serial.println(" error=peer_key_unknown");
      return true;
    }
    if (!pki.decryptFromPeer(peerPublicKey,
                             meshId,
                             meshFrom,
                             rawFrame + MESH_NATIVE_HEADER_LEN,
                             encryptedLen,
                             plain,
                             sizeof(plain),
                             plainLen)) {
      pkiDecryptFailCounter++;
      frameKindText = "MESH_PKI_FAIL";
      payloadText = "认证或解密失败";
      Serial.print("MESH_PKI_DECODE_FAIL from=");
      Serial.print(formatHex32(meshFrom));
      Serial.print(" id=");
      Serial.print(formatHex32(meshId));
      Serial.println(" error=auth");
      return true;
    }
    pkiEncrypted = true;
    pkiDecryptOkCounter++;
  } else {
    return false;
  }

  return handleDecodedMeshData(plain, plainLen, pkiEncrypted);
}

bool LoRaManager::handleDecodedMeshData(const uint8_t* plain, size_t plainLen, bool pkiEncrypted) {
  uint32_t portnum = 0;
  const uint8_t* appPayload = nullptr;
  size_t appLen = 0;
  bool wantResponse = false;
  if (!parseDataProto(plain, plainLen, portnum, appPayload, appLen, wantResponse)) {
    Serial.print(pkiEncrypted ? "MESH_PKI_DECODE_FAIL from=" : "MESH_DECODE_FAIL from=");
    Serial.print(formatHex32(meshFrom));
    Serial.print(" id=");
    Serial.print(formatHex32(meshId));
    Serial.println(" error=data_proto");
    return false;
  }

  if (meshTo == MESH_NATIVE_NODE_NUM && meshFrom != 0 && meshFrom != MESH_NATIVE_NODE_NUM &&
      (meshFlags & MESH_FLAG_WANT_ACK) != 0) {
    ackReplyPending = true;
    ackReplyTo = meshFrom;
    ackRequestId = meshId;
    // Meshtastic makes text-message ACKs reliable. ACKs for routing packets
    // themselves must not request another ACK, otherwise an ACK loop forms.
    ackReplyReliable = portnum == MESH_PORT_TEXT_MESSAGE_APP;
  }

  if (portnum == MESH_PORT_TEXT_MESSAGE_APP) {
    frameKindText = pkiEncrypted ? "MESH_PKI_TEXT" : "MESH_TEXT";
    payloadText = decodeTextPayload(appPayload, appLen);
    Serial.print(pkiEncrypted ? "MESH_PKI_DECODE_OK portnum=TEXT_MESSAGE_APP from=" : "MESH_DECODE encrypted=true portnum=TEXT_MESSAGE_APP from=");
    Serial.print(formatHex32(meshFrom));
    Serial.print(" id=");
    Serial.print(formatHex32(meshId));
    Serial.print(" text_len=");
    Serial.print((int)appLen);
    Serial.println(" text=redacted");
    return true;
  }

  if (portnum == MESH_PORT_NODEINFO_APP) {
    frameKindText = "MESH_NODEINFO";
    ParsedUserInfo user;
    bool parsedUser = parseUserInfo(appPayload, appLen, user);
    bool learnedKey = parsedUser && user.hasPublicKey;
    uint32_t identityNode = parsedUser && user.nodeNumber != 0 ? user.nodeNumber : meshFrom;
    bool identityMatchesHeader = identityNode == meshFrom;
    if (meshFrom != MESH_NATIVE_NODE_NUM && parsedUser && identityMatchesHeader) {
      rememberNodeName(identityNode, user.longName, user.shortName);
      if (learnedKey) {
        pki.rememberPeer(identityNode, user.publicKey);
        retryPendingForPeer(identityNode);
      }
    }
    if (wantResponse && meshFrom != MESH_NATIVE_NODE_NUM) nodeInfoReplyPending = true;
    String name = nodeDisplayName(meshFrom);
    payloadText = name + (learnedKey ? " PKI" : " NODEINFO");
    Serial.print("MESH_NODEINFO_RX from=");
    Serial.print(formatHex32(meshFrom));
    Serial.print(" user_id=");
    Serial.print(parsedUser ? user.id : String("--"));
    Serial.print(" identity_match=");
    Serial.print(identityMatchesHeader ? "true" : "false");
    Serial.print(" name=");
    Serial.print(name);
    Serial.print(" pki_key=");
    Serial.print(learnedKey ? "learned" : "absent");
    Serial.print(" peers=");
    Serial.print((int)pki.peerCount());
    Serial.print(" want_response=");
    Serial.println(wantResponse ? "true" : "false");
    return true;
  }

  frameKindText = pkiEncrypted ? "MESH_PKI_DATA" : "MESH_DECODED";
  payloadText = String("portnum=") + String(portnum) + String(" payload=") + String((int)appLen);
  Serial.print(pkiEncrypted ? "MESH_PKI_DECODE_OK portnum=" : "MESH_DECODE encrypted=true portnum=");
  Serial.print(portnum);
  Serial.print(" from=");
  Serial.print(formatHex32(meshFrom));
  Serial.print(" id=");
  Serial.print(formatHex32(meshId));
  Serial.print(" payload_len=");
  Serial.println((int)appLen);
  return true;
}



String LoRaManager::decodeTextPayload(const uint8_t* payload, size_t len) const {
  String text;
  if (!payload) return text;
  text.reserve(min(len, (size_t)160));
  for (size_t i = 0; i < len && text.length() < 160; i++) {
    uint8_t b = payload[i];
    if (b == '\r' || b == '\n' || b == '\t') b = ' ';
    if (b >= 0x20 && b != 0x7F) text += (char)b;
  }
  return text;
}

void LoRaManager::rememberPendingPki(const uint8_t* encrypted, size_t encryptedLen) {
  if (!encrypted || encryptedLen == 0 || encryptedLen > PKI_PENDING_PAYLOAD_MAX) return;

  size_t slot = PKI_PENDING_MAX;
  for (size_t i = 0; i < PKI_PENDING_MAX; i++) {
    if (pendingPki[i].valid && pendingPki[i].from == meshFrom && pendingPki[i].id == meshId) return;
    if (!pendingPki[i].valid && slot == PKI_PENDING_MAX) slot = i;
  }
  if (slot == PKI_PENDING_MAX) {
    slot = pendingPkiReplaceIndex;
    pendingPkiReplaceIndex = (pendingPkiReplaceIndex + 1) % PKI_PENDING_MAX;
  }

  PendingPkiFrame& pending = pendingPki[slot];
  pending.valid = true;
  pending.from = meshFrom;
  pending.to = meshTo;
  pending.id = meshId;
  pending.flags = meshFlags;
  pending.encryptedLen = encryptedLen;
  memcpy(pending.encrypted, encrypted, encryptedLen);
}

bool LoRaManager::updateInboxDecoded(uint32_t from, uint32_t packetId, const String& kind, const String& text) {
  for (size_t i = 0; i < MESH_INBOX_MAX; i++) {
    LoRaFrameView& frame = inbox[i];
    if (frame.valid && frame.from == from && frame.id == packetId) {
      frame.kind = kind;
      frame.text = text;
      frame.lbxType = kind;
      frame.lbxDevice = nodeDisplayName(from);
      messageRevisionCounter++;
      return true;
    }
  }
  return false;
}

void LoRaManager::retryPendingForPeer(uint32_t node) {
  const uint8_t* peerPublicKey = pki.peerPublicKey(node);
  if (!peerPublicKey) return;

  for (size_t i = 0; i < PKI_PENDING_MAX; i++) {
    PendingPkiFrame& pending = pendingPki[i];
    if (!pending.valid || pending.from != node) continue;

    uint8_t plain[PKI_PENDING_PAYLOAD_MAX];
    size_t plainLen = 0;
    if (!pki.decryptFromPeer(peerPublicKey,
                             pending.id,
                             pending.from,
                             pending.encrypted,
                             pending.encryptedLen,
                             plain,
                             sizeof(plain),
                             plainLen)) {
      pkiDecryptFailCounter++;
      updateInboxDecoded(pending.from, pending.id, "MESH_PKI_FAIL", "认证或解密失败");
      pending.valid = false;
      Serial.print("MESH_PKI_RETRY_FAIL from=");
      Serial.print(formatHex32(pending.from));
      Serial.print(" id=");
      Serial.println(formatHex32(pending.id));
      continue;
    }

    uint32_t portnum = 0;
    const uint8_t* appPayload = nullptr;
    size_t appLen = 0;
    bool wantResponse = false;
    if (!parseDataProto(plain, plainLen, portnum, appPayload, appLen, wantResponse)) {
      pkiDecryptFailCounter++;
      updateInboxDecoded(pending.from, pending.id, "MESH_PKI_FAIL", "数据格式错误");
      pending.valid = false;
      continue;
    }

    if (portnum == MESH_PORT_TEXT_MESSAGE_APP) {
      String text = decodeTextPayload(appPayload, appLen);
      updateInboxDecoded(pending.from, pending.id, "MESH_PKI_TEXT", text);
      pkiDecryptOkCounter++;
      Serial.print("MESH_PKI_RETRY_OK from=");
      Serial.print(formatHex32(pending.from));
      Serial.print(" id=");
      Serial.print(formatHex32(pending.id));
      Serial.println(" text=redacted");
    } else {
      updateInboxDecoded(pending.from, pending.id, "MESH_PKI_DATA", String("portnum=") + String(portnum));
      pkiDecryptOkCounter++;
    }
    if ((pending.flags & MESH_FLAG_WANT_ACK) != 0) {
      sendRoutingAck(pending.from, pending.id, portnum == MESH_PORT_TEXT_MESSAGE_APP);
    }
    pending.valid = false;
  }
}

bool LoRaManager::isMessageKind(const String& kind) const {
  return kind == "LBX_TEXT" || kind == "MESH_TEXT" || kind == "MESH_PKI_TEXT" ||
         kind == "MESH_PKI_WAIT" || kind == "MESH_PKI_FAIL";
}

String LoRaManager::sanitizeText(const String& text, size_t maxLen) const {
  String out;
  for (int i = 0; i < text.length() && (size_t)out.length() < maxLen; i++) {
    char c = text[i];
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    if ((uint8_t)c < 0x20 || (uint8_t)c > 0x7E) continue;
    if (c == '|') {
      out += '/';
    } else {
      out += c;
    }
  }
  return out;
}

bool LoRaManager::parseLbxFrame(const uint8_t* data, size_t len) {
  (void)data;
  (void)len;
  return false;
}

void LoRaManager::printLbxFrame(const char* prefix) const {
  Serial.println(prefix);
}

void LoRaManager::printMeshHeader(const char* prefix) const {
  if (meshPayloadLen < 0) return;
  Serial.print(prefix);
  Serial.print(" to=");
  Serial.print(meshIsBroadcast ? String("Broadcast(") + formatHex32(meshTo) + ")" : formatHex32(meshTo));
  Serial.print(" from=");
  Serial.print(formatHex32(meshFrom));
  Serial.print(" id=");
  Serial.print(formatHex32(meshId));
  Serial.print(" flags=");
  Serial.print(formatHex8(meshFlags));
  Serial.print(" channel=");
  Serial.print(formatHex8(meshChannel));
  Serial.print(" payload_len=");
  Serial.print(meshPayloadLen);
  Serial.print(" duplicate=");
  Serial.print(meshDuplicatePacket ? "true" : "false");
  Serial.print(" senders=");
  Serial.println((int)uniqueMeshSenderCount);
}

void LoRaManager::rememberFrameSummary() {
  if (rawFrameLen == 0) return;
  bool isLbx = (frameKindText == "LBX_TEXT");
  bool isDecodedMesh = (frameKindText == "MESH_TEXT" || frameKindText == "MESH_PKI_TEXT" || frameKindText == "MESH_PKI_WAIT" || frameKindText == "MESH_PKI_FAIL" || frameKindText == "MESH_NODEINFO" || frameKindText == "MESH_DECODED" || frameKindText == "MESH_PKI_DATA");
  if (!isLbx && !isDecodedMesh && meshPayloadLen < 0) return;

  if (!isLbx && meshDuplicatePacket) {
    duplicateDropCounter++;
    Serial.print("LORA_INBOX_SKIP_DUP from=");
    Serial.print(formatHex32(meshFrom));
    Serial.print(" id=");
    Serial.println(formatHex32(meshId));
    return;
  }

  LoRaFrameView& frame = inbox[inboxWriteIndex];
  frame.valid = true;
  frame.to = meshTo;
  frame.from = meshFrom;
  frame.id = meshId;
  frame.flags = meshFlags;
  frame.channel = meshChannel;
  frame.payloadLen = isLbx ? (int)rawFrameLen : meshPayloadLen;
  frame.bytes = (int)rawFrameLen;
  frame.rssi = rssi;
  frame.snr = snr;
  frame.ageMs = millis();
  frame.broadcast = meshIsBroadcast;
  frame.duplicate = meshDuplicatePacket;
  frame.hex = payloadHexText;
  frame.kind = frameKindText;
  frame.text = isLbx ? lbxText : (isDecodedMesh ? payloadText : "");
  frame.lbxType = isLbx ? lbxType : (isDecodedMesh ? frameKindText : "");
  frame.lbxDevice = isLbx ? lbxDevice : (isDecodedMesh ? nodeDisplayName(meshFrom) : "");
  frame.lbxSeq = isLbx ? lbxSeq : 0;

  Serial.print("LORA_INBOX_ADD slot=");
  Serial.print((int)inboxWriteIndex);
  Serial.print(" total=");
  Serial.print((int)(inboxStored < MESH_INBOX_MAX ? inboxStored + 1 : inboxStored));
  Serial.print(" kind=");
  Serial.print(frame.kind);
  if (isLbx || isDecodedMesh) {
    Serial.print(" device=");
    Serial.print(frame.lbxDevice);
    Serial.print(" type=");
    Serial.print(frame.lbxType.length() ? frame.lbxType : frame.kind);
    if (isLbx) {
      Serial.print(" seq=");
      Serial.print(frame.lbxSeq);
    }
    Serial.print(" text=redacted");
  } else {
    Serial.print(" from=");
    Serial.print(formatHex32(frame.from));
    Serial.print(" id=");
    Serial.print(formatHex32(frame.id));
    Serial.print(" payload_len=");
    Serial.print(frame.payloadLen);
  }
  Serial.print(" rssi=");
  Serial.print(frame.rssi, 1);
  Serial.print(" snr=");
  Serial.println(frame.snr, 1);

  if (isMessageKind(frame.kind)) {
    if (unreadMessageCounter < MESH_INBOX_MAX) unreadMessageCounter++;
    messageRevisionCounter++;
  }

  inboxWriteIndex = (inboxWriteIndex + 1) % MESH_INBOX_MAX;
  if (inboxStored < MESH_INBOX_MAX) inboxStored++;
}

String LoRaManager::toHexPreview(const uint8_t* data, size_t len, size_t maxBytes) const {
  const char* hex = "0123456789ABCDEF";
  String out;
  size_t n = len;
  if (n > maxBytes) n = maxBytes;
  for (size_t i = 0; i < n; i++) {
    uint8_t b = data[i];
    if (i > 0) out += " ";
    out += hex[(b >> 4) & 0x0F];
    out += hex[b & 0x0F];
  }
  return out;
}

void LoRaManager::copyUtf8Name(char* destination, size_t destinationSize, const String& source) const {
  if (!destination || destinationSize == 0) return;
  destination[0] = '\0';
  size_t sourcePos = 0;
  size_t outputPos = 0;
  const size_t sourceLen = (size_t)source.length();
  while (sourcePos < sourceLen) {
    uint8_t first = (uint8_t)source[(int)sourcePos];
    size_t charBytes = 1;
    if ((first & 0xE0) == 0xC0) charBytes = 2;
    else if ((first & 0xF0) == 0xE0) charBytes = 3;
    else if ((first & 0xF8) == 0xF0) charBytes = 4;
    if (sourcePos + charBytes > sourceLen || outputPos + charBytes >= destinationSize) break;
    memcpy(destination + outputPos, source.c_str() + sourcePos, charBytes);
    sourcePos += charBytes;
    outputPos += charBytes;
  }
  destination[outputPos] = '\0';
}

void LoRaManager::loadNodeNames() {
  nodeNameCount = 0;
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    nodeNames[i].node = 0;
    nodeNames[i].longName[0] = '\0';
    nodeNames[i].shortName[0] = '\0';
    nodeLastSeenMs[i] = 0;
  }

  Preferences preferences;
  if (!preferences.begin("ft01nodes", true)) return;
  size_t stored = preferences.getBytesLength("names");
  if (stored > 0 && stored <= sizeof(nodeNames) && stored % sizeof(NodeNameEntry) == 0) {
    preferences.getBytes("names", nodeNames, stored);
    size_t records = stored / sizeof(NodeNameEntry);
    for (size_t i = 0; i < records; i++) {
      nodeNames[i].longName[NODE_LONG_NAME_BYTES - 1] = '\0';
      nodeNames[i].shortName[NODE_SHORT_NAME_BYTES - 1] = '\0';
      if (nodeNames[i].node != 0 && nodeNames[i].node != MESH_NATIVE_NODE_NUM &&
          nodeNames[i].node != MESH_NATIVE_BROADCAST) {
        nodeNameCount++;
      } else {
        nodeNames[i].node = 0;
      }
    }
  }
  preferences.end();
}

void LoRaManager::saveNodeNames() {
  Preferences preferences;
  if (!preferences.begin("ft01nodes", false)) return;
  preferences.putBytes("names", nodeNames, sizeof(nodeNames));
  preferences.end();
}

int LoRaManager::findNodeSlot(uint32_t node) const {
  if (node == 0) return -1;
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    if (nodeNames[i].node == node) return (int)i;
  }
  return -1;
}

int LoRaManager::ensureNodeSlot(uint32_t node) {
  if (node == 0 || node == MESH_NATIVE_BROADCAST || node == MESH_NATIVE_NODE_NUM) return -1;
  int existing = findNodeSlot(node);
  if (existing >= 0) return existing;

  size_t slot = NODE_NAME_MAX;
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    if (nodeNames[i].node == 0) {
      slot = i;
      break;
    }
  }
  if (slot == NODE_NAME_MAX) {
    slot = 0;
    uint32_t oldest = nodeLastSeenMs[0];
    for (size_t i = 1; i < NODE_NAME_MAX; i++) {
      if (nodeLastSeenMs[i] < oldest) {
        oldest = nodeLastSeenMs[i];
        slot = i;
      }
    }
  } else if (nodeNameCount < NODE_NAME_MAX) {
    nodeNameCount++;
  }

  nodeNames[slot].node = node;
  nodeNames[slot].longName[0] = '\0';
  nodeNames[slot].shortName[0] = '\0';
  nodeLastSeenMs[slot] = millis();
  return (int)slot;
}

void LoRaManager::rememberNodeSeen(uint32_t node) {
  if (node == 0 || node == MESH_NATIVE_BROADCAST || node == MESH_NATIVE_NODE_NUM) return;
  int existing = findNodeSlot(node);
  int slot = ensureNodeSlot(node);
  if (slot < 0) return;
  nodeLastSeenMs[slot] = millis();
  if (existing < 0) saveNodeNames();
}

void LoRaManager::rememberNodeName(uint32_t node, const String& longName, const String& shortName) {
  if (node == 0 || node == MESH_NATIVE_BROADCAST || node == MESH_NATIVE_NODE_NUM) return;
  int slot = ensureNodeSlot(node);
  if (slot < 0) return;

  char nextLong[NODE_LONG_NAME_BYTES];
  char nextShort[NODE_SHORT_NAME_BYTES];
  copyUtf8Name(nextLong, sizeof(nextLong), longName);
  copyUtf8Name(nextShort, sizeof(nextShort), shortName);
  bool changed = strcmp(nodeNames[slot].longName, nextLong) != 0 ||
                 strcmp(nodeNames[slot].shortName, nextShort) != 0;

  nodeLastSeenMs[slot] = millis();
  if (!changed) return;
  memcpy(nodeNames[slot].longName, nextLong, sizeof(nextLong));
  memcpy(nodeNames[slot].shortName, nextShort, sizeof(nextShort));
  saveNodeNames();
  refreshInboxNodeName(node);
}

bool LoRaManager::hasKnownNodeName(uint32_t node) const {
  int slot = findNodeSlot(node);
  return slot >= 0 && (nodeNames[slot].shortName[0] != '\0' || nodeNames[slot].longName[0] != '\0');
}

void LoRaManager::refreshInboxNodeName(uint32_t node) {
  String display = nodeDisplayName(node);
  for (size_t i = 0; i < MESH_INBOX_MAX; i++) {
    if (inbox[i].valid && inbox[i].from == node) inbox[i].lbxDevice = display;
  }
}

String LoRaManager::nodeDisplayName(uint32_t value) const {
  if (value == MESH_NATIVE_NODE_NUM) return "FT01";
  int slot = findNodeSlot(value);
  if (slot >= 0) {
    if (nodeNames[slot].shortName[0] != '\0') return String(nodeNames[slot].shortName);
    if (nodeNames[slot].longName[0] != '\0') return String(nodeNames[slot].longName);
  }
  return formatNodeShort(value);
}

size_t LoRaManager::networkNodeCount() const {
  size_t count = 0;
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    if (nodeNames[i].node != 0 && nodeNames[i].node != MESH_NATIVE_NODE_NUM &&
        nodeNames[i].node != MESH_NATIVE_BROADCAST) count++;
  }
  for (size_t i = 0; i < pki.peerCount(); i++) {
    uint32_t node = 0;
    if (pki.peerNodeAt(i, node) && findNodeSlot(node) < 0 && node != MESH_NATIVE_NODE_NUM) count++;
  }
  return count;
}

bool LoRaManager::getNetworkNode(size_t index, LoRaNodeView& out) const {
  size_t found = 0;
  for (size_t i = 0; i < NODE_NAME_MAX; i++) {
    uint32_t node = nodeNames[i].node;
    if (node == 0 || node == MESH_NATIVE_NODE_NUM || node == MESH_NATIVE_BROADCAST) continue;
    if (found == index) {
      out.valid = true;
      out.node = node;
      out.longName = String(nodeNames[i].longName);
      out.shortName = String(nodeNames[i].shortName);
      out.displayName = nodeDisplayName(node);
      out.pkiAvailable = pki.peerPublicKey(node) != nullptr;
      out.ageMs = nodeLastSeenMs[i] == 0 ? 0xFFFFFFFFUL : millis() - nodeLastSeenMs[i];
      return true;
    }
    found++;
  }

  for (size_t i = 0; i < pki.peerCount(); i++) {
    uint32_t node = 0;
    if (!pki.peerNodeAt(i, node) || findNodeSlot(node) >= 0 || node == MESH_NATIVE_NODE_NUM) continue;
    if (found == index) {
      out.valid = true;
      out.node = node;
      out.longName = "";
      out.shortName = "";
      out.displayName = formatNodeShort(node);
      out.pkiAvailable = true;
      out.ageMs = 0xFFFFFFFFUL;
      return true;
    }
    found++;
  }

  out.valid = false;
  out.node = 0;
  return false;
}

bool LoRaManager::nodeSupportsPrivateMessage(uint32_t node) const {
  return node != 0 && node != MESH_NATIVE_NODE_NUM && pki.peerPublicKey(node) != nullptr;
}

String LoRaManager::formatMeshHex32(uint32_t value) const { return formatHex32(value); }
String LoRaManager::formatMeshHex8(uint8_t value) const { return formatHex8(value); }
String LoRaManager::formatMeshNodeShort(uint32_t value) const { return formatNodeShort(value); }

String LoRaManager::lastStatus() const { return statusText; }
size_t LoRaManager::inboxCount() const {
  return inboxStored;
}

bool LoRaManager::getInboxFrame(size_t indexFromNewest, LoRaFrameView& out) const {
  if (indexFromNewest >= inboxStored || inboxStored == 0) {
    out.valid = false;
    return false;
  }
  size_t slot = (inboxWriteIndex + MESH_INBOX_MAX - 1 - indexFromNewest) % MESH_INBOX_MAX;
  if (!inbox[slot].valid) {
    out.valid = false;
    return false;
  }
  out = inbox[slot];
  if (lastRxMillis == 0) {
    out.ageMs = 0xFFFFFFFF;
  } else {
    // Store age relative to when this frame was copied.  The inbox entry keeps
    // a timestamp in ageMs field as millis-at-receive in v0.4.7.
    out.ageMs = millis() - inbox[slot].ageMs;
  }
  return true;
}

size_t LoRaManager::messageCount() const {
  size_t count = 0;
  for (size_t index = 0; index < inboxStored; index++) {
    LoRaFrameView frame;
    if (getInboxFrame(index, frame) && isMessageKind(frame.kind)) count++;
  }
  return count;
}

bool LoRaManager::getMessageFrame(size_t indexFromNewest, LoRaFrameView& out) const {
  size_t found = 0;
  for (size_t index = 0; index < inboxStored; index++) {
    LoRaFrameView frame;
    if (!getInboxFrame(index, frame) || !isMessageKind(frame.kind)) continue;
    if (found == indexFromNewest) {
      out = frame;
      return true;
    }
    found++;
  }
  out.valid = false;
  return false;
}

void LoRaManager::dumpInboxFrameToSerial(size_t indexFromNewest) const {
  LoRaFrameView frame;
  if (!getInboxFrame(indexFromNewest, frame)) {
    Serial.print("LORA_INBOX_DUMP index=");
    Serial.print((int)indexFromNewest);
    Serial.println(" ok=false error=empty");
    return;
  }
  Serial.print("LORA_INBOX_DUMP index=");
  Serial.print((int)indexFromNewest);
  Serial.print(" kind=");
  Serial.print(frame.kind);
  if (frame.kind == "LBX_TEXT" || frame.kind == "MESH_TEXT" || frame.kind == "MESH_PKI_TEXT" || frame.kind == "MESH_PKI_WAIT" || frame.kind == "MESH_PKI_FAIL" || frame.kind == "MESH_NODEINFO" || frame.kind == "MESH_DECODED" || frame.kind == "MESH_PKI_DATA") {
    Serial.print(" device=");
    Serial.print(frame.lbxDevice);
    Serial.print(" type=");
    Serial.print(frame.lbxType.length() ? frame.lbxType : frame.kind);
    if (frame.lbxSeq > 0) {
      Serial.print(" seq=");
      Serial.print(frame.lbxSeq);
    }
    Serial.print(" text=redacted");
  } else {
    Serial.print(" from=");
    Serial.print(formatHex32(frame.from));
    Serial.print(" to=");
    Serial.print(frame.broadcast ? String("Broadcast(") + formatHex32(frame.to) + ")" : formatHex32(frame.to));
    Serial.print(" id=");
    Serial.print(formatHex32(frame.id));
    Serial.print(" flags=");
    Serial.print(formatHex8(frame.flags));
    Serial.print(" channel=");
    Serial.print(formatHex8(frame.channel));
    Serial.print(" payload_len=");
    Serial.print(frame.payloadLen);
    Serial.print(" duplicate=");
    Serial.print(frame.duplicate ? "true" : "false");
  }
  Serial.print(" bytes=");
  Serial.print(frame.bytes);
  Serial.print(" rssi=");
  Serial.print(frame.rssi, 1);
  Serial.print(" snr=");
  Serial.print(frame.snr, 1);
  Serial.print(" hex=");
  Serial.println(frame.hex);
}
uint8_t LoRaManager::meshSenderCount() const { return (uint8_t)uniqueMeshSenderCount; }
uint32_t LoRaManager::rxCount() const { return rxOkCounter; }
uint32_t LoRaManager::rxTimeoutCount() const { return rxTimeoutCounter; }
void LoRaManager::resetStats() {
  rxOkCounter = 0;
  rxTimeoutCounter = 0;
  rxFailCounter = 0;
  pkiDecryptOkCounter = 0;
  pkiDecryptFailCounter = 0;
  ackSentCounter = 0;
  ackFailCounter = 0;
  duplicateDropCounter = 0;
  lastRxMillis = 0;
  lastAutomaticNodeInfoRequestMs = 0;
  payloadText = "";
  payloadHexText = "";
  frameKindText = "--";
  rawFrameLen = 0;
  meshTo = 0;
  meshFrom = 0;
  meshId = 0;
  meshFlags = 0;
  meshChannel = 0;
  meshPayloadLen = -1;
  meshIsBroadcast = false;
  meshDuplicatePacket = false;
  nodeInfoReplyPending = false;
  ackReplyPending = false;
  ackReplyTo = 0;
  ackRequestId = 0;
  ackReplyReliable = false;
  lbxType = "";
  lbxDevice = "";
  lbxText = "";
  lbxSeq = 0;
  recentPacketCount = 0;
  uniqueMeshSenderCount = 0;
  for (size_t i = 0; i < MESH_RECENT_PACKET_COUNT; i++) {
    recentPacketFrom[i] = 0;
    recentPacketIds[i] = 0;
  }
  for (size_t i = 0; i < MESH_SENDER_MAX; i++) uniqueMeshSenders[i] = 0;
  for (size_t i = 0; i < MESH_INBOX_MAX; i++) {
    inbox[i].valid = false;
    inbox[i].hex = "";
    inbox[i].kind = "";
    inbox[i].text = "";
    inbox[i].lbxType = "";
    inbox[i].lbxDevice = "";
    inbox[i].lbxSeq = 0;
  }
  inboxWriteIndex = 0;
  inboxStored = 0;
  for (size_t i = 0; i < PKI_PENDING_MAX; i++) {
    pendingPki[i].valid = false;
    pendingPki[i].encryptedLen = 0;
  }
  pendingPkiReplaceIndex = 0;
  payloadBytes = 0;
  rssi = 0.0f;
  snr = 0.0f;
  statusText = "STATS RESET";
  Serial.println("LORA_PROBE_STATS_RESET");
}
