#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/test.cpp" <<'CPP'
#include "FT02_MeshProtocol.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(){
  uint8_t key[32]; for(int i=0;i<32;i++) key[i]=(uint8_t)(i+1);
  uint8_t user[128]{}; size_t userLen=0;
  assert(FT02_MeshBuildUserProto(key,user,sizeof(user),userLen));
  FT02MeshUserView u{};
  assert(FT02_MeshParseUserProto(user,userLen,u));
  assert(strcmp(u.id,"!4c423002")==0);
  assert(strcmp(u.longName,"LanternBox FT02")==0);
  assert(strcmp(u.shortName,"FT02")==0);
  assert(u.nodeNumber==0x4C423002u);
  assert(u.hasPublicKey && memcmp(u.publicKey,key,32)==0);
  assert(u.hasIsUnmessagable && !u.isUnmessagable);

  uint8_t namesOnly[96]{}; size_t namesOnlyLen=0;
  assert(FT02_MeshBuildUserProto(nullptr,namesOnly,sizeof(namesOnly),namesOnlyLen));
  FT02MeshUserView noKey{}; assert(FT02_MeshParseUserProto(namesOnly,namesOnlyLen,noKey));
  assert(strcmp(noKey.id,"!4c423002")==0);
  assert(strcmp(noKey.longName,"LanternBox FT02")==0);
  assert(strcmp(noKey.shortName,"FT02")==0);
  assert(!noKey.hasPublicKey);

  // Real FT01-style NodeInfo wire payload fields: id=1, long=2, short=3, key=8.
  uint8_t ft01[128]{}; size_t p=0;
  auto put=[&](uint8_t v){ft01[p++]=v;};
  auto str=[&](uint8_t tag,const char*s){put(tag); size_t n=strlen(s); put((uint8_t)n); memcpy(ft01+p,s,n); p+=n;};
  str(0x0A,"!4c423001"); str(0x12,"LanternBox FT01"); str(0x1A,"FT01");
  put(0x42); put(32); memcpy(ft01+p,key,32); p+=32; put(0x48); put(0x00);
  FT02MeshUserView f{}; assert(FT02_MeshParseUserProto(ft01,p,f));
  assert(f.nodeNumber==0x4C423001u); assert(strcmp(f.longName,"LanternBox FT01")==0); assert(strcmp(f.shortName,"FT01")==0); assert(f.hasPublicKey);

  uint8_t d[64]{}; size_t dl=0; const uint8_t txt[]={'1','2','3'};
  assert(FT02_MeshBuildDataProto(FT02_MESH_PORT_TEXT_MESSAGE_APP,txt,sizeof(txt),false,0,d,sizeof(d),dl));
  FT02MeshDataView dv{}; assert(FT02_MeshParseDataProto(d,dl,dv));
  assert(dv.portnum==1 && dv.payloadLen==3 && memcmp(dv.payload,txt,3)==0);

  uint8_t voiceWire[16]={1,2,3,4,5}; size_t voiceLen=0;
  assert(FT02_MeshBuildDataProto(FT02_MESH_PORT_LANTERNBOX_VOICE,voiceWire,5,false,0,d,sizeof(d),voiceLen));
  FT02MeshDataView vv{}; assert(FT02_MeshParseDataProto(d,voiceLen,vv));
  assert(vv.portnum==256u && vv.payloadLen==5 && memcmp(vv.payload,voiceWire,5)==0);

  const uint8_t ack[]={0x18,0x00}; uint32_t err=123; assert(FT02_MeshParseRoutingError(ack,sizeof(ack),err)); assert(err==0);
  puts("PASS Mesh P2P NodeDB protocol host vectors");
}
CPP
g++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/src" "$ROOT/src/FT02_MeshProtocol.cpp" "$TMP/test.cpp" -o "$TMP/test"
"$TMP/test"
