#include "FT02_LoRaNodeRuntime.h"

#include <string.h>
#include <stdio.h>

namespace
{
constexpr uint32_t FT02_MT_CONFIG_ID = 0x4C420266u;
constexpr size_t FT02_NODE_MAX = 64;

struct NodeSlot
{
    bool valid;
    uint32_t node;
    char longName[25];
    char shortName[13];
    bool pkiAvailable;
    bool publicKeyValid;
    uint8_t publicKey[32];
    bool favorite;
    bool hasHops;
    uint8_t hops;
    float snr;
    uint32_t lastHeardEpoch;
};

NodeSlot g_nodes[FT02_NODE_MAX] = {};
size_t g_nodeCount = 0;
uint32_t g_localNode = 0;
uint32_t g_expectedNodeCount = 0;
uint32_t g_revision = 0;
bool g_ready = false;
bool g_configCompleteSeen = false;

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

bool readLength(
    const uint8_t* data,
    size_t length,
    size_t& offset,
    const uint8_t*& ptr,
    size_t& itemLength
)
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
        case 0:
            return readVarint(data, length, offset, value);
        case 1:
            if(offset + 8 > length) return false;
            offset += 8;
            return true;
        case 2:
            return readLength(data, length, offset, ptr, itemLength);
        case 5:
            if(offset + 4 > length) return false;
            offset += 4;
            return true;
        default:
            return false;
    }
}

void copyUtf8(char* dst, size_t dstSize, const uint8_t* src, size_t srcLength)
{
    if(dstSize == 0) return;
    size_t count = srcLength < dstSize - 1 ? srcLength : dstSize - 1;

    // Avoid ending on a UTF-8 continuation byte after truncation.
    while(count > 0 && count < srcLength && (src[count] & 0xC0u) == 0x80u)
    {
        --count;
    }

    if(count > 0) memcpy(dst, src, count);
    dst[count] = '\0';
}

int findNode(uint32_t node)
{
    for(size_t i = 0; i < g_nodeCount; ++i)
    {
        if(g_nodes[i].valid && g_nodes[i].node == node)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int ensureNode(uint32_t node)
{
    const int found = findNode(node);
    if(found >= 0) return found;
    if(node == 0 || g_nodeCount >= FT02_NODE_MAX) return -1;

    const size_t index = g_nodeCount++;
    memset(&g_nodes[index], 0, sizeof(g_nodes[index]));
    g_nodes[index].valid = true;
    g_nodes[index].node = node;
    ++g_revision;
    return static_cast<int>(index);
}

void parseUser(const uint8_t* data, size_t length, NodeSlot& node)
{
    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 0x07u);

        if((field == 2u || field == 3u || field == 8u) && wire == 2u)
        {
            const uint8_t* ptr = nullptr;
            size_t itemLength = 0;
            if(!readLength(data, length, offset, ptr, itemLength)) return;

            if(field == 2u)
            {
                copyUtf8(node.longName, sizeof(node.longName), ptr, itemLength);
            }
            else if(field == 3u)
            {
                copyUtf8(node.shortName, sizeof(node.shortName), ptr, itemLength);
            }
            else
            {
                node.pkiAvailable = itemLength == 32u;
                node.publicKeyValid = itemLength == 32u;
                if(node.publicKeyValid) memcpy(node.publicKey, ptr, 32u);
            }
        }
        else if(!skipField(data, length, offset, wire))
        {
            return;
        }
    }
}

void evaluateReady()
{
    const bool completeEnough =
        g_configCompleteSeen &&
        ((g_expectedNodeCount == 0u && g_nodeCount > 0u) ||
         (g_expectedNodeCount > 0u && g_nodeCount >= g_expectedNodeCount));

    if(completeEnough && !g_ready)
    {
        g_ready = true;
        ++g_revision;
        Serial.printf(
            "[LORA-RUNTIME] NodeDB READY local=!%08lX nodes=%u expected=%lu\n",
            static_cast<unsigned long>(g_localNode),
            static_cast<unsigned>(g_nodeCount),
            static_cast<unsigned long>(g_expectedNodeCount)
        );
    }
}

void parseMyInfo(const uint8_t* data, size_t length)
{
    uint32_t local = g_localNode;
    uint32_t expected = g_expectedNodeCount;

    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 0x07u);

        if((field == 1u || field == 15u) && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            if(field == 1u) local = static_cast<uint32_t>(value);
            else expected = static_cast<uint32_t>(value);
        }
        else if(!skipField(data, length, offset, wire))
        {
            return;
        }
    }

    const bool changed = local != g_localNode || expected != g_expectedNodeCount;
    g_localNode = local;
    g_expectedNodeCount = expected;
    if(changed) ++g_revision;
    evaluateReady();

    Serial.printf(
        "[LORA-RUNTIME] my_info local=!%08lX expected_nodes=%lu\n",
        static_cast<unsigned long>(g_localNode),
        static_cast<unsigned long>(g_expectedNodeCount)
    );
}

void parseNodeInfo(const uint8_t* data, size_t length)
{
    uint32_t nodeNum = 0;
    const uint8_t* userPtr = nullptr;
    size_t userLength = 0;
    float snr = 0.0f;
    uint32_t lastHeard = 0;
    bool hasHops = false;
    uint8_t hops = 0;
    bool favorite = false;

    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(data, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 0x07u);

        if(field == 1u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            nodeNum = static_cast<uint32_t>(value);
        }
        else if(field == 2u && wire == 2u)
        {
            if(!readLength(data, length, offset, userPtr, userLength)) return;
        }
        else if(field == 4u && wire == 5u)
        {
            uint32_t bits = 0;
            if(!readFixed32(data, length, offset, bits)) return;
            memcpy(&snr, &bits, sizeof(snr));
        }
        else if(field == 5u && wire == 5u)
        {
            if(!readFixed32(data, length, offset, lastHeard)) return;
        }
        else if(field == 9u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            hasHops = true;
            hops = static_cast<uint8_t>(value);
        }
        else if(field == 10u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(data, length, offset, value)) return;
            favorite = value != 0;
        }
        else if(!skipField(data, length, offset, wire))
        {
            return;
        }
    }

    if(nodeNum == 0) return;
    const int index = ensureNode(nodeNum);
    if(index < 0) return;

    NodeSlot& node = g_nodes[index];
    node.snr = snr;
    node.lastHeardEpoch = lastHeard;
    node.hasHops = hasHops;
    node.hops = hops;
    node.favorite = favorite;
    if(userPtr != nullptr) parseUser(userPtr, userLength, node);
    ++g_revision;
    evaluateReady();

    Serial.printf(
        "[LORA-RUNTIME] node !%08lX name=%s short=%s hops=%s%u snr=%.2f\n",
        static_cast<unsigned long>(node.node),
        node.longName[0] ? node.longName : "--",
        node.shortName[0] ? node.shortName : "--",
        node.hasHops ? "" : "--/",
        static_cast<unsigned>(node.hops),
        static_cast<double>(node.snr)
    );
}

void handleConfigComplete(uint32_t id)
{
    if(id != FT02_MT_CONFIG_ID) return;

    g_configCompleteSeen = true;
    ++g_revision;
    evaluateReady();

    if(!g_ready)
    {
        Serial.printf(
            "[LORA-RUNTIME] NodeDB INCOMPLETE at config_complete local=!%08lX nodes=%u expected=%lu\n",
            static_cast<unsigned long>(g_localNode),
            static_cast<unsigned>(g_nodeCount),
            static_cast<unsigned long>(g_expectedNodeCount)
        );
    }
}

}

void FT02_LoRaNodeRuntimeOnFromRadio(const uint8_t* payload, uint16_t length)
{
    if(payload == nullptr || length == 0) return;

    size_t offset = 0;
    while(offset < length)
    {
        uint64_t key = 0;
        if(!readVarint(payload, length, offset, key)) return;
        const uint32_t field = static_cast<uint32_t>(key >> 3);
        const uint8_t wire = static_cast<uint8_t>(key & 0x07u);

        if((field == 3u || field == 4u) && wire == 2u)
        {
            const uint8_t* ptr = nullptr;
            size_t itemLength = 0;
            if(!readLength(payload, length, offset, ptr, itemLength)) return;
            if(field == 3u) parseMyInfo(ptr, itemLength);
            else parseNodeInfo(ptr, itemLength);
        }
        else if(field == 7u && wire == 0u)
        {
            uint64_t value = 0;
            if(!readVarint(payload, length, offset, value)) return;
            handleConfigComplete(static_cast<uint32_t>(value));
        }
        else if(!skipField(payload, length, offset, wire))
        {
            return;
        }
    }
}

void FT02_LoRaNodeRuntimeReset()
{
    memset(g_nodes, 0, sizeof(g_nodes));
    g_nodeCount = 0;
    g_localNode = 0;
    g_expectedNodeCount = 0;
    g_ready = false;
    g_configCompleteSeen = false;
    ++g_revision;
    Serial.println("[LORA-RUNTIME] state reset for fresh radio sync");
}

bool FT02_LoRaNodeRuntimeConfigCompleteSeen()
{
    return g_configCompleteSeen;
}

bool FT02_LoRaNodeRuntimeReady()
{
    return g_ready;
}

uint32_t FT02_LoRaNodeRuntimeLocalNode()
{
    return g_localNode;
}

uint32_t FT02_LoRaNodeRuntimeExpectedNodeCount()
{
    return g_expectedNodeCount;
}

size_t FT02_LoRaNodeRuntimeNodeCount()
{
    return g_nodeCount;
}

uint32_t FT02_LoRaNodeRuntimeRevision()
{
    return g_revision;
}

bool FT02_LoRaNodeRuntimeGetNode(size_t index, FT02LoRaNodeView& out)
{
    memset(&out, 0, sizeof(out));
    if(index >= g_nodeCount || !g_nodes[index].valid) return false;

    const NodeSlot& src = g_nodes[index];
    out.valid = true;
    out.node = src.node;
    snprintf(out.longName, sizeof(out.longName), "%s", src.longName);
    snprintf(out.shortName, sizeof(out.shortName), "%s", src.shortName);
    out.pkiAvailable = src.pkiAvailable;
    out.publicKeyValid = src.publicKeyValid;
    if(src.publicKeyValid) memcpy(out.publicKey, src.publicKey, 32u);
    out.favorite = src.favorite;
    out.hasHops = src.hasHops;
    out.hops = src.hops;
    out.snr = src.snr;
    out.lastHeardEpoch = src.lastHeardEpoch;
    return true;
}

bool FT02_LoRaNodeRuntimeFindNode(uint32_t node, FT02LoRaNodeView& out)
{
    memset(&out, 0, sizeof(out));
    const int index = findNode(node);
    if(index < 0) return false;
    return FT02_LoRaNodeRuntimeGetNode(static_cast<size_t>(index), out);
}

bool FT02_LoRaNodeRuntimeGetPublicKey(uint32_t node, uint8_t outKey[32])
{
    if(outKey == nullptr) return false;
    const int index = findNode(node);
    if(index < 0 || !g_nodes[index].publicKeyValid) return false;
    memcpy(outKey, g_nodes[index].publicKey, 32u);
    return true;
}

void FT02_LoRaNodeRuntimeUpdatePacketMetrics(
    uint32_t node,
    bool hasSnr,
    float snr,
    bool hasHops,
    uint8_t hops,
    uint32_t lastHeardEpoch
)
{
    const int index = ensureNode(node);
    if(index < 0) return;
    NodeSlot& slot = g_nodes[index];
    if(hasSnr) slot.snr = snr;
    if(hasHops)
    {
        slot.hasHops = true;
        slot.hops = hops;
    }
    if(lastHeardEpoch != 0) slot.lastHeardEpoch = lastHeardEpoch;
    ++g_revision;
}

void FT02_LoRaNodeRuntimeUpdateUserFromPacket(
    uint32_t node,
    const uint8_t* userPayload,
    size_t userLength,
    bool hasSnr,
    float snr,
    bool hasHops,
    uint8_t hops,
    uint32_t lastHeardEpoch
)
{
    const int index = ensureNode(node);
    if(index < 0 || userPayload == nullptr) return;
    NodeSlot& slot = g_nodes[index];
    parseUser(userPayload, userLength, slot);
    if(hasSnr) slot.snr = snr;
    if(hasHops)
    {
        slot.hasHops = true;
        slot.hops = hops;
    }
    if(lastHeardEpoch != 0) slot.lastHeardEpoch = lastHeardEpoch;
    ++g_revision;
}

const char* FT02_LoRaNodeRuntimeLocalLongName()
{
    const int index = findNode(g_localNode);
    if(index < 0) return "";
    return g_nodes[index].longName;
}

const char* FT02_LoRaNodeRuntimeLocalShortName()
{
    const int index = findNode(g_localNode);
    if(index < 0) return "";
    return g_nodes[index].shortName;
}
