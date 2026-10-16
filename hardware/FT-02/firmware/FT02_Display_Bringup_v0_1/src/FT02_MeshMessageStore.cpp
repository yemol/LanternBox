#include "FT02_MeshMessageStore.h"

#include "FT02_Log.h"
#include "FT02_Storage.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

namespace
{
constexpr const char* STORE_PATH = "/lanternbox/mesh/messages_v1.dat";
constexpr const char* STORE_TEMP = "/lanternbox/mesh/messages_v1.tmp";
constexpr const char* STORE_BACKUP = "/lanternbox/mesh/messages_v1.bak";
constexpr uint16_t STORE_VERSION = 1u;
constexpr uint32_t STORE_FLUSH_IDLE_MS = 1500u;
constexpr uint32_t STORE_LOAD_RETRY_MS = 5000u;
constexpr uint8_t FLAG_PKI = 0x01u;
constexpr uint8_t FLAG_UNREAD = 0x02u;
constexpr uint8_t FLAG_TIME = 0x04u;
constexpr uint8_t FLAG_RSSI = 0x08u;
constexpr uint8_t FLAG_SNR = 0x10u;
constexpr uint8_t FLAG_HOPS = 0x20u;

#pragma pack(push, 1)
struct DiskHeader
{
    char magic[4];
    uint16_t version;
    uint16_t recordSize;
    uint16_t count;
    uint16_t reserved;
    uint32_t nextRecordId;
    uint32_t recordsCrc32;
};

struct DiskRecord
{
    uint32_t recordId;
    uint32_t packetId;
    uint32_t logicalId;
    uint32_t from;
    uint32_t to;
    uint32_t timeEpoch;
    uint32_t voiceDurationMs;
    int16_t rssi;
    int16_t snrX10;
    uint8_t type;
    uint8_t direction;
    uint8_t scope;
    uint8_t status;
    uint8_t flags;
    uint8_t hops;
    uint8_t reserved[2];
    char text[FT02_MESH_HISTORY_TEXT_BYTES];
    char voicePath[FT02_MESH_HISTORY_VOICE_PATH_BYTES];
};
#pragma pack(pop)

DiskRecord* g_records = nullptr;
size_t g_count = 0u;
uint32_t g_nextRecordId = 1u;
uint16_t g_unread = 0u;
uint32_t g_revision = 0u;
bool g_loaded = false;
bool g_dirty = false;
uint32_t g_dirtyMs = 0u;
uint32_t g_lastLoadAttemptMs = 0u;


constexpr size_t recordBufferBytes()
{
    return FT02_MESH_HISTORY_LIMIT * sizeof(DiskRecord);
}

bool ensureRecordBuffer()
{
    if(g_records != nullptr) return true;
#if defined(ARDUINO_ARCH_ESP32)
    g_records = static_cast<DiskRecord*>(heap_caps_calloc(
        FT02_MESH_HISTORY_LIMIT, sizeof(DiskRecord), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    g_records = static_cast<DiskRecord*>(calloc(FT02_MESH_HISTORY_LIMIT, sizeof(DiskRecord)));
#endif
    if(g_records == nullptr)
    {
        FT02_LOGE("[ERROR][MESH-HISTORY] PSRAM record buffer alloc failed bytes=%u\n",
                  static_cast<unsigned>(recordBufferBytes()));
        return false;
    }
#if defined(ARDUINO_ARCH_ESP32)
    FT02_LOGI("[MESH-HISTORY] PSRAM record buffer ready bytes=%u free_psram=%u\n",
              static_cast<unsigned>(recordBufferBytes()),
              static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
#else
    FT02_LOGI("[MESH-HISTORY] host record buffer ready bytes=%u\n",
              static_cast<unsigned>(recordBufferBytes()));
#endif
    return true;
}

uint32_t crc32(const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for(size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

uint32_t validEpochNow()
{
    const time_t now = time(nullptr);
    if(now < static_cast<time_t>(1600000000)) return 0u;
    if(static_cast<uint64_t>(now) > 0xFFFFFFFFULL) return 0xFFFFFFFFu;
    return static_cast<uint32_t>(now);
}

size_t safeUtf8Prefix(const char* text, size_t maxBytes)
{
    if(text == nullptr) return 0u;
    const size_t len = strlen(text);
    size_t n = len < maxBytes ? len : maxBytes;
    if(n == len) return n;
    while(n > 0u && (static_cast<uint8_t>(text[n]) & 0xC0u) == 0x80u) --n;
    return n;
}

void copyText(char* out, size_t outSize, const char* text)
{
    if(out == nullptr || outSize == 0u) return;
    out[0] = '\0';
    if(text == nullptr) return;
    const size_t n = safeUtf8Prefix(text, outSize - 1u);
    memcpy(out, text, n);
    out[n] = '\0';
}

void copyPath(char* out, size_t outSize, const char* path)
{
    if(out == nullptr || outSize == 0u) return;
    snprintf(out, outSize, "%s", path != nullptr ? path : "");
}

void markDirty()
{
    g_dirty = true;
    g_dirtyMs = millis();
    ++g_revision;
}

void recountUnread()
{
    g_unread = 0u;
    for(size_t i = 0; i < g_count; ++i)
    {
        if((g_records[i].flags & FLAG_UNREAD) != 0u && g_unread < 999u) ++g_unread;
    }
}

bool validDiskRecord(const DiskRecord& r)
{
    if(r.recordId == 0u) return false;
    if(r.type > FT02_MESH_MESSAGE_VOICE) return false;
    if(r.direction > FT02_MESH_MESSAGE_TX) return false;
    if(r.scope > FT02_MESH_SCOPE_DIRECT) return false;
    if(r.status > FT02_MESH_STATUS_INCOMPLETE) return false;
    if(r.text[sizeof(r.text) - 1u] != '\0') return false;
    if(r.voicePath[sizeof(r.voicePath) - 1u] != '\0') return false;
    return true;
}

bool loadFromPath(const char* path)
{
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;
    DiskHeader header{};
    bool ok = fread(&header, 1, sizeof(header), file) == sizeof(header);
    ok = ok && memcmp(header.magic, "LMSG", 4u) == 0;
    ok = ok && header.version == STORE_VERSION;
    ok = ok && header.recordSize == sizeof(DiskRecord);
    ok = ok && header.count <= FT02_MESH_HISTORY_LIMIT;

    // Read directly into the static history buffer. A 100-record temporary
    // array is ~33 KiB and would be unsafe on the ESP32-S3 task stack. g_count
    // is committed only after CRC/schema validation succeeds.
    memset(g_records, 0, recordBufferBytes());
    if(ok && header.count > 0u)
    {
        ok = fread(g_records, sizeof(DiskRecord), header.count, file) == header.count;
        if(ok)
        {
            const uint32_t actual = crc32(reinterpret_cast<const uint8_t*>(g_records),
                                          static_cast<size_t>(header.count) * sizeof(DiskRecord));
            ok = actual == header.recordsCrc32;
        }
    }
    fclose(file);
    if(!ok) return false;

    for(size_t i = 0; i < header.count; ++i)
        if(!validDiskRecord(g_records[i])) return false;

    g_count = header.count;
    g_nextRecordId = header.nextRecordId == 0u ? 1u : header.nextRecordId;
    recountUnread();
    return true;
}

bool loadSnapshot()
{
    if(!FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;
    if(!FT02_StorageFileExists(STORE_PATH))
    {
        // Interrupted atomic replace can leave TEMP and/or BACKUP but no main.
        // Both files carry a full CRC-protected snapshot, so recover before
        // ever creating a new empty history. Prefer TEMP because it is the
        // newest fully-synced candidate.
        if(FT02_StorageFileExists(STORE_TEMP) && loadFromPath(STORE_TEMP))
        {
            g_loaded = true;
            markDirty();
            FT02_LOGW("[WARN][MESH-HISTORY] recovered interrupted temp snapshot count=%u\n",
                      static_cast<unsigned>(g_count));
            return true;
        }
        if(FT02_StorageFileExists(STORE_BACKUP) && loadFromPath(STORE_BACKUP))
        {
            g_loaded = true;
            markDirty();
            FT02_LOGW("[WARN][MESH-HISTORY] recovered interrupted backup snapshot count=%u\n",
                      static_cast<unsigned>(g_count));
            return true;
        }
        memset(g_records, 0, recordBufferBytes());
        g_count = 0u;
        g_nextRecordId = 1u;
        g_unread = 0u;
        g_loaded = true;
        FT02_LOGI("[MESH-HISTORY] new empty store path=%s limit=%u\n",
                  STORE_PATH, static_cast<unsigned>(FT02_MESH_HISTORY_LIMIT));
        return true;
    }

    if(loadFromPath(STORE_PATH))
    {
        g_loaded = true;
        FT02_LOGI("[MESH-HISTORY] loaded count=%u unread=%u\n",
                  static_cast<unsigned>(g_count), static_cast<unsigned>(g_unread));
        return true;
    }

    FT02_LOGW("[WARN][MESH-HISTORY] primary snapshot invalid; trying recovery snapshots\n");
    if(FT02_StorageFileExists(STORE_TEMP) && loadFromPath(STORE_TEMP))
    {
        g_loaded = true;
        markDirty();
        FT02_LOGW("[WARN][MESH-HISTORY] recovered temp count=%u\n",
                  static_cast<unsigned>(g_count));
        return true;
    }
    if(FT02_StorageFileExists(STORE_BACKUP) && loadFromPath(STORE_BACKUP))
    {
        g_loaded = true;
        markDirty(); // heal the primary snapshot on the next poll.
        FT02_LOGW("[WARN][MESH-HISTORY] recovered backup count=%u\n",
                  static_cast<unsigned>(g_count));
        return true;
    }
    return false;
}

bool writeSnapshot()
{
    if(!g_loaded || !g_dirty || !FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;

    DiskHeader header{};
    memcpy(header.magic, "LMSG", 4u);
    header.version = STORE_VERSION;
    header.recordSize = sizeof(DiskRecord);
    header.count = static_cast<uint16_t>(g_count);
    header.nextRecordId = g_nextRecordId;
    header.recordsCrc32 = crc32(reinterpret_cast<const uint8_t*>(g_records), g_count * sizeof(DiskRecord));

    (void)FT02_StorageDeleteFile(STORE_TEMP);
    FILE* file = FT02_StorageOpenWriteFile(STORE_TEMP, true);
    if(file == nullptr) return false;
    bool ok = fwrite(&header, 1, sizeof(header), file) == sizeof(header);
    if(ok && g_count > 0u)
        ok = fwrite(g_records, sizeof(DiskRecord), g_count, file) == g_count;
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(STORE_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(STORE_BACKUP);
    const bool had = FT02_StorageFileExists(STORE_PATH);
    if(had && !FT02_StorageRenameFile(STORE_PATH, STORE_BACKUP))
    {
        (void)FT02_StorageDeleteFile(STORE_TEMP);
        return false;
    }
    if(!FT02_StorageRenameFile(STORE_TEMP, STORE_PATH))
    {
        if(had) (void)FT02_StorageRenameFile(STORE_BACKUP, STORE_PATH);
        (void)FT02_StorageDeleteFile(STORE_TEMP);
        return false;
    }
    (void)FT02_StorageDeleteFile(STORE_BACKUP);
    g_dirty = false;
    return true;
}

bool ensureLoadedForMutation()
{
    if(!ensureRecordBuffer()) return false;
    if(g_loaded) return true;
    if(loadSnapshot()) return true;
    // Keep messages usable even if SD is absent. When storage returns this RAM
    // timeline becomes the source of truth and is flushed as a fresh snapshot.
    memset(g_records, 0, recordBufferBytes());
    g_count = 0u;
    g_nextRecordId = 1u;
    g_unread = 0u;
    g_loaded = true;
    g_dirty = true;
    g_dirtyMs = millis();
    return true;
}

DiskRecord* appendRecord()
{
    if(!ensureLoadedForMutation()) return nullptr;
    if(g_count >= FT02_MESH_HISTORY_LIMIT)
    {
        // History owns received/transmitted voice files. When the oldest
        // voice record ages out, remove its payload too so the 100-message
        // limit also bounds SD growth.
        if(g_records[0].type == FT02_MESH_MESSAGE_VOICE && g_records[0].voicePath[0] != '\0' &&
           FT02_StorageIsReady() && !FT02_StorageSleepSuspended())
        {
            (void)FT02_StorageDeleteFile(g_records[0].voicePath);
        }
        memmove(&g_records[0], &g_records[1], (FT02_MESH_HISTORY_LIMIT - 1u) * sizeof(DiskRecord));
        g_count = FT02_MESH_HISTORY_LIMIT - 1u;
    }
    DiskRecord& record = g_records[g_count++];
    memset(&record, 0, sizeof(record));
    record.recordId = g_nextRecordId++;
    if(g_nextRecordId == 0u) g_nextRecordId = 1u;
    return &record;
}

DiskRecord* findLogical(uint32_t logicalId)
{
    if(logicalId == 0u) return nullptr;
    for(size_t n = 0; n < g_count; ++n)
    {
        const size_t i = g_count - 1u - n;
        if(g_records[i].direction == FT02_MESH_MESSAGE_TX && g_records[i].logicalId == logicalId)
            return &g_records[i];
    }
    return nullptr;
}

void fillCommon(DiskRecord& r,
                uint32_t from,
                uint32_t to,
                bool pkiEncrypted,
                FT02MeshMessageType type,
                FT02MeshMessageDirection direction,
                FT02MeshMessageStatus status,
                uint32_t epoch)
{
    r.from = from;
    r.to = to;
    r.type = static_cast<uint8_t>(type);
    r.direction = static_cast<uint8_t>(direction);
    r.scope = static_cast<uint8_t>(to == 0xFFFFFFFFu ? FT02_MESH_SCOPE_BROADCAST : FT02_MESH_SCOPE_DIRECT);
    r.status = static_cast<uint8_t>(status);
    if(pkiEncrypted) r.flags |= FLAG_PKI;
    if(direction == FT02_MESH_MESSAGE_RX) r.flags |= FLAG_UNREAD;
    r.timeEpoch = epoch != 0u ? epoch : validEpochNow();
    if(r.timeEpoch != 0u) r.flags |= FLAG_TIME;
}

void toView(const DiskRecord& r, FT02MeshMessageView& out)
{
    memset(&out, 0, sizeof(out));
    out.valid = true;
    out.recordId = r.recordId;
    out.packetId = r.packetId;
    out.logicalId = r.logicalId;
    out.from = r.from;
    out.to = r.to;
    out.type = static_cast<FT02MeshMessageType>(r.type);
    out.direction = static_cast<FT02MeshMessageDirection>(r.direction);
    out.scope = static_cast<FT02MeshMessageScope>(r.scope);
    out.status = static_cast<FT02MeshMessageStatus>(r.status);
    out.pkiEncrypted = (r.flags & FLAG_PKI) != 0u;
    out.unread = (r.flags & FLAG_UNREAD) != 0u;
    out.hasTime = (r.flags & FLAG_TIME) != 0u;
    out.timeEpoch = r.timeEpoch;
    out.hasRssi = (r.flags & FLAG_RSSI) != 0u;
    out.rssi = r.rssi;
    out.hasSnr = (r.flags & FLAG_SNR) != 0u;
    out.snr = static_cast<float>(r.snrX10) / 10.0f;
    out.hasHops = (r.flags & FLAG_HOPS) != 0u;
    out.hops = r.hops;
    out.voiceDurationMs = r.voiceDurationMs;
    memcpy(out.text, r.text, sizeof(out.text));
    memcpy(out.voicePath, r.voicePath, sizeof(out.voicePath));
}
}

void FT02_MeshMessageStoreBegin()
{
    g_lastLoadAttemptMs = millis();
    if(!ensureRecordBuffer()) return;
    (void)loadSnapshot();
}

void FT02_MeshMessageStorePoll()
{
    const uint32_t now = millis();
    if(g_records == nullptr)
    {
        if(!ensureRecordBuffer()) return;
    }
    if(!g_loaded)
    {
        if(static_cast<uint32_t>(now - g_lastLoadAttemptMs) >= STORE_LOAD_RETRY_MS)
        {
            g_lastLoadAttemptMs = now;
            (void)loadSnapshot();
        }
        return;
    }
    if(g_dirty && static_cast<uint32_t>(now - g_dirtyMs) >= STORE_FLUSH_IDLE_MS)
    {
        if(!writeSnapshot())
            FT02_LOGW("[WARN][MESH-HISTORY] snapshot flush deferred/failed\n");
    }
}

size_t FT02_MeshMessageStoreCount()
{
    return (g_records != nullptr && g_loaded) ? g_count : 0u;
}

bool FT02_MeshMessageStoreGetNewest(size_t newestIndex, FT02MeshMessageView& out)
{
    memset(&out, 0, sizeof(out));
    if(g_records == nullptr || !g_loaded || newestIndex >= g_count) return false;
    toView(g_records[g_count - 1u - newestIndex], out);
    return true;
}

uint16_t FT02_MeshMessageStoreUnreadCount()
{
    return (g_records != nullptr && g_loaded) ? g_unread : 0u;
}

void FT02_MeshMessageStoreMarkAllRead()
{
    if(g_records == nullptr || !g_loaded || g_unread == 0u) return;
    for(size_t i = 0; i < g_count; ++i) g_records[i].flags &= static_cast<uint8_t>(~FLAG_UNREAD);
    g_unread = 0u;
    markDirty();
}

uint32_t FT02_MeshMessageStoreRevision()
{
    return g_revision;
}

bool FT02_MeshMessageStoreAppendRxText(uint32_t packetId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       bool hasRssi,
                                       int16_t rssi,
                                       bool hasSnr,
                                       float snr,
                                       bool hasHops,
                                       uint8_t hops)
{
    if(from == 0u || text == nullptr || text[0] == '\0') return false;
    DiskRecord* r = appendRecord();
    if(r == nullptr) return false;
    fillCommon(*r, from, to, pkiEncrypted, FT02_MESH_MESSAGE_TEXT,
               FT02_MESH_MESSAGE_RX, FT02_MESH_STATUS_RECEIVED, 0u);
    r->packetId = packetId;
    copyText(r->text, sizeof(r->text), text);
    if(hasRssi) { r->flags |= FLAG_RSSI; r->rssi = rssi; }
    if(hasSnr) { r->flags |= FLAG_SNR; r->snrX10 = static_cast<int16_t>(lroundf(snr * 10.0f)); }
    if(hasHops) { r->flags |= FLAG_HOPS; r->hops = hops; }
    recountUnread();
    markDirty();
    return true;
}

bool FT02_MeshMessageStoreAppendTxText(uint32_t logicalId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       FT02MeshMessageStatus status,
                                       uint32_t timeEpoch)
{
    if(logicalId == 0u || from == 0u || text == nullptr || text[0] == '\0') return false;
    DiskRecord* r = appendRecord();
    if(r == nullptr) return false;
    fillCommon(*r, from, to, pkiEncrypted, FT02_MESH_MESSAGE_TEXT,
               FT02_MESH_MESSAGE_TX, status, timeEpoch);
    r->logicalId = logicalId;
    copyText(r->text, sizeof(r->text), text);
    markDirty();
    return true;
}

bool FT02_MeshMessageStoreEnsureTxText(uint32_t logicalId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       FT02MeshMessageStatus status,
                                       uint32_t timeEpoch)
{
    if(!ensureLoadedForMutation()) return false;
    if(findLogical(logicalId) != nullptr) return true;
    return FT02_MeshMessageStoreAppendTxText(logicalId, from, to, pkiEncrypted, text, status, timeEpoch);
}

bool FT02_MeshMessageStoreUpdateTx(uint32_t logicalId,
                                   uint32_t packetId,
                                   FT02MeshMessageStatus status)
{
    if(!ensureLoadedForMutation()) return false;
    DiskRecord* r = findLogical(logicalId);
    if(r == nullptr) return false;
    bool changed = false;
    if(packetId != 0u && r->packetId != packetId) { r->packetId = packetId; changed = true; }
    if(r->status != static_cast<uint8_t>(status)) { r->status = static_cast<uint8_t>(status); changed = true; }
    if(changed) markDirty();
    return true;
}

bool FT02_MeshMessageStoreAppendRxVoice(uint32_t packetId,
                                        uint32_t from,
                                        uint32_t to,
                                        bool pkiEncrypted,
                                        uint32_t durationMs,
                                        const char* voicePath,
                                        FT02MeshMessageStatus status)
{
    if(from == 0u || voicePath == nullptr || voicePath[0] == '\0') return false;
    DiskRecord* r = appendRecord();
    if(r == nullptr) return false;
    fillCommon(*r, from, to, pkiEncrypted, FT02_MESH_MESSAGE_VOICE,
               FT02_MESH_MESSAGE_RX, status, 0u);
    r->packetId = packetId;
    r->voiceDurationMs = durationMs;
    copyPath(r->voicePath, sizeof(r->voicePath), voicePath);
    recountUnread();
    markDirty();
    return true;
}

bool FT02_MeshMessageStoreAppendTxVoice(uint32_t logicalId,
                                        uint32_t from,
                                        uint32_t to,
                                        bool pkiEncrypted,
                                        uint32_t durationMs,
                                        const char* voicePath,
                                        FT02MeshMessageStatus status,
                                        uint32_t timeEpoch)
{
    if(logicalId == 0u || from == 0u || voicePath == nullptr || voicePath[0] == '\0') return false;
    DiskRecord* r = appendRecord();
    if(r == nullptr) return false;
    fillCommon(*r, from, to, pkiEncrypted, FT02_MESH_MESSAGE_VOICE,
               FT02_MESH_MESSAGE_TX, status, timeEpoch);
    r->logicalId = logicalId;
    r->voiceDurationMs = durationMs;
    copyPath(r->voicePath, sizeof(r->voicePath), voicePath);
    markDirty();
    return true;
}

const char* FT02_MeshMessageStatusText(FT02MeshMessageStatus status)
{
    switch(status)
    {
        case FT02_MESH_STATUS_RECEIVED: return "已收到";
        case FT02_MESH_STATUS_QUEUED: return "排队中";
        case FT02_MESH_STATUS_WAITING_ACK: return "等待确认";
        case FT02_MESH_STATUS_SENT: return "已发送";
        case FT02_MESH_STATUS_DELIVERED: return "已送达";
        case FT02_MESH_STATUS_FAILED: return "失败";
        case FT02_MESH_STATUS_EXPIRED: return "已过期";
        case FT02_MESH_STATUS_CANCELED: return "已撤销";
        case FT02_MESH_STATUS_RECEIVING: return "接收中";
        case FT02_MESH_STATUS_INCOMPLETE: return "不完整";
        default: return "--";
    }
}

const char* FT02_MeshMessageStorePath()
{
    return STORE_PATH;
}
