#pragma once

#include <stddef.h>
#include <stdint.h>

// LanternBox Voice Message A1 wire protocol carried inside Meshtastic private
// application port 256. It is asynchronous store-and-forward voice, not live PTT.
static constexpr uint8_t FT02_VOICE_PROTOCOL_VERSION = 1u;
static constexpr uint8_t FT02_VOICE_CODEC_CODEC2_1300 = 1u; // legacy A1.5 voice
static constexpr uint8_t FT02_VOICE_CODEC_CODEC2_1600 = 2u; // A1.5.7 legacy/HQ1
static constexpr uint8_t FT02_VOICE_CODEC_CODEC2_3200 = 3u; // A1.5.8 legacy HQ
static constexpr uint8_t FT02_VOICE_CODEC_GSM_FR = 4u;        // A1.5.9 legacy TX: GSM 06.10 Full Rate
static constexpr uint8_t FT02_VOICE_CODEC_OPUS_6000 = 5u;     // current TX: Opus VOIP 6 kbps CBR
static constexpr uint8_t FT02_VOICE_CODEC_OPUS_4800 = 6u;     // legacy A1.5.11 RX/playback: Opus VOIP 4.8 kbps CBR
static constexpr uint16_t FT02_VOICE_SAMPLE_RATE = 8000u;      // legacy Codec2/GSM rate
static constexpr uint16_t FT02_VOICE_SAMPLE_RATE_OPUS = 16000u; // current Opus TX rate
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES_1300 = 320u; // 40 ms @ 8 kHz
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES_1600 = 320u; // 40 ms @ 8 kHz
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES_3200 = 160u; // 20 ms @ 8 kHz
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_1300 = 7u;
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_1600 = 8u;
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_3200 = 8u;
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES_GSM_FR = 160u; // 20 ms @ 8 kHz
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_GSM_FR = 33u;     // GSM 06.10 FR: 260 bits packed to 33 B
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES_OPUS = 960u;   // 60 ms @ 16 kHz
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_OPUS_6000 = 45u;  // current TX: 6000 * 60 ms / 8
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_OPUS_4800 = 36u;  // legacy RX/playback: 4800 * 60 ms / 8
static constexpr uint8_t FT02_VOICE_FRAME_BYTES_OPUS = FT02_VOICE_FRAME_BYTES_OPUS_6000; // current TX alias
static constexpr uint8_t FT02_VOICE_TX_CODEC = FT02_VOICE_CODEC_OPUS_6000;
static constexpr uint8_t FT02_VOICE_FRAME_BYTES = FT02_VOICE_FRAME_BYTES_OPUS_6000; // current TX
static constexpr uint16_t FT02_VOICE_FRAME_SAMPLES = FT02_VOICE_FRAME_SAMPLES_OPUS; // current TX
// Protocol/file compatibility remains 30 s so old A1.5.x history stays playable.
// New recordings are intentionally capped at 15 s.
static constexpr uint32_t FT02_VOICE_MAX_DURATION_MS = 30000u;
static constexpr uint32_t FT02_VOICE_TX_MAX_DURATION_MS = 15000u;
static constexpr uint32_t FT02_VOICE_CAPTURE_PREROLL_MS = 700u;
// Capture begins before the e-paper START SPEAKING cue. Allow the actual
// blocking partial-refresh time to be included and trimmed, while keeping a
// hard fail-safe bound on pre-cue capture.
static constexpr uint32_t FT02_VOICE_CAPTURE_MAX_PREROLL_MS = 5000u;
static constexpr uint32_t FT02_VOICE_CAPTURE_RAW_MAX_MS =
    FT02_VOICE_TX_MAX_DURATION_MS + FT02_VOICE_CAPTURE_MAX_PREROLL_MS;
// Keep the historical 49.5 KiB receive/playback ceiling so A1.5.9 GSM-FR history
// remains valid. New TX returns to the proven clean Opus-6000 profile:
// 15 s = 250 * 45 = 11,250 B. Current chunk data stays at 192 B so
// both broadcast and private/PKI envelopes remain within the proven RF limit.
static constexpr size_t FT02_VOICE_MAX_ENCODED_BYTES = 49500u;
static constexpr size_t FT02_VOICE_TX_MAX_ENCODED_BYTES = 11250u;
// 192 B is deliberately below the maximum broadcast payload.  More
// importantly it also fits the private/PKI path after Data protobuf, CCM tag,
// extra nonce and the 16-byte Mesh header are added.  A1.5.12 used 204 B and
// produced 250-byte private air frames, which the SX1262 backend correctly
// rejected (>240 B).
static constexpr size_t FT02_VOICE_CHUNK_BYTES = 192u;
static constexpr uint8_t FT02_VOICE_MAX_CHUNKS = 254u;
static constexpr uint8_t FT02_VOICE_NACK_WINDOWS = 4u; // 4 x 64-bit selective retransmit windows
static constexpr size_t FT02_VOICE_WIRE_MAX_BYTES = 204u;

// On-SD file header for /lanternbox/mesh/voice/*.lbv.
static constexpr size_t FT02_VOICE_FILE_HEADER_BYTES = 24u;

struct FT02MeshVoiceFileHeader
{
    uint8_t codec;
    uint8_t frameBytes;
    uint16_t sampleRate;
    uint16_t frameSamples;
    uint32_t durationMs;
    uint32_t payloadBytes;
    uint32_t payloadCrc32;
};

enum FT02MeshVoicePacketType : uint8_t
{
    FT02_VOICE_PKT_BEGIN = 1u,
    FT02_VOICE_PKT_CHUNK = 2u,
    FT02_VOICE_PKT_END = 3u,
    FT02_VOICE_PKT_NACK = 4u,
    FT02_VOICE_PKT_ACK = 5u,
    FT02_VOICE_PKT_CANCEL = 6u
};

struct FT02MeshVoicePacketView
{
    bool valid;
    FT02MeshVoicePacketType type;
    uint32_t voiceId;

    // BEGIN metadata.
    uint8_t codec;
    uint8_t frameBytes;
    uint16_t durationMs;
    uint16_t totalBytes;
    uint8_t totalChunks;
    uint32_t payloadCrc32;

    // CHUNK metadata.
    uint8_t chunkIndex;
    const uint8_t* chunkData;
    uint8_t chunkLen;

    // NACK/CANCEL metadata.
    uint64_t missingMask;
    uint8_t nackWindow;
    uint8_t round;
    uint8_t reason;
};

uint16_t FT02_MeshVoiceSampleRateForCodec(uint8_t codec);
uint16_t FT02_MeshVoiceFrameSamplesForCodec(uint8_t codec);
uint8_t FT02_MeshVoiceFrameBytesForCodec(uint8_t codec);
uint32_t FT02_MeshVoiceCrc32(const uint8_t* data, size_t len);

bool FT02_MeshVoiceBuildFileHeader(const FT02MeshVoiceFileHeader& header,
                                   uint8_t out[FT02_VOICE_FILE_HEADER_BYTES]);
bool FT02_MeshVoiceParseFileHeader(const uint8_t* data,
                                   size_t len,
                                   FT02MeshVoiceFileHeader& out);

bool FT02_MeshVoiceBuildBegin(uint32_t voiceId,
                              uint8_t codec,
                              uint8_t frameBytes,
                              uint16_t durationMs,
                              uint16_t totalBytes,
                              uint8_t totalChunks,
                              uint32_t payloadCrc32,
                              uint8_t* out,
                              size_t maxLen,
                              size_t& outLen);
bool FT02_MeshVoiceBuildChunk(uint32_t voiceId,
                              uint8_t chunkIndex,
                              const uint8_t* chunk,
                              uint8_t chunkLen,
                              uint8_t* out,
                              size_t maxLen,
                              size_t& outLen);
bool FT02_MeshVoiceBuildSimple(FT02MeshVoicePacketType type,
                               uint32_t voiceId,
                               uint8_t* out,
                               size_t maxLen,
                               size_t& outLen);
bool FT02_MeshVoiceBuildNack(uint32_t voiceId,
                             uint64_t missingMask,
                             uint8_t nackWindow,
                             uint8_t round,
                             uint8_t* out,
                             size_t maxLen,
                             size_t& outLen);
bool FT02_MeshVoiceBuildCancel(uint32_t voiceId,
                               uint8_t reason,
                               uint8_t* out,
                               size_t maxLen,
                               size_t& outLen);
bool FT02_MeshVoiceParsePacket(const uint8_t* data,
                               size_t len,
                               FT02MeshVoicePacketView& out);
