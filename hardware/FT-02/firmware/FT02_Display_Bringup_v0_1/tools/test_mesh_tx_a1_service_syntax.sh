#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/mbedtls"
cat > "$TMP/Arduino.h" <<'EOT'
#pragma once
#include <stdint.h>
#include <stddef.h>
#define INPUT 1
#define OUTPUT 2
#define LOW 0
#define HIGH 1
struct SerialMock { template<class... T> void printf(const char*,T...){} void println(const char*){} void print(const char*){} void write(char){} };
extern SerialMock Serial;
inline uint32_t millis(){return 1;} inline uint32_t micros(){return 1;}
EOT
cat > "$TMP/esp_system.h" <<'EOT'
#pragma once
#include <stdint.h>
inline uint32_t esp_random(){return 0x12345678u;}
EOT
cat > "$TMP/mbedtls/aes.h" <<'EOT'
#pragma once
#include <stddef.h>
typedef struct { int dummy; } mbedtls_aes_context;
inline void mbedtls_aes_init(mbedtls_aes_context*){}
inline void mbedtls_aes_free(mbedtls_aes_context*){}
inline int mbedtls_aes_setkey_enc(mbedtls_aes_context*,const unsigned char*,unsigned int){return 0;}
inline int mbedtls_aes_crypt_ctr(mbedtls_aes_context*,size_t,size_t*,unsigned char[16],unsigned char[16],const unsigned char*,unsigned char*){return 0;}
EOT
cat > "$TMP/stubs.cpp" <<'EOT'
#include "Arduino.h"
#include "FT02_MeshRadioBackend.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_LoRaNodeRuntime.h"
SerialMock Serial;
bool FT02_MeshRadioBackendBegin(){return true;} void FT02_MeshRadioBackendPoll(){} bool FT02_MeshRadioBackendTakeFrame(FT02MeshRadioFrame&){return false;} bool FT02_MeshRadioBackendTransmit(const uint8_t*,size_t){return true;} bool FT02_MeshRadioBackendReady(){return true;} uint32_t FT02_MeshRadioBackendRxCount(){return 0;} uint32_t FT02_MeshRadioBackendCrcErrorCount(){return 0;} int16_t FT02_MeshRadioBackendLastState(){return 0;} bool FT02_MeshRadioBackendSleepAvailable(){return true;} bool FT02_MeshRadioBackendSleepEnter(){return true;} void FT02_MeshRadioBackendSleepService(){} void FT02_MeshRadioBackendSleepExit(){}
void FT02_LoRaCommunicationHostReceive(uint32_t,uint32_t,uint32_t,const char*,bool,float,float,const char*){}
void FT02_LoRaNodeRuntimeHostBegin(uint32_t,const char*,const char*){} void FT02_LoRaNodeRuntimeHostSetStatus(bool,uint16_t,uint16_t){} void FT02_LoRaNodeRuntimeHostObservePeer(uint32_t,const char*,float,bool){}
void FT02_LoRaNodeRuntimeHostLearnIdentity(uint32_t,const char*,const char*,const uint8_t*,bool,float,float){}
void FT02_LoRaCommunicationHostDelivery(uint32_t,uint32_t,bool){}
EOT
g++ -std=c++17 -Wall -Wextra -Werror -I"$TMP" -I"$ROOT/src" -I"$ROOT/include" -I"$ROOT/lib/FT02Log/src" -fsyntax-only "$ROOT/src/FT02_MeshService.cpp" "$ROOT/src/FT02_MeshProtocol.cpp" "$TMP/stubs.cpp"
echo 'PASS Mesh TX A1 service C++17 syntax'
