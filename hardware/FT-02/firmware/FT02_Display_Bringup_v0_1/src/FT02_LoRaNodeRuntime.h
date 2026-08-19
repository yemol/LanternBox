#pragma once
#include <Arduino.h>

// Core-side presentation cache populated exclusively by LR01 MESH_NODE / MESH_RX.
struct FT02LoRaNodeView
{
    bool valid;
    uint32_t node;
    char longName[25];
    char shortName[13];
    bool pkiAvailable;
    bool publicKeyValid; // compatibility alias of pkiAvailable; Core stores no key bytes
    uint8_t publicKey[32]; // always zero; compatibility only
    bool favorite;
    bool hasHops;
    uint8_t hops;
    float snr;
    uint32_t lastHeardEpoch;
};
bool FT02_LoRaNodeRuntimeReady();
uint32_t FT02_LoRaNodeRuntimeLocalNode();
uint32_t FT02_LoRaNodeRuntimeExpectedNodeCount();
size_t FT02_LoRaNodeRuntimeNodeCount();
uint32_t FT02_LoRaNodeRuntimeRevision();
bool FT02_LoRaNodeRuntimeGetNode(size_t index, FT02LoRaNodeView& out);
bool FT02_LoRaNodeRuntimeFindNode(uint32_t node, FT02LoRaNodeView& out);
const char* FT02_LoRaNodeRuntimeLocalLongName();
const char* FT02_LoRaNodeRuntimeLocalShortName();
void FT02_LoRaNodeRuntimeHostBegin(uint32_t localNode,const char* longName,const char* shortName);
void FT02_LoRaNodeRuntimeHostSetStatus(bool ready,uint16_t peerCount,uint16_t pkiPeerCount);
void FT02_LoRaNodeRuntimeHostObservePeer(uint32_t node,const char* name,float snr,bool pkiObserved);
void FT02_LoRaNodeRuntimeHostUpsertPeer(uint32_t node,const char* longName,const char* shortName,bool online,int32_t hops,float rssi,float snr,bool pkiObserved,uint32_t lastSeconds);
