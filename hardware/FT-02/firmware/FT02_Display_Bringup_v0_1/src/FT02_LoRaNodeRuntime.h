#pragma once

#include <Arduino.h>

struct FT02LoRaNodeView
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

// Upper-layer NodeInfo / NodeDB parser for the proven v2.66b PROTO transport.
// IMPORTANT: this runtime receives only complete FromRadio protobuf payloads.
// It must never own or reconfigure UART2, framing, retry timing, radio reset, or want_config.
void FT02_LoRaNodeRuntimeOnFromRadio(const uint8_t* payload, uint16_t length);
void FT02_LoRaNodeRuntimeReset();
bool FT02_LoRaNodeRuntimeConfigCompleteSeen();

bool FT02_LoRaNodeRuntimeReady();
uint32_t FT02_LoRaNodeRuntimeLocalNode();
uint32_t FT02_LoRaNodeRuntimeExpectedNodeCount();
size_t FT02_LoRaNodeRuntimeNodeCount();
uint32_t FT02_LoRaNodeRuntimeRevision();
bool FT02_LoRaNodeRuntimeGetNode(size_t index, FT02LoRaNodeView& out);
bool FT02_LoRaNodeRuntimeFindNode(uint32_t node, FT02LoRaNodeView& out);
bool FT02_LoRaNodeRuntimeGetPublicKey(uint32_t node, uint8_t outKey[32]);
void FT02_LoRaNodeRuntimeUpdatePacketMetrics(uint32_t node, bool hasSnr, float snr, bool hasHops, uint8_t hops, uint32_t lastHeardEpoch);
void FT02_LoRaNodeRuntimeUpdateUserFromPacket(uint32_t node, const uint8_t* userPayload, size_t userLength, bool hasSnr, float snr, bool hasHops, uint8_t hops, uint32_t lastHeardEpoch);
const char* FT02_LoRaNodeRuntimeLocalLongName();
const char* FT02_LoRaNodeRuntimeLocalShortName();
