#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/Arduino.h" <<'EOT'
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
inline uint32_t millis(){ static uint32_t n=1000; return n+=100; }
EOT
cat > "$TMP/FT02_Log.h" <<'EOT'
#pragma once
#define FT02_LOGI(...) do{}while(0)
#define FT02_LOGW(...) do{}while(0)
#define FT02_LOGE(...) do{}while(0)
#define FT02_LOGD(...) do{}while(0)
EOT
cat > "$TMP/FT02_Storage.h" <<'EOT'
#pragma once
#include <stdio.h>
inline bool FT02_StorageIsReady(){return true;}
inline bool FT02_StorageSleepSuspended(){return false;}
inline FILE* FT02_StorageOpenReadFile(const char*){return nullptr;}
inline FILE* FT02_StorageOpenWriteFile(const char*, bool=true){return nullptr;}
inline bool FT02_StorageSyncFile(FILE*){return true;}
inline bool FT02_StorageDeleteFile(const char*){return true;}
EOT
cat > "$TMP/stubs.cpp" <<'EOT'
#include "FT02_MeshService.h"
#include "FT02_MeshMessageStore.h"
uint32_t FT02_MeshServiceAllocatePacketId(){return 1;}
bool FT02_MeshServiceSendBroadcastApp(uint32_t,const uint8_t*,size_t,uint32_t){return true;}
bool FT02_MeshServiceSendPrivateApp(uint32_t,uint32_t,const uint8_t*,size_t,uint32_t,bool){return true;}
bool FT02_MeshServiceCanSendPrivate(uint32_t){return true;}
bool FT02_MeshServiceReady(){return true;}
bool FT02_MeshMessageStoreAppendRxVoice(uint32_t,uint32_t,uint32_t,bool,uint32_t,const char*,FT02MeshMessageStatus){return true;}
bool FT02_MeshMessageStoreAppendTxVoice(uint32_t,uint32_t,uint32_t,bool,uint32_t,const char*,FT02MeshMessageStatus,uint32_t){return true;}
bool FT02_MeshMessageStoreUpdateTx(uint32_t,uint32_t,FT02MeshMessageStatus){return true;}
EOT
g++ -std=c++17 -Wall -Wextra -Werror -I"$TMP" -I"$ROOT/src" -fsyntax-only \
  "$ROOT/src/FT02_MeshVoiceTransport.cpp" "$ROOT/src/FT02_MeshVoiceProtocol.cpp" "$TMP/stubs.cpp"
echo 'PASS Mesh Voice transport syntax'
