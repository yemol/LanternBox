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
#include "FT02_LR01HostRuntime.h"

namespace
{
constexpr uint32_t FT02_BROADCAST_NODE = 0xFFFFFFFFu;
constexpr uint32_t FT02_FRESH_GNSS_MS = 15000u;
constexpr size_t FT02_INBOX_MAX = 50;
constexpr size_t FT02_DEDUP_MAX = 32;
constexpr size_t FT02_PENDING_MAX = 8;
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

size_t safeUtf8Prefix(const char* text, size_t maxBytes)
{
    if(text == nullptr) return 0;
    const size_t len = strlen(text);
    size_t n = len < maxBytes ? len : maxBytes;
    if(n == len) return n;
    while(n > 0 && (static_cast<uint8_t>(text[n]) & 0xC0u) == 0x80u) --n;
    return n;
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
    slot.view.state = FT02_LORA_TX_NONE;
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
    (void)publicKey; // LR01 owns PKI/public-key material in the new architecture.
    if(!FT02_LR01HostRadioReady()) return false;

    char text[FT02_LORA_MESSAGE_TEXT_BYTES];
    if(!makeTextPayload(userText, attachFreshGnss, text, sizeof(text))) return false;

    // LR01 Host A2 accepts the frozen protocol payload limit, while Core intentionally limits user
    // traffic to 120 UTF-8 bytes. Truncate at a valid UTF-8 boundary after any
    // optional GPS suffix has been appended.
    const size_t limited = safeUtf8Prefix(text, FT02_LORA_USER_TEXT_MAX_BYTES);
    text[limited] = '\0';
    if(limited == 0) return false;

    const uint32_t packetId = generatePacketId();
    const bool submitted = direct
        ? FT02_LR01HostSendPrivate(packetId, destination, text)
        : FT02_LR01HostSendBroadcast(packetId, text);
    if(!submitted) return false;

    registerTx(packetId, destination, !direct, direct, text);
    if(outPacketId != nullptr) *outPacketId = packetId;
    ++g_txCount;
    ++g_revision;
    Serial.printf(
        "[LORA-MSG] LR01 submit %s to=!%08lX local_id=0x%08lX bytes=%u text=%s\n",
        direct ? "private" : "broadcast",
        static_cast<unsigned long>(destination),
        static_cast<unsigned long>(packetId),
        static_cast<unsigned>(limited),
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
        // LR01 accepted the Host transmission. A later routing NAK is simply
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
    if(!FT02_LR01HostRadioReady()) return false;
    if(g_lastDeliverySubmitMs != 0 && static_cast<uint32_t>(nowMs - g_lastDeliverySubmitMs) < 2500u) return false;

    if(slot.view.state == FT02_DELIVERY_WAITING_ACK)
    {
        // A2 keeps exactly one Core outbox attempt waiting for the correlated
        // LR01 delivery ACK/TIMEOUT. Do not submit another attempt until that
        // lifecycle completes.
        return false;
    }
    else if(slot.view.attemptCount > 0)
    {
        const uint32_t retryMs = slot.view.mode == FT02_MESSAGE_PERSISTENT
            ? FT02_PERSISTENT_RETRY_MS : FT02_RELIABLE_RETRY_MS;
        if(slot.view.mode != FT02_MESSAGE_IMMEDIATE &&
           static_cast<uint32_t>(nowMs - slot.view.lastAttemptMs) < retryMs) return false;
    }

    const bool direct = !slot.view.broadcast;
    uint32_t packetId = 0;
    if(!sendText(slot.view.destination, direct, nullptr, slot.view.text, false, &packetId)) return false;
    slot.view.lastPacketId = packetId;
    slot.view.lastAttemptMs = nowMs;
    if(slot.view.attemptCount < 0xFFFFu) ++slot.view.attemptCount;
    g_lastDeliverySubmitMs = nowMs;
    // LR01 Host A2 correlates this packetId as the Core lifecycle id. Private
    // reliable/persistent items remain WAITING_ACK until MESH_DELIVERY ACK or
    // TIMEOUT; TIMEOUT returns retryable modes to QUEUED.
    slot.view.state = FT02_DELIVERY_WAITING_ACK;
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
    return sendText(destination, true, nullptr, userText, attachFreshGnss);
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

bool FT02_LoRaMessageClearOutbox()
{
    if(g_outboxCount == 0) return true;

    memset(g_outbox, 0, sizeof(g_outbox));
    g_outboxHead = 0;
    g_outboxCount = 0;
    markOutboxDirty();

    Serial.println("[LORA-DELIVERY] outbox cleared");
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

void FT02_LoRaCommunicationHostReceive(uint32_t airPacketId,
                                       uint32_t from,
                                       uint32_t to,
                                       const char* senderName,
                                       bool pkiEncrypted,
                                       float rssi,
                                       float snr,
                                       const char* text)
{
    (void)senderName;
    if(from == 0 || text == nullptr || text[0] == '\0') return;

    const uint32_t packetId = airPacketId != 0 ? airPacketId : generatePacketId();
    if(isDuplicate(from, packetId))
    {
        ++g_duplicateCount;
        ++g_revision;
        Serial.printf("[LORA-MSG] LR01 duplicate from=!%08lX id=0x%08lX dropped\n",
                      static_cast<unsigned long>(from),
                      static_cast<unsigned long>(packetId));
        return;
    }

    FT02LoRaMessageView message = {};
    message.valid = true;
    message.from = from;
    message.to = to != 0 ? to : (pkiEncrypted ? FT02_LoRaNodeRuntimeLocalNode() : FT02_BROADCAST_NODE);
    message.packetId = packetId;
    message.broadcast = message.to == FT02_BROADCAST_NODE;
    message.pkiEncrypted = pkiEncrypted;
    const size_t bytes = safeUtf8Prefix(text, sizeof(message.text) - 1);
    memcpy(message.text, text, bytes);
    message.text[bytes] = '\0';
    message.hasRxTime = false;
    message.hasRssi = true;
    message.rssi = static_cast<int32_t>(lroundf(rssi));
    message.hasSnr = true;
    message.snr = snr;
    message.hasHops = false;
    pushInbox(message);
    g_lastRxPacketId = message.packetId;
    Serial.printf("[LORA-MSG] LR01 RX %s from=!%08lX to=!%08lX air_id=0x%08lX rssi=%ld snr=%.1f text=%s\n",
                  message.broadcast ? "broadcast" : "private",
                  static_cast<unsigned long>(from),
                  static_cast<unsigned long>(message.to),
                  static_cast<unsigned long>(message.packetId),
                  static_cast<long>(message.rssi),
                  static_cast<double>(snr),
                  message.text);
}

void FT02_LoRaCommunicationHostTxAccepted(uint32_t hostId)
{
    PendingTx* pending = findPending(hostId);
    if(pending == nullptr) return;
    // ACCEPTED confirms LR01 ownership of the request but not RF transmission.
    ++g_revision;
}

void FT02_LoRaCommunicationHostTxSent(uint32_t hostId)
{
    PendingTx* pending = findPending(hostId);
    if(pending != nullptr)
    {
        pending->view.state = FT02_LORA_TX_SENT;
        if(pending->view.sentAtMs == 0) pending->view.sentAtMs = millis();
    }

    const int outboxIndex = findOutboxPacket(hostId);
    if(outboxIndex >= 0)
    {
        OutboxSlot& slot = g_outbox[outboxIndex];
        if(slot.view.valid && slot.view.broadcast)
        {
            slot.view.state = FT02_DELIVERY_SENT;
            markOutboxDirty();
        }
        // Private reliable/persistent messages intentionally remain WAITING_ACK
        // until LR01 reports MESH_DELIVERY ACK/TIMEOUT.
    }
    ++g_revision;
}

void FT02_LoRaCommunicationHostTxFailed(uint32_t hostId, const char* reason)
{
    PendingTx* pending = findPending(hostId);
    if(pending != nullptr)
    {
        pending->view.completedAtMs = millis();
        pending->view.state = FT02_LORA_TX_FAILED;
        pending->view.routingError = 8; // NO_RESPONSE/general host failure mapping for existing UI.
    }
    resolveOutboxPacket(hostId, 8);
    ++g_nakCount;
    ++g_revision;
    Serial.printf("[LORA-MSG] LR01 TX failed id=0x%08lX reason=%s\n",
                  static_cast<unsigned long>(hostId), reason != nullptr ? reason : "");
}

void FT02_LoRaCommunicationHostTxResult(uint32_t hostId, const char* type, bool ok, const char* reason)
{
    (void)type;
    // A2 SENT and DELIVERY carry the authoritative lifecycle. RESULT remains a
    // compatibility summary; only a negative result is terminal here.
    if(!ok) FT02_LoRaCommunicationHostTxFailed(hostId, reason);
}

void FT02_LoRaCommunicationHostDelivery(uint32_t hostId, uint32_t node, bool ack)
{
    (void)node;
    if(hostId == 0) return;
    if(ack)
    {
        resolveAck(hostId, 0);
        resolveOutboxPacket(hostId, 0);
    }
    else
    {
        resolveAck(hostId, 3); // TIMEOUT
        resolveOutboxPacket(hostId, 3);
    }
    ++g_revision;
}

void FT02_LoRaCommunicationHostTxResult(const char* type, bool ok)
{
    const bool wantsPrivate = type != nullptr && strcmp(type, "PRIVATE") == 0;
    const bool wantsBroadcast = type != nullptr && strcmp(type, "TEXT") == 0;

    // A1 fallback only: resolve newest local TX of the same class because there
    // is no host lifecycle id in the legacy line.
    for(size_t n = 0; n < FT02_PENDING_MAX; ++n)
    {
        const size_t index = (g_pendingHead + FT02_PENDING_MAX - 1 - n) % FT02_PENDING_MAX;
        PendingTx& pending = g_pending[index];
        if(!pending.view.valid) continue;
        if(wantsPrivate && pending.view.broadcast) continue;
        if(wantsBroadcast && !pending.view.broadcast) continue;
        if(pending.view.completedAtMs != 0) continue;
        pending.view.completedAtMs = millis();
        pending.view.state = ok ? FT02_LORA_TX_SENT : FT02_LORA_TX_FAILED;
        if(ok) ++g_ackCount; else ++g_nakCount;
        break;
    }
    ++g_revision;
}
