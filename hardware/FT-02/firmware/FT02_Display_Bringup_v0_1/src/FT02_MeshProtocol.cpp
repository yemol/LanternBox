#include "FT02_MeshProtocol.h"
#include <string.h>

namespace {

uint32_t readLe32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool readVarint(const uint8_t *data, size_t len, size_t &pos, uint32_t &value) {
    value = 0;
    uint8_t shift = 0;
    while (pos < len && shift <= 28) {
        const uint8_t b = data[pos++];
        value |= static_cast<uint32_t>(b & 0x7Fu) << shift;
        if ((b & 0x80u) == 0u) return true;
        shift = static_cast<uint8_t>(shift + 7u);
    }
    return false;
}

bool appendByte(uint8_t *out, size_t &pos, size_t maxLen, uint8_t value) {
    if (!out || pos >= maxLen) return false;
    out[pos++] = value;
    return true;
}

bool appendVarintRaw(uint8_t *out, size_t &pos, size_t maxLen, uint32_t value) {
    while (value >= 0x80u) {
        if (!appendByte(out, pos, maxLen, static_cast<uint8_t>((value & 0x7Fu) | 0x80u))) return false;
        value >>= 7u;
    }
    return appendByte(out, pos, maxLen, static_cast<uint8_t>(value));
}

bool appendTag(uint8_t *out, size_t &pos, size_t maxLen, uint8_t fieldNo, uint8_t wireType) {
    return appendVarintRaw(out, pos, maxLen, (static_cast<uint32_t>(fieldNo) << 3u) | wireType);
}

bool appendVarintField(uint8_t *out, size_t &pos, size_t maxLen, uint8_t fieldNo, uint32_t value) {
    return appendTag(out, pos, maxLen, fieldNo, 0u) && appendVarintRaw(out, pos, maxLen, value);
}

bool appendFixed32Field(uint8_t *out, size_t &pos, size_t maxLen, uint8_t fieldNo, uint32_t value) {
    if (!appendTag(out, pos, maxLen, fieldNo, 5u) || pos + 4u > maxLen) return false;
    out[pos++] = static_cast<uint8_t>(value & 0xFFu);
    out[pos++] = static_cast<uint8_t>((value >> 8u) & 0xFFu);
    out[pos++] = static_cast<uint8_t>((value >> 16u) & 0xFFu);
    out[pos++] = static_cast<uint8_t>((value >> 24u) & 0xFFu);
    return true;
}

bool appendBytesField(uint8_t *out, size_t &pos, size_t maxLen, uint8_t fieldNo,
                      const uint8_t *data, size_t dataLen) {
    if (!data && dataLen != 0u) return false;
    if (!appendTag(out, pos, maxLen, fieldNo, 2u)) return false;
    if (!appendVarintRaw(out, pos, maxLen, static_cast<uint32_t>(dataLen))) return false;
    if (dataLen > maxLen - pos) return false;
    if (dataLen) memcpy(out + pos, data, dataLen);
    pos += dataLen;
    return true;
}

bool appendStringField(uint8_t *out, size_t &pos, size_t maxLen, uint8_t fieldNo, const char *value) {
    if (!value) return false;
    return appendBytesField(out, pos, maxLen, fieldNo, reinterpret_cast<const uint8_t*>(value), strlen(value));
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

uint32_t parseNodeNumberFromUserId(const char *id) {
    if (!id || strlen(id) != 9u || id[0] != '!') return 0;
    uint32_t value = 0;
    for (size_t i = 1; i < 9; ++i) {
        const int n = hexNibble(id[i]);
        if (n < 0) return 0;
        value = (value << 4u) | static_cast<uint32_t>(n);
    }
    return value;
}

void copyProtoString(char *out, size_t outSize, const uint8_t *data, size_t len) {
    if (!out || outSize == 0u) return;
    size_t w = 0;
    for (size_t i = 0; i < len && w + 1u < outSize; ++i) {
        const uint8_t c = data[i];
        if (c == 0u || c == '\r' || c == '\n' || c == '\t') continue;
        if (c >= 0x20u) out[w++] = static_cast<char>(c);
    }
    out[w] = '\0';
}

bool skipField(const uint8_t *data, size_t len, size_t &pos, uint8_t wireType) {
    if (wireType == 0u) {
        uint32_t ignored = 0;
        return readVarint(data, len, pos, ignored);
    }
    if (wireType == 1u) {
        if (pos + 8u > len) return false;
        pos += 8u;
        return true;
    }
    if (wireType == 2u) {
        uint32_t fieldLen = 0;
        if (!readVarint(data, len, pos, fieldLen)) return false;
        if (fieldLen > len - pos) return false;
        pos += fieldLen;
        return true;
    }
    if (wireType == 5u) {
        if (pos + 4u > len) return false;
        pos += 4u;
        return true;
    }
    return false;
}

} // namespace

bool FT02_MeshParseHeader(const uint8_t *data, size_t len, FT02MeshHeader &out) {
    if (!data || len < FT02_MESH_HEADER_LEN) return false;
    out.to = readLe32(data + 0);
    out.from = readLe32(data + 4);
    out.id = readLe32(data + 8);
    out.flags = data[12];
    out.channel = data[13];
    out.nextHop = data[14];
    out.relayNode = data[15];
    return true;
}

bool FT02_MeshParseDataProto(const uint8_t *data, size_t len, FT02MeshDataView &out) {
    out.portnum = 0;
    out.payload = nullptr;
    out.payloadLen = 0;
    out.wantResponse = false;
    out.requestId = 0;
    if (!data || len == 0) return false;

    size_t pos = 0;
    while (pos < len) {
        uint32_t tag = 0;
        if (!readVarint(data, len, pos, tag)) return false;
        const uint32_t fieldNo = tag >> 3;
        const uint8_t wireType = static_cast<uint8_t>(tag & 0x07u);

        if (fieldNo == 1u && wireType == 0u) {
            if (!readVarint(data, len, pos, out.portnum)) return false;
        } else if (fieldNo == 2u && wireType == 2u) {
            uint32_t fieldLen = 0;
            if (!readVarint(data, len, pos, fieldLen)) return false;
            if (fieldLen > len - pos) return false;
            out.payload = data + pos;
            out.payloadLen = fieldLen;
            pos += fieldLen;
        } else if (fieldNo == 3u && wireType == 0u) {
            uint32_t value = 0;
            if (!readVarint(data, len, pos, value)) return false;
            out.wantResponse = value != 0u;
        } else if (fieldNo == 6u && wireType == 5u) {
            if (pos + 4u > len) return false;
            out.requestId = readLe32(data + pos);
            pos += 4u;
        } else if (!skipField(data, len, pos, wireType)) {
            return false;
        }
    }
    return out.portnum != 0u;
}

bool FT02_MeshBuildDataProto(uint32_t portnum,
                             const uint8_t *payload,
                             size_t payloadLen,
                             bool wantResponse,
                             uint32_t requestId,
                             uint8_t *out,
                             size_t maxLen,
                             size_t &outLen) {
    outLen = 0;
    if (!out || maxLen == 0u || portnum == 0u) return false;
    if (!appendVarintField(out, outLen, maxLen, 1u, portnum)) return false;
    if (!appendBytesField(out, outLen, maxLen, 2u, payload, payloadLen)) return false;
    if (wantResponse && !appendVarintField(out, outLen, maxLen, 3u, 1u)) return false;
    if (requestId != 0u && !appendFixed32Field(out, outLen, maxLen, 6u, requestId)) return false;
    return true;
}


bool FT02_MeshParseUserProto(const uint8_t *data, size_t len, FT02MeshUserView &out) {
    memset(&out, 0, sizeof(out));
    if (!data || len == 0u) return false;

    size_t pos = 0;
    bool any = false;
    while (pos < len) {
        uint32_t tag = 0;
        if (!readVarint(data, len, pos, tag)) return false;
        const uint32_t fieldNo = tag >> 3u;
        const uint8_t wireType = static_cast<uint8_t>(tag & 0x07u);
        if (wireType == 2u) {
            uint32_t fieldLen = 0;
            if (!readVarint(data, len, pos, fieldLen) || fieldLen > len - pos) return false;
            if (fieldNo == 1u) { copyProtoString(out.id, sizeof(out.id), data + pos, fieldLen); any = true; }
            else if (fieldNo == 2u) { copyProtoString(out.longName, sizeof(out.longName), data + pos, fieldLen); any = true; }
            else if (fieldNo == 3u) { copyProtoString(out.shortName, sizeof(out.shortName), data + pos, fieldLen); any = true; }
            else if (fieldNo == 8u && fieldLen == 32u) {
                memcpy(out.publicKey, data + pos, 32u);
                uint8_t combined = 0;
                for (size_t i = 0; i < 32u; ++i) combined |= out.publicKey[i];
                out.hasPublicKey = combined != 0u;
                any = true;
            }
            pos += fieldLen;
        } else if (wireType == 0u) {
            uint32_t value = 0;
            if (!readVarint(data, len, pos, value)) return false;
            if (fieldNo == 9u) {
                out.hasIsUnmessagable = true;
                out.isUnmessagable = value != 0u;
                any = true;
            }
        } else if (!skipField(data, len, pos, wireType)) {
            return false;
        }
    }
    out.nodeNumber = parseNodeNumberFromUserId(out.id);
    return any;
}

bool FT02_MeshBuildUserProto(const uint8_t publicKey[32],
                             uint8_t *out, size_t maxLen, size_t &outLen) {
    outLen = 0;
    if (!out || maxLen == 0u) return false;
    if (!appendStringField(out, outLen, maxLen, 1u, "!4c423002")) return false;
    if (!appendStringField(out, outLen, maxLen, 2u, "LanternBox FT02")) return false;
    if (!appendStringField(out, outLen, maxLen, 3u, "FT02")) return false;
    // Names are valid NodeInfo even if PKI initialization is temporarily unavailable.
    // When PKI is ready, field 8 carries the current 32-byte Curve25519 public key.
    if (publicKey && !appendBytesField(out, outLen, maxLen, 8u, publicKey, 32u)) return false;
    // Meshtastic User field 9: is_unmessagable=false.
    if (!appendVarintField(out, outLen, maxLen, 9u, 0u)) return false;
    return true;
}

bool FT02_MeshParseRoutingError(const uint8_t *data, size_t len, uint32_t &error) {
    error = 0xFFFFFFFFu;
    if (!data || len == 0u) return false;
    size_t pos = 0;
    while (pos < len) {
        uint32_t tag = 0;
        if (!readVarint(data, len, pos, tag)) return false;
        const uint32_t fieldNo = tag >> 3u;
        const uint8_t wireType = static_cast<uint8_t>(tag & 0x07u);
        if (fieldNo == 3u && wireType == 0u) {
            return readVarint(data, len, pos, error);
        }
        if (!skipField(data, len, pos, wireType)) return false;
    }
    return false;
}

const char *FT02_MeshPortName(uint32_t portnum) {
    switch (portnum) {
        case FT02_MESH_PORT_TEXT_MESSAGE_APP: return "TEXT_MESSAGE_APP";
        case FT02_MESH_PORT_NODEINFO_APP: return "NODEINFO_APP";
        case FT02_MESH_PORT_ROUTING_APP: return "ROUTING_APP";
        case FT02_MESH_PORT_LANTERNBOX_VOICE: return "LANTERNBOX_VOICE";
        default: return "OTHER";
    }
}
