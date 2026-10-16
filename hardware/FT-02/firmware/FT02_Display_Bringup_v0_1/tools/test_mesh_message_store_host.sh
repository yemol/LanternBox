#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
STORE_ROOT="$TMP/sd"
mkdir -p "$STORE_ROOT"
cat > "$TMP/Arduino.h" <<'H'
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
extern uint32_t g_test_millis;
inline uint32_t millis(){ return g_test_millis; }
struct SerialMock {
  template<class... T> void printf(const char*, T...) {}
  template<class T> void print(T) {}
  template<class T> void println(T) {}
  template<class T> void write(T) {}
};
extern SerialMock Serial;
H
cat > "$TMP/FT02_Storage.h" <<'H'
#pragma once
#include <stdio.h>
bool FT02_StorageIsReady();
bool FT02_StorageSleepSuspended();
bool FT02_StorageFileExists(const char*);
FILE* FT02_StorageOpenReadFile(const char*);
FILE* FT02_StorageOpenWriteFile(const char*, bool=true);
bool FT02_StorageSyncFile(FILE*);
bool FT02_StorageDeleteFile(const char*);
bool FT02_StorageRenameFile(const char*, const char*);
H
cat > "$TMP/stubs.cpp" <<'CPP'
#include "Arduino.h"
#include "FT02_Storage.h"
#include <filesystem>
#include <string>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
SerialMock Serial;
uint32_t g_test_millis = 0;
static std::string root(){ const char* p=getenv("FT02_TEST_SD"); return p?p:"/tmp/ft02-test-sd"; }
static std::string full(const char* path){ return root() + std::string(path ? path : ""); }
bool FT02_StorageIsReady(){ return true; }
bool FT02_StorageSleepSuspended(){ return false; }
bool FT02_StorageFileExists(const char* p){ return std::filesystem::is_regular_file(full(p)); }
FILE* FT02_StorageOpenReadFile(const char* p){ return fopen(full(p).c_str(), "rb"); }
FILE* FT02_StorageOpenWriteFile(const char* p, bool trunc){ auto f=full(p); std::filesystem::create_directories(std::filesystem::path(f).parent_path()); return fopen(f.c_str(), trunc?"wb+":"ab+"); }
bool FT02_StorageSyncFile(FILE* f){ return f && fflush(f)==0 && fsync(fileno(f))==0; }
bool FT02_StorageDeleteFile(const char* p){ auto f=full(p); std::error_code ec; std::filesystem::remove(f,ec); return !ec; }
bool FT02_StorageRenameFile(const char* a,const char* b){ auto fb=full(b); std::filesystem::create_directories(std::filesystem::path(fb).parent_path()); std::error_code ec; std::filesystem::rename(full(a),fb,ec); return !ec; }
CPP
cat > "$TMP/test.cpp" <<'CPP'
#include "FT02_MeshMessageStore.h"
#include "Arduino.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

static void write_phase(){
  FT02_MeshMessageStoreBegin();
  assert(FT02_MeshMessageStoreCount()==0);
  for(unsigned i=0;i<105;i++){
    char text[64]; std::snprintf(text,sizeof(text),"rx-%03u",i);
    assert(FT02_MeshMessageStoreAppendRxText(0x1000u+i,0x4C423001u,0xFFFFFFFFu,false,text,true,-90,true,7.5f));
  }
  assert(FT02_MeshMessageStoreCount()==100);
  assert(FT02_MeshMessageStoreUnreadCount()==100);
  FT02MeshMessageView newest{}; assert(FT02_MeshMessageStoreGetNewest(0,newest));
  assert(std::strcmp(newest.text,"rx-104")==0);
  FT02MeshMessageView oldest{}; assert(FT02_MeshMessageStoreGetNewest(99,oldest));
  assert(std::strcmp(oldest.text,"rx-005")==0);
  assert(FT02_MeshMessageStoreAppendTxText(0xABCDEF01u,0x4C423002u,0x4C423001u,true,"outgoing",FT02_MESH_STATUS_QUEUED,1700000000u));
  assert(FT02_MeshMessageStoreCount()==100);
  assert(FT02_MeshMessageStoreUpdateTx(0xABCDEF01u,0x22223333u,FT02_MESH_STATUS_DELIVERED));
  FT02MeshMessageView tx{}; assert(FT02_MeshMessageStoreGetNewest(0,tx));
  assert(tx.direction==FT02_MESH_MESSAGE_TX && tx.status==FT02_MESH_STATUS_DELIVERED && tx.packetId==0x22223333u);
  assert(FT02_MeshMessageStoreAppendRxVoice(0x5555u,0x4C423001u,0x4C423002u,true,12300u,"/lanternbox/mesh/voice/5555.c2"));
  FT02MeshMessageView voice{}; assert(FT02_MeshMessageStoreGetNewest(0,voice));
  assert(voice.type==FT02_MESH_MESSAGE_VOICE && voice.voiceDurationMs==12300u);
  g_test_millis=2000;
  FT02_MeshMessageStorePoll();
  std::puts("WRITE PASS");
}

static void read_phase(){
  FT02_MeshMessageStoreBegin();
  assert(FT02_MeshMessageStoreCount()==100);
  FT02MeshMessageView voice{}; assert(FT02_MeshMessageStoreGetNewest(0,voice));
  assert(voice.type==FT02_MESH_MESSAGE_VOICE && std::strcmp(voice.voicePath,"/lanternbox/mesh/voice/5555.c2")==0);
  FT02MeshMessageView tx{}; assert(FT02_MeshMessageStoreGetNewest(1,tx));
  assert(tx.logicalId==0xABCDEF01u && tx.status==FT02_MESH_STATUS_DELIVERED);
  FT02_MeshMessageStoreMarkAllRead();
  assert(FT02_MeshMessageStoreUnreadCount()==0);
  std::puts("READ PASS");
}

int main(int argc,char**argv){ assert(argc==2); if(std::string(argv[1])=="write") write_phase(); else read_phase(); }
CPP
BIN="$TMP/test"
g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$TMP" -I"$ROOT/src" -I"$ROOT/lib/FT02Log/src" \
  "$ROOT/src/FT02_MeshMessageStore.cpp" "$TMP/stubs.cpp" "$TMP/test.cpp" -o "$BIN"
FT02_TEST_SD="$STORE_ROOT" "$BIN" write
FT02_TEST_SD="$STORE_ROOT" "$BIN" read
printf 'PASS Mesh Message Store host persistence / 100-ring / mixed-type\n'
