#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include "FtMeshPki.h"

struct LoRaFrameView {
  bool valid;
  uint32_t to;
  uint32_t from;
  uint32_t id;
  uint8_t flags;
  uint8_t channel;
  int payloadLen;
  int bytes;
  float rssi;
  float snr;
  uint32_t ageMs;
  bool broadcast;
  bool duplicate;
  String hex;
  String kind;
  String text;
  String lbxType;
  String lbxDevice;
  uint32_t lbxSeq;
};
struct LoRaNodeView {
  bool valid;
  uint32_t node;
  String displayName;
  String longName;
  String shortName;
  bool pkiAvailable;
  uint32_t ageMs;
  float lastRssi;
  float lastSnr;
  int8_t hops;
};

class LoRaManager {
public:
  LoRaManager();

  bool begin();
  bool isReady() const;
  bool startListening();
  bool pollReceive();
  bool isListening() const;
  bool sendMeshtasticText(const String& text);
  bool sendMeshtasticPrivateText(uint32_t toNode, const String& text);
  bool sendMeshtasticNodeInfo(bool requestReplies = true);
  bool pkiReady() const;
  uint8_t pkiPeerCount() const;
  uint32_t pkiDecryptOkCount() const;
  uint32_t pkiDecryptFailCount() const;
  uint8_t pkiPendingCount() const;
  uint32_t ackSentCount() const;
  uint32_t ackFailCount() const;
  uint32_t duplicateDropCount() const;
  size_t unreadMessageCount() const;
  uint32_t messageRevision() const;
  void markMessagesRead();
  bool receiveProbeOnce(uint32_t timeoutMs); // legacy/manual diagnostic helper
  void cycleProfile();

  float currentFreqMhz() const;
  String currentProfileName() const;

  String lastStatus() const;
  size_t inboxCount() const;
  bool getInboxFrame(size_t indexFromNewest, LoRaFrameView& out) const;
  size_t messageCount() const;
  bool getMessageFrame(size_t indexFromNewest, LoRaFrameView& out) const;
  void dumpInboxFrameToSerial(size_t indexFromNewest) const;
  String formatMeshHex32(uint32_t value) const;
  String formatMeshHex8(uint8_t value) const;
  String formatMeshNodeShort(uint32_t value) const;
  String nodeDisplayName(uint32_t value) const;
  size_t networkNodeCount() const;
  bool getNetworkNode(size_t index, LoRaNodeView& out) const;
  bool nodeSupportsPrivateMessage(uint32_t node) const;
  uint8_t meshSenderCount() const;
  uint32_t rxCount() const;
  uint32_t rxTimeoutCount() const;
  uint32_t rxErrorCount() const;
  uint32_t lastTxPacketId() const;
  uint32_t txRevision() const;
  uint32_t deliveryRevision() const;
  uint32_t lastDeliveryRequestId() const;
  uint32_t lastDeliveryFrom() const;
  bool lastDeliveryAck() const;
  void resetStats();

private:
  static const size_t RAW_FRAME_MAX = 256;
  static const size_t MESH_RECENT_PACKET_COUNT = 16;
  static const size_t MESH_SENDER_MAX = 64;
  static const size_t MESH_INBOX_MAX = 50;
  static const size_t PKI_PENDING_MAX = 4;
  static const size_t PKI_PENDING_PAYLOAD_MAX = 240;
  static const size_t NODE_NAME_MAX = 64;
  static const size_t NODE_LONG_NAME_BYTES = 25;
  static const size_t NODE_SHORT_NAME_BYTES = 13;

  struct PendingPkiFrame {
    bool valid;
    uint32_t from;
    uint32_t to;
    uint32_t id;
    uint8_t flags;
    size_t encryptedLen;
    uint8_t encrypted[PKI_PENDING_PAYLOAD_MAX];
  };

  struct NodeNameEntry {
    uint32_t node;
    char longName[NODE_LONG_NAME_BYTES];
    char shortName[NODE_SHORT_NAME_BYTES];
  };

  String toHexPreview(const uint8_t* data, size_t len, size_t maxBytes) const;
  String classifyFrame(const uint8_t* data, size_t len) const;
  uint32_t readLe32(const uint8_t* data) const;
  String formatHex32(uint32_t value) const;
  String formatHex8(uint8_t value) const;
  String formatNodeShort(uint32_t value) const;
  void rememberMeshPacket(uint32_t from, uint32_t packetId);
  void parseMeshHeader(const uint8_t* data, size_t len);
  bool parseLbxFrame(const uint8_t* data, size_t len);
  bool tryDecodeMeshtasticPayload();
  bool handleDecodedMeshData(const uint8_t* plain, size_t plainLen, bool pkiEncrypted);
  bool sendMeshtasticData(uint8_t portnum, const uint8_t* appPayload, size_t appLen, const char* typeName, bool wantResponse);
  bool sendChannelPacket(uint32_t toNode,
                         uint8_t portnum,
                         const uint8_t* appPayload,
                         size_t appLen,
                         const char* typeName,
                         bool wantResponse,
                         bool wantAck,
                         uint32_t requestId);
  bool transmitMeshFrame(uint32_t toNode,
                         uint8_t channel,
                         const uint8_t* payload,
                         size_t payloadLen,
                         const char* typeName,
                         bool wantAck,
                         uint32_t packetId);
  bool sendRoutingAck(uint32_t toNode, uint32_t requestId, bool reliableAck);
  bool processReceivedFrame(const uint8_t* data, size_t len);
  void printMeshHeader(const char* prefix) const;
  void printLbxFrame(const char* prefix) const;
  String sanitizeText(const String& text, size_t maxLen) const;
  String decodeTextPayload(const uint8_t* payload, size_t len) const;
  void rememberFrameSummary();
  void rememberPendingPki(const uint8_t* encrypted, size_t encryptedLen);
  void retryPendingForPeer(uint32_t node);
  bool updateInboxDecoded(uint32_t from, uint32_t packetId, const String& kind, const String& text);
  bool isMessageKind(const String& kind) const;
  void loadNodeNames();
  void saveNodeNames();
  void rememberNodeSeen(uint32_t node);
  void rememberNodeName(uint32_t node, const String& longName, const String& shortName);
  int findNodeSlot(uint32_t node) const;
  int ensureNodeSlot(uint32_t node);
  void copyUtf8Name(char* destination, size_t destinationSize, const String& source) const;
  bool hasKnownNodeName(uint32_t node) const;
  void refreshInboxNodeName(uint32_t node);

  SX1262 radio;
  FtMeshPki pki;
  bool ready;
  bool listening;
  uint32_t seq;
  uint32_t rxOkCounter;
  uint32_t rxTimeoutCounter;
  uint32_t rxFailCounter;
  uint32_t pkiDecryptOkCounter;
  uint32_t pkiDecryptFailCounter;
  uint32_t ackSentCounter;
  uint32_t ackFailCounter;
  uint32_t duplicateDropCounter;
  size_t unreadMessageCounter;
  uint32_t messageRevisionCounter;
  uint32_t lastTxPacketIdValue;
  uint32_t txRevisionCounter;
  uint32_t lastDeliveryRequestIdValue;
  uint32_t lastDeliveryFromValue;
  bool lastDeliveryAckValue;
  uint32_t deliveryRevisionCounter;
  uint32_t lastRxMillis;
  uint32_t lastAutomaticNodeInfoRequestMs;
  String statusText;
  String payloadText;
  String payloadHexText;
  String frameKindText;
  uint8_t rawFrame[RAW_FRAME_MAX];
  size_t rawFrameLen;
  uint32_t meshTo;
  uint32_t meshFrom;
  uint32_t meshId;
  uint8_t meshFlags;
  uint8_t meshChannel;
  int meshPayloadLen;
  bool meshIsBroadcast;
  String lbxType;
  String lbxDevice;
  String lbxText;
  uint32_t lbxSeq;
  bool meshDuplicatePacket;
  bool nodeInfoReplyPending;
  bool ackReplyPending;
  uint32_t ackReplyTo;
  uint32_t ackRequestId;
  bool ackReplyReliable;
  uint32_t recentPacketFrom[MESH_RECENT_PACKET_COUNT];
  uint32_t recentPacketIds[MESH_RECENT_PACKET_COUNT];
  size_t recentPacketCount;
  uint32_t uniqueMeshSenders[MESH_SENDER_MAX];
  size_t uniqueMeshSenderCount;
  LoRaFrameView inbox[MESH_INBOX_MAX];
  size_t inboxWriteIndex;
  size_t inboxStored;
  PendingPkiFrame pendingPki[PKI_PENDING_MAX];
  size_t pendingPkiReplaceIndex;
  NodeNameEntry nodeNames[NODE_NAME_MAX];
  uint32_t nodeLastSeenMs[NODE_NAME_MAX];
  float nodeLastRssi[NODE_NAME_MAX];
  float nodeLastSnr[NODE_NAME_MAX];
  int8_t nodeLastHops[NODE_NAME_MAX];
  size_t nodeNameCount;
  int payloadBytes;
  float rssi;
  float snr;
  size_t profileIndex;
};
