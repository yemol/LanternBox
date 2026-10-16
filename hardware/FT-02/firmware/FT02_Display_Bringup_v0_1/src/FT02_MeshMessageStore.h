#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr size_t FT02_MESH_HISTORY_LIMIT = 100u;
static constexpr size_t FT02_MESH_HISTORY_TEXT_BYTES = 192u;
static constexpr size_t FT02_MESH_HISTORY_VOICE_PATH_BYTES = 96u;

// Unified FT-02 Mesh history model. Text and voice share one chronological
// timeline so UI/storage/Core sync do not need parallel message systems.
enum FT02MeshMessageType : uint8_t
{
    FT02_MESH_MESSAGE_TEXT = 0,
    FT02_MESH_MESSAGE_VOICE = 1
};

enum FT02MeshMessageDirection : uint8_t
{
    FT02_MESH_MESSAGE_RX = 0,
    FT02_MESH_MESSAGE_TX = 1
};

enum FT02MeshMessageScope : uint8_t
{
    FT02_MESH_SCOPE_BROADCAST = 0,
    FT02_MESH_SCOPE_DIRECT = 1
};

enum FT02MeshMessageStatus : uint8_t
{
    FT02_MESH_STATUS_RECEIVED = 0,
    FT02_MESH_STATUS_QUEUED,
    FT02_MESH_STATUS_WAITING_ACK,
    FT02_MESH_STATUS_SENT,
    FT02_MESH_STATUS_DELIVERED,
    FT02_MESH_STATUS_FAILED,
    FT02_MESH_STATUS_EXPIRED,
    FT02_MESH_STATUS_CANCELED,
    FT02_MESH_STATUS_RECEIVING,
    FT02_MESH_STATUS_INCOMPLETE
};

struct FT02MeshMessageView
{
    bool valid;
    uint32_t recordId;
    uint32_t packetId;
    uint32_t logicalId;
    uint32_t from;
    uint32_t to;
    FT02MeshMessageType type;
    FT02MeshMessageDirection direction;
    FT02MeshMessageScope scope;
    FT02MeshMessageStatus status;
    bool pkiEncrypted;
    bool unread;
    bool hasTime;
    uint32_t timeEpoch;
    bool hasRssi;
    int16_t rssi;
    bool hasSnr;
    float snr;
    bool hasHops;
    uint8_t hops;
    uint32_t voiceDurationMs;
    char text[FT02_MESH_HISTORY_TEXT_BYTES];
    char voicePath[FT02_MESH_HISTORY_VOICE_PATH_BYTES];
};

void FT02_MeshMessageStoreBegin();
void FT02_MeshMessageStorePoll();

size_t FT02_MeshMessageStoreCount();
bool FT02_MeshMessageStoreGetNewest(size_t newestIndex, FT02MeshMessageView& out);
uint16_t FT02_MeshMessageStoreUnreadCount();
void FT02_MeshMessageStoreMarkAllRead();
uint32_t FT02_MeshMessageStoreRevision();

bool FT02_MeshMessageStoreAppendRxText(uint32_t packetId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       bool hasRssi,
                                       int16_t rssi,
                                       bool hasSnr,
                                       float snr,
                                       bool hasHops = false,
                                       uint8_t hops = 0u);

bool FT02_MeshMessageStoreAppendTxText(uint32_t logicalId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       FT02MeshMessageStatus status,
                                       uint32_t timeEpoch = 0u);

// Idempotent migration helper for outbox entries created before the unified
// history layer was introduced.
bool FT02_MeshMessageStoreEnsureTxText(uint32_t logicalId,
                                       uint32_t from,
                                       uint32_t to,
                                       bool pkiEncrypted,
                                       const char* text,
                                       FT02MeshMessageStatus status,
                                       uint32_t timeEpoch = 0u);

bool FT02_MeshMessageStoreUpdateTx(uint32_t logicalId,
                                   uint32_t packetId,
                                   FT02MeshMessageStatus status);

// Voice hooks are part of the A1 storage contract now. A later transport step
// will feed Codec2 files into these without changing the history/UI schema.
bool FT02_MeshMessageStoreAppendRxVoice(uint32_t packetId,
                                        uint32_t from,
                                        uint32_t to,
                                        bool pkiEncrypted,
                                        uint32_t durationMs,
                                        const char* voicePath,
                                        FT02MeshMessageStatus status = FT02_MESH_STATUS_RECEIVED);

bool FT02_MeshMessageStoreAppendTxVoice(uint32_t logicalId,
                                        uint32_t from,
                                        uint32_t to,
                                        bool pkiEncrypted,
                                        uint32_t durationMs,
                                        const char* voicePath,
                                        FT02MeshMessageStatus status,
                                        uint32_t timeEpoch = 0u);

const char* FT02_MeshMessageStatusText(FT02MeshMessageStatus status);
const char* FT02_MeshMessageStorePath();
