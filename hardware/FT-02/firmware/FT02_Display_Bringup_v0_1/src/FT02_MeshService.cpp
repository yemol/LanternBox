#include "FT02_MeshService.h"
#include "FT02_Log.h"
#include "FT02_MeshProtocol.h"
#include "FT02_MeshSecrets.h"
#include "FT02_MeshRadioBackend.h"
#include "FT02_MeshPki.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_LoRaNodeRuntime.h"

#include <Arduino.h>
#include <esp_system.h>
#include <mbedtls/aes.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr uint32_t NODEINFO_BOOT_DELAY_MS = 3500u;
constexpr uint32_t NODEINFO_REPLY_MIN_MS = 15000u;
constexpr size_t PENDING_PKI_MAX = 4u;
constexpr size_t MESH_MAX_PAYLOAD = 239u;

struct PendingPkiFrame {
    bool valid;
    uint32_t from;
    uint32_t to;
    uint32_t id;
    uint8_t flags;
    size_t encryptedLen;
    uint8_t encrypted[MESH_MAX_PAYLOAD];
    float rssi;
    float snr;
};

uint32_t g_rawFrames = 0;
uint32_t g_decodedFrames = 0;
uint32_t g_crcRejected = 0;
uint32_t g_decryptFailures = 0;
bool g_serviceReady = false;
uint32_t g_txFrames = 0;
uint32_t g_txFailures = 0;
uint32_t g_privateRx = 0;
uint32_t g_privateTx = 0;
uint32_t g_nodeInfoRx = 0;
uint32_t g_nodeInfoTx = 0;
FT02MeshPki g_pki;
bool g_pkiReady = false;
PendingPkiFrame g_pendingPki[PENDING_PKI_MAX] = {};
size_t g_pendingReplace = 0;
bool g_nodeInfoBootPending = false;
bool g_nodeInfoReplyPending = false;
uint32_t g_bootMs = 0;
uint32_t g_lastNodeInfoTxMs = 0;

void writeLe32(uint8_t *out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

void writeLe64(uint8_t *out, uint64_t value) {
    for (uint8_t i = 0; i < 8; ++i) out[i] = static_cast<uint8_t>((value >> (8u * i)) & 0xFFu);
}

uint32_t newPacketId() {
    uint32_t id = 0;
    while (id == 0u || id == FT02_MESH_BROADCAST_NODE) id = esp_random();
    return id;
}

bool aesCtrCrypt(uint8_t *data, size_t len, uint32_t fromNode, uint32_t packetId) {
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    if (mbedtls_aes_setkey_enc(&ctx, FT02_MESH_CHANNEL_AES_KEY, 256) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    uint8_t nonceCounter[16] = {};
    writeLe64(nonceCounter + 0, static_cast<uint64_t>(packetId));
    writeLe32(nonceCounter + 8, fromNode);
    writeLe32(nonceCounter + 12, 0u);
    uint8_t streamBlock[16] = {};
    size_t ncOff = 0;
    const int rc = mbedtls_aes_crypt_ctr(&ctx, len, &ncOff, nonceCounter, streamBlock, data, data);
    mbedtls_aes_free(&ctx);
    return rc == 0;
}

void printHexDebug(const uint8_t *data, size_t len) {
#if FT02_LOG_LEVEL >= FT02_LOG_LEVEL_DEBUG
    for (size_t i = 0; i < len; ++i) FT02_LOGD("%02X", data[i]);
#else
    (void)data;
    (void)len;
#endif
}

void printSanitizedTextInfo(const uint8_t *data, size_t len) {
    const size_t cap = len > 180u ? 180u : len;
    for (size_t i = 0; i < cap; ++i) {
        const uint8_t c = data[i];
        if (c == '\r' || c == '\n' || c == '\t') FT02_LOGI_WRITE(' ');
        else if (c >= 0x20u && c != 0x7Fu) FT02_LOGI_WRITE(c);
        else FT02_LOGI_WRITE('.');
    }
    if (cap < len) FT02_LOGI_PRINT("...");
}

void nodeLabel(uint32_t node, char *out, size_t outSize) {
    if (!out || outSize == 0u) return;
    if (node == FT02_MESH_BROADCAST_NODE) {
        snprintf(out, outSize, "broadcast");
        return;
    }
    if (node == FT02_MESH_LOCAL_NODE) {
        snprintf(out, outSize, "FT02");
        return;
    }
    FT02LoRaNodeView view{};
    if (FT02_LoRaNodeRuntimeFindNode(node, view)) {
        if (view.shortName[0]) {
            snprintf(out, outSize, "%s", view.shortName);
            return;
        }
        if (view.longName[0]) {
            snprintf(out, outSize, "%s", view.longName);
            return;
        }
    }
    snprintf(out, outSize, "!%08lX", static_cast<unsigned long>(node));
}

bool transmitPrepared(uint32_t toNode,
                      uint8_t channel,
                      const uint8_t *payload,
                      size_t payloadLen,
                      bool wantAck,
                      uint32_t packetId,
                      const char *typeName) {
    if (!g_serviceReady || !payload || payloadLen == 0u || packetId == 0u ||
        FT02_MESH_HEADER_LEN + payloadLen > 255u) return false;

    uint8_t frame[255] = {};
    writeLe32(frame + 0, toNode);
    writeLe32(frame + 4, FT02_MESH_LOCAL_NODE);
    writeLe32(frame + 8, packetId);
    frame[12] = static_cast<uint8_t>((FT02_MESH_HOP_START << 5u) |
                                     (wantAck ? FT02_MESH_FLAG_WANT_ACK : 0u) |
                                     (FT02_MESH_HOP_LIMIT & 0x07u));
    frame[13] = channel;
    frame[14] = 0x00u;
    frame[15] = static_cast<uint8_t>(FT02_MESH_LOCAL_NODE & 0xFFu);
    memcpy(frame + FT02_MESH_HEADER_LEN, payload, payloadLen);
    const size_t frameLen = FT02_MESH_HEADER_LEN + payloadLen;

    FT02_LOGD("[MESH-TX] air type=%s id=0x%08lX from=0x%08lX to=0x%08lX channel=0x%02X flags=0x%02X len=%u\n",
                  typeName ? typeName : "DATA",
                  static_cast<unsigned long>(packetId),
                  static_cast<unsigned long>(FT02_MESH_LOCAL_NODE),
                  static_cast<unsigned long>(toNode), channel, frame[12],
                  static_cast<unsigned>(frameLen));

    if (!FT02_MeshRadioBackendTransmit(frame, frameLen)) {
        ++g_txFailures;
        FT02_LOGE("[ERROR][MESH] TX failed type=%s id=0x%08lX count=%lu\n",
                      typeName ? typeName : "DATA", static_cast<unsigned long>(packetId),
                      static_cast<unsigned long>(g_txFailures));
        return false;
    }
    ++g_txFrames;
    return true;
}

bool sendChannelData(uint32_t toNode,
                     uint8_t portnum,
                     const uint8_t *appPayload,
                     size_t appLen,
                     bool wantResponse,
                     bool wantAck,
                     uint32_t requestId,
                     uint32_t packetId,
                     const char *typeName) {
    uint8_t dataProto[190] = {};
    size_t dataLen = 0;
    if (!FT02_MeshBuildDataProto(portnum, appPayload, appLen, wantResponse, requestId,
                                 dataProto, sizeof(dataProto), dataLen)) return false;
    if (!aesCtrCrypt(dataProto, dataLen, FT02_MESH_LOCAL_NODE, packetId)) return false;
    return transmitPrepared(toNode, FT02_MESH_CHANNEL_HASH, dataProto, dataLen, wantAck, packetId, typeName);
}

bool sendNodeInfo(bool requestReplies) {
    if (!g_serviceReady || !g_pkiReady) return false;
    uint8_t userProto[128] = {};
    size_t userLen = 0;
    if (!FT02_MeshBuildUserProto(g_pki.publicKey(), userProto, sizeof(userProto), userLen)) return false;
    const uint32_t packetId = newPacketId();
    if (!sendChannelData(FT02_MESH_BROADCAST_NODE, FT02_MESH_PORT_NODEINFO_APP,
                         userProto, userLen, requestReplies, false, 0u, packetId,
                         requestReplies ? "NODEINFO_REQUEST" : "NODEINFO")) return false;
    ++g_nodeInfoTx;
    g_lastNodeInfoTxMs = millis();
    FT02_LOGD("[MESH-NODE] TX n=%lu request_replies=%d id=0x%08lX name=LanternBox FT02 short=FT02 pki=1\n",
                  static_cast<unsigned long>(g_nodeInfoTx), requestReplies ? 1 : 0,
                  static_cast<unsigned long>(packetId));
    return true;
}

bool sendRoutingAck(uint32_t toNode, uint32_t requestId) {
    const uint8_t routingOk[] = {0x18u, 0x00u}; // Routing.error_reason = NONE
    const uint32_t packetId = newPacketId();
    const bool ok = sendChannelData(toNode, FT02_MESH_PORT_ROUTING_APP,
                                    routingOk, sizeof(routingOk), false, true,
                                    requestId, packetId, "ACK");
    FT02_LOGI("[MESH-ACK] TX %s to=0x%08lX request=0x%08lX packet=0x%08lX\n",
                  ok ? "TX_OK" : "TX_FAIL", static_cast<unsigned long>(toNode),
                  static_cast<unsigned long>(requestId), static_cast<unsigned long>(packetId));
    return ok;
}

void rememberPendingPki(const FT02MeshHeader &header,
                        const uint8_t *encrypted,
                        size_t encryptedLen,
                        float rssi,
                        float snr) {
    if (!encrypted || encryptedLen == 0u || encryptedLen > MESH_MAX_PAYLOAD) return;
    size_t slot = PENDING_PKI_MAX;
    for (size_t i = 0; i < PENDING_PKI_MAX; ++i) {
        if (!g_pendingPki[i].valid) { slot = i; break; }
    }
    if (slot == PENDING_PKI_MAX) {
        slot = g_pendingReplace;
        g_pendingReplace = (g_pendingReplace + 1u) % PENDING_PKI_MAX;
    }
    PendingPkiFrame &pending = g_pendingPki[slot];
    pending.valid = true;
    pending.from = header.from;
    pending.to = header.to;
    pending.id = header.id;
    pending.flags = header.flags;
    pending.encryptedLen = encryptedLen;
    memcpy(pending.encrypted, encrypted, encryptedLen);
    pending.rssi = rssi;
    pending.snr = snr;
}

void processDecoded(const FT02MeshHeader &header,
                    const uint8_t *plain,
                    size_t plainLen,
                    bool pkiEncrypted,
                    float rssi,
                    float snr);

void retryPendingForPeer(uint32_t node) {
    const uint8_t *peerKey = g_pki.peerPublicKey(node);
    if (!peerKey) return;
    for (size_t i = 0; i < PENDING_PKI_MAX; ++i) {
        PendingPkiFrame &pending = g_pendingPki[i];
        if (!pending.valid || pending.from != node) continue;
        uint8_t plain[MESH_MAX_PAYLOAD] = {};
        size_t plainLen = 0;
        if (!g_pki.decryptFromPeer(peerKey, pending.id, pending.from,
                                   pending.encrypted, pending.encryptedLen,
                                   plain, sizeof(plain), plainLen)) {
            ++g_decryptFailures;
            FT02_LOGE("[ERROR][MESH-PKI] retry decrypt failed from=0x%08lX id=0x%08lX\n",
                          static_cast<unsigned long>(pending.from),
                          static_cast<unsigned long>(pending.id));
            pending.valid = false;
            continue;
        }
        FT02MeshHeader header{};
        header.from = pending.from;
        header.to = pending.to;
        header.id = pending.id;
        header.flags = pending.flags;
        header.channel = 0u;
        processDecoded(header, plain, plainLen, true, pending.rssi, pending.snr);
        pending.valid = false;
        FT02_LOGD("[MESH-PKI] pending message decrypted from=0x%08lX id=0x%08lX\n",
                      static_cast<unsigned long>(header.from), static_cast<unsigned long>(header.id));
    }
}

void processDecoded(const FT02MeshHeader &header,
                    const uint8_t *plain,
                    size_t plainLen,
                    bool pkiEncrypted,
                    float rssi,
                    float snr) {
    FT02MeshDataView data{};
    if (!FT02_MeshParseDataProto(plain, plainLen, data)) {
        FT02_LOGW("[WARN][MESH] data decode failed from=0x%08lX pki=%d len=%u\n",
                  static_cast<unsigned long>(header.from), pkiEncrypted ? 1 : 0,
                  static_cast<unsigned>(plainLen));
        FT02_LOGD_PRINT(pkiEncrypted ? "[MESH-PKI] plain_hex=" : "[MESH-RX] plain_hex=");
        printHexDebug(plain, plainLen);
        FT02_LOGD_PRINTLN("");
        return;
    }

    ++g_decodedFrames;
    FT02_LOGD("[MESH-RX] decode n=%lu port=%lu(%s) app_len=%u want_response=%d request=0x%08lX pki=%d\n",
              static_cast<unsigned long>(g_decodedFrames),
              static_cast<unsigned long>(data.portnum), FT02_MeshPortName(data.portnum),
              static_cast<unsigned>(data.payloadLen), data.wantResponse ? 1 : 0,
              static_cast<unsigned long>(data.requestId), pkiEncrypted ? 1 : 0);

    if (data.portnum == FT02_MESH_PORT_NODEINFO_APP && data.payload && data.payloadLen) {
        FT02MeshUserView user{};
        const bool parsed = FT02_MeshParseUserProto(data.payload, data.payloadLen, user);
        const uint32_t identityNode = parsed && user.nodeNumber ? user.nodeNumber : header.from;
        const bool identityMatch = identityNode == header.from;

        FT02LoRaNodeView before{};
        const bool existed = FT02_LoRaNodeRuntimeFindNode(header.from, before);
        const bool longChanged = parsed && strcmp(before.longName, user.longName) != 0;
        const bool shortChanged = parsed && strcmp(before.shortName, user.shortName) != 0;
        const bool keyFirst = parsed && user.hasPublicKey && (!existed || !before.publicKeyValid);
        const bool keyChanged = parsed && user.hasPublicKey && existed && before.publicKeyValid &&
                                memcmp(before.publicKey, user.publicKey, sizeof(before.publicKey)) != 0;

        bool learnedKey = false;
        if (parsed && identityMatch && header.from != FT02_MESH_LOCAL_NODE) {
            if (user.hasPublicKey && g_pkiReady) learnedKey = g_pki.rememberPeer(header.from, user.publicKey);
            FT02_LoRaNodeRuntimeHostLearnIdentity(header.from,
                                                  user.longName,
                                                  user.shortName,
                                                  user.publicKey,
                                                  user.hasPublicKey,
                                                  rssi, snr);
            retryPendingForPeer(header.from);
        }
        ++g_nodeInfoRx;

        const bool identityChanged = !existed || longChanged || shortChanged;
        if (!parsed || !identityMatch) {
            FT02_LOGW("[WARN][MESH-NODE] invalid identity from=0x%08lX parsed=%d identity_match=%d\n",
                      static_cast<unsigned long>(header.from), parsed ? 1 : 0, identityMatch ? 1 : 0);
        } else if (identityChanged || keyFirst || keyChanged) {
            char source[28] = {};
            nodeLabel(header.from, source, sizeof(source));
            FT02_LOGI("[MESH-NODE] %s node=%s name=", existed ? "updated" : "new", source);
            printSanitizedTextInfo(reinterpret_cast<const uint8_t*>(user.longName), strlen(user.longName));
            FT02_LOGI_PRINT(" short=");
            printSanitizedTextInfo(reinterpret_cast<const uint8_t*>(user.shortName), strlen(user.shortName));
            FT02_LOGI(" pki=%s\n",
                      keyChanged ? "changed" : (keyFirst && learnedKey ? "learned" : (user.hasPublicKey ? "present" : "none")));
        } else {
            FT02_LOGD("[MESH-NODE] repeat from=0x%08lX rx=%lu\n",
                      static_cast<unsigned long>(header.from), static_cast<unsigned long>(g_nodeInfoRx));
        }
        if (data.wantResponse && header.from != FT02_MESH_LOCAL_NODE) g_nodeInfoReplyPending = true;
    } else if (data.portnum == FT02_MESH_PORT_TEXT_MESSAGE_APP && data.payload && data.payloadLen) {
        char source[28] = {};
        char destination[28] = {};
        nodeLabel(header.from, source, sizeof(source));
        nodeLabel(pkiEncrypted ? FT02_MESH_LOCAL_NODE : header.to, destination, sizeof(destination));
        FT02_LOGI("[MESH-RX] %s -> %s text=\"", source, destination);
        printSanitizedTextInfo(data.payload, data.payloadLen);
        FT02_LOGI("\" RSSI=%.1f SNR=%.1f%s\n",
                  static_cast<double>(rssi), static_cast<double>(snr), pkiEncrypted ? " PKI" : "");

        char inboxText[FT02_LORA_MESSAGE_TEXT_BYTES] = {};
        const size_t copyLen = data.payloadLen < sizeof(inboxText) - 1u ? data.payloadLen : sizeof(inboxText) - 1u;
        memcpy(inboxText, data.payload, copyLen);
        inboxText[copyLen] = '\0';
        FT02_LoRaCommunicationHostReceive(header.id, header.from, header.to, nullptr, pkiEncrypted, rssi, snr, inboxText);
        if (pkiEncrypted) {
            ++g_privateRx;
            if ((header.flags & FT02_MESH_FLAG_WANT_ACK) != 0u) (void)sendRoutingAck(header.from, header.id);
        }
    } else if (data.portnum == FT02_MESH_PORT_ROUTING_APP) {
        uint32_t error = 0xFFFFFFFFu;
        const bool parsed = data.payload && FT02_MeshParseRoutingError(data.payload, data.payloadLen, error);
        char source[28] = {};
        nodeLabel(header.from, source, sizeof(source));
        if (parsed) {
            FT02_LOGI("[MESH-ACK] RX from=%s request=0x%08lX %s\n", source,
                      static_cast<unsigned long>(data.requestId), error == 0u ? "OK" : "FAIL");
        } else {
            FT02_LOGW("[WARN][MESH-ACK] invalid routing payload from=%s request=0x%08lX\n",
                      source, static_cast<unsigned long>(data.requestId));
        }
        if (data.requestId != 0u && parsed) {
            FT02_LoRaCommunicationHostDelivery(data.requestId, header.from, error == 0u);
        }
    } else {
        FT02_LOGD("[MESH-RX] app ignored port=%lu from=0x%08lX len=%u\n",
                  static_cast<unsigned long>(data.portnum), static_cast<unsigned long>(header.from),
                  static_cast<unsigned>(data.payloadLen));
    }
}


} // namespace

bool FT02_MeshServiceBegin() {
    g_rawFrames = g_decodedFrames = g_crcRejected = g_decryptFailures = 0;
    g_txFrames = g_txFailures = g_privateRx = g_privateTx = 0;
    g_nodeInfoRx = g_nodeInfoTx = 0;
    memset(g_pendingPki, 0, sizeof(g_pendingPki));
    g_pendingReplace = 0;
    g_nodeInfoReplyPending = false;
    g_nodeInfoBootPending = false;
    g_lastNodeInfoTxMs = 0;

    g_pkiReady = g_pki.begin();
    g_serviceReady = FT02_MeshRadioBackendBegin();
    if (g_serviceReady) {
        FT02_LoRaNodeRuntimeHostBegin(FT02_MESH_LOCAL_NODE, "LanternBox FT02", "FT02");
        FT02_LoRaNodeRuntimeHostSetStatus(true, 0, g_pki.peerCount());
        g_bootMs = millis();
        g_nodeInfoBootPending = g_pkiReady;
    }
    if (g_serviceReady) {
        FT02_LOGI("[MESH] ready SX1262/HW-SPI LongFast node=!%08lX pki=%d peers=%u\n",
                  static_cast<unsigned long>(FT02_MESH_LOCAL_NODE), g_pkiReady ? 1 : 0,
                  static_cast<unsigned>(g_pki.peerCount()));
    } else {
        FT02_LOGE_PRINTLN("[ERROR][MESH] service init failed");
    }
    return g_serviceReady;
}

void FT02_MeshServicePoll() {
    if (!g_serviceReady) return;
    FT02_MeshRadioBackendPoll();
    FT02MeshRadioFrame frame{};
    while (FT02_MeshRadioBackendTakeFrame(frame)) {
        FT02_MeshServiceOnRadioFrame(frame.data, frame.len, frame.rssi, frame.snr, frame.crcOk, true);
    }

    const uint32_t now = millis();
    if (g_nodeInfoBootPending && static_cast<uint32_t>(now - g_bootMs) >= NODEINFO_BOOT_DELAY_MS) {
        if (sendNodeInfo(true)) g_nodeInfoBootPending = false;
    }
    if (g_nodeInfoReplyPending && (g_lastNodeInfoTxMs == 0u || static_cast<uint32_t>(now - g_lastNodeInfoTxMs) >= NODEINFO_REPLY_MIN_MS)) {
        if (sendNodeInfo(false)) g_nodeInfoReplyPending = false;
    }
}

void FT02_MeshServiceOnRadioFrame(const uint8_t *frame,
                                  size_t len,
                                  float rssi,
                                  float snr,
                                  bool crcOk,
                                  bool headerOk) {
    ++g_rawFrames;
    FT02_LOGD("[MESH-RX] raw n=%lu len=%u rssi=%.1f snr=%.2f crc_ok=%d header_ok=%d hex=",
                  static_cast<unsigned long>(g_rawFrames), static_cast<unsigned>(len),
                  static_cast<double>(rssi), static_cast<double>(snr), crcOk ? 1 : 0, headerOk ? 1 : 0);
    printHexDebug(frame, len);
    FT02_LOGD_PRINTLN("");

    FT02MeshHeader header{};
    if (!FT02_MeshParseHeader(frame, len, header)) { FT02_LOGW_PRINTLN("[WARN][MESH] reject short header"); return; }
    FT02_LOGD("[MESH-RX] header to=0x%08lX from=0x%08lX id=0x%08lX flags=0x%02X channel=0x%02X next=0x%02X relay=0x%02X encrypted_len=%u trusted=%d\n",
                  static_cast<unsigned long>(header.to), static_cast<unsigned long>(header.from),
                  static_cast<unsigned long>(header.id), header.flags, header.channel, header.nextHop,
                  header.relayNode, static_cast<unsigned>(len - FT02_MESH_HEADER_LEN), (crcOk && headerOk) ? 1 : 0);
    if (!crcOk || !headerOk) { ++g_crcRejected; return; }
    if (header.from == 0u || header.from == FT02_MESH_LOCAL_NODE) return;

    const size_t encryptedLen = len - FT02_MESH_HEADER_LEN;
    if (encryptedLen == 0u || encryptedLen > MESH_MAX_PAYLOAD) return;
    uint8_t plain[MESH_MAX_PAYLOAD] = {};
    size_t plainLen = encryptedLen;
    bool pkiEncrypted = false;

    if (header.channel == FT02_MESH_CHANNEL_HASH) {
        FT02_LoRaNodeRuntimeHostObservePeer(header.from, nullptr, snr, false);
        memcpy(plain, frame + FT02_MESH_HEADER_LEN, encryptedLen);
        if (!aesCtrCrypt(plain, encryptedLen, header.from, header.id)) { ++g_decryptFailures; return; }
    } else if (header.channel == 0u && header.to == FT02_MESH_LOCAL_NODE && g_pkiReady) {
        FT02_LoRaNodeRuntimeHostObservePeer(header.from, nullptr, snr, false);
        const uint8_t *peerKey = g_pki.peerPublicKey(header.from);
        if (!peerKey) {
            rememberPendingPki(header, frame + FT02_MESH_HEADER_LEN, encryptedLen, rssi, snr);
            g_nodeInfoBootPending = true; // broadcast our NodeInfo request to solicit peer identity/key
            g_bootMs = millis() - NODEINFO_BOOT_DELAY_MS;
            FT02_LOGI("[MESH-PKI] peer key unknown from=0x%08lX id=0x%08lX; requesting NodeInfo\n",
                          static_cast<unsigned long>(header.from), static_cast<unsigned long>(header.id));
            return;
        }
        if (!g_pki.decryptFromPeer(peerKey, header.id, header.from,
                                   frame + FT02_MESH_HEADER_LEN, encryptedLen,
                                   plain, sizeof(plain), plainLen)) {
            ++g_decryptFailures;
            FT02_LOGE("[ERROR][MESH-PKI] decrypt failed from=0x%08lX id=0x%08lX count=%lu\n",
                          static_cast<unsigned long>(header.from), static_cast<unsigned long>(header.id),
                          static_cast<unsigned long>(g_decryptFailures));
            return;
        }
        pkiEncrypted = true;
    } else {
        FT02_LOGD("[MESH-RX] opaque reason=unsupported_channel_or_target channel=0x%02X to=0x%08lX\n",
                      header.channel, static_cast<unsigned long>(header.to));
        return;
    }

    processDecoded(header, plain, plainLen, pkiEncrypted, rssi, snr);
}

bool FT02_MeshServiceSendBroadcastText(const char* text, uint32_t packetId) {
    if (!FT02_MeshServiceReady() || !text || !text[0] || packetId == 0u) return false;
    size_t textLen = strlen(text);
    if (textLen > 120u) textLen = 120u;
    while (textLen > 0u && (static_cast<uint8_t>(text[textLen]) & 0xC0u) == 0x80u) --textLen;
    if (textLen == 0u) return false;
    const bool ok = sendChannelData(FT02_MESH_BROADCAST_NODE, FT02_MESH_PORT_TEXT_MESSAGE_APP,
                                    reinterpret_cast<const uint8_t*>(text), textLen,
                                    false, false, 0u, packetId, "TEXT");
    FT02_LOGI_PRINT("[MESH-TX] broadcast text=\"");
    printSanitizedTextInfo(reinterpret_cast<const uint8_t*>(text), textLen);
    FT02_LOGI("\" %s\n", ok ? "OK" : "FAIL");
    return ok;
}

bool FT02_MeshServiceCanSendPrivate(uint32_t destination) {
    return FT02_MeshServiceReady() && g_pkiReady && destination != 0u &&
           destination != FT02_MESH_LOCAL_NODE && destination != FT02_MESH_BROADCAST_NODE &&
           g_pki.peerPublicKey(destination) != nullptr;
}

bool FT02_MeshServiceSendPrivateText(uint32_t destination, const char* text, uint32_t packetId) {
    if (!FT02_MeshServiceCanSendPrivate(destination) || !text || !text[0] || packetId == 0u) {
        FT02_LOGW("[WARN][MESH-TX] PKI reject to=0x%08lX key=%d ready=%d\n",
                      static_cast<unsigned long>(destination),
                      g_pki.peerPublicKey(destination) ? 1 : 0, FT02_MeshServiceReady() ? 1 : 0);
        return false;
    }
    size_t textLen = strlen(text);
    if (textLen > 120u) textLen = 120u;
    while (textLen > 0u && (static_cast<uint8_t>(text[textLen]) & 0xC0u) == 0x80u) --textLen;
    if (textLen == 0u) return false;

    uint8_t dataProto[180] = {};
    size_t dataLen = 0;
    if (!FT02_MeshBuildDataProto(FT02_MESH_PORT_TEXT_MESSAGE_APP,
                                 reinterpret_cast<const uint8_t*>(text), textLen,
                                 false, 0u, dataProto, sizeof(dataProto), dataLen)) return false;
    uint8_t encrypted[MESH_MAX_PAYLOAD] = {};
    size_t encryptedLen = 0;
    const uint8_t *peerKey = g_pki.peerPublicKey(destination);
    if (!g_pki.encryptForPeer(peerKey, packetId, FT02_MESH_LOCAL_NODE,
                              dataProto, dataLen, encrypted, sizeof(encrypted), encryptedLen)) {
        ++g_txFailures;
        FT02_LOGE("[ERROR][MESH-PKI] encrypt failed to=0x%08lX id=0x%08lX\n",
                      static_cast<unsigned long>(destination), static_cast<unsigned long>(packetId));
        return false;
    }
    const bool ok = transmitPrepared(destination, 0u, encrypted, encryptedLen, true, packetId, "PKI_TEXT");
    if (ok) ++g_privateTx;
    char destinationLabel[28] = {};
    nodeLabel(destination, destinationLabel, sizeof(destinationLabel));
    FT02_LOGI("[MESH-TX] %s text=\"", destinationLabel);
    printSanitizedTextInfo(reinterpret_cast<const uint8_t*>(text), textLen);
    FT02_LOGI("\" PKI %s\n", ok ? "OK" : "FAIL");
    return ok;
}

bool FT02_MeshServiceRequestNodeInfo() {
    if (!FT02_MeshServiceReady() || !g_pkiReady) return false;
    const uint32_t now = millis();
    // Node list refresh can be requested from the UI and from queued private
    // delivery. Rate-limit it so a missing peer key cannot create a TX storm.
    if (g_lastNodeInfoTxMs != 0u && static_cast<uint32_t>(now - g_lastNodeInfoTxMs) < NODEINFO_REPLY_MIN_MS) {
        return true;
    }
    return sendNodeInfo(true);
}

bool FT02_MeshServicePkiReady() { return g_pkiReady; }
uint8_t FT02_MeshServicePkiPeerCount() { return g_pki.peerCount(); }
uint32_t FT02_MeshServiceRawFrameCount() { return g_rawFrames; }
uint32_t FT02_MeshServiceDecodedFrameCount() { return g_decodedFrames; }
bool FT02_MeshServiceReady() { return g_serviceReady && FT02_MeshRadioBackendReady(); }
bool FT02_MeshServiceSleepAvailable() { return FT02_MeshRadioBackendSleepAvailable(); }
void FT02_MeshServiceSleepEnter() { FT02_MeshRadioBackendSleepEnter(); }
void FT02_MeshServiceSleepService() { FT02_MeshRadioBackendSleepService(); }
void FT02_MeshServiceSleepExit() { FT02_MeshRadioBackendSleepExit(); }
