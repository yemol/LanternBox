#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/stub"
cat > "$TMP/stub/Arduino.h" <<'EOF'
#pragma once
#include <stddef.h>
#include <stdint.h>
struct FakeSerialType { template <typename... A> void printf(const char*, A...) {} template <typename T> void print(T) {} template <typename T> void println(T) {} void println() {} template <typename... A> void write(A...) {} };
extern FakeSerialType Serial;
inline void yield() {}
EOF
cat > "$TMP/stub/codec2.h" <<'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct CODEC2;
#define CODEC2_MODE_3200 0
#define CODEC2_MODE_1600 2
#define CODEC2_MODE_1300 4
struct CODEC2* codec2_create(int mode);
void codec2_destroy(struct CODEC2* c2);
int codec2_samples_per_frame(struct CODEC2* c2);
int codec2_bytes_per_frame(struct CODEC2* c2);
void codec2_encode(struct CODEC2* c2, unsigned char* bits, short speech[]);
void codec2_decode(struct CODEC2* c2, short speech[], const unsigned char* bits);
#ifdef __cplusplus
}
#endif
EOF


cat > "$TMP/stub/opus.h" <<'EOF'
#pragma once
#include <stdint.h>
typedef int16_t opus_int16;
typedef int32_t opus_int32;
typedef struct OpusEncoder OpusEncoder;
typedef struct OpusDecoder OpusDecoder;
#define OPUS_OK 0
#define OPUS_APPLICATION_VOIP 2048
#define OPUS_SIGNAL_VOICE 3001
#define OPUS_SET_BITRATE(x) 4002, (x)
#define OPUS_SET_VBR(x) 4006, (x)
#define OPUS_SET_COMPLEXITY(x) 4010, (x)
#define OPUS_SET_INBAND_FEC(x) 4012, (x)
#define OPUS_SET_DTX(x) 4016, (x)
#define OPUS_SET_SIGNAL(x) 4024, (x)
#ifdef __cplusplus
extern "C" {
#endif
int opus_encoder_get_size(int channels);
int opus_encoder_init(OpusEncoder*, opus_int32, int, int);
int opus_encoder_ctl(OpusEncoder*, int, ...);
int opus_encode(OpusEncoder*, const opus_int16*, int, unsigned char*, opus_int32);
int opus_decoder_get_size(int channels);
int opus_decoder_init(OpusDecoder*, opus_int32, int);
int opus_decode(OpusDecoder*, const unsigned char*, opus_int32, opus_int16*, int, int);
#ifdef __cplusplus
}
#endif
EOF
cat > "$TMP/stub/gsm.h" <<'EOF'
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
typedef short gsm_signal;
typedef unsigned char gsm_byte;
typedef struct gsm_state *gsm;
gsm gsm_create(void);
void gsm_destroy(gsm);
void gsm_encode(gsm, gsm_signal*, gsm_byte*);
int gsm_decode(gsm, gsm_byte*, gsm_signal*);
#ifdef __cplusplus
}
#endif
EOF

g++ -std=gnu++17 -fsyntax-only \
  -I"$TMP/stub" \
  -I"$ROOT/src" \
  -I"$ROOT/lib/FT02Log/src" \
  -I"$ROOT/lib/FT02Storage/src" \
  "$ROOT/src/FT02_MeshVoiceCodec.cpp"
echo "PASS MeshVoiceCodec syntax + Opus-6000 TX + Opus-4800/GSM-FR/Codec2 legacy decode C API surface"
