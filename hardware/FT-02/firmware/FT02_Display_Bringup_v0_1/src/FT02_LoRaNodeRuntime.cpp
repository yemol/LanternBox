#include "FT02_LoRaNodeRuntime.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

namespace {
constexpr size_t MAX_NODES = 64;
struct NodeSlot {
    FT02LoRaNodeView v;
    bool online;
    float rssi;
    uint32_t lastSeconds;
};
NodeSlot g_nodes[MAX_NODES] = {};
size_t g_count = 0;
uint32_t g_local = 0;
uint32_t g_expected = 0;
uint32_t g_revision = 0;
bool g_ready = false;

int findNode(uint32_t node) {
    for (size_t i = 0; i < g_count; ++i)
        if (g_nodes[i].v.valid && g_nodes[i].v.node == node) return static_cast<int>(i);
    return -1;
}

int ensureNode(uint32_t node) {
    const int found = findNode(node);
    if (found >= 0) return found;
    if (!node || g_count >= MAX_NODES) return -1;
    NodeSlot &slot = g_nodes[g_count++];
    memset(&slot, 0, sizeof(slot));
    slot.v.valid = true;
    slot.v.node = node;
    g_expected = static_cast<uint32_t>(g_count);
    ++g_revision;
    return static_cast<int>(g_count - 1u);
}

uint32_t epochNow() {
    const time_t now = time(nullptr);
    if (now < static_cast<time_t>(1600000000)) return 0;
    if (static_cast<uint64_t>(now) > 0xFFFFFFFFULL) return 0xFFFFFFFFu;
    return static_cast<uint32_t>(now);
}
}

bool FT02_LoRaNodeRuntimeReady() { return g_ready; }
uint32_t FT02_LoRaNodeRuntimeLocalNode() { return g_local; }
uint32_t FT02_LoRaNodeRuntimeExpectedNodeCount() { return g_expected ? g_expected : static_cast<uint32_t>(g_count); }
size_t FT02_LoRaNodeRuntimeNodeCount() { return g_count; }
uint32_t FT02_LoRaNodeRuntimeRevision() { return g_revision; }

bool FT02_LoRaNodeRuntimeGetNode(size_t index, FT02LoRaNodeView& out) {
    if (index >= g_count || !g_nodes[index].v.valid) return false;
    out = g_nodes[index].v;
    return true;
}

bool FT02_LoRaNodeRuntimeFindNode(uint32_t node, FT02LoRaNodeView& out) {
    const int i = findNode(node);
    if (i < 0) return false;
    out = g_nodes[i].v;
    return true;
}

const char* FT02_LoRaNodeRuntimeLocalLongName() {
    const int i = findNode(g_local);
    return i < 0 ? "" : g_nodes[i].v.longName;
}
const char* FT02_LoRaNodeRuntimeLocalShortName() {
    const int i = findNode(g_local);
    return i < 0 ? "" : g_nodes[i].v.shortName;
}

void FT02_LoRaNodeRuntimeHostBegin(uint32_t localNode, const char* longName, const char* shortName) {
    g_local = localNode;
    const int i = ensureNode(localNode);
    if (i >= 0) {
        auto &v = g_nodes[i].v;
        if (longName && *longName) snprintf(v.longName, sizeof(v.longName), "%s", longName);
        if (shortName && *shortName) snprintf(v.shortName, sizeof(v.shortName), "%s", shortName);
        v.lastHeardEpoch = epochNow();
    }
    g_expected = static_cast<uint32_t>(g_count);
    ++g_revision;
}

void FT02_LoRaNodeRuntimeHostSetStatus(bool ready, uint16_t peerCount, uint16_t pkiPeerCount) {
    (void)pkiPeerCount;
    g_ready = ready;
    const uint32_t announced = static_cast<uint32_t>(peerCount) + (g_local ? 1u : 0u);
    g_expected = announced > g_count ? announced : static_cast<uint32_t>(g_count);
    ++g_revision;
}

void FT02_LoRaNodeRuntimeHostObservePeer(uint32_t node, const char* name, float snr, bool pkiObserved) {
    if (!node || node == g_local) return;
    const int i = ensureNode(node);
    if (i < 0) return;
    auto &v = g_nodes[i].v;
    if (name && *name) {
        snprintf(v.longName, sizeof(v.longName), "%s", name);
        if (!v.shortName[0]) snprintf(v.shortName, sizeof(v.shortName), "%s", name);
    }
    v.snr = snr;
    v.lastHeardEpoch = epochNow();
    if (pkiObserved) { v.pkiAvailable = true; v.publicKeyValid = true; }
    g_expected = static_cast<uint32_t>(g_count);
    ++g_revision;
}

void FT02_LoRaNodeRuntimeHostUpsertPeer(uint32_t node, const char* longName, const char* shortName, bool online, int32_t hops, float rssi, float snr, bool pkiObserved, uint32_t lastSeconds) {
    if (!node || node == g_local) return;
    const int i = ensureNode(node);
    if (i < 0) return;
    auto &slot = g_nodes[i];
    auto &v = slot.v;
    if (longName && *longName) snprintf(v.longName, sizeof(v.longName), "%s", longName);
    if (shortName && *shortName) snprintf(v.shortName, sizeof(v.shortName), "%s", shortName);
    v.hasHops = hops >= 0;
    v.hops = static_cast<uint8_t>(hops < 0 ? 0 : (hops > 255 ? 255 : hops));
    v.snr = snr;
    if (pkiObserved) { v.pkiAvailable = true; v.publicKeyValid = true; }
    v.lastHeardEpoch = epochNow();
    slot.online = online;
    slot.rssi = rssi;
    slot.lastSeconds = lastSeconds;
    g_expected = static_cast<uint32_t>(g_count);
    ++g_revision;
}

void FT02_LoRaNodeRuntimeHostLearnIdentity(uint32_t node,
                                           const char* longName,
                                           const char* shortName,
                                           const uint8_t* publicKey,
                                           bool publicKeyValid,
                                           float rssi,
                                           float snr) {
    if (!node || node == g_local) return;
    const int i = ensureNode(node);
    if (i < 0) return;
    NodeSlot &slot = g_nodes[i];
    FT02LoRaNodeView &v = slot.v;
    if (longName && *longName) snprintf(v.longName, sizeof(v.longName), "%s", longName);
    if (shortName && *shortName) snprintf(v.shortName, sizeof(v.shortName), "%s", shortName);
    if (publicKeyValid && publicKey) {
        memcpy(v.publicKey, publicKey, sizeof(v.publicKey));
        v.publicKeyValid = true;
        v.pkiAvailable = true;
    }
    slot.online = true;
    slot.rssi = rssi;
    v.snr = snr;
    v.lastHeardEpoch = epochNow();
    g_expected = static_cast<uint32_t>(g_count);
    ++g_revision;
}
