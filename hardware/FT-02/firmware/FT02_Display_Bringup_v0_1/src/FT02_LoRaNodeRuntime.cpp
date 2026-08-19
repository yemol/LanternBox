#include "FT02_LoRaNodeRuntime.h"
#include <string.h>
#include <stdio.h>
namespace {
constexpr size_t MAX_NODES=64;
struct NodeSlot { FT02LoRaNodeView v; bool online; float rssi; uint32_t lastSeconds; };
NodeSlot g_nodes[MAX_NODES]={}; size_t g_count=0; uint32_t g_local=0,g_expected=0,g_revision=0; bool g_ready=false;
int find(uint32_t n){for(size_t i=0;i<g_count;++i)if(g_nodes[i].v.valid&&g_nodes[i].v.node==n)return (int)i;return -1;}
int ensure(uint32_t n){int f=find(n);if(f>=0)return f;if(!n||g_count>=MAX_NODES)return -1;auto&i=g_nodes[g_count++];memset(&i,0,sizeof(i));i.v.valid=true;i.v.node=n;++g_revision;return (int)(g_count-1);}
}
bool FT02_LoRaNodeRuntimeReady(){return g_ready;}
uint32_t FT02_LoRaNodeRuntimeLocalNode(){return g_local;}
uint32_t FT02_LoRaNodeRuntimeExpectedNodeCount(){return g_expected;}
size_t FT02_LoRaNodeRuntimeNodeCount(){return g_count;}
uint32_t FT02_LoRaNodeRuntimeRevision(){return g_revision;}
bool FT02_LoRaNodeRuntimeGetNode(size_t index,FT02LoRaNodeView& out){if(index>=g_count||!g_nodes[index].v.valid)return false;out=g_nodes[index].v;return true;}
bool FT02_LoRaNodeRuntimeFindNode(uint32_t node,FT02LoRaNodeView& out){int i=find(node);if(i<0)return false;out=g_nodes[i].v;return true;}
const char* FT02_LoRaNodeRuntimeLocalLongName(){int i=find(g_local);return i<0?"":g_nodes[i].v.longName;}
const char* FT02_LoRaNodeRuntimeLocalShortName(){int i=find(g_local);return i<0?"":g_nodes[i].v.shortName;}
void FT02_LoRaNodeRuntimeHostBegin(uint32_t localNode,const char* longName,const char* shortName){g_local=localNode;int i=ensure(localNode);if(i>=0){auto&v=g_nodes[i].v;if(longName&&*longName)snprintf(v.longName,sizeof(v.longName),"%s",longName);if(shortName&&*shortName)snprintf(v.shortName,sizeof(v.shortName),"%s",shortName);}++g_revision;}
void FT02_LoRaNodeRuntimeHostSetStatus(bool ready,uint16_t peerCount,uint16_t pkiPeerCount){(void)pkiPeerCount;g_ready=ready;g_expected=(uint32_t)peerCount+(g_local?1u:0u);++g_revision;}
void FT02_LoRaNodeRuntimeHostObservePeer(uint32_t node,const char* name,float snr,bool pkiObserved){if(!node||node==g_local)return;int i=ensure(node);if(i<0)return;auto&v=g_nodes[i].v;if(name&&*name){snprintf(v.longName,sizeof(v.longName),"%s",name);snprintf(v.shortName,sizeof(v.shortName),"%s",name);}v.snr=snr;if(pkiObserved){v.pkiAvailable=true;v.publicKeyValid=true;}++g_revision;}
void FT02_LoRaNodeRuntimeHostUpsertPeer(uint32_t node,const char* longName,const char* shortName,bool online,int32_t hops,float rssi,float snr,bool pkiObserved,uint32_t lastSeconds){if(!node||node==g_local)return;int i=ensure(node);if(i<0)return;auto&slot=g_nodes[i];auto&v=slot.v;if(longName&&*longName)snprintf(v.longName,sizeof(v.longName),"%s",longName);if(shortName&&*shortName)snprintf(v.shortName,sizeof(v.shortName),"%s",shortName);v.hasHops=hops>=0;v.hops=(uint8_t)(hops<0?0:(hops>255?255:hops));v.snr=snr;if(pkiObserved){v.pkiAvailable=true;v.publicKeyValid=true;}slot.online=online;slot.rssi=rssi;slot.lastSeconds=lastSeconds;++g_revision;}
