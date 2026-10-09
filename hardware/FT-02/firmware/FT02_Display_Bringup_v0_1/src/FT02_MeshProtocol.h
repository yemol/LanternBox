#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr size_t FT02_MESH_HEADER_LEN = 16;
static constexpr uint8_t FT02_MESH_CHANNEL_HASH = 0xA4;
static constexpr uint8_t FT02_MESH_PORT_TEXT_MESSAGE_APP = 1;
static constexpr uint8_t FT02_MESH_PORT_NODEINFO_APP = 4;
static constexpr uint8_t FT02_MESH_PORT_ROUTING_APP = 5;
static constexpr uint32_t FT02_MESH_BROADCAST_NODE = 0xFFFFFFFFu;
static constexpr uint32_t FT02_MESH_LOCAL_NODE = 0x4C423002u; // !4c423002 / LanternBox FT02
static constexpr uint8_t FT02_MESH_HOP_LIMIT = 3;
static constexpr uint8_t FT02_MESH_HOP_START = 3;
static constexpr uint8_t FT02_MESH_FLAG_WANT_ACK = 0x08;

struct FT02MeshHeader {
    uint32_t to;
    uint32_t from;
    uint32_t id;
    uint8_t flags;
    uint8_t channel;
    uint8_t nextHop;
    uint8_t relayNode;
};


struct FT02MeshUserView {
    char id[10];
    char longName[25];
    char shortName[13];
    uint32_t nodeNumber;
    bool hasPublicKey;
    uint8_t publicKey[32];
    bool hasIsUnmessagable;
    bool isUnmessagable;
};

bool FT02_MeshParseUserProto(const uint8_t *data, size_t len, FT02MeshUserView &out);
bool FT02_MeshBuildUserProto(const uint8_t publicKey[32],
                             uint8_t *out, size_t maxLen, size_t &outLen);
bool FT02_MeshParseRoutingError(const uint8_t *data, size_t len, uint32_t &error);

struct FT02MeshDataView {
    uint32_t portnum;
    const uint8_t *payload;
    size_t payloadLen;
    bool wantResponse;
    uint32_t requestId;
};

bool FT02_MeshParseHeader(const uint8_t *data, size_t len, FT02MeshHeader &out);
bool FT02_MeshParseDataProto(const uint8_t *data, size_t len, FT02MeshDataView &out);
bool FT02_MeshBuildDataProto(uint8_t portnum,
                             const uint8_t *payload,
                             size_t payloadLen,
                             bool wantResponse,
                             uint32_t requestId,
                             uint8_t *out,
                             size_t maxLen,
                             size_t &outLen);
const char *FT02_MeshPortName(uint32_t portnum);
