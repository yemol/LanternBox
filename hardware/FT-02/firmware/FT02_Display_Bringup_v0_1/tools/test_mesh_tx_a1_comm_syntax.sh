#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/Arduino.h" <<'EOT'
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
struct SerialMock { template<class... T> void printf(const char*,T...){} void println(const char*){} void print(const char*){} void write(char){} };
extern SerialMock Serial;
inline uint32_t millis(){return 1;}
EOT
cat > "$TMP/esp_system.h" <<'EOT'
#pragma once
#include <stdint.h>
inline uint32_t esp_random(){ static uint32_t x=0x12345678; return ++x; }
EOT
cat > "$TMP/FT02_Storage.h" <<'EOT'
#pragma once
#include <stdio.h>
#include <stdint.h>
inline bool FT02_StorageIsReady(){return true;}
inline bool FT02_StorageFileExists(const char*){return false;}
inline FILE* FT02_StorageOpenReadFile(const char*){return nullptr;}
inline FILE* FT02_StorageOpenWriteFile(const char*, bool=true){return nullptr;}
inline bool FT02_StorageSyncFile(FILE*){return true;}
inline bool FT02_StorageDeleteFile(const char*){return true;}
inline bool FT02_StorageRenameFile(const char*,const char*){return true;}
EOT
cat > "$TMP/stubs.cpp" <<'EOT'
#include "Arduino.h"
#include "FT02_Gnss.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_MeshService.h"
SerialMock Serial;
FT02GnssSnapshot FT02_GnssSnapshotCurrent(){ return {}; }
uint32_t FT02_LoRaNodeRuntimeLocalNode(){return 0x4C423002u;}
bool FT02_MeshServiceReady(){return true;}
bool FT02_MeshServiceSendBroadcastText(const char*,uint32_t){return true;}
EOT
g++ -std=c++17 -Wall -Wextra -I"$TMP" -I"$ROOT/src" -I"$ROOT/include" -I"$ROOT/lib/FT02Log/src" -fsyntax-only "$ROOT/src/FT02_LoRaCommunicationRuntime.cpp" "$TMP/stubs.cpp"
echo 'PASS communication runtime Mesh TX A1 syntax'
