#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/test.cpp" <<'CPP'
#include "FT02_MeshVoiceProtocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

static void roundTripLegacy(uint8_t codec, uint8_t frameBytes, uint16_t frameSamples, size_t payloadBytes) {
  std::vector<uint8_t> payload(payloadBytes, 0x5A);
  const uint32_t crc=FT02_MeshVoiceCrc32(payload.data(),payload.size());
  FT02MeshVoiceFileHeader fh{}; fh.codec=codec; fh.frameBytes=frameBytes;
  fh.sampleRate=FT02_MeshVoiceSampleRateForCodec(codec); fh.frameSamples=frameSamples;
  fh.durationMs=30000; fh.payloadBytes=payloadBytes; fh.payloadCrc32=crc;
  uint8_t rawh[FT02_VOICE_FILE_HEADER_BYTES]{}; assert(FT02_MeshVoiceBuildFileHeader(fh,rawh));
  FT02MeshVoiceFileHeader parsed{}; assert(FT02_MeshVoiceParseFileHeader(rawh,sizeof(rawh),parsed));
  assert(parsed.codec==codec && parsed.frameBytes==frameBytes && parsed.frameSamples==frameSamples);
}

int main(){
  // A1.5.13 TX format: Opus VOIP 6.0 kbps CBR, 60 ms fixed 45-byte packet.
  std::vector<uint8_t> opus(11250);
  for(size_t i=0;i<opus.size();++i) opus[i]=(uint8_t)(i*17u+3u);
  const uint32_t opusCrc=FT02_MeshVoiceCrc32(opus.data(),opus.size()); assert(opusCrc!=0u);
  FT02MeshVoiceFileHeader oh{}; oh.codec=FT02_VOICE_CODEC_OPUS_6000; oh.frameBytes=FT02_VOICE_FRAME_BYTES_OPUS_6000;
  oh.sampleRate=FT02_VOICE_SAMPLE_RATE_OPUS; oh.frameSamples=FT02_VOICE_FRAME_SAMPLES_OPUS;
  oh.durationMs=15000; oh.payloadBytes=opus.size(); oh.payloadCrc32=opusCrc;
  uint8_t rawh[FT02_VOICE_FILE_HEADER_BYTES]{}; assert(FT02_MeshVoiceBuildFileHeader(oh,rawh));
  FT02MeshVoiceFileHeader parsed{}; assert(FT02_MeshVoiceParseFileHeader(rawh,sizeof(rawh),parsed));
  assert(parsed.codec==FT02_VOICE_CODEC_OPUS_6000 && parsed.sampleRate==16000 && parsed.frameBytes==45 && parsed.frameSamples==960);

  uint8_t wire[FT02_VOICE_WIRE_MAX_BYTES]{}; size_t n=0;
  const uint8_t opusChunks=(uint8_t)((opus.size()+FT02_VOICE_CHUNK_BYTES-1)/FT02_VOICE_CHUNK_BYTES);
  assert(opusChunks==59);
  assert(FT02_MeshVoiceBuildBegin(0x12345678u,FT02_VOICE_CODEC_OPUS_6000,FT02_VOICE_FRAME_BYTES_OPUS_6000,15000,(uint16_t)opus.size(),opusChunks,opusCrc,wire,sizeof(wire),n));
  FT02MeshVoicePacketView v{}; assert(FT02_MeshVoiceParsePacket(wire,n,v));
  assert(v.type==FT02_VOICE_PKT_BEGIN && v.codec==FT02_VOICE_CODEC_OPUS_6000 && v.totalChunks==59 && v.totalBytes==11250);

  // A1.5.10 Opus-6000 remains a valid legacy file/header format.
  roundTripLegacy(FT02_VOICE_CODEC_OPUS_6000,FT02_VOICE_FRAME_BYTES_OPUS_6000,FT02_VOICE_FRAME_SAMPLES_OPUS,11250);

  // Preserve the A1.5.9 receive/playback ceiling for historical GSM-FR messages.
  std::vector<uint8_t> gsm(49500,0x33);
  const uint32_t gsmCrc=FT02_MeshVoiceCrc32(gsm.data(),gsm.size());
  FT02MeshVoiceFileHeader gh{}; gh.codec=FT02_VOICE_CODEC_GSM_FR; gh.frameBytes=FT02_VOICE_FRAME_BYTES_GSM_FR;
  gh.sampleRate=FT02_VOICE_SAMPLE_RATE; gh.frameSamples=FT02_VOICE_FRAME_SAMPLES_GSM_FR; gh.durationMs=30000;
  gh.payloadBytes=gsm.size(); gh.payloadCrc32=gsmCrc;
  assert(FT02_MeshVoiceBuildFileHeader(gh,rawh)); assert(FT02_MeshVoiceParseFileHeader(rawh,sizeof(rawh),parsed));
  assert(parsed.payloadBytes==49500 && parsed.sampleRate==8000);

  const uint64_t mask=(1ull<<2)|(1ull<<31)|(1ull<<47)|(1ull<<55);
  assert(FT02_MeshVoiceBuildNack(0x12345678u,mask,3,2,wire,sizeof(wire),n)); assert(n==20);
  assert(FT02_MeshVoiceParsePacket(wire,n,v));
  assert(v.type==FT02_VOICE_PKT_NACK && v.nackWindow==3 && v.round==2 && v.missingMask==mask);

  // A1.5.8 64-bit NACK remains parseable as window 0.
  memset(wire,0,sizeof(wire)); memcpy(wire,"LBV1",4); wire[4]=FT02_VOICE_PROTOCOL_VERSION; wire[5]=FT02_VOICE_PKT_NACK;
  wire[6]=0x78; wire[7]=0x56; wire[8]=0x34; wire[9]=0x12;
  for(int i=0;i<8;++i) { wire[10+i]=(uint8_t)(mask>>(8*i)); }
  wire[18]=2;
  assert(FT02_MeshVoiceParsePacket(wire,19,v)); assert(v.nackWindow==0 && v.round==2 && v.missingMask==mask);

  // Legacy 32-bit NACK remains parseable as window 0.
  memset(wire,0,sizeof(wire)); memcpy(wire,"LBV1",4); wire[4]=FT02_VOICE_PROTOCOL_VERSION; wire[5]=FT02_VOICE_PKT_NACK;
  wire[6]=0x78; wire[7]=0x56; wire[8]=0x34; wire[9]=0x12; wire[10]=0x04; wire[14]=1;
  assert(FT02_MeshVoiceParsePacket(wire,15,v)); assert(v.nackWindow==0 && v.missingMask==4u && v.round==1);

  roundTripLegacy(FT02_VOICE_CODEC_CODEC2_3200,FT02_VOICE_FRAME_BYTES_3200,FT02_VOICE_FRAME_SAMPLES_3200,12000);
  roundTripLegacy(FT02_VOICE_CODEC_CODEC2_1600,FT02_VOICE_FRAME_BYTES_1600,FT02_VOICE_FRAME_SAMPLES_1600,6000);
  roundTripLegacy(FT02_VOICE_CODEC_CODEC2_1300,FT02_VOICE_FRAME_BYTES_1300,FT02_VOICE_FRAME_SAMPLES_1300,5250);

  assert(FT02_MeshVoiceBuildSimple(FT02_VOICE_PKT_ACK,0x12345678u,wire,sizeof(wire),n));
  assert(FT02_MeshVoiceParsePacket(wire,n,v) && v.type==FT02_VOICE_PKT_ACK);
  puts("PASS Mesh Voice A1.5.13 Opus-6000 15s/59-chunk private-safe TX + legacy Opus-4800/GSM/Codec2 + 4-window NACK");
}
CPP
g++ -std=c++17 -Wall -Wextra -Werror -I"$ROOT/src" "$ROOT/src/FT02_MeshVoiceProtocol.cpp" "$TMP/test.cpp" -o "$TMP/test"
"$TMP/test"
