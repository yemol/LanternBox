#include "FT02_MeshVoiceTransport.h"

#include "FT02_Log.h"
#include "FT02_MeshMessageStore.h"
#include "FT02_MeshProtocol.h"
#include "FT02_MeshService.h"
#include "FT02_MeshVoiceProtocol.h"
#include "FT02_Storage.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

namespace
{
constexpr size_t RX_SESSION_MAX = 2u;
constexpr size_t COMPLETE_CACHE_MAX = 8u;
constexpr uint32_t TX_GAP_MS = 120u;
constexpr uint32_t DIRECT_NACK_IDLE_MS = 6500u;
constexpr uint32_t RX_ABORT_MS = 45000u;
constexpr uint32_t BROADCAST_ABORT_MS = 18000u;
constexpr uint32_t TX_ACK_HOLD_MS = 90000u;
constexpr uint8_t MAX_NACK_ROUNDS = 3u;
constexpr uint8_t TX_MAX_CONSECUTIVE_FAILURES = 5u;

struct RxSession
{
    bool active;
    bool direct;
    uint32_t from;
    uint32_t to;
    uint32_t voiceId;
    uint8_t codec;
    uint8_t frameBytes;
    uint16_t durationMs;
    uint16_t totalBytes;
    uint8_t totalChunks;
    uint32_t expectedCrc;
    uint64_t receivedMask[FT02_VOICE_NACK_WINDOWS];
    uint8_t nackRound[FT02_VOICE_NACK_WINDOWS];
    uint8_t nackWindowCursor;
    uint32_t lastActivityMs;
    float rssi;
    float snr;
    uint8_t* data;
};

struct CompleteEntry
{
    uint32_t from;
    uint32_t voiceId;
};

enum TxPhase : uint8_t
{
    TX_IDLE = 0,
    TX_BEGIN_1,
    TX_BEGIN_2,
    TX_CHUNKS,
    TX_END,
    TX_WAIT_ACK,
    TX_RESEND,
    TX_RESEND_END
};

struct TxSession
{
    bool active;
    bool direct;
    uint32_t destination;
    uint32_t logicalId;
    uint32_t voiceId;
    char path[FT02_MESH_HISTORY_VOICE_PATH_BYTES];
    uint8_t codec;
    uint8_t frameBytes;
    uint16_t durationMs;
    uint16_t totalBytes;
    uint8_t totalChunks;
    uint32_t crc;
    uint8_t* data;
    TxPhase phase;
    uint8_t nextChunk;
    uint64_t resendMask;
    uint8_t resendWindow;
    uint8_t resendRound;
    uint32_t lastSendMs;
    uint32_t waitStartedMs;
    uint32_t firstPacketId;
    uint8_t consecutiveFailures;
};

RxSession g_rx[RX_SESSION_MAX] = {};
CompleteEntry g_complete[COMPLETE_CACHE_MAX] = {};
size_t g_completeNext = 0u;
TxSession g_tx{};
uint8_t* g_rxBuffers[RX_SESSION_MAX] = {};
uint8_t* g_txBuffer = nullptr;
uint32_t g_txRevision = 0u;
FT02MeshVoiceTxOutcome g_lastTxOutcome = FT02_MESH_VOICE_TX_NONE;

uint8_t windowCount(uint8_t chunks)
{
    return chunks == 0u ? 0u : static_cast<uint8_t>((chunks + 63u) / 64u);
}

uint64_t fullMaskForWindow(uint8_t chunks, uint8_t window)
{
    const uint16_t first = static_cast<uint16_t>(window) * 64u;
    if(first >= chunks) return 0u;
    const uint16_t remain = static_cast<uint16_t>(chunks) - first;
    if(remain >= 64u) return 0xFFFFFFFFFFFFFFFFull;
    return (1ull << remain) - 1ull;
}

bool allChunksReceived(const RxSession& rx)
{
    const uint8_t windows = windowCount(rx.totalChunks);
    for(uint8_t w = 0u; w < windows; ++w)
    {
        const uint64_t full = fullMaskForWindow(rx.totalChunks, w);
        if((rx.receivedMask[w] & full) != full) return false;
    }
    return rx.totalChunks != 0u;
}

uint64_t missingForWindow(const RxSession& rx, uint8_t window)
{
    return fullMaskForWindow(rx.totalChunks, window) & ~rx.receivedMask[window];
}

bool missingWindowsExhausted(const RxSession& rx)
{
    const uint8_t windows = windowCount(rx.totalChunks);
    bool anyMissing = false;
    for(uint8_t w = 0u; w < windows; ++w)
    {
        if(missingForWindow(rx, w) == 0u) continue;
        anyMissing = true;
        if(rx.nackRound[w] < MAX_NACK_ROUNDS) return false;
    }
    return anyMissing;
}

uint8_t* allocVoiceBuffer()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint8_t*>(heap_caps_malloc(FT02_VOICE_MAX_ENCODED_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    return static_cast<uint8_t*>(malloc(FT02_VOICE_MAX_ENCODED_BYTES));
#endif
}

void resetRx(RxSession& rx)
{
    uint8_t* keep = rx.data;
    memset(&rx, 0, sizeof(rx));
    rx.data = keep;
}

void resetTx()
{
    uint8_t* keep = g_tx.data;
    memset(&g_tx, 0, sizeof(g_tx));
    g_tx.data = keep;
    ++g_txRevision;
}

bool sendApp(uint32_t destination, const uint8_t* data, size_t len, uint32_t* packetIdOut = nullptr)
{
    if(data == nullptr || len == 0u) return false;
    const uint32_t packetId = FT02_MeshServiceAllocatePacketId();
    if(packetId == 0u) return false;
    const bool direct = destination != FT02_MESH_BROADCAST_NODE;
    const bool ok = direct
        ? FT02_MeshServiceSendPrivateApp(destination, FT02_MESH_PORT_LANTERNBOX_VOICE,
                                         data, len, packetId, false)
        : FT02_MeshServiceSendBroadcastApp(FT02_MESH_PORT_LANTERNBOX_VOICE,
                                           data, len, packetId);
    if(ok && packetIdOut != nullptr) *packetIdOut = packetId;
    return ok;
}

bool wasCompleted(uint32_t from, uint32_t voiceId)
{
    for(size_t i = 0u; i < COMPLETE_CACHE_MAX; ++i)
        if(g_complete[i].from == from && g_complete[i].voiceId == voiceId) return true;
    return false;
}

void rememberCompleted(uint32_t from, uint32_t voiceId)
{
    g_complete[g_completeNext].from = from;
    g_complete[g_completeNext].voiceId = voiceId;
    g_completeNext = (g_completeNext + 1u) % COMPLETE_CACHE_MAX;
}

RxSession* findRx(uint32_t from, uint32_t voiceId)
{
    for(size_t i = 0u; i < RX_SESSION_MAX; ++i)
        if(g_rx[i].active && g_rx[i].from == from && g_rx[i].voiceId == voiceId) return &g_rx[i];
    return nullptr;
}

RxSession* allocateRx()
{
    for(size_t i = 0u; i < RX_SESSION_MAX; ++i)
        if(!g_rx[i].active && g_rx[i].data != nullptr) return &g_rx[i];
    size_t oldest = RX_SESSION_MAX;
    for(size_t i = 0u; i < RX_SESSION_MAX; ++i)
    {
        if(g_rx[i].data == nullptr) continue;
        if(oldest == RX_SESSION_MAX ||
           static_cast<int32_t>(g_rx[i].lastActivityMs - g_rx[oldest].lastActivityMs) < 0) oldest = i;
    }
    if(oldest == RX_SESSION_MAX) return nullptr;
    FT02_LOGW("[WARN][MESH-VOICE] replacing stale RX from=0x%08lX voice=0x%08lX\n",
              static_cast<unsigned long>(g_rx[oldest].from),
              static_cast<unsigned long>(g_rx[oldest].voiceId));
    return &g_rx[oldest];
}

bool sendAck(uint32_t destination, uint32_t voiceId)
{
    uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
    size_t wireLen = 0u;
    return FT02_MeshVoiceBuildSimple(FT02_VOICE_PKT_ACK, voiceId, wire, sizeof(wire), wireLen) &&
           sendApp(destination, wire, wireLen);
}

bool sendNack(RxSession& rx)
{
    if(!rx.direct) return false;
    const uint8_t windows = windowCount(rx.totalChunks);
    if(windows == 0u) return false;
    for(uint8_t step = 0u; step < windows; ++step)
    {
        const uint8_t w = static_cast<uint8_t>((rx.nackWindowCursor + step) % windows);
        const uint64_t missing = missingForWindow(rx, w);
        if(missing == 0u || rx.nackRound[w] >= MAX_NACK_ROUNDS) continue;
        ++rx.nackRound[w];
        rx.nackWindowCursor = static_cast<uint8_t>((w + 1u) % windows);
        uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
        size_t wireLen = 0u;
        const bool built = FT02_MeshVoiceBuildNack(rx.voiceId, missing, w, rx.nackRound[w],
                                                   wire, sizeof(wire), wireLen);
        const bool ok = built && sendApp(rx.from, wire, wireLen);
        if(ok)
        {
            rx.lastActivityMs = millis();
            FT02_LOGI("[MESH-VOICE] NACK to=0x%08lX voice=0x%08lX win=%u round=%u missing=%08lX%08lX\n",
                      static_cast<unsigned long>(rx.from), static_cast<unsigned long>(rx.voiceId),
                      static_cast<unsigned>(w), static_cast<unsigned>(rx.nackRound[w]),
                      static_cast<unsigned long>(missing >> 32u), static_cast<unsigned long>(missing));
        }
        return ok;
    }
    return false;
}

bool saveReceived(RxSession& rx, uint32_t packetId)
{
    if(!FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;
    if(!allChunksReceived(rx)) return false;
    const uint32_t actualCrc = FT02_MeshVoiceCrc32(rx.data, rx.totalBytes);
    if(actualCrc != rx.expectedCrc)
    {
        FT02_LOGW("[WARN][MESH-VOICE] CRC mismatch from=0x%08lX voice=0x%08lX expected=%08lX actual=%08lX\n",
                  static_cast<unsigned long>(rx.from), static_cast<unsigned long>(rx.voiceId),
                  static_cast<unsigned long>(rx.expectedCrc), static_cast<unsigned long>(actualCrc));
        return false;
    }

    char path[FT02_MESH_HISTORY_VOICE_PATH_BYTES] = {};
    snprintf(path, sizeof(path), "/lanternbox/mesh/voice/rx_%08lX_%08lX.lbv",
             static_cast<unsigned long>(rx.from), static_cast<unsigned long>(rx.voiceId));

    FT02MeshVoiceFileHeader header{};
    header.codec = rx.codec;
    header.frameBytes = rx.frameBytes;
    header.sampleRate = FT02_MeshVoiceSampleRateForCodec(rx.codec);
    header.frameSamples = FT02_MeshVoiceFrameSamplesForCodec(rx.codec);
    header.durationMs = rx.durationMs;
    header.payloadBytes = rx.totalBytes;
    header.payloadCrc32 = rx.expectedCrc;
    uint8_t diskHeader[FT02_VOICE_FILE_HEADER_BYTES] = {};
    if(!FT02_MeshVoiceBuildFileHeader(header, diskHeader)) return false;

    FILE* file = FT02_StorageOpenWriteFile(path, true);
    if(file == nullptr) return false;
    bool ok = fwrite(diskHeader, 1u, sizeof(diskHeader), file) == sizeof(diskHeader);
    if(ok) ok = fwrite(rx.data, 1u, rx.totalBytes, file) == rx.totalBytes;
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(path);
        return false;
    }

    ok = FT02_MeshMessageStoreAppendRxVoice(packetId != 0u ? packetId : rx.voiceId,
                                             rx.from, rx.to, rx.direct,
                                             rx.durationMs, path, FT02_MESH_STATUS_RECEIVED);
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(path);
        return false;
    }

    rememberCompleted(rx.from, rx.voiceId);
    FT02_LOGI("[MESH-VOICE] complete from=0x%08lX voice=0x%08lX bytes=%u duration=%ums path=%s\n",
              static_cast<unsigned long>(rx.from), static_cast<unsigned long>(rx.voiceId),
              static_cast<unsigned>(rx.totalBytes), static_cast<unsigned>(rx.durationMs), path);
    if(rx.direct) (void)sendAck(rx.from, rx.voiceId);
    resetRx(rx);
    return true;
}

bool loadVoiceFile(const char* path,
                   FT02MeshVoiceFileHeader& header,
                   uint8_t* payload,
                   size_t payloadCapacity)
{
    memset(&header, 0, sizeof(header));
    if(path == nullptr || !path[0] || payload == nullptr || !FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;
    uint8_t raw[FT02_VOICE_FILE_HEADER_BYTES] = {};
    bool ok = fread(raw, 1u, sizeof(raw), file) == sizeof(raw) &&
              FT02_MeshVoiceParseFileHeader(raw, sizeof(raw), header) &&
              header.payloadBytes <= payloadCapacity;
    if(ok) ok = fread(payload, 1u, header.payloadBytes, file) == header.payloadBytes;
    fclose(file);
    if(!ok) return false;
    return FT02_MeshVoiceCrc32(payload, header.payloadBytes) == header.payloadCrc32;
}

bool txWire(const uint8_t* wire, size_t wireLen)
{
    uint32_t packetId = 0u;
    const bool ok = sendApp(g_tx.destination, wire, wireLen, &packetId);
    if(ok)
    {
        g_tx.lastSendMs = millis();
        if(g_tx.firstPacketId == 0u)
        {
            g_tx.firstPacketId = packetId;
            (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, packetId,
                                                g_tx.direct ? FT02_MESH_STATUS_WAITING_ACK : FT02_MESH_STATUS_QUEUED);
        }
    }
    return ok;
}

bool transmitBegin()
{
    uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
    size_t len = 0u;
    return FT02_MeshVoiceBuildBegin(g_tx.voiceId, g_tx.codec, g_tx.frameBytes,
                                    g_tx.durationMs, g_tx.totalBytes,
                                    g_tx.totalChunks, g_tx.crc, wire, sizeof(wire), len) && txWire(wire, len);
}

bool transmitChunk(uint8_t index)
{
    if(index >= g_tx.totalChunks) return false;
    const size_t offset = static_cast<size_t>(index) * FT02_VOICE_CHUNK_BYTES;
    const size_t remain = g_tx.totalBytes - offset;
    const uint8_t n = static_cast<uint8_t>(remain > FT02_VOICE_CHUNK_BYTES ? FT02_VOICE_CHUNK_BYTES : remain);
    uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
    size_t len = 0u;
    return FT02_MeshVoiceBuildChunk(g_tx.voiceId, index, g_tx.data + offset, n,
                                    wire, sizeof(wire), len) && txWire(wire, len);
}

bool transmitEnd()
{
    uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
    size_t len = 0u;
    return FT02_MeshVoiceBuildSimple(FT02_VOICE_PKT_END, g_tx.voiceId,
                                     wire, sizeof(wire), len) && txWire(wire, len);
}

int firstSetBit(uint64_t mask, uint8_t limit)
{
    for(uint8_t i = 0u; i < limit; ++i) if((mask & (1ull << i)) != 0u) return i;
    return -1;
}

void finishBroadcastTx()
{
    (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_SENT);
    g_lastTxOutcome = FT02_MESH_VOICE_TX_SENT;
    FT02_LOGI("[MESH-VOICE] broadcast sent voice=0x%08lX chunks=%u\n",
              static_cast<unsigned long>(g_tx.voiceId), static_cast<unsigned>(g_tx.totalChunks));
    resetTx();
}
}

void FT02_MeshVoiceTransportBegin()
{
    memset(g_complete, 0, sizeof(g_complete));
    for(size_t i = 0u; i < RX_SESSION_MAX; ++i)
    {
        if(g_rxBuffers[i] == nullptr) g_rxBuffers[i] = allocVoiceBuffer();
        memset(&g_rx[i], 0, sizeof(g_rx[i]));
        g_rx[i].data = g_rxBuffers[i];
        if(g_rx[i].data == nullptr)
            FT02_LOGE("[ERROR][MESH-VOICE] RX PSRAM alloc failed slot=%u bytes=%u\n",
                      static_cast<unsigned>(i), static_cast<unsigned>(FT02_VOICE_MAX_ENCODED_BYTES));
    }
    if(g_txBuffer == nullptr) g_txBuffer = allocVoiceBuffer();
    memset(&g_tx, 0, sizeof(g_tx));
    g_tx.data = g_txBuffer;
    if(g_tx.data == nullptr)
        FT02_LOGE("[ERROR][MESH-VOICE] TX PSRAM alloc failed bytes=%u\n",
                  static_cast<unsigned>(FT02_VOICE_MAX_ENCODED_BYTES));
    g_completeNext = 0u;
    g_txRevision = 1u;
    g_lastTxOutcome = FT02_MESH_VOICE_TX_NONE;
}

void FT02_MeshVoiceTransportPoll()
{
    const uint32_t now = millis();

    // Receiver repair/expiry service.
    for(size_t i = 0u; i < RX_SESSION_MAX; ++i)
    {
        RxSession& rx = g_rx[i];
        if(!rx.active) continue;
        const uint32_t idle = static_cast<uint32_t>(now - rx.lastActivityMs);
        if(allChunksReceived(rx))
        {
            (void)saveReceived(rx, rx.voiceId);
            continue;
        }
        if(rx.direct && idle >= DIRECT_NACK_IDLE_MS && !missingWindowsExhausted(rx))
        {
            (void)sendNack(rx);
            continue;
        }
        const uint32_t abortMs = rx.direct ? RX_ABORT_MS : BROADCAST_ABORT_MS;
        if(idle >= abortMs || (rx.direct && missingWindowsExhausted(rx) && idle >= DIRECT_NACK_IDLE_MS))
        {
            FT02_LOGW("[WARN][MESH-VOICE] RX incomplete discarded from=0x%08lX voice=0x%08lX chunks=%u\n",
                      static_cast<unsigned long>(rx.from), static_cast<unsigned long>(rx.voiceId),
                      static_cast<unsigned>(rx.totalChunks));
            resetRx(rx);
        }
    }

    if(!g_tx.active || !FT02_MeshServiceReady()) return;
    if(static_cast<uint32_t>(now - g_tx.lastSendMs) < TX_GAP_MS) return;

    bool ok = false;
    bool attempted = false;
    switch(g_tx.phase)
    {
        case TX_BEGIN_1:
            attempted = true;
            ok = transmitBegin();
            if(ok)
            {
                FT02_LOGI("[MESH-VOICE-TX] RF OK BEGIN 1/2 voice=0x%08lX\n", static_cast<unsigned long>(g_tx.voiceId));
                g_tx.phase = TX_BEGIN_2;
                ++g_txRevision;
            }
            break;
        case TX_BEGIN_2:
            attempted = true;
            ok = transmitBegin(); // small duplicate protects against a lost BEGIN.
            if(ok)
            {
                FT02_LOGI("[MESH-VOICE-TX] RF OK BEGIN 2/2 voice=0x%08lX\n", static_cast<unsigned long>(g_tx.voiceId));
                g_tx.phase = TX_CHUNKS;
                g_tx.nextChunk = 0u;
                ++g_txRevision;
            }
            break;
        case TX_CHUNKS:
            attempted = true;
            ok = transmitChunk(g_tx.nextChunk);
            if(ok)
            {
                FT02_LOGI("[MESH-VOICE-TX] RF OK CHUNK %u/%u voice=0x%08lX\n",
                          static_cast<unsigned>(g_tx.nextChunk + 1u),
                          static_cast<unsigned>(g_tx.totalChunks),
                          static_cast<unsigned long>(g_tx.voiceId));
                ++g_tx.nextChunk;
                ++g_txRevision;
                if(g_tx.nextChunk >= g_tx.totalChunks) g_tx.phase = TX_END;
            }
            break;
        case TX_END:
            attempted = true;
            ok = transmitEnd();
            if(ok)
            {
                FT02_LOGI("[MESH-VOICE-TX] RF OK END voice=0x%08lX\n", static_cast<unsigned long>(g_tx.voiceId));
                ++g_txRevision;
                if(g_tx.direct)
                {
                    g_tx.phase = TX_WAIT_ACK;
                    g_tx.waitStartedMs = now;
                    (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_WAITING_ACK);
                }
                else finishBroadcastTx();
            }
            break;
        case TX_RESEND:
        {
            const int bit = firstSetBit(g_tx.resendMask, 64u);
            if(bit < 0) { g_tx.phase = TX_RESEND_END; break; }
            const uint16_t absolute = static_cast<uint16_t>(g_tx.resendWindow) * 64u + static_cast<uint16_t>(bit);
            if(absolute >= g_tx.totalChunks) { g_tx.resendMask &= ~(1ull << bit); break; }
            attempted = true;
            ok = transmitChunk(static_cast<uint8_t>(absolute));
            if(ok) g_tx.resendMask &= ~(1ull << bit);
            break;
        }
        case TX_RESEND_END:
            attempted = true;
            ok = transmitEnd();
            if(ok)
            {
                g_tx.phase = TX_WAIT_ACK;
                g_tx.waitStartedMs = now;
            }
            break;
        case TX_WAIT_ACK:
            // No NACK does not prove delivery; after the hold window retain a
            // conservative SENT state and release RAM. A received ACK upgrades
            // to DELIVERED immediately in OnPayload().
            if(static_cast<uint32_t>(now - g_tx.waitStartedMs) >= TX_ACK_HOLD_MS)
            {
                (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_SENT);
                g_lastTxOutcome = FT02_MESH_VOICE_TX_SENT;
                FT02_LOGI("[MESH-VOICE] direct ACK hold expired voice=0x%08lX; state=SENT\n",
                          static_cast<unsigned long>(g_tx.voiceId));
                resetTx();
            }
            break;
        default:
            break;
    }

    if(ok && g_tx.active)
    {
        g_tx.consecutiveFailures = 0u;
    }
    else if(attempted && g_tx.active && g_tx.phase != TX_WAIT_ACK)
    {
        ++g_tx.consecutiveFailures;
        g_tx.lastSendMs = now;
        if(g_tx.consecutiveFailures >= TX_MAX_CONSECUTIVE_FAILURES)
        {
            FT02_LOGE("[ERROR][MESH-VOICE] TX aborted after %u consecutive failures voice=0x%08lX phase=%u\n",
                      static_cast<unsigned>(g_tx.consecutiveFailures),
                      static_cast<unsigned long>(g_tx.voiceId),
                      static_cast<unsigned>(g_tx.phase));
            (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_FAILED);
            g_lastTxOutcome = FT02_MESH_VOICE_TX_FAILED;
            resetTx();
        }
    }
}

void FT02_MeshVoiceTransportOnPayload(uint32_t packetId,
                                      uint32_t from,
                                      uint32_t to,
                                      bool pkiEncrypted,
                                      float rssi,
                                      float snr,
                                      const uint8_t* payload,
                                      size_t payloadLen)
{
    FT02MeshVoicePacketView p{};
    if(!FT02_MeshVoiceParsePacket(payload, payloadLen, p))
    {
        FT02_LOGW("[WARN][MESH-VOICE] invalid packet from=0x%08lX len=%u\n",
                  static_cast<unsigned long>(from), static_cast<unsigned>(payloadLen));
        return;
    }
    const bool direct = pkiEncrypted && to == FT02_MESH_LOCAL_NODE;

    // Sender control packets are meaningful only for the current direct TX.
    if(p.type == FT02_VOICE_PKT_ACK || p.type == FT02_VOICE_PKT_NACK || p.type == FT02_VOICE_PKT_CANCEL)
    {
        if(!g_tx.active || !g_tx.direct || from != g_tx.destination || p.voiceId != g_tx.voiceId) return;
        if(p.type == FT02_VOICE_PKT_ACK)
        {
            (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_DELIVERED);
            g_lastTxOutcome = FT02_MESH_VOICE_TX_DELIVERED;
            FT02_LOGI("[MESH-VOICE] ACK from=0x%08lX voice=0x%08lX DELIVERED\n",
                      static_cast<unsigned long>(from), static_cast<unsigned long>(p.voiceId));
            resetTx();
        }
        else if(p.type == FT02_VOICE_PKT_NACK)
        {
            const uint64_t valid = fullMaskForWindow(g_tx.totalChunks, p.nackWindow);
            const uint64_t missing = p.missingMask & valid;
            if(missing == 0u) return;
            if(p.round > MAX_NACK_ROUNDS)
            {
                (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_FAILED);
                g_lastTxOutcome = FT02_MESH_VOICE_TX_FAILED;
                resetTx();
                return;
            }
            g_tx.resendMask = missing;
            g_tx.resendWindow = p.nackWindow;
            g_tx.resendRound = p.round;
            g_tx.phase = TX_RESEND;
            g_tx.lastSendMs = 0u;
            FT02_LOGI("[MESH-VOICE] NACK RX voice=0x%08lX win=%u round=%u missing=%08lX%08lX\n",
                      static_cast<unsigned long>(p.voiceId), static_cast<unsigned>(p.nackWindow),
                      static_cast<unsigned>(p.round), static_cast<unsigned long>(missing >> 32u),
                      static_cast<unsigned long>(missing));
        }
        else
        {
            (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_CANCELED);
            g_lastTxOutcome = FT02_MESH_VOICE_TX_CANCELED;
            resetTx();
        }
        return;
    }

    if(wasCompleted(from, p.voiceId))
    {
        if(direct && (p.type == FT02_VOICE_PKT_BEGIN || p.type == FT02_VOICE_PKT_END)) (void)sendAck(from, p.voiceId);
        return;
    }

    if(p.type == FT02_VOICE_PKT_BEGIN)
    {
        RxSession* rx = findRx(from, p.voiceId);
        if(rx == nullptr) rx = allocateRx();
        if(rx == nullptr) return;
        const bool same = rx->active && rx->from == from && rx->voiceId == p.voiceId &&
                          rx->totalBytes == p.totalBytes && rx->totalChunks == p.totalChunks &&
                          rx->expectedCrc == p.payloadCrc32;
        if(!same)
        {
            resetRx(*rx);
            rx->active = true;
            rx->direct = direct;
            rx->from = from;
            rx->to = direct ? FT02_MESH_LOCAL_NODE : FT02_MESH_BROADCAST_NODE;
            rx->voiceId = p.voiceId;
            rx->codec = p.codec;
            rx->frameBytes = p.frameBytes;
            rx->durationMs = p.durationMs;
            rx->totalBytes = p.totalBytes;
            rx->totalChunks = p.totalChunks;
            rx->expectedCrc = p.payloadCrc32;
            rx->rssi = rssi;
            rx->snr = snr;
            FT02_LOGI("[MESH-VOICE] BEGIN from=0x%08lX voice=0x%08lX %s bytes=%u chunks=%u duration=%ums\n",
                      static_cast<unsigned long>(from), static_cast<unsigned long>(p.voiceId),
                      direct ? "direct" : "broadcast", static_cast<unsigned>(p.totalBytes),
                      static_cast<unsigned>(p.totalChunks), static_cast<unsigned>(p.durationMs));
        }
        rx->lastActivityMs = millis();
        return;
    }

    RxSession* rx = findRx(from, p.voiceId);
    if(rx == nullptr) return; // A duplicated BEGIN is sent by the sender; wait for it rather than guessing metadata.
    rx->lastActivityMs = millis();
    rx->rssi = rssi;
    rx->snr = snr;

    if(p.type == FT02_VOICE_PKT_CHUNK)
    {
        if(p.chunkIndex >= rx->totalChunks) return;
        const size_t offset = static_cast<size_t>(p.chunkIndex) * FT02_VOICE_CHUNK_BYTES;
        if(offset >= rx->totalBytes) return;
        const size_t remain = rx->totalBytes - offset;
        const uint8_t expected = static_cast<uint8_t>(remain > FT02_VOICE_CHUNK_BYTES ? FT02_VOICE_CHUNK_BYTES : remain);
        if(p.chunkLen != expected) return;
        const uint8_t window = static_cast<uint8_t>(p.chunkIndex / 64u);
        const uint8_t bitIndex = static_cast<uint8_t>(p.chunkIndex % 64u);
        const uint64_t bit = 1ull << bitIndex;
        if(window >= FT02_VOICE_NACK_WINDOWS) return;
        if((rx->receivedMask[window] & bit) == 0u)
        {
            memcpy(rx->data + offset, p.chunkData, p.chunkLen);
            rx->receivedMask[window] |= bit;
        }
        if(allChunksReceived(*rx))
            (void)saveReceived(*rx, packetId);
        return;
    }

    if(p.type == FT02_VOICE_PKT_END)
    {
        if(allChunksReceived(*rx))
            (void)saveReceived(*rx, packetId);
        else if(rx->direct)
            (void)sendNack(*rx);
        return;
    }
}

bool FT02_MeshVoiceTransportQueueFile(uint32_t destination,
                                      const char* voicePath,
                                      uint32_t logicalId)
{
    if(g_tx.active || voicePath == nullptr || voicePath[0] == '\0') return false;
    const bool direct = destination != FT02_MESH_BROADCAST_NODE;
    if(direct && !FT02_MeshServiceCanSendPrivate(destination)) return false;
    if(direct && FT02_VOICE_WIRE_MAX_BYTES > FT02_MeshServiceMaxPrivateAppPayload())
    {
        FT02_LOGE("[ERROR][MESH-VOICE] private wire envelope unsafe wire=%u max=%u\n",
                  static_cast<unsigned>(FT02_VOICE_WIRE_MAX_BYTES),
                  static_cast<unsigned>(FT02_MeshServiceMaxPrivateAppPayload()));
        return false;
    }
    if(!direct && destination != FT02_MESH_BROADCAST_NODE) return false;

    // Load directly into the PSRAM-backed TX payload window. Keep the legacy
    // 49.5 KiB receive/history ceiling while current Opus-6000 TX is <=11.25 KiB.
    // The TX session is not published active until header/payload/CRC validate.
    resetTx();
    FT02MeshVoiceFileHeader header{};
    if(g_tx.data == nullptr || !loadVoiceFile(voicePath, header, g_tx.data, FT02_VOICE_MAX_ENCODED_BYTES))
    {
        resetTx();
        return false;
    }
    const uint8_t chunks = static_cast<uint8_t>((header.payloadBytes + FT02_VOICE_CHUNK_BYTES - 1u) / FT02_VOICE_CHUNK_BYTES);
    if(chunks == 0u || chunks > FT02_VOICE_MAX_CHUNKS || header.durationMs > 0xFFFFu || header.payloadBytes > 0xFFFFu)
    {
        resetTx();
        return false;
    }

    g_tx.active = true;
    g_tx.direct = direct;
    g_tx.destination = destination;
    g_tx.logicalId = logicalId != 0u ? logicalId : FT02_MeshServiceAllocatePacketId();
    g_tx.voiceId = g_tx.logicalId;
    if(g_tx.logicalId == 0u) { resetTx(); return false; }
    snprintf(g_tx.path, sizeof(g_tx.path), "%s", voicePath);
    g_tx.codec = header.codec;
    g_tx.frameBytes = header.frameBytes;
    g_tx.durationMs = static_cast<uint16_t>(header.durationMs);
    g_tx.totalBytes = static_cast<uint16_t>(header.payloadBytes);
    g_tx.totalChunks = chunks;
    g_tx.crc = header.payloadCrc32;
    g_tx.phase = TX_BEGIN_1;
    g_tx.lastSendMs = 0u;
    g_tx.consecutiveFailures = 0u;
    g_lastTxOutcome = FT02_MESH_VOICE_TX_NONE;
    ++g_txRevision;

    if(!FT02_MeshMessageStoreAppendTxVoice(g_tx.logicalId, FT02_MESH_LOCAL_NODE, destination,
                                            direct, header.durationMs, voicePath,
                                            FT02_MESH_STATUS_QUEUED, 0u))
    {
        resetTx();
        return false;
    }
    FT02_LOGI("[MESH-VOICE] queued voice=0x%08lX to=0x%08lX bytes=%u chunks=%u duration=%lums\n",
              static_cast<unsigned long>(g_tx.voiceId), static_cast<unsigned long>(destination),
              static_cast<unsigned>(g_tx.totalBytes), static_cast<unsigned>(g_tx.totalChunks),
              static_cast<unsigned long>(header.durationMs));
    return true;
}

bool FT02_MeshVoiceTransportBusy()
{
    return g_tx.active;
}

uint32_t FT02_MeshVoiceTransportVoiceId()
{
    return g_tx.active ? g_tx.voiceId : 0u;
}

uint8_t FT02_MeshVoiceTransportProgressPercent()
{
    if(!g_tx.active) return 0u;
    if(g_tx.phase == TX_WAIT_ACK) return 100u;
    uint32_t sent = g_tx.nextChunk;
    if(g_tx.phase == TX_END || g_tx.phase == TX_RESEND || g_tx.phase == TX_RESEND_END) sent = g_tx.totalChunks;
    const uint32_t pct = g_tx.totalChunks ? (sent * 100u) / g_tx.totalChunks : 0u;
    return static_cast<uint8_t>(pct > 100u ? 100u : pct);
}

uint8_t FT02_MeshVoiceTransportChunksSent()
{
    if(!g_tx.active) return 0u;
    if(g_tx.phase == TX_WAIT_ACK || g_tx.phase == TX_END || g_tx.phase == TX_RESEND || g_tx.phase == TX_RESEND_END)
        return g_tx.totalChunks;
    return g_tx.nextChunk;
}

uint8_t FT02_MeshVoiceTransportTotalChunks()
{
    return g_tx.active ? g_tx.totalChunks : 0u;
}

uint32_t FT02_MeshVoiceTransportRevision()
{
    return g_txRevision;
}

bool FT02_MeshVoiceTransportAwaitingAck()
{
    return g_tx.active && g_tx.direct && g_tx.phase == TX_WAIT_ACK;
}

FT02MeshVoiceTxOutcome FT02_MeshVoiceTransportLastOutcome()
{
    return g_lastTxOutcome;
}

void FT02_MeshVoiceTransportCancel()
{
    if(!g_tx.active) return;
    if(g_tx.direct)
    {
        uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES] = {};
        size_t len = 0u;
        if(FT02_MeshVoiceBuildCancel(g_tx.voiceId, 1u, wire, sizeof(wire), len))
            (void)sendApp(g_tx.destination, wire, len);
    }
    (void)FT02_MeshMessageStoreUpdateTx(g_tx.logicalId, g_tx.firstPacketId, FT02_MESH_STATUS_CANCELED);
    g_lastTxOutcome = FT02_MESH_VOICE_TX_CANCELED;
    resetTx();
}
