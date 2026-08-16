#include "FT02_LoRaCommunicationRuntime.h"

#include <esp_system.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "FT02_Storage.h"

#include "FT02_Gnss.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LoRaTransport.h"

namespace
{
constexpr uint32_t FT02_BROADCAST_NODE = 0xFFFFFFFFu;
constexpr uint32_t FT02_PORT_TEXT_MESSAGE_APP = 1u;
constexpr uint32_t FT02_PORT_NODEINFO_APP = 4u;
constexpr uint32_t FT02_PORT_ROUTING_APP = 5u;
constexpr uint32_t FT02_PRIORITY_RELIABLE = 70u;
constexpr uint32_t FT02_HOP_LIMIT = 3u;
constexpr uint32_t FT02_FRESH_GNSS_MS = 15000u;
constexpr size_t FT02_INBOX_MAX = 50;
constexpr size_t FT02_DEDUP_MAX = 32;
constexpr size_t FT02_PENDING_MAX = 8;
constexpr size_t FT02_PROTO_DATA_MAX = 240;
constexpr size_t FT02_PROTO_MESH_MAX = 360;
constexpr size_t FT02_PROTO_TORADIO_MAX = 400;
constexpr size_t FT02_OUTBOX_MAX = 16;
constexpr uint32_t FT02_IMMEDIATE_TTL_SEC = 10u * 60u;
constexpr uint32_t FT02_RELIABLE_TTL_SEC = 2u * 60u * 60u;
constexpr uint32_t FT02_RELIABLE_RETRY_MS = 180000u;
constexpr uint32_t FT02_PERSISTENT_RETRY_MS = 300000u;
constexpr uint32_t FT02_OUTBOX_FLUSH_IDLE_MS = 1500u;
constexpr const char* FT02_OUTBOX_PATH = "/lanternbox/messages/outbox_v1.tsv";
constexpr const char* FT02_OUTBOX_TEMP = "/lanternbox/messages/outbox_v1.tmp";
constexpr const char* FT02_OUTBOX_BACKUP = "/lanternbox/messages/outbox_v1.bak";
constexpr const char* FT02_OUTBOX_HEADER = "# FT02_OUTBOX_V1";

struct InboxSlot
{
    FT02LoRaMessageView message;
};

struct DedupKey
{
    bool valid;
    uint32_t from;
    uint32_t id;
};

struct PendingTx
{
    FT02LoRaTxStatusView view;
};

struct OutboxSlot
{
    FT02LoRaOutboxView view;
    uint32_t createdMs;
};

InboxSlot g_inbox[FT02_INBOX_MAX] = {};
size_t g_inboxHead = 0; // next write
size_t g_inboxCount = 0;
uint16_t g_unread = 0;

DedupKey g_dedup[FT02_DEDUP_MAX] = {};
size_t g_dedupHead = 0;
PendingTx g_pending[FT02_PENDING_MAX] = {};
size_t g_pendingHead = 0;
OutboxSlot g_outbox[FT02_OUTBOX_MAX] = {};
size_t g_outboxHead = 0;
size_t g_outboxCount = 0;
bool g_outboxLoaded = false;
bool g_outboxDirty = false;
uint32_t g_outboxDirtyMs = 0;
uint32_t g_outboxLoadRetryMs = 0;
uint32_t g_deliveryRevision = 0;

uint32_t g_revision = 0;
uint32_t g_rxTextCount = 0;
uint32_t g_txCount = 0;
uint32_t g_duplicateCount = 0;
uint32_t g_ackCount = 0;
uint32_t g_nakCount = 0;
uint32_t g_lastRxPacketId = 0;
uint32_t g_packetCounter = 0x20000001u;

void resolveOutboxPacket(uint32_t packetId, uint32_t error);
void outboxOnSessionReset();

bool readVarint(const uint8_t* data, size_t length, size_t& offset, uint64_t& value)
{
    value = 0;
    uint8_t shift = 0;
    while(offset < length && shift < 64)
    {
        const uint8_t b = data[offset++];
        value |= static_cast<uint64_t>(b & 0x7Fu) << shift;
        if((b & 0x80u) == 0) return true;
        shift = static_cast<uint8_t>(shift + 7);
    }
    return false;
}

bool readLength(const uint8_t* data, size_t length, size_t& offset, const uint8_t*& ptr, size_t& itemLength)
{
    uint64_t value = 0;
    if(!readVarint(data, length, offset, value)) return false;
    if(value > static_cast<uint64_t>(length - offset)) return false;
    itemLength = static_cast<size_t>(value);
    ptr = data + offset;
    offset += itemLength;
    return true;
}

bool readFixed32(const uint8_t* data, size_t length, size_t& offset, uint32_t& value)
{
    if(offset + 4 > length) return false;
    value = static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8) |
        (static_cast<uint32_t>(data[offset + 2]) << 16) |
        (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return true;
}

bool skipField(const uint8_t* data, size_t length, size_t& offset, uint8_t wire)
{
    uint64_t value = 0;
    const uint8_t* ptr = nullptr;
    size_t itemLength = 0;
    switch(wire)
    {
        case 0: return readVarint(data, length, offset, value);
        case 1:
            if(offset + 8 > length) return false;
            offset += 8;
            return true;
        case 2: return readLength(data, length, offset, ptr, itemLength);
        case 5:
            if(offset + 4 > length) return false;
            offset += 4;
            return true;
        default: return false;
    }
}

size_t encodeVarint(uint64_t value, uint8_t* out, size_t capacity)
{
    size_t count = 0;
    do
    {
        if(count >= capacity) return 0;
        uint8_t b = static_cast<uint8_t>(value & 0x7Fu);
        value >>= 7;
        if(value != 0) b |= 0x80u;
        out[count++] = b;
    } while(value != 0);
    return count;
}

struct ProtoWriter
{
    uint8_t* data;
    size_t capacity;
    size_t length;

    bool byte(uint8_t value)
    {
        if(length >= capacity) return false;
        data[length++] = value;
        return true;
    }

    bool bytes(const uint8_t* src, size_t count)
    {
        if(count > capacity - length) return false;
        if(count > 0) memcpy(data + length, src, count);
        length += count;
        return true;
    }

    bool varint(uint64_t value)
    {
        uint8_t tmp[10];
        const size_t n = encodeVarint(value, tmp, sizeof(tmp));
        return n > 0 && bytes(tmp, n);
    }

    bool key(uint32_t field, uint8_t wire)
    {
        return varint((static_cast<uint64_t>(field) << 3) | wire);
    }

    bool varintField(uint32_t field, uint64_t value)
    {
        return key(field, 0u) && varint(value);
    }

    bool fixed32Field(uint32_t field, uint32_t value)
    {
        if(!key(field, 5u)) return false;
        const uint8_t raw[4] = {
            static_cast<uint8_t>(value & 0xFFu),
            static_cast<uint8_t>((value >> 8) & 0xFFu),
            static_cast<uint8_t>((value >> 16) & 0xFFu),
            static_cast<uint8_t>((value >> 24) & 0xFFu)
        };
        return bytes(raw, sizeof(raw));
    }

    bool lengthField(uint32_t field, const uint8_t* src, size_t count)
    {
        return key(field, 2u) && varint(count) && bytes(src, count);
    }
};

size_t safeUtf8Prefix(const char* text, size_t maxBytes)
{
    if(text == nullptr) return 0;
    const size_t len = strlen(text);
    size_t n = len < maxBytes ? len : maxBytes;
    if(n == len) return n;
    while(n > 0 && (static_cast<uint8_t>(text[n]) & 0xC0u) == 0x80u) --n;
    return n;
}

void copyUtf8(char* dst, size_t dstSize, const uint8_t* src, size_t srcLength)
{
    if(dstSize == 0) return;
    size_t count = srcLength < dstSize - 1 ? srcLength : dstSize - 1;
    while(count > 0 && count < srcLength && (src[count] & 0xC0u) == 0x80u) --count;
    if(count > 0) memcpy(dst, src, count);
    dst[count] = '\0';
}

bool packetIdInUse(uint32_t id)
{
    if(id == 0 || id == FT02_BROADCAST_NODE) return true;
    for(size_t i = 0; i < FT02_PENDING_MAX; ++i)
    {
        if(g_pending[i].view.valid && g_pending[i].view.packetId == id) return true;
    }
    return false;
}

uint32_t generatePacketId()
{
    uint32_t id = esp_random();
    if(!packetIdInUse(id)) return id;
    do
    {
        id = ++g_packetCounter;
    } while(packetIdInUse(id));
    return id;
}

bool isDuplicate(uint32_t from, uint32_t id)
{
    if(from == 0 || id == 0) return false;
    for(size_t i = 0; i < FT02_DEDUP_MAX; ++i)
    {
        if(g_dedup[i].valid && g_dedup[i].from == from && g_dedup[i].id == id)
        {
            return true;
        }
    }
    g_dedup[g_dedupHead] = {true, from, id};
    g_dedupHead = (g_dedupHead + 1) % FT02_DEDUP_MAX;
    return false;
}

void pushInbox(const FT02LoRaMessageView& message)
{
    g_inbox[g_inboxHead].message = message;
    g_inboxHead = (g_inboxHead + 1) % FT02_INBOX_MAX;
    if(g_inboxCount < FT02_INBOX_MAX) ++g_inboxCount;
    if(g_unread < 999u) ++g_unread;
    ++g_rxTextCount;
    ++g_revision;
}

PendingTx* findPending(uint32_t packetId)
{
    if(packetId == 0) return nullptr;
    for(size_t i = 0; i < FT02_PENDING_MAX; ++i)
    {
        if(g_pending[i].view.valid && g_pending[i].view.packetId == packetId)
        {
            return &g_pending[i];
        }
    }
    return nullptr;
}

void registerTx(uint32_t packetId, uint32_t destination, bool broadcast, bool pki, const char* text)
{
    PendingTx& slot = g_pending[g_pendingHead];
    memset(&slot, 0, sizeof(slot));
    slot.view.valid = true;
    slot.view.packetId = packetId;
    slot.view.destination = destination;
    slot.view.broadcast = broadcast;
    slot.view.pkiEncrypted = pki;
    slot.view.state = FT02_LORA_TX_SENT;
    slot.view.sentAtMs = millis();
    const size_t previewBytes = safeUtf8Prefix(text, sizeof(slot.view.preview) - 1);
    memcpy(slot.view.preview, text, previewBytes);
    slot.view.preview[previewBytes] = '\0';
    g_pendingHead = (g_pendingHead + 1) % FT02_PENDING_MAX;
}

void resolveAck(uint32_t requestId, uint32_t error)
{
    PendingTx* pending = findPending(requestId);
    if(pending == nullptr) return;

    pending->view.completedAtMs = millis();
    pending->view.routingError = error;
    if(error == 0)
    {
        pending->view.state = FT02_LORA_TX_ACKED;
        ++g_ackCount;
        Serial.printf("[LORA-MSG] ACK packet=0x%08lX\n", static_cast<unsigned long>(requestId));
    }
    else
    {
        pending->view.state = FT02_LORA_TX_NAKED;
        ++g_nakCount;
        Serial.printf(
            "[LORA-MSG] NAK packet=0x%08lX error=%lu (%s)\n",
            static_cast<unsigned long>(requestId),
            static_cast<unsigned long>(error),
            FT02_LoRaCommunicationRoutingErrorText(error)
        );
    }
    resolveOutboxPacket(requestId, error);
    ++g_revision;
}

struct DecodedData
{
    uint32_t portnum = 0;
    const uint8_t* payload = nullptr;
    size_t payloadLength = 0;
    uint32_t requestId = 0;
};

bool parseData(const uint8_t* data, size_t length, DecodedData& out)
{
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return false;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 7u);
        if(field == 1u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return false;
            out.portnum = static_cast<uint32_t>(value);
        }
        else if(field == 2u && wire == 2u)
        {
            if(!readLength(data, length, offset, out.payload, out.payloadLength)) return false;
        }
        else if(field == 6u && wire == 5u)
        {
            if(!readFixed32(data, length, offset, out.requestId)) return false;
        }
        else if(!skipField(data, length, offset, wire))
        {
            return false;
        }
    }
    return true;
}

uint32_t parseRoutingError(const uint8_t* data, size_t length)
{
    uint32_t error = 0;
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return error;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 7u);
        if(field == 3u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return error;
            return static_cast<uint32_t>(value);
        }
        if(!skipField(data, length, offset, wire)) return error;
    }
    return error;
}

void parseQueueStatus(const uint8_t* data, size_t length)
{
    int32_t result = 0;
    uint32_t packetId = 0;
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 7u);
        if((field == 1u || field == 4u) && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            if(field == 1u) result = static_cast<int32_t>(static_cast<uint32_t>(value));
            else packetId = static_cast<uint32_t>(value);
        }
        else if(!skipField(data, length, offset, wire)) return;
    }
    if(packetId == 0 || result == 0) return;
    PendingTx* pending = findPending(packetId);
    if(pending == nullptr) return;
    pending->view.state = FT02_LORA_TX_FAILED;
    pending->view.completedAtMs = millis();
    pending->view.routingError = static_cast<uint32_t>(result);
    ++g_nakCount;
    resolveOutboxPacket(packetId, static_cast<uint32_t>(result));
    ++g_revision;
    Serial.printf(
        "[LORA-MSG] queue reject packet=0x%08lX result=%ld\n",
        static_cast<unsigned long>(packetId),
        static_cast<long>(result)
    );
}

void parseMeshPacket(const uint8_t* data, size_t length)
{
    uint32_t from = 0;
    uint32_t to = 0;
    uint32_t packetId = 0;
    uint32_t rxTime = 0;
    bool hasRxTime = false;
    bool hasSnr = false;
    float snr = 0.0f;
    bool hasRssi = false;
    int32_t rssi = 0;
    uint32_t hopLimit = 0;
    uint32_t hopStart = 0;
    bool pkiEncrypted = false;
    const uint8_t* decodedPtr = nullptr;
    size_t decodedLength = 0;

    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 7u);

        if((field == 1u || field == 2u || field == 6u || field == 7u) && wire == 5u)
        {
            uint32_t value = 0;
            if(!readFixed32(data, length, offset, value)) return;
            if(field == 1u) from = value;
            else if(field == 2u) to = value;
            else if(field == 6u) packetId = value;
            else { rxTime = value; hasRxTime = true; }
        }
        else if(field == 4u && wire == 2u)
        {
            if(!readLength(data, length, offset, decodedPtr, decodedLength)) return;
        }
        else if(field == 8u && wire == 5u)
        {
            uint32_t bits = 0;
            if(!readFixed32(data, length, offset, bits)) return;
            memcpy(&snr, &bits, sizeof(snr));
            hasSnr = true;
        }
        else if((field == 9u || field == 12u || field == 15u || field == 17u) && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            if(field == 9u) hopLimit = static_cast<uint32_t>(value);
            else if(field == 12u) { rssi = static_cast<int32_t>(static_cast<uint32_t>(value)); hasRssi = true; }
            else if(field == 15u) hopStart = static_cast<uint32_t>(value);
            else pkiEncrypted = value != 0;
        }
        else if(!skipField(data, length, offset, wire))
        {
            return;
        }
    }

    bool hasHops = hopStart > 0 && hopStart >= hopLimit;
    uint8_t hops = hasHops ? static_cast<uint8_t>(hopStart - hopLimit) : 0;
    if(from != 0)
    {
        FT02_LoRaNodeRuntimeUpdatePacketMetrics(from, hasSnr, snr, hasHops, hops, hasRxTime ? rxTime : 0);
    }
    if(decodedPtr == nullptr || decodedLength == 0) return;

    DecodedData decoded;
    if(!parseData(decodedPtr, decodedLength, decoded)) return;

    if(decoded.portnum == FT02_PORT_ROUTING_APP && decoded.requestId != 0)
    {
        const uint32_t error = decoded.payload != nullptr
            ? parseRoutingError(decoded.payload, decoded.payloadLength)
            : 0u;
        resolveAck(decoded.requestId, error);
        return;
    }

    if(decoded.portnum == FT02_PORT_NODEINFO_APP && from != 0 && decoded.payload != nullptr)
    {
        FT02_LoRaNodeRuntimeUpdateUserFromPacket(
            from,
            decoded.payload,
            decoded.payloadLength,
            hasSnr,
            snr,
            hasHops,
            hops,
            hasRxTime ? rxTime : 0
        );
        return;
    }

    if(decoded.portnum != FT02_PORT_TEXT_MESSAGE_APP || decoded.payload == nullptr) return;
    const uint32_t localNode = FT02_LoRaNodeRuntimeLocalNode();
    if(from == 0 || from == localNode) return; // don't create unread entries for our own TX echoes

    if(isDuplicate(from, packetId))
    {
        ++g_duplicateCount;
        ++g_revision;
        Serial.printf(
            "[LORA-MSG] duplicate from=!%08lX id=0x%08lX\n",
            static_cast<unsigned long>(from),
            static_cast<unsigned long>(packetId)
        );
        return;
    }

    FT02LoRaMessageView message = {};
    message.valid = true;
    message.from = from;
    message.to = to;
    message.packetId = packetId;
    message.broadcast = to == FT02_BROADCAST_NODE;
    message.pkiEncrypted = pkiEncrypted;
    copyUtf8(message.text, sizeof(message.text), decoded.payload, decoded.payloadLength);
    message.hasRxTime = hasRxTime;
    message.rxTimeEpoch = rxTime;
    message.hasRssi = hasRssi;
    message.rssi = rssi;
    message.hasSnr = hasSnr;
    message.snr = snr;
    message.hasHops = hasHops;
    message.hops = hops;

    g_lastRxPacketId = packetId;
    pushInbox(message);
    Serial.printf(
        "[LORA-MSG] RX from=!%08lX id=0x%08lX kind=%s pki=%u text=%s\n",
        static_cast<unsigned long>(from),
        static_cast<unsigned long>(packetId),
        message.broadcast ? "broadcast" : "direct",
        pkiEncrypted ? 1u : 0u,
        message.text
    );
}

bool makeTextPayload(const char* userText, bool attachFreshGnss, char* out, size_t outSize)
{
    if(userText == nullptr || out == nullptr || outSize == 0) return false;
    const size_t userBytes = safeUtf8Prefix(userText, FT02_LORA_USER_TEXT_MAX_BYTES);
    if(userBytes == 0) return false;
    if(userBytes >= outSize) return false;
    memcpy(out, userText, userBytes);
    out[userBytes] = '\0';

    if(!attachFreshGnss) return true;
    const FT02GnssSnapshot gnss = FT02_GnssSnapshotCurrent();
    const bool fresh = gnss.fixValid && gnss.hasPosition && gnss.lastFixAgeMs <= FT02_FRESH_GNSS_MS;
    const bool range = gnss.latitude >= -90.0 && gnss.latitude <= 90.0 &&
                       gnss.longitude >= -180.0 && gnss.longitude <= 180.0;
    const bool nonZero = fabs(gnss.latitude) > 0.0000001 || fabs(gnss.longitude) > 0.0000001;
    if(!fresh || !range || !nonZero) return true;

    char suffix[64];
    snprintf(suffix, sizeof(suffix), " [GPS:%.6f,%.6f]", gnss.latitude, gnss.longitude);
    const size_t used = strlen(out);
    const size_t suffixLen = strlen(suffix);
    if(used + suffixLen >= outSize) return true;
    memcpy(out + used, suffix, suffixLen + 1);
    return true;
}

bool sendText(uint32_t destination, bool direct, const uint8_t* publicKey, const char* userText, bool attachFreshGnss, uint32_t* outPacketId = nullptr)
{
    if(!FT02_LoRaNodeRuntimeReady() || !FT02_LoRaTransportLinkUp()) return false;

    char text[FT02_LORA_MESSAGE_TEXT_BYTES];
    if(!makeTextPayload(userText, attachFreshGnss, text, sizeof(text))) return false;
    const size_t textLen = strlen(text);

    uint8_t dataProto[FT02_PROTO_DATA_MAX];
    ProtoWriter dataWriter{dataProto, sizeof(dataProto), 0};
    if(!dataWriter.varintField(1u, FT02_PORT_TEXT_MESSAGE_APP) ||
       !dataWriter.lengthField(2u, reinterpret_cast<const uint8_t*>(text), textLen))
    {
        return false;
    }

    const uint32_t packetId = generatePacketId();
    uint8_t meshProto[FT02_PROTO_MESH_MAX];
    ProtoWriter meshWriter{meshProto, sizeof(meshProto), 0};
    if(!meshWriter.fixed32Field(2u, destination) ||
       !meshWriter.lengthField(4u, dataProto, dataWriter.length) ||
       !meshWriter.fixed32Field(6u, packetId) ||
       !meshWriter.varintField(9u, FT02_HOP_LIMIT) ||
       (direct && !meshWriter.varintField(10u, 1u)) ||
       !meshWriter.varintField(11u, FT02_PRIORITY_RELIABLE))
    {
        return false;
    }

    if(direct)
    {
        if(publicKey == nullptr) return false;
        if(!meshWriter.lengthField(16u, publicKey, 32u) ||
           !meshWriter.varintField(17u, 1u))
        {
            return false;
        }
    }

    uint8_t toRadio[FT02_PROTO_TORADIO_MAX];
    ProtoWriter toRadioWriter{toRadio, sizeof(toRadio), 0};
    if(!toRadioWriter.lengthField(1u, meshProto, meshWriter.length)) return false;

    if(!FT02_LoRaTransportSendToRadio(toRadio, toRadioWriter.length)) return false;

    registerTx(packetId, destination, !direct, direct, text);
    if(outPacketId != nullptr) *outPacketId = packetId;
    ++g_txCount;
    ++g_revision;
    Serial.printf(
        "[LORA-MSG] TX %s to=!%08lX id=0x%08lX pki=%u bytes=%u text=%s\n",
        direct ? "private" : "broadcast",
        static_cast<unsigned long>(destination),
        static_cast<unsigned long>(packetId),
        direct ? 1u : 0u,
        static_cast<unsigned>(textLen),
        text
    );
    return true;
}

uint32_t deliveryEpochNow()
{
    const time_t now = time(nullptr);
    if(now < static_cast<time_t>(1600000000)) return 0;
    if(static_cast<uint64_t>(now) > 0xFFFFFFFFULL) return 0xFFFFFFFFu;
    return static_cast<uint32_t>(now);
}

bool deliveryStateActive(FT02MessageDeliveryState state)
{
    return state == FT02_DELIVERY_QUEUED || state == FT02_DELIVERY_WAITING_ACK;
}

uint32_t deliveryTtlSeconds(FT02MessageDeliveryMode mode)
{
    if(mode == FT02_MESSAGE_IMMEDIATE) return FT02_IMMEDIATE_TTL_SEC;
    if(mode == FT02_MESSAGE_RELIABLE) return FT02_RELIABLE_TTL_SEC;
    return 0;
}

void markOutboxDirty()
{
    g_outboxDirty = true;
    g_outboxDirtyMs = millis();
    ++g_deliveryRevision;
    ++g_revision;
}

int findOutboxLogical(uint32_t logicalId)
{
    if(logicalId == 0) return -1;
    for(size_t i = 0; i < g_outboxCount; ++i)
        if(g_outbox[i].view.valid && g_outbox[i].view.logicalId == logicalId) return static_cast<int>(i);
    return -1;
}

int findOutboxPacket(uint32_t packetId)
{
    if(packetId == 0) return -1;
    for(size_t i = 0; i < g_outboxCount; ++i)
        if(g_outbox[i].view.valid && g_outbox[i].view.lastPacketId == packetId) return static_cast<int>(i);
    return -1;
}

void resolveOutboxPacket(uint32_t packetId, uint32_t error)
{
    const int index = findOutboxPacket(packetId);
    if(index < 0) return;
    OutboxSlot& slot = g_outbox[index];
    if(!slot.view.valid) return;

    if(error == 0)
    {
        slot.view.state = FT02_DELIVERY_DELIVERED;
        Serial.printf("[LORA-DELIVERY] delivered logical=0x%08lX packet=0x%08lX mode=%s\n",
                      static_cast<unsigned long>(slot.view.logicalId),
                      static_cast<unsigned long>(packetId),
                      FT02_LoRaMessageDeliveryModeText(slot.view.mode));
    }
    else if(slot.view.mode == FT02_MESSAGE_IMMEDIATE)
    {
        // Immediate messages are never application-layer retried after the
        // coprocessor accepted the transmission. A later routing NAK is simply
        // surfaced as a failed terminal state.
        slot.view.state = FT02_DELIVERY_FAILED;
    }
    else
    {
        // Reliable and persistent messages return to the queue. The scheduler
        // waits for the configured retry interval before the next attempt.
        slot.view.state = FT02_DELIVERY_QUEUED;
    }
    markOutboxDirty();
}

void outboxOnSessionReset()
{
    bool changed = false;
    for(size_t i = 0; i < g_outboxCount; ++i)
    {
        OutboxSlot& slot = g_outbox[i];
        if(!slot.view.valid || slot.view.state != FT02_DELIVERY_WAITING_ACK) continue;
        if(slot.view.mode == FT02_MESSAGE_IMMEDIATE)
            slot.view.state = FT02_DELIVERY_SENT;
        else
            slot.view.state = FT02_DELIVERY_QUEUED;
        changed = true;
    }
    if(changed) markOutboxDirty();
}

char hexDigit(uint8_t value)
{
    value &= 0x0Fu;
    return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + (value - 10));
}

int hexValue(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool encodeHexText(const char* text, char* out, size_t outSize)
{
    if(text == nullptr || out == nullptr) return false;
    const size_t n = strlen(text);
    if(n * 2u + 1u > outSize) return false;
    for(size_t i = 0; i < n; ++i)
    {
        const uint8_t b = static_cast<uint8_t>(text[i]);
        out[i * 2u] = hexDigit(static_cast<uint8_t>(b >> 4));
        out[i * 2u + 1u] = hexDigit(b);
    }
    out[n * 2u] = '\0';
    return true;
}

bool decodeHexText(const char* hex, char* out, size_t outSize)
{
    if(hex == nullptr || out == nullptr || outSize == 0) return false;
    const size_t n = strlen(hex);
    if((n & 1u) != 0 || n / 2u >= outSize) return false;
    for(size_t i = 0; i < n; i += 2u)
    {
        const int hi = hexValue(hex[i]);
        const int lo = hexValue(hex[i + 1u]);
        if(hi < 0 || lo < 0) return false;
        out[i / 2u] = static_cast<char>((hi << 4) | lo);
    }
    out[n / 2u] = '\0';
    return out[0] != '\0';
}

bool outboxExpired(const OutboxSlot& slot, uint32_t nowMs, uint32_t nowEpoch)
{
    if(slot.view.mode == FT02_MESSAGE_PERSISTENT) return false;
    const uint32_t ttl = deliveryTtlSeconds(slot.view.mode);
    if(slot.view.expiresEpoch != 0 && nowEpoch != 0)
        return nowEpoch >= slot.view.expiresEpoch;
    if(slot.createdMs != 0)
        return static_cast<uint32_t>(nowMs - slot.createdMs) >= ttl * 1000u;
    // A time-sensitive item restored without a trustworthy wall clock is safer
    // to expire than to replay stale operational information after a reboot.
    return true;
}

bool writeOutboxSnapshot()
{
    if(!FT02_StorageIsReady()) return false;
    (void)FT02_StorageDeleteFile(FT02_OUTBOX_TEMP);
    FILE* file = FT02_StorageOpenWriteFile(FT02_OUTBOX_TEMP, true);
    if(file == nullptr) return false;

    bool ok = fprintf(file, "%s\n", FT02_OUTBOX_HEADER) > 0;
    char hexText[FT02_LORA_MESSAGE_TEXT_BYTES * 2 + 1] = {};
    for(size_t i = 0; ok && i < g_outboxCount; ++i)
    {
        const FT02LoRaOutboxView& v = g_outbox[i].view;
        if(!v.valid) continue;
        if(!encodeHexText(v.text, hexText, sizeof(hexText))) { ok = false; break; }
        if(fprintf(file, "%lu\t%u\t%u\t%lu\t%u\t%lu\t%lu\t%u\t%lu\t%s\n",
                   static_cast<unsigned long>(v.logicalId),
                   static_cast<unsigned>(v.mode),
                   static_cast<unsigned>(v.state),
                   static_cast<unsigned long>(v.destination),
                   v.broadcast ? 1u : 0u,
                   static_cast<unsigned long>(v.createdEpoch),
                   static_cast<unsigned long>(v.expiresEpoch),
                   static_cast<unsigned>(v.attemptCount),
                   static_cast<unsigned long>(v.lastPacketId),
                   hexText) <= 0) ok = false;
    }
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(FT02_OUTBOX_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_OUTBOX_BACKUP);
    const bool had = FT02_StorageFileExists(FT02_OUTBOX_PATH);
    if(had && !FT02_StorageRenameFile(FT02_OUTBOX_PATH, FT02_OUTBOX_BACKUP))
    {
        (void)FT02_StorageDeleteFile(FT02_OUTBOX_TEMP);
        return false;
    }
    if(!FT02_StorageRenameFile(FT02_OUTBOX_TEMP, FT02_OUTBOX_PATH))
    {
        if(had) (void)FT02_StorageRenameFile(FT02_OUTBOX_BACKUP, FT02_OUTBOX_PATH);
        (void)FT02_StorageDeleteFile(FT02_OUTBOX_TEMP);
        return false;
    }
    (void)FT02_StorageDeleteFile(FT02_OUTBOX_BACKUP);
    g_outboxDirty = false;
    Serial.printf("[LORA-DELIVERY] outbox saved entries=%u\n", static_cast<unsigned>(g_outboxCount));
    return true;
}

bool appendLoadedOutbox(const FT02LoRaOutboxView& loaded)
{
    if(!loaded.valid || loaded.logicalId == 0 || loaded.text[0] == '\0') return false;
    if(findOutboxLogical(loaded.logicalId) >= 0) return true;
    if(g_outboxCount >= FT02_OUTBOX_MAX) return false;
    OutboxSlot& slot = g_outbox[g_outboxCount++];
    memset(&slot, 0, sizeof(slot));
    slot.view = loaded;
    slot.createdMs = 0;
    return true;
}

bool loadOutboxSnapshot()
{
    if(!FT02_StorageIsReady()) return false;
    if(!FT02_StorageFileExists(FT02_OUTBOX_PATH))
    {
        g_outboxLoaded = true;
        Serial.println("[LORA-DELIVERY] outbox file absent; starting fresh");
        return true;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_OUTBOX_PATH);
    if(file == nullptr) return false;
    char line[640] = {};
    bool headerSeen = false;
    const uint32_t nowEpoch = deliveryEpochNow();
    bool normalized = false;
    while(fgets(line, sizeof(line), file) != nullptr)
    {
        size_t len = strlen(line);
        while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if(line[0] == '\0') continue;
        if(line[0] == '#')
        {
            if(strcmp(line, FT02_OUTBOX_HEADER) == 0) headerSeen = true;
            continue;
        }

        char* save = nullptr;
        char* logicalText = strtok_r(line, "\t", &save);
        char* modeText = strtok_r(nullptr, "\t", &save);
        char* stateText = strtok_r(nullptr, "\t", &save);
        char* destText = strtok_r(nullptr, "\t", &save);
        char* broadcastText = strtok_r(nullptr, "\t", &save);
        char* createdText = strtok_r(nullptr, "\t", &save);
        char* expiresText = strtok_r(nullptr, "\t", &save);
        char* attemptsText = strtok_r(nullptr, "\t", &save);
        char* packetText = strtok_r(nullptr, "\t", &save);
        char* hexText = strtok_r(nullptr, "\t", &save);
        if(!logicalText || !modeText || !stateText || !destText || !broadcastText ||
           !createdText || !expiresText || !attemptsText || !packetText || !hexText) continue;

        FT02LoRaOutboxView v = {};
        v.valid = true;
        v.logicalId = static_cast<uint32_t>(strtoul(logicalText, nullptr, 10));
        const unsigned mode = static_cast<unsigned>(strtoul(modeText, nullptr, 10));
        const unsigned state = static_cast<unsigned>(strtoul(stateText, nullptr, 10));
        if(mode > static_cast<unsigned>(FT02_MESSAGE_PERSISTENT) ||
           state > static_cast<unsigned>(FT02_DELIVERY_CANCELED)) continue;
        v.mode = static_cast<FT02MessageDeliveryMode>(mode);
        v.state = static_cast<FT02MessageDeliveryState>(state);
        v.destination = static_cast<uint32_t>(strtoul(destText, nullptr, 10));
        v.broadcast = strtoul(broadcastText, nullptr, 10) != 0;
        v.createdEpoch = static_cast<uint32_t>(strtoul(createdText, nullptr, 10));
        v.expiresEpoch = static_cast<uint32_t>(strtoul(expiresText, nullptr, 10));
        v.attemptCount = static_cast<uint16_t>(strtoul(attemptsText, nullptr, 10));
        v.lastPacketId = static_cast<uint32_t>(strtoul(packetText, nullptr, 10));
        if(!decodeHexText(hexText, v.text, sizeof(v.text))) continue;

        if(v.state == FT02_DELIVERY_WAITING_ACK && v.mode != FT02_MESSAGE_IMMEDIATE)
        {
            v.state = FT02_DELIVERY_QUEUED;
            normalized = true;
        }
        if(deliveryStateActive(v.state) && v.mode != FT02_MESSAGE_PERSISTENT)
        {
            if(v.expiresEpoch == 0 || (nowEpoch != 0 && nowEpoch >= v.expiresEpoch))
            {
                v.state = FT02_DELIVERY_EXPIRED;
                normalized = true;
            }
        }
        (void)appendLoadedOutbox(v);
    }
    fclose(file);
    g_outboxLoaded = true;
    if(!headerSeen) Serial.println("[LORA-DELIVERY] outbox header missing/different; valid rows recovered");
    if(normalized) markOutboxDirty();
    Serial.printf("[LORA-DELIVERY] outbox loaded entries=%u\n", static_cast<unsigned>(g_outboxCount));
    return true;
}

uint32_t nextLogicalId()
{
    uint32_t id = 0;
    do { id = esp_random(); } while(id == 0 || findOutboxLogical(id) >= 0);
    return id;
}

bool ensureOutboxCapacity()
{
    if(g_outboxCount < FT02_OUTBOX_MAX) return true;
    // Never evict an active message. Remove the oldest terminal history item.
    size_t victim = FT02_OUTBOX_MAX;
    for(size_t i = 0; i < g_outboxCount; ++i)
    {
        if(!deliveryStateActive(g_outbox[i].view.state)) { victim = i; break; }
    }
    if(victim >= g_outboxCount) return false;
    for(size_t i = victim + 1; i < g_outboxCount; ++i) g_outbox[i - 1] = g_outbox[i];
    --g_outboxCount;
    memset(&g_outbox[g_outboxCount], 0, sizeof(g_outbox[g_outboxCount]));
    return true;
}

bool queuePreparedMessage(uint32_t destination, bool broadcast, const char* preparedText, FT02MessageDeliveryMode mode)
{
    if(preparedText == nullptr || preparedText[0] == '\0') return false;
    if(broadcast && mode != FT02_MESSAGE_IMMEDIATE) return false;
    if(!broadcast && (destination == 0 || destination == FT02_LoRaNodeRuntimeLocalNode())) return false;
    if(!ensureOutboxCapacity()) return false;

    OutboxSlot& slot = g_outbox[g_outboxCount++];
    memset(&slot, 0, sizeof(slot));
    slot.view.valid = true;
    slot.view.logicalId = nextLogicalId();
    slot.view.destination = destination;
    slot.view.broadcast = broadcast;
    slot.view.mode = mode;
    slot.view.state = FT02_DELIVERY_QUEUED;
    slot.view.createdEpoch = deliveryEpochNow();
    const uint32_t ttl = deliveryTtlSeconds(mode);
    if(ttl != 0 && slot.view.createdEpoch != 0)
        slot.view.expiresEpoch = slot.view.createdEpoch + ttl;
    slot.createdMs = millis();
    snprintf(slot.view.text, sizeof(slot.view.text), "%s", preparedText);
    markOutboxDirty();
    Serial.printf("[LORA-DELIVERY] queued logical=0x%08lX mode=%s to=!%08lX broadcast=%u\n",
                  static_cast<unsigned long>(slot.view.logicalId),
                  FT02_LoRaMessageDeliveryModeText(mode),
                  static_cast<unsigned long>(destination), broadcast ? 1u : 0u);
    return true;
}

uint32_t g_lastDeliverySubmitMs = 0;

bool trySubmitOutbox(OutboxSlot& slot, uint32_t nowMs)
{
    if(!slot.view.valid || !deliveryStateActive(slot.view.state)) return false;
    if(!FT02_LoRaTransportLinkUp() || !FT02_LoRaNodeRuntimeReady()) return false;
    if(g_lastDeliverySubmitMs != 0 && static_cast<uint32_t>(nowMs - g_lastDeliverySubmitMs) < 2500u) return false;

    if(slot.view.state == FT02_DELIVERY_WAITING_ACK)
    {
        const uint32_t retryMs = slot.view.mode == FT02_MESSAGE_PERSISTENT
            ? FT02_PERSISTENT_RETRY_MS : FT02_RELIABLE_RETRY_MS;
        if(static_cast<uint32_t>(nowMs - slot.view.lastAttemptMs) < retryMs) return false;
    }
    else if(slot.view.attemptCount > 0)
    {
        const uint32_t retryMs = slot.view.mode == FT02_MESSAGE_PERSISTENT
            ? FT02_PERSISTENT_RETRY_MS : FT02_RELIABLE_RETRY_MS;
        if(slot.view.mode != FT02_MESSAGE_IMMEDIATE &&
           static_cast<uint32_t>(nowMs - slot.view.lastAttemptMs) < retryMs) return false;
    }

    uint8_t key[32] = {};
    const uint8_t* keyPtr = nullptr;
    const bool direct = !slot.view.broadcast;
    if(direct)
    {
        if(!FT02_LoRaNodeRuntimeGetPublicKey(slot.view.destination, key)) return false;
        keyPtr = key;
    }

    uint32_t packetId = 0;
    if(!sendText(slot.view.destination, direct, keyPtr, slot.view.text, false, &packetId)) return false;
    slot.view.lastPacketId = packetId;
    slot.view.lastAttemptMs = nowMs;
    if(slot.view.attemptCount < 0xFFFFu) ++slot.view.attemptCount;
    g_lastDeliverySubmitMs = nowMs;
    slot.view.state = slot.view.mode == FT02_MESSAGE_IMMEDIATE
        ? FT02_DELIVERY_SENT : FT02_DELIVERY_WAITING_ACK;
    markOutboxDirty();
    Serial.printf("[LORA-DELIVERY] submit logical=0x%08lX packet=0x%08lX attempt=%u state=%s\n",
                  static_cast<unsigned long>(slot.view.logicalId),
                  static_cast<unsigned long>(packetId),
                  static_cast<unsigned>(slot.view.attemptCount),
                  FT02_LoRaMessageDeliveryStateText(slot.view.state));
    return true;
}

void pollOutboxDelivery()
{
    const uint32_t nowMs = millis();
    const uint32_t nowEpoch = deliveryEpochNow();
    bool expiryChanged = false;
    for(size_t i = 0; i < g_outboxCount; ++i)
    {
        OutboxSlot& slot = g_outbox[i];
        if(!slot.view.valid || !deliveryStateActive(slot.view.state)) continue;
        if(outboxExpired(slot, nowMs, nowEpoch))
        {
            slot.view.state = FT02_DELIVERY_EXPIRED;
            expiryChanged = true;
            Serial.printf("[LORA-DELIVERY] expired logical=0x%08lX mode=%s\n",
                          static_cast<unsigned long>(slot.view.logicalId),
                          FT02_LoRaMessageDeliveryModeText(slot.view.mode));
            continue;
        }
        if(trySubmitOutbox(slot, nowMs)) break; // respect Meshtastic client rate limit
    }
    if(expiryChanged) markOutboxDirty();
}
}

void FT02_LoRaCommunicationRuntimeOnFromRadio(const uint8_t* payload, uint16_t length)
{
    if(payload == nullptr || length == 0) return;
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(payload, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 7u);
        if((field == 2u || field == 11u) && wire == 2u)
        {
            const uint8_t* item = nullptr;
            size_t itemLength = 0;
            if(!readLength(payload, length, offset, item, itemLength)) return;
            if(field == 2u) parseMeshPacket(item, itemLength);
            else parseQueueStatus(item, itemLength);
        }
        else if(!skipField(payload, length, offset, wire))
        {
            return;
        }
    }
}

void FT02_LoRaCommunicationRuntimeResetSession()
{
    outboxOnSessionReset();
    // Keep inbox/history across radio resync. Only pending delivery state tied to
    // the old coprocessor session is invalidated.
    for(size_t i = 0; i < FT02_PENDING_MAX; ++i)
    {
        if(g_pending[i].view.valid && g_pending[i].view.state == FT02_LORA_TX_SENT)
        {
            g_pending[i].view.state = FT02_LORA_TX_FAILED;
            g_pending[i].view.completedAtMs = millis();
        }
    }
    ++g_revision;
}

bool FT02_LoRaCommunicationSendBroadcast(const char* userText, bool attachFreshGnss)
{
    return sendText(FT02_BROADCAST_NODE, false, nullptr, userText, attachFreshGnss);
}

bool FT02_LoRaCommunicationSendPrivate(uint32_t destination, const char* userText, bool attachFreshGnss)
{
    if(destination == 0 || destination == FT02_LoRaNodeRuntimeLocalNode()) return false;
    uint8_t key[32];
    if(!FT02_LoRaNodeRuntimeGetPublicKey(destination, key)) return false;
    return sendText(destination, true, key, userText, attachFreshGnss);
}

void FT02_LoRaMessageDeliveryBegin()
{
    if(g_outboxLoaded) return;
    if(!loadOutboxSnapshot())
    {
        g_outboxLoadRetryMs = millis();
        Serial.println("[LORA-DELIVERY] outbox load deferred: storage unavailable");
    }
}

void FT02_LoRaMessageDeliveryPoll()
{
    const uint32_t now = millis();
    if(!g_outboxLoaded)
    {
        if(FT02_StorageIsReady() && static_cast<uint32_t>(now - g_outboxLoadRetryMs) >= 5000u)
        {
            g_outboxLoadRetryMs = now;
            (void)loadOutboxSnapshot();
        }
    }

    pollOutboxDelivery();
    if(g_outboxDirty && FT02_StorageIsReady() &&
       static_cast<uint32_t>(now - g_outboxDirtyMs) >= FT02_OUTBOX_FLUSH_IDLE_MS)
    {
        (void)writeOutboxSnapshot();
    }
}

bool FT02_LoRaMessageQueuePrivate(
    uint32_t destination,
    const char* userText,
    bool attachFreshGnss,
    FT02MessageDeliveryMode mode)
{
    if(mode > FT02_MESSAGE_PERSISTENT) return false;
    char prepared[FT02_LORA_MESSAGE_TEXT_BYTES] = {};
    if(!makeTextPayload(userText, attachFreshGnss, prepared, sizeof(prepared))) return false;
    if(!queuePreparedMessage(destination, false, prepared, mode)) return false;
    pollOutboxDelivery();
    return true;
}

bool FT02_LoRaMessageQueueBroadcast(const char* userText, bool attachFreshGnss)
{
    char prepared[FT02_LORA_MESSAGE_TEXT_BYTES] = {};
    if(!makeTextPayload(userText, attachFreshGnss, prepared, sizeof(prepared))) return false;
    if(!queuePreparedMessage(FT02_BROADCAST_NODE, true, prepared, FT02_MESSAGE_IMMEDIATE)) return false;
    pollOutboxDelivery();
    return true;
}

size_t FT02_LoRaMessageOutboxCount()
{
    return g_outboxCount;
}

bool FT02_LoRaMessageGetOutboxNewest(size_t newestIndex, FT02LoRaOutboxView& out)
{
    memset(&out, 0, sizeof(out));
    if(newestIndex >= g_outboxCount) return false;
    const size_t index = g_outboxCount - 1u - newestIndex;
    if(!g_outbox[index].view.valid) return false;
    out = g_outbox[index].view;
    return true;
}

bool FT02_LoRaMessageCancel(uint32_t logicalId)
{
    const int index = findOutboxLogical(logicalId);
    if(index < 0) return false;
    OutboxSlot& slot = g_outbox[index];
    if(!deliveryStateActive(slot.view.state)) return false;
    slot.view.state = FT02_DELIVERY_CANCELED;
    markOutboxDirty();
    Serial.printf("[LORA-DELIVERY] canceled logical=0x%08lX\n", static_cast<unsigned long>(logicalId));
    return true;
}

uint32_t FT02_LoRaMessageDeliveryRevision()
{
    return g_deliveryRevision;
}

const char* FT02_LoRaMessageDeliveryModeText(FT02MessageDeliveryMode mode)
{
    switch(mode)
    {
        case FT02_MESSAGE_IMMEDIATE: return "即时";
        case FT02_MESSAGE_RELIABLE: return "可靠";
        case FT02_MESSAGE_PERSISTENT: return "持久";
        default: return "未知";
    }
}

const char* FT02_LoRaMessageDeliveryStateText(FT02MessageDeliveryState state)
{
    switch(state)
    {
        case FT02_DELIVERY_QUEUED: return "等待发送";
        case FT02_DELIVERY_WAITING_ACK: return "等待确认";
        case FT02_DELIVERY_SENT: return "已发送";
        case FT02_DELIVERY_DELIVERED: return "已送达";
        case FT02_DELIVERY_EXPIRED: return "已过期";
        case FT02_DELIVERY_FAILED: return "发送失败";
        case FT02_DELIVERY_CANCELED: return "已撤销";
        default: return "--";
    }
}

size_t FT02_LoRaCommunicationMessageCount()
{
    return g_inboxCount;
}

bool FT02_LoRaCommunicationGetMessageNewest(size_t newestIndex, FT02LoRaMessageView& out)
{
    memset(&out, 0, sizeof(out));
    if(newestIndex >= g_inboxCount) return false;
    const size_t index = (g_inboxHead + FT02_INBOX_MAX - 1 - newestIndex) % FT02_INBOX_MAX;
    out = g_inbox[index].message;
    return out.valid;
}

uint16_t FT02_LoRaCommunicationUnreadCount()
{
    return g_unread;
}

void FT02_LoRaCommunicationMarkAllRead()
{
    if(g_unread == 0) return;
    g_unread = 0;
    ++g_revision;
}

uint32_t FT02_LoRaCommunicationRevision() { return g_revision; }
uint32_t FT02_LoRaCommunicationRxTextCount() { return g_rxTextCount; }
uint32_t FT02_LoRaCommunicationTxCount() { return g_txCount; }
uint32_t FT02_LoRaCommunicationDuplicateCount() { return g_duplicateCount; }
uint32_t FT02_LoRaCommunicationAckCount() { return g_ackCount; }
uint32_t FT02_LoRaCommunicationNakCount() { return g_nakCount; }
uint32_t FT02_LoRaCommunicationLastRxPacketId() { return g_lastRxPacketId; }

bool FT02_LoRaCommunicationGetLastTx(FT02LoRaTxStatusView& out)
{
    memset(&out, 0, sizeof(out));
    for(size_t n = 0; n < FT02_PENDING_MAX; ++n)
    {
        const size_t index = (g_pendingHead + FT02_PENDING_MAX - 1 - n) % FT02_PENDING_MAX;
        if(g_pending[index].view.valid)
        {
            out = g_pending[index].view;
            return true;
        }
    }
    return false;
}

const char* FT02_LoRaCommunicationTxStateText(FT02LoRaTxState state)
{
    switch(state)
    {
        case FT02_LORA_TX_SENT: return "等待ACK";
        case FT02_LORA_TX_ACKED: return "已送达";
        case FT02_LORA_TX_NAKED: return "发送失败";
        case FT02_LORA_TX_FAILED: return "会话中断";
        default: return "--";
    }
}

const char* FT02_LoRaCommunicationRoutingErrorText(uint32_t error)
{
    switch(error)
    {
        case 0: return "NONE";
        case 1: return "NO_ROUTE";
        case 2: return "GOT_NAK";
        case 3: return "TIMEOUT";
        case 4: return "NO_INTERFACE";
        case 5: return "MAX_RETRANSMIT";
        case 6: return "NO_CHANNEL";
        case 7: return "TOO_LARGE";
        case 8: return "NO_RESPONSE";
        case 9: return "DUTY_CYCLE_LIMIT";
        case 32: return "BAD_REQUEST";
        case 33: return "NOT_AUTHORIZED";
        case 34: return "PKI_FAILED";
        case 35: return "PKI_UNKNOWN_PUBKEY";
        case 38: return "RATE_LIMIT_EXCEEDED";
        case 39: return "PKI_SEND_FAIL_PUBLIC_KEY";
        default: return "ROUTING_ERROR";
    }
}
