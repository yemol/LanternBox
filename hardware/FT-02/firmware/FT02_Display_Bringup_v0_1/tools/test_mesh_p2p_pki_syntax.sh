#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/mbedtls"
cat > "$TMP/Arduino.h" <<'EOF'
#pragma once
#include <stdint.h>
#include <stddef.h>
EOF
cat > "$TMP/Preferences.h" <<'EOF'
#pragma once
#include <stddef.h>
class Preferences { public: bool begin(const char*,bool){return true;} size_t getBytesLength(const char*){return 0;} size_t getBytes(const char*,void*,size_t){return 0;} size_t putBytes(const char*,const void*,size_t n){return n;} void end(){} };
EOF
cat > "$TMP/esp_system.h" <<'EOF'
#pragma once
#include <stdint.h>
inline uint32_t esp_random(){return 0x12345678u;}
EOF
cat > "$TMP/mbedtls/ccm.h" <<'EOF'
#pragma once
#include <stddef.h>
#define MBEDTLS_CIPHER_ID_AES 1
typedef struct {int x;} mbedtls_ccm_context;
inline void mbedtls_ccm_init(mbedtls_ccm_context*){}
inline void mbedtls_ccm_free(mbedtls_ccm_context*){}
inline int mbedtls_ccm_setkey(mbedtls_ccm_context*,int,const unsigned char*,unsigned int){return 0;}
inline int mbedtls_ccm_encrypt_and_tag(mbedtls_ccm_context*,size_t,const unsigned char*,size_t,const unsigned char*,size_t,const unsigned char*,unsigned char*,unsigned char*,size_t){return 0;}
inline int mbedtls_ccm_auth_decrypt(mbedtls_ccm_context*,size_t,const unsigned char*,size_t,const unsigned char*,size_t,const unsigned char*,unsigned char*,const unsigned char*,size_t){return 0;}
EOF
cat > "$TMP/mbedtls/sha256.h" <<'EOF'
#pragma once
#include <stddef.h>
inline int mbedtls_sha256(const unsigned char*,size_t,unsigned char*,int){return 0;}
EOF
g++ -std=c++17 -Wall -Wextra -Werror -I"$TMP" -I"$ROOT/src" -fsyntax-only "$ROOT/src/FT02_MeshPki.cpp"
echo 'PASS FT02 Mesh PKI C++17 syntax'
