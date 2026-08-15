#include "FT02_LoRaCommunicationRuntime.h"

#include <esp_system.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

InboxSlot g_inbox[FT02_INBOX_MAX] = {};
size_t g_inboxHead = 0; // next write
size_t g_inboxCount = 0;
uint16_t g_unread = 0;

DedupKey g_dedup[FT02_DEDUP_MAX] = {};
size_t g_dedupHead = 0;
PendingTx g_pending[FT02_PENDING_MAX] = {};
size_t g_pendingHead = 0;

uint32_t g_revision = 0;
uint32_t g_rxTextCount = 0;
uint32_t g_txCount = 0;
uint32_t g_duplicateCount = 0;
uint32_t g_ackCount = 0;
uint32_t g_nakCount = 0;
uint32_t g_lastRxPacketId = 0;
uint32_t g_packetCounter = 0x20000001u;

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

bool sendText(uint32_t destination, bool direct, const uint8_t* publicKey, const char* userText, bool attachFreshGnss)
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
