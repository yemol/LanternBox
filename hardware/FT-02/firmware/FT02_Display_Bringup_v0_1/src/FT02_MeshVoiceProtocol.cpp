#include "FT02_MeshVoiceProtocol.h"

#include <string.h>

namespace
{
constexpr uint8_t WIRE_MAGIC[4] = {'L','B','V','1'};
constexpr uint8_t FILE_MAGIC[4] = {'L','B','V','F'};
constexpr size_t COMMON_BYTES = 10u;

void put16(uint8_t* out, uint16_t v)
{
    out[0] = static_cast<uint8_t>(v & 0xFFu);
    out[1] = static_cast<uint8_t>((v >> 8u) & 0xFFu);
}

void put32(uint8_t* out, uint32_t v)
{
    out[0] = static_cast<uint8_t>(v & 0xFFu);
    out[1] = static_cast<uint8_t>((v >> 8u) & 0xFFu);
    out[2] = static_cast<uint8_t>((v >> 16u) & 0xFFu);
    out[3] = static_cast<uint8_t>((v >> 24u) & 0xFFu);
}

void put64(uint8_t* out, uint64_t v)
{
    put32(out, static_cast<uint32_t>(v & 0xFFFFFFFFull));
    put32(out + 4u, static_cast<uint32_t>(v >> 32u));
}

uint16_t get16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8u);
}

uint32_t get32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8u) |
           (static_cast<uint32_t>(p[2]) << 16u) |
           (static_cast<uint32_t>(p[3]) << 24u);
}

uint64_t get64(const uint8_t* p)
{
    return static_cast<uint64_t>(get32(p)) | (static_cast<uint64_t>(get32(p + 4u)) << 32u);
}

bool beginPacket(FT02MeshVoicePacketType type,
                 uint32_t voiceId,
                 uint8_t* out,
                 size_t maxLen,
                 size_t needed)
{
    if(out == nullptr || voiceId == 0u || maxLen < needed || needed < COMMON_BYTES) return false;
    memcpy(out, WIRE_MAGIC, sizeof(WIRE_MAGIC));
    out[4] = FT02_VOICE_PROTOCOL_VERSION;
    out[5] = static_cast<uint8_t>(type);
    put32(out + 6u, voiceId);
    return true;
}

bool supportedCodec(uint8_t codec, uint8_t frameBytes)
{
    return (codec == FT02_VOICE_CODEC_CODEC2_1300 && frameBytes == FT02_VOICE_FRAME_BYTES_1300) ||
           (codec == FT02_VOICE_CODEC_CODEC2_1600 && frameBytes == FT02_VOICE_FRAME_BYTES_1600) ||
           (codec == FT02_VOICE_CODEC_CODEC2_3200 && frameBytes == FT02_VOICE_FRAME_BYTES_3200) ||
           (codec == FT02_VOICE_CODEC_GSM_FR && frameBytes == FT02_VOICE_FRAME_BYTES_GSM_FR) ||
           (codec == FT02_VOICE_CODEC_OPUS_6000 && frameBytes == FT02_VOICE_FRAME_BYTES_OPUS_6000) ||
           (codec == FT02_VOICE_CODEC_OPUS_4800 && frameBytes == FT02_VOICE_FRAME_BYTES_OPUS_4800);
}
}

uint16_t FT02_MeshVoiceSampleRateForCodec(uint8_t codec)
{
    if(codec == FT02_VOICE_CODEC_OPUS_6000 || codec == FT02_VOICE_CODEC_OPUS_4800) return FT02_VOICE_SAMPLE_RATE_OPUS;
    if(codec == FT02_VOICE_CODEC_GSM_FR ||
       codec == FT02_VOICE_CODEC_CODEC2_3200 ||
       codec == FT02_VOICE_CODEC_CODEC2_1600 ||
       codec == FT02_VOICE_CODEC_CODEC2_1300) return FT02_VOICE_SAMPLE_RATE;
    return 0u;
}

uint16_t FT02_MeshVoiceFrameSamplesForCodec(uint8_t codec)
{
    if(codec == FT02_VOICE_CODEC_OPUS_6000 || codec == FT02_VOICE_CODEC_OPUS_4800) return FT02_VOICE_FRAME_SAMPLES_OPUS;
    if(codec == FT02_VOICE_CODEC_GSM_FR) return FT02_VOICE_FRAME_SAMPLES_GSM_FR;
    if(codec == FT02_VOICE_CODEC_CODEC2_3200) return FT02_VOICE_FRAME_SAMPLES_3200;
    if(codec == FT02_VOICE_CODEC_CODEC2_1600) return FT02_VOICE_FRAME_SAMPLES_1600;
    if(codec == FT02_VOICE_CODEC_CODEC2_1300) return FT02_VOICE_FRAME_SAMPLES_1300;
    return 0u;
}

uint8_t FT02_MeshVoiceFrameBytesForCodec(uint8_t codec)
{
    if(codec == FT02_VOICE_CODEC_OPUS_6000) return FT02_VOICE_FRAME_BYTES_OPUS_6000;
    if(codec == FT02_VOICE_CODEC_OPUS_4800) return FT02_VOICE_FRAME_BYTES_OPUS_4800;
    if(codec == FT02_VOICE_CODEC_GSM_FR) return FT02_VOICE_FRAME_BYTES_GSM_FR;
    if(codec == FT02_VOICE_CODEC_CODEC2_3200) return FT02_VOICE_FRAME_BYTES_3200;
    if(codec == FT02_VOICE_CODEC_CODEC2_1600) return FT02_VOICE_FRAME_BYTES_1600;
    if(codec == FT02_VOICE_CODEC_CODEC2_1300) return FT02_VOICE_FRAME_BYTES_1300;
    return 0u;
}

uint32_t FT02_MeshVoiceCrc32(const uint8_t* data, size_t len)
{
    if(data == nullptr && len != 0u) return 0u;
    uint32_t crc = 0xFFFFFFFFu;
    for(size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for(uint8_t bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

bool FT02_MeshVoiceBuildFileHeader(const FT02MeshVoiceFileHeader& header,
                                   uint8_t out[FT02_VOICE_FILE_HEADER_BYTES])
{
    if(out == nullptr || !supportedCodec(header.codec, header.frameBytes) ||
       header.sampleRate != FT02_MeshVoiceSampleRateForCodec(header.codec) ||
       header.frameSamples != FT02_MeshVoiceFrameSamplesForCodec(header.codec) ||
       header.durationMs == 0u || header.durationMs > FT02_VOICE_MAX_DURATION_MS ||
       header.payloadBytes == 0u || header.payloadBytes > FT02_VOICE_MAX_ENCODED_BYTES) return false;
    memset(out, 0, FT02_VOICE_FILE_HEADER_BYTES);
    memcpy(out, FILE_MAGIC, sizeof(FILE_MAGIC));
    out[4] = FT02_VOICE_PROTOCOL_VERSION;
    out[5] = header.codec;
    out[6] = header.frameBytes;
    out[7] = 0u;
    put16(out + 8u, header.sampleRate);
    put16(out + 10u, header.frameSamples);
    put32(out + 12u, header.durationMs);
    put32(out + 16u, header.payloadBytes);
    put32(out + 20u, header.payloadCrc32);
    return true;
}

bool FT02_MeshVoiceParseFileHeader(const uint8_t* data,
                                   size_t len,
                                   FT02MeshVoiceFileHeader& out)
{
    memset(&out, 0, sizeof(out));
    if(data == nullptr || len < FT02_VOICE_FILE_HEADER_BYTES ||
       memcmp(data, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0 ||
       data[4] != FT02_VOICE_PROTOCOL_VERSION) return false;
    out.codec = data[5];
    out.frameBytes = data[6];
    out.sampleRate = get16(data + 8u);
    out.frameSamples = get16(data + 10u);
    out.durationMs = get32(data + 12u);
    out.payloadBytes = get32(data + 16u);
    out.payloadCrc32 = get32(data + 20u);
    return supportedCodec(out.codec, out.frameBytes) &&
           out.sampleRate == FT02_MeshVoiceSampleRateForCodec(out.codec) &&
           out.frameSamples == FT02_MeshVoiceFrameSamplesForCodec(out.codec) &&
           out.durationMs > 0u && out.durationMs <= FT02_VOICE_MAX_DURATION_MS &&
           out.payloadBytes > 0u && out.payloadBytes <= FT02_VOICE_MAX_ENCODED_BYTES;
}

bool FT02_MeshVoiceBuildBegin(uint32_t voiceId,
                              uint8_t codec,
                              uint8_t frameBytes,
                              uint16_t durationMs,
                              uint16_t totalBytes,
                              uint8_t totalChunks,
                              uint32_t payloadCrc32,
                              uint8_t* out,
                              size_t maxLen,
                              size_t& outLen)
{
    outLen = 0u;
    constexpr size_t N = 22u;
    if(!supportedCodec(codec, frameBytes) ||
       durationMs == 0u || durationMs > FT02_VOICE_MAX_DURATION_MS ||
       totalBytes == 0u || totalBytes > FT02_VOICE_MAX_ENCODED_BYTES ||
       totalChunks == 0u || totalChunks > FT02_VOICE_MAX_CHUNKS ||
       totalChunks != static_cast<uint8_t>((totalBytes + FT02_VOICE_CHUNK_BYTES - 1u) / FT02_VOICE_CHUNK_BYTES) ||
       !beginPacket(FT02_VOICE_PKT_BEGIN, voiceId, out, maxLen, N)) return false;
    out[10] = codec;
    out[11] = frameBytes;
    put16(out + 12u, durationMs);
    put16(out + 14u, totalBytes);
    out[16] = totalChunks;
    out[17] = 0u;
    put32(out + 18u, payloadCrc32);
    outLen = N;
    return true;
}

bool FT02_MeshVoiceBuildChunk(uint32_t voiceId,
                              uint8_t chunkIndex,
                              const uint8_t* chunk,
                              uint8_t chunkLen,
                              uint8_t* out,
                              size_t maxLen,
                              size_t& outLen)
{
    outLen = 0u;
    const size_t needed = 12u + chunkLen;
    if(chunk == nullptr || chunkLen == 0u || chunkLen > FT02_VOICE_CHUNK_BYTES ||
       chunkIndex >= FT02_VOICE_MAX_CHUNKS || needed > FT02_VOICE_WIRE_MAX_BYTES ||
       !beginPacket(FT02_VOICE_PKT_CHUNK, voiceId, out, maxLen, needed)) return false;
    out[10] = chunkIndex;
    out[11] = chunkLen;
    memcpy(out + 12u, chunk, chunkLen);
    outLen = needed;
    return true;
}

bool FT02_MeshVoiceBuildSimple(FT02MeshVoicePacketType type,
                               uint32_t voiceId,
                               uint8_t* out,
                               size_t maxLen,
                               size_t& outLen)
{
    outLen = 0u;
    if(type != FT02_VOICE_PKT_END && type != FT02_VOICE_PKT_ACK) return false;
    if(!beginPacket(type, voiceId, out, maxLen, COMMON_BYTES)) return false;
    outLen = COMMON_BYTES;
    return true;
}

bool FT02_MeshVoiceBuildNack(uint32_t voiceId,
                             uint64_t missingMask,
                             uint8_t nackWindow,
                             uint8_t round,
                             uint8_t* out,
                             size_t maxLen,
                             size_t& outLen)
{
    outLen = 0u;
    constexpr size_t N = 20u;
    if(missingMask == 0u || nackWindow >= FT02_VOICE_NACK_WINDOWS ||
       !beginPacket(FT02_VOICE_PKT_NACK, voiceId, out, maxLen, N)) return false;
    put64(out + 10u, missingMask);
    out[18] = nackWindow;
    out[19] = round;
    outLen = N;
    return true;
}

bool FT02_MeshVoiceBuildCancel(uint32_t voiceId,
                               uint8_t reason,
                               uint8_t* out,
                               size_t maxLen,
                               size_t& outLen)
{
    outLen = 0u;
    constexpr size_t N = 11u;
    if(!beginPacket(FT02_VOICE_PKT_CANCEL, voiceId, out, maxLen, N)) return false;
    out[10] = reason;
    outLen = N;
    return true;
}

bool FT02_MeshVoiceParsePacket(const uint8_t* data,
                               size_t len,
                               FT02MeshVoicePacketView& out)
{
    memset(&out, 0, sizeof(out));
    if(data == nullptr || len < COMMON_BYTES ||
       memcmp(data, WIRE_MAGIC, sizeof(WIRE_MAGIC)) != 0 ||
       data[4] != FT02_VOICE_PROTOCOL_VERSION) return false;
    const uint8_t rawType = data[5];
    if(rawType < FT02_VOICE_PKT_BEGIN || rawType > FT02_VOICE_PKT_CANCEL) return false;
    out.type = static_cast<FT02MeshVoicePacketType>(rawType);
    out.voiceId = get32(data + 6u);
    if(out.voiceId == 0u) return false;

    switch(out.type)
    {
        case FT02_VOICE_PKT_BEGIN:
            if(len != 22u) return false;
            out.codec = data[10];
            out.frameBytes = data[11];
            out.durationMs = get16(data + 12u);
            out.totalBytes = get16(data + 14u);
            out.totalChunks = data[16];
            out.payloadCrc32 = get32(data + 18u);
            if(!supportedCodec(out.codec, out.frameBytes) ||
               out.durationMs == 0u || out.durationMs > FT02_VOICE_MAX_DURATION_MS ||
               out.totalBytes == 0u || out.totalBytes > FT02_VOICE_MAX_ENCODED_BYTES ||
               out.totalChunks == 0u || out.totalChunks > FT02_VOICE_MAX_CHUNKS ||
               out.totalChunks != static_cast<uint8_t>((out.totalBytes + FT02_VOICE_CHUNK_BYTES - 1u) / FT02_VOICE_CHUNK_BYTES)) return false;
            break;
        case FT02_VOICE_PKT_CHUNK:
            if(len < 13u) return false;
            out.chunkIndex = data[10];
            out.chunkLen = data[11];
            if(out.chunkIndex >= FT02_VOICE_MAX_CHUNKS || out.chunkLen == 0u ||
               out.chunkLen > FT02_VOICE_CHUNK_BYTES || len != 12u + out.chunkLen) return false;
            out.chunkData = data + 12u;
            break;
        case FT02_VOICE_PKT_END:
        case FT02_VOICE_PKT_ACK:
            if(len != COMMON_BYTES) return false;
            break;
        case FT02_VOICE_PKT_NACK:
            // Backward compatible: 15 B = legacy 32-bit window 0,
            // 19 B = legacy 64-bit window 0, 20 B = segmented 64-bit window.
            if(len == 15u)
            {
                out.missingMask = static_cast<uint64_t>(get32(data + 10u));
                out.nackWindow = 0u;
                out.round = data[14];
            }
            else if(len == 19u)
            {
                out.missingMask = get64(data + 10u);
                out.nackWindow = 0u;
                out.round = data[18];
            }
            else if(len == 20u)
            {
                out.missingMask = get64(data + 10u);
                out.nackWindow = data[18];
                out.round = data[19];
            }
            else return false;
            if(out.missingMask == 0u || out.nackWindow >= FT02_VOICE_NACK_WINDOWS) return false;
            break;
        case FT02_VOICE_PKT_CANCEL:
            if(len != 11u) return false;
            out.reason = data[10];
            break;
        default:
            return false;
    }
    out.valid = true;
    return true;
}
