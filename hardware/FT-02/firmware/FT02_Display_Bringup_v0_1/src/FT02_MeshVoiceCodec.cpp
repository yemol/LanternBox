#include "FT02_MeshVoiceCodec.h"

#include "FT02_Log.h"
#include "FT02_MeshVoiceProtocol.h"
#include "FT02_Storage.h"

#include <Arduino.h>
#include <codec2.h>
#include <gsm.h>
#include <opus.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#endif

namespace
{
constexpr uint32_t FT02_CODEC_TRACE_MAGIC = 0x43325452u; // "C2TR"
enum FT02CodecTracePhase : uint32_t
{
    FT02_CODEC_TRACE_NONE = 0u,
    FT02_CODEC_TRACE_ENTER = 1u,
    FT02_CODEC_TRACE_SCRATCH_ALLOC = 2u,
    FT02_CODEC_TRACE_WAV_OPEN = 3u,
    FT02_CODEC_TRACE_WAV_INFO = 4u,
    FT02_CODEC_TRACE_CREATE_BEGIN = 5u,
    FT02_CODEC_TRACE_CREATE_DONE = 6u,
    FT02_CODEC_TRACE_FRAME_BEGIN = 7u,
    FT02_CODEC_TRACE_FRAME_DONE = 8u,
    FT02_CODEC_TRACE_DESTROY_BEGIN = 9u,
    FT02_CODEC_TRACE_DESTROY_DONE = 10u,
    FT02_CODEC_TRACE_LBV_WRITE_BEGIN = 11u,
    FT02_CODEC_TRACE_LBV_WRITE_DONE = 12u,
    FT02_CODEC_TRACE_COMPLETE = 13u
};

#if defined(ARDUINO_ARCH_ESP32)
RTC_NOINIT_ATTR uint32_t g_ft02CodecTraceMagic;
RTC_NOINIT_ATTR uint32_t g_ft02CodecTracePhase;
RTC_NOINIT_ATTR uint32_t g_ft02CodecTraceDetail;
#else
uint32_t g_ft02CodecTraceMagic = 0u;
uint32_t g_ft02CodecTracePhase = 0u;
uint32_t g_ft02CodecTraceDetail = 0u;
#endif

static const char* codecTraceText(uint32_t phase)
{
    switch(phase)
    {
        case FT02_CODEC_TRACE_ENTER: return "ENTER";
        case FT02_CODEC_TRACE_SCRATCH_ALLOC: return "SCRATCH_ALLOC";
        case FT02_CODEC_TRACE_WAV_OPEN: return "WAV_OPEN";
        case FT02_CODEC_TRACE_WAV_INFO: return "WAV_INFO";
        case FT02_CODEC_TRACE_CREATE_BEGIN: return "CREATE_BEGIN";
        case FT02_CODEC_TRACE_CREATE_DONE: return "CREATE_DONE";
        case FT02_CODEC_TRACE_FRAME_BEGIN: return "FRAME_BEGIN";
        case FT02_CODEC_TRACE_FRAME_DONE: return "FRAME_DONE";
        case FT02_CODEC_TRACE_DESTROY_BEGIN: return "DESTROY_BEGIN";
        case FT02_CODEC_TRACE_DESTROY_DONE: return "DESTROY_DONE";
        case FT02_CODEC_TRACE_LBV_WRITE_BEGIN: return "LBV_WRITE_BEGIN";
        case FT02_CODEC_TRACE_LBV_WRITE_DONE: return "LBV_WRITE_DONE";
        case FT02_CODEC_TRACE_COMPLETE: return "COMPLETE";
        default: return "UNKNOWN";
    }
}

static inline void codecTrace(uint32_t phase, uint32_t detail = 0u)
{
    g_ft02CodecTraceMagic = FT02_CODEC_TRACE_MAGIC;
    g_ft02CodecTracePhase = phase;
    g_ft02CodecTraceDetail = detail;
}

static uint32_t codecStackHwm()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint32_t>(uxTaskGetStackHighWaterMark(nullptr));
#else
    return 0u;
#endif
}

static uint32_t codecInternalFree()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    return 0u;
#endif
}

static uint32_t codecInternalLargest()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint32_t>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
    return 0u;
#endif
}

static uint32_t codecFreeHeap()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint32_t>(ESP.getFreeHeap());
#else
    return 0u;
#endif
}

static uint32_t codecFreePsram()
{
#if defined(ARDUINO_ARCH_ESP32)
    return static_cast<uint32_t>(ESP.getFreePsram());
#else
    return 0u;
#endif
}

static void codecSerialFlush()
{
#if defined(ARDUINO_ARCH_ESP32)
    Serial.flush();
#endif
}

constexpr uint32_t CAPTURE_RATE = 44100u;
constexpr uint16_t PCM_BITS = 16u;
constexpr uint16_t PCM_MONO = 1u;
constexpr size_t TX_CODEC_SAMPLES = FT02_VOICE_FRAME_SAMPLES_OPUS;
constexpr size_t TX_CODEC_BYTES = FT02_VOICE_FRAME_BYTES_OPUS;
constexpr uint32_t TX_CODEC_RATE = FT02_VOICE_SAMPLE_RATE_OPUS;
constexpr size_t TX_CAPTURE_PER_CODEC_FRAME =
    (static_cast<size_t>(CAPTURE_RATE) * TX_CODEC_SAMPLES) / TX_CODEC_RATE; // Opus: 2646 samples / 60 ms
constexpr size_t MAX_CAPTURE_PER_CODEC_FRAME = 2646u; // Opus 60 ms @44.1 kHz dominates legacy modes
constexpr size_t MAX_CODEC_SAMPLES = 960u;           // Opus 60 ms @16 kHz

struct CodecScratch
{
    uint8_t* block = nullptr;
    int16_t* captureFrame = nullptr;
    int16_t* codecPcm = nullptr;
    int16_t* playFrame = nullptr;
    uint8_t* encoded = nullptr;
};

constexpr size_t CAPTURE_FRAME_BYTES = MAX_CAPTURE_PER_CODEC_FRAME * sizeof(int16_t);
constexpr size_t CODEC_PCM_BYTES = MAX_CODEC_SAMPLES * sizeof(int16_t);
constexpr size_t PLAY_FRAME_BYTES = MAX_CAPTURE_PER_CODEC_FRAME * sizeof(int16_t);
constexpr size_t CODEC_SCRATCH_BYTES =
    CAPTURE_FRAME_BYTES + CODEC_PCM_BYTES + PLAY_FRAME_BYTES + FT02_VOICE_MAX_ENCODED_BYTES;

bool allocScratch(CodecScratch& scratch)
{
#if defined(ARDUINO_ARCH_ESP32)
    scratch.block = static_cast<uint8_t*>(heap_caps_malloc(
        CODEC_SCRATCH_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    scratch.block = static_cast<uint8_t*>(malloc(CODEC_SCRATCH_BYTES));
#endif
    if(scratch.block == nullptr)
    {
        FT02_LOGE("[ERROR][MESH-VOICE-CODEC] PSRAM scratch alloc failed bytes=%u\n",
                  static_cast<unsigned>(CODEC_SCRATCH_BYTES));
        return false;
    }
    uint8_t* p = scratch.block;
    scratch.captureFrame = reinterpret_cast<int16_t*>(p); p += CAPTURE_FRAME_BYTES;
    scratch.codecPcm = reinterpret_cast<int16_t*>(p); p += CODEC_PCM_BYTES;
    scratch.playFrame = reinterpret_cast<int16_t*>(p); p += PLAY_FRAME_BYTES;
    scratch.encoded = p;
    return true;
}


bool codecRuntimeHeadroom(const char* stage)
{
#if defined(ARDUINO_ARCH_ESP32)
    const UBaseType_t stackFree = uxTaskGetStackHighWaterMark(nullptr);
    FT02_LOGI("[MESH-VOICE-CODEC] %s stack_hwm=%u heap=%lu psram=%lu\n",
              stage != nullptr ? stage : "state",
              static_cast<unsigned>(stackFree),
              static_cast<unsigned long>(codecFreeHeap()),
              static_cast<unsigned long>(codecFreePsram()));
    // Do not enter a speech codec with an already-starved loop task. The FT-02
    // build keeps the larger task stack proven during Codec2 bring-up; <4 KiB
    // here indicates a regression and should fail safely instead of resetting.
    return stackFree >= 4096u;
#else
    (void)stage;
    return true;
#endif
}

void freeScratch(CodecScratch& scratch)
{
    if(scratch.block != nullptr)
    {
#if defined(ARDUINO_ARCH_ESP32)
        heap_caps_free(scratch.block);
#else
        free(scratch.block);
#endif
    }
    scratch = CodecScratch{};
}

uint16_t readLe16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8u);
}

uint32_t readLe32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8u) |
           (static_cast<uint32_t>(p[2]) << 16u) |
           (static_cast<uint32_t>(p[3]) << 24u);
}

void writeLe16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8u) & 0xFFu);
}

void writeLe32(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8u) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16u) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24u) & 0xFFu);
}

struct WavInfo
{
    uint16_t channels;
    uint16_t bits;
    uint32_t rate;
    uint32_t dataOffset;
    uint32_t dataBytes;
};

bool readWavInfo(FILE* f, WavInfo& out)
{
    memset(&out, 0, sizeof(out));
    if(f == nullptr || fseek(f, 0, SEEK_SET) != 0) return false;
    uint8_t root[12] = {};
    if(fread(root, 1u, sizeof(root), f) != sizeof(root) ||
       memcmp(root, "RIFF", 4u) != 0 || memcmp(root + 8u, "WAVE", 4u) != 0) return false;

    bool haveFmt = false;
    bool haveData = false;
    while(!haveData)
    {
        uint8_t ch[8] = {};
        if(fread(ch, 1u, sizeof(ch), f) != sizeof(ch)) break;
        const uint32_t size = readLe32(ch + 4u);
        const long body = ftell(f);
        if(body < 0) return false;
        if(memcmp(ch, "fmt ", 4u) == 0)
        {
            if(size < 16u) return false;
            uint8_t fmt[16] = {};
            if(fread(fmt, 1u, sizeof(fmt), f) != sizeof(fmt)) return false;
            if(readLe16(fmt) != 1u) return false; // PCM only.
            out.channels = readLe16(fmt + 2u);
            out.rate = readLe32(fmt + 4u);
            out.bits = readLe16(fmt + 14u);
            haveFmt = true;
        }
        else if(memcmp(ch, "data", 4u) == 0)
        {
            out.dataOffset = static_cast<uint32_t>(body);
            out.dataBytes = size;
            haveData = true;
        }
        const long next = body + static_cast<long>(size + (size & 1u));
        if(fseek(f, next, SEEK_SET) != 0) return false;
    }
    return haveFmt && haveData && out.channels == PCM_MONO && out.bits == PCM_BITS && out.rate == CAPTURE_RATE;
}

bool writeWavHeader(FILE* f, uint32_t dataBytes)
{
    if(f == nullptr) return false;
    uint8_t h[44] = {};
    memcpy(h + 0u, "RIFF", 4u);
    writeLe32(h + 4u, 36u + dataBytes);
    memcpy(h + 8u, "WAVEfmt ", 8u);
    writeLe32(h + 16u, 16u);
    writeLe16(h + 20u, 1u);
    writeLe16(h + 22u, PCM_MONO);
    writeLe32(h + 24u, CAPTURE_RATE);
    writeLe32(h + 28u, CAPTURE_RATE * 2u);
    writeLe16(h + 32u, 2u);
    writeLe16(h + 34u, PCM_BITS);
    memcpy(h + 36u, "data", 4u);
    writeLe32(h + 40u, dataBytes);
    return fwrite(h, 1u, sizeof(h), f) == sizeof(h);
}

void downsampleFrame(const int16_t* src, size_t captureSamples, int16_t* dst, size_t codecSamples)
{
    // Box-average each 44.1 kHz interval that maps to one codec-rate sample.
    for(size_t j = 0u; j < codecSamples; ++j)
    {
        size_t a = (j * captureSamples) / codecSamples;
        size_t b = ((j + 1u) * captureSamples) / codecSamples;
        if(b <= a) b = a + 1u;
        if(b > captureSamples) b = captureSamples;
        int64_t sum = 0;
        for(size_t i = a; i < b; ++i) sum += src[i];
        dst[j] = static_cast<int16_t>(sum / static_cast<int64_t>(b - a));
    }
}

void upsampleFrame(const int16_t* src, size_t codecSamples, int16_t* dst, size_t captureSamples)
{
    // Linear interpolation from the codec rate back to the established 44.1 kHz path.
    for(size_t i = 0u; i < captureSamples; ++i)
    {
        const uint64_t posNum = static_cast<uint64_t>(i) * static_cast<uint64_t>(codecSamples - 1u);
        const uint32_t denom = captureSamples > 1u ? static_cast<uint32_t>(captureSamples - 1u) : 1u;
        const size_t index = static_cast<size_t>(posNum / denom);
        const uint32_t frac = static_cast<uint32_t>(posNum % denom);
        if(index >= codecSamples - 1u)
        {
            dst[i] = src[codecSamples - 1u];
            continue;
        }
        const int32_t a = src[index];
        const int32_t b = src[index + 1u];
        dst[i] = static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(frac)) / static_cast<int32_t>(denom));
    }
}

void* codecPsramAlloc(size_t bytes)
{
#if defined(ARDUINO_ARCH_ESP32)
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return malloc(bytes);
#endif
}

void codecPsramFree(void* p)
{
    if(p == nullptr) return;
#if defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(p);
#else
    free(p);
#endif
}

int codecModeForId(uint8_t codecId)
{
    if(codecId == FT02_VOICE_CODEC_CODEC2_3200) return CODEC2_MODE_3200;
    if(codecId == FT02_VOICE_CODEC_CODEC2_1600) return CODEC2_MODE_1600;
    if(codecId == FT02_VOICE_CODEC_CODEC2_1300) return CODEC2_MODE_1300;
    return -1;
}
}

bool FT02_MeshVoiceEncodeWavToLbv(const char* wavPath,
                                  const char* lbvPath,
                                  uint32_t trimLeadMs,
                                  uint32_t& durationMs,
                                  uint32_t& encodedBytes)
{
    durationMs = 0u;
    encodedBytes = 0u;
    if(wavPath == nullptr || lbvPath == nullptr || !FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;
    if(!codecRuntimeHeadroom("encode-begin")) return false;
    codecTrace(FT02_CODEC_TRACE_ENTER, codecStackHwm());
    CodecScratch scratch{};
    if(!allocScratch(scratch)) return false;
    codecTrace(FT02_CODEC_TRACE_SCRATCH_ALLOC, static_cast<uint32_t>(CODEC_SCRATCH_BYTES));

    FILE* in = FT02_StorageOpenReadFile(wavPath);
    if(in == nullptr) { freeScratch(scratch); return false; }
    codecTrace(FT02_CODEC_TRACE_WAV_OPEN, 0u);
    WavInfo info{};
    bool ok = readWavInfo(in, info);
    const uint32_t rawSamples = ok ? info.dataBytes / sizeof(int16_t) : 0u;
    // trimLeadMs is captured after the START SPEAKING partial refresh has
    // completed, so everything recorded before the cue is removed. Keep a
    // bounded fallback so corrupt state cannot discard an arbitrary recording.
    if(trimLeadMs < FT02_VOICE_CAPTURE_PREROLL_MS) trimLeadMs = FT02_VOICE_CAPTURE_PREROLL_MS;
    if(trimLeadMs > FT02_VOICE_CAPTURE_MAX_PREROLL_MS) trimLeadMs = FT02_VOICE_CAPTURE_MAX_PREROLL_MS;
    const uint32_t skipSamples = static_cast<uint32_t>(
        (static_cast<uint64_t>(CAPTURE_RATE) * trimLeadMs) / 1000u);
    const uint32_t maxTxSamples = static_cast<uint32_t>(
        (static_cast<uint64_t>(CAPTURE_RATE) * FT02_VOICE_TX_MAX_DURATION_MS) / 1000u);
    const uint32_t maxRawSamples = static_cast<uint32_t>(
        (static_cast<uint64_t>(CAPTURE_RATE) * FT02_VOICE_CAPTURE_RAW_MAX_MS) / 1000u);
    if(!ok || rawSamples <= skipSamples || rawSamples > maxRawSamples)
    {
        fclose(in);
        freeScratch(scratch);
        return false;
    }
    codecTrace(FT02_CODEC_TRACE_WAV_INFO, rawSamples);

    uint32_t samples = rawSamples - skipSamples;
    if(samples > maxTxSamples) samples = maxTxSamples;
    durationMs = static_cast<uint32_t>((static_cast<uint64_t>(samples) * 1000u) / CAPTURE_RATE);
    if(durationMs == 0u || durationMs > FT02_VOICE_TX_MAX_DURATION_MS)
    {
        fclose(in);
        freeScratch(scratch);
        return false;
    }
    const uint32_t frames = (samples + TX_CAPTURE_PER_CODEC_FRAME - 1u) / TX_CAPTURE_PER_CODEC_FRAME;
    if(frames == 0u || static_cast<size_t>(frames) * TX_CODEC_BYTES > FT02_VOICE_TX_MAX_ENCODED_BYTES)
    {
        fclose(in);
        freeScratch(scratch);
        return false;
    }

    const int encoderBytes = opus_encoder_get_size(1);
    if(encoderBytes <= 0)
    {
        fclose(in);
        freeScratch(scratch);
        return false;
    }
    codecTrace(FT02_CODEC_TRACE_CREATE_BEGIN, static_cast<uint32_t>(encoderBytes));
    FT02_LOGI("[MESH-VOICE-CODEC] Opus create begin state=%dB internal_free=%lu largest=%lu heap=%lu psram=%lu stack_hwm=%u\n",
              encoderBytes,
              static_cast<unsigned long>(codecInternalFree()),
              static_cast<unsigned long>(codecInternalLargest()),
              static_cast<unsigned long>(codecFreeHeap()),
              static_cast<unsigned long>(codecFreePsram()),
              static_cast<unsigned>(codecStackHwm()));
    codecSerialFlush();

    OpusEncoder* encoder = static_cast<OpusEncoder*>(codecPsramAlloc(static_cast<size_t>(encoderBytes)));
    if(encoder == nullptr)
    {
        fclose(in);
        freeScratch(scratch);
        return false;
    }
    int rc = opus_encoder_init(encoder, FT02_VOICE_SAMPLE_RATE_OPUS, 1, OPUS_APPLICATION_VOIP);
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(6000));
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_VBR(0));
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(7));
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(0));
    if(rc == OPUS_OK) rc = opus_encoder_ctl(encoder, OPUS_SET_DTX(0));
    codecTrace(FT02_CODEC_TRACE_CREATE_DONE, rc == OPUS_OK ? 1u : 0u);
    if(rc != OPUS_OK)
    {
        FT02_LOGE("[ERROR][MESH-VOICE-CODEC] Opus encoder init/ctl failed rc=%d\n", rc);
        codecPsramFree(encoder);
        fclose(in);
        freeScratch(scratch);
        return false;
    }

    const uint32_t dataStart = info.dataOffset + skipSamples * sizeof(int16_t);
    if(fseek(in, static_cast<long>(dataStart), SEEK_SET) != 0)
    {
        codecPsramFree(encoder);
        fclose(in);
        freeScratch(scratch);
        return false;
    }

    uint32_t left = samples;
    size_t outOffset = 0u;
    for(uint32_t frame = 0u; frame < frames; ++frame)
    {
        memset(scratch.captureFrame, 0, CAPTURE_FRAME_BYTES);
        const size_t want = left > TX_CAPTURE_PER_CODEC_FRAME ? TX_CAPTURE_PER_CODEC_FRAME : left;
        const size_t got = fread(scratch.captureFrame, sizeof(int16_t), want, in);
        if(got != want) { ok = false; break; }
        left -= static_cast<uint32_t>(got);
        downsampleFrame(scratch.captureFrame, TX_CAPTURE_PER_CODEC_FRAME, scratch.codecPcm, TX_CODEC_SAMPLES);
        codecTrace(FT02_CODEC_TRACE_FRAME_BEGIN, frame);
        const int packetBytes = opus_encode(encoder,
                                            reinterpret_cast<const opus_int16*>(scratch.codecPcm),
                                            static_cast<int>(TX_CODEC_SAMPLES),
                                            scratch.encoded + outOffset,
                                            static_cast<opus_int32>(TX_CODEC_BYTES));
        codecTrace(FT02_CODEC_TRACE_FRAME_DONE, frame);
        if(packetBytes != static_cast<int>(TX_CODEC_BYTES))
        {
            FT02_LOGE("[ERROR][MESH-VOICE-CODEC] Opus frame=%lu expected=%u got=%d\n",
                      static_cast<unsigned long>(frame),
                      static_cast<unsigned>(TX_CODEC_BYTES),
                      packetBytes);
            ok = false;
            break;
        }
        outOffset += static_cast<size_t>(packetBytes);
        yield();
    }
    codecTrace(FT02_CODEC_TRACE_DESTROY_BEGIN, frames);
    codecPsramFree(encoder);
    codecTrace(FT02_CODEC_TRACE_DESTROY_DONE, frames);
    fclose(in);
    if(!ok || left != 0u)
    {
        freeScratch(scratch);
        return false;
    }

    encodedBytes = static_cast<uint32_t>(outOffset);
    FT02MeshVoiceFileHeader vh{};
    vh.codec = FT02_VOICE_CODEC_OPUS_6000;
    vh.frameBytes = FT02_VOICE_FRAME_BYTES_OPUS_6000;
    vh.sampleRate = FT02_VOICE_SAMPLE_RATE_OPUS;
    vh.frameSamples = FT02_VOICE_FRAME_SAMPLES_OPUS;
    vh.durationMs = durationMs;
    vh.payloadBytes = encodedBytes;
    vh.payloadCrc32 = FT02_MeshVoiceCrc32(scratch.encoded, outOffset);
    uint8_t fileHeader[FT02_VOICE_FILE_HEADER_BYTES] = {};
    if(!FT02_MeshVoiceBuildFileHeader(vh, fileHeader)) { freeScratch(scratch); return false; }

    codecTrace(FT02_CODEC_TRACE_LBV_WRITE_BEGIN, encodedBytes);
    FILE* out = FT02_StorageOpenWriteFile(lbvPath, true);
    if(out == nullptr) { freeScratch(scratch); return false; }
    ok = fwrite(fileHeader, 1u, sizeof(fileHeader), out) == sizeof(fileHeader) &&
         fwrite(scratch.encoded, 1u, outOffset, out) == outOffset &&
         FT02_StorageSyncFile(out);
    if(fclose(out) != 0) ok = false;
    if(!ok) (void)FT02_StorageDeleteFile(lbvPath);
    codecTrace(FT02_CODEC_TRACE_LBV_WRITE_DONE, encodedBytes);
    FT02_LOGI("[MESH-VOICE-CODEC] encode opus-6000 %s raw=%lums trim=%lums duration=%lums frames=%lu bytes=%lu chunks~%lu %s\n",
              wavPath,
              static_cast<unsigned long>((static_cast<uint64_t>(rawSamples) * 1000u) / CAPTURE_RATE),
              static_cast<unsigned long>(trimLeadMs),
              static_cast<unsigned long>(durationMs),
              static_cast<unsigned long>(frames),
              static_cast<unsigned long>(encodedBytes),
              static_cast<unsigned long>((encodedBytes + FT02_VOICE_CHUNK_BYTES - 1u) / FT02_VOICE_CHUNK_BYTES),
              ok ? "OK" : "FAIL");
    freeScratch(scratch);
    (void)codecRuntimeHeadroom("encode-end");
    codecTrace(FT02_CODEC_TRACE_COMPLETE, encodedBytes);
    if(ok) FT02_MeshVoiceCodecClearResetTrace();
    return ok;
}

bool FT02_MeshVoiceDecodeLbvToWav(const char* lbvPath,
                                  const char* wavPath,
                                  uint32_t& durationMs)
{
    durationMs = 0u;
    if(lbvPath == nullptr || wavPath == nullptr || !FT02_StorageIsReady() || FT02_StorageSleepSuspended()) return false;
    if(!codecRuntimeHeadroom("decode-begin")) return false;
    CodecScratch scratch{};
    if(!allocScratch(scratch)) return false;
    FILE* in = FT02_StorageOpenReadFile(lbvPath);
    if(in == nullptr) { freeScratch(scratch); return false; }
    uint8_t rawHeader[FT02_VOICE_FILE_HEADER_BYTES] = {};
    FT02MeshVoiceFileHeader header{};
    bool ok = fread(rawHeader, 1u, sizeof(rawHeader), in) == sizeof(rawHeader) &&
              FT02_MeshVoiceParseFileHeader(rawHeader, sizeof(rawHeader), header) &&
              header.payloadBytes <= FT02_VOICE_MAX_ENCODED_BYTES &&
              header.frameSamples == FT02_MeshVoiceFrameSamplesForCodec(header.codec) &&
              header.frameBytes > 0u &&
              (header.payloadBytes % header.frameBytes) == 0u;
    if(ok) ok = fread(scratch.encoded, 1u, header.payloadBytes, in) == header.payloadBytes;
    fclose(in);
    if(!ok || FT02_MeshVoiceCrc32(scratch.encoded, header.payloadBytes) != header.payloadCrc32)
    {
        freeScratch(scratch);
        return false;
    }

    if(header.codec == FT02_VOICE_CODEC_OPUS_6000 || header.codec == FT02_VOICE_CODEC_OPUS_4800)
    {
        const int decoderBytes = opus_decoder_get_size(1);
        if(decoderBytes <= 0) { freeScratch(scratch); return false; }
        OpusDecoder* decoder = static_cast<OpusDecoder*>(codecPsramAlloc(static_cast<size_t>(decoderBytes)));
        if(decoder == nullptr) { freeScratch(scratch); return false; }
        const int initRc = opus_decoder_init(decoder, FT02_VOICE_SAMPLE_RATE_OPUS, 1);
        if(initRc != OPUS_OK)
        {
            codecPsramFree(decoder);
            freeScratch(scratch);
            return false;
        }

        durationMs = header.durationMs;
        const uint32_t targetSamples = static_cast<uint32_t>(
            (static_cast<uint64_t>(durationMs) * CAPTURE_RATE + 500u) / 1000u);
        const uint32_t totalFrames = header.payloadBytes / header.frameBytes;
        const size_t capturePerFrame =
            (static_cast<size_t>(CAPTURE_RATE) * FT02_VOICE_FRAME_SAMPLES_OPUS) / FT02_VOICE_SAMPLE_RATE_OPUS;
        const uint32_t maxSamples = totalFrames * static_cast<uint32_t>(capturePerFrame);
        const uint32_t samplesToWrite = targetSamples < maxSamples ? targetSamples : maxSamples;
        const uint32_t dataBytes = samplesToWrite * sizeof(int16_t);
        FILE* out = FT02_StorageOpenWriteFile(wavPath, true);
        if(out == nullptr)
        {
            codecPsramFree(decoder);
            freeScratch(scratch);
            return false;
        }
        ok = writeWavHeader(out, dataBytes);
        uint32_t writtenSamples = 0u;
        for(uint32_t frame = 0u; ok && frame < totalFrames && writtenSamples < samplesToWrite; ++frame)
        {
            const int decoded = opus_decode(decoder,
                                            scratch.encoded + static_cast<size_t>(frame) * header.frameBytes,
                                            header.frameBytes,
                                            reinterpret_cast<opus_int16*>(scratch.codecPcm),
                                            FT02_VOICE_FRAME_SAMPLES_OPUS,
                                            0);
            if(decoded != static_cast<int>(FT02_VOICE_FRAME_SAMPLES_OPUS)) { ok = false; break; }
            upsampleFrame(scratch.codecPcm, FT02_VOICE_FRAME_SAMPLES_OPUS,
                          scratch.playFrame, capturePerFrame);
            const uint32_t remain = samplesToWrite - writtenSamples;
            const uint32_t writeSamples = remain < capturePerFrame ? remain : static_cast<uint32_t>(capturePerFrame);
            ok = fwrite(scratch.playFrame, sizeof(int16_t), writeSamples, out) == writeSamples;
            writtenSamples += writeSamples;
            yield();
        }
        if(ok) ok = writtenSamples == samplesToWrite && FT02_StorageSyncFile(out);
        if(fclose(out) != 0) ok = false;
        codecPsramFree(decoder);
        if(!ok) (void)FT02_StorageDeleteFile(wavPath);
        FT02_LOGI("[MESH-VOICE-CODEC] decode opus-%s %s duration=%lums frames=%lu %s\n",
                  header.codec == FT02_VOICE_CODEC_OPUS_4800 ? "4800" : "6000",
                  lbvPath, static_cast<unsigned long>(durationMs),
                  static_cast<unsigned long>(totalFrames), ok ? "OK" : "FAIL");
        freeScratch(scratch);
        (void)codecRuntimeHeadroom("decode-end");
        return ok;
    }

    if(header.codec == FT02_VOICE_CODEC_GSM_FR)
    {
        gsm state = gsm_create();
        if(state == nullptr) { freeScratch(scratch); return false; }
        durationMs = header.durationMs;
        const uint32_t targetSamples = static_cast<uint32_t>((static_cast<uint64_t>(durationMs) * CAPTURE_RATE + 500u) / 1000u);
        const uint32_t totalFrames = header.payloadBytes / FT02_VOICE_FRAME_BYTES_GSM_FR;
        const size_t capturePerFrame = (static_cast<size_t>(CAPTURE_RATE) * FT02_VOICE_FRAME_SAMPLES_GSM_FR) / FT02_VOICE_SAMPLE_RATE;
        const uint32_t maxSamples = totalFrames * static_cast<uint32_t>(capturePerFrame);
        const uint32_t samplesToWrite = targetSamples < maxSamples ? targetSamples : maxSamples;
        const uint32_t dataBytes = samplesToWrite * sizeof(int16_t);
        FILE* out = FT02_StorageOpenWriteFile(wavPath, true);
        if(out == nullptr) { gsm_destroy(state); freeScratch(scratch); return false; }
        ok = writeWavHeader(out, dataBytes);
        uint32_t writtenSamples = 0u;
        for(uint32_t frame = 0u; ok && frame < totalFrames && writtenSamples < samplesToWrite; ++frame)
        {
            const int rc = gsm_decode(state,
                                      reinterpret_cast<gsm_byte*>(scratch.encoded + static_cast<size_t>(frame) * FT02_VOICE_FRAME_BYTES_GSM_FR),
                                      reinterpret_cast<gsm_signal*>(scratch.codecPcm));
            if(rc != 0) { ok = false; break; }
            upsampleFrame(scratch.codecPcm, FT02_VOICE_FRAME_SAMPLES_GSM_FR, scratch.playFrame, capturePerFrame);
            const uint32_t remain = samplesToWrite - writtenSamples;
            const uint32_t writeSamples = remain < capturePerFrame ? remain : static_cast<uint32_t>(capturePerFrame);
            ok = fwrite(scratch.playFrame, sizeof(int16_t), writeSamples, out) == writeSamples;
            writtenSamples += writeSamples;
            yield();
        }
        if(ok) ok = FT02_StorageSyncFile(out);
        if(fclose(out) != 0) ok = false;
        gsm_destroy(state);
        if(!ok || writtenSamples != samplesToWrite) (void)FT02_StorageDeleteFile(wavPath);
        FT02_LOGI("[MESH-VOICE-CODEC] decode gsm-fr %s duration=%lums frames=%lu %s\n",
                  lbvPath, static_cast<unsigned long>(durationMs), static_cast<unsigned long>(totalFrames), ok ? "OK" : "FAIL");
        freeScratch(scratch);
        (void)codecRuntimeHeadroom("decode-end");
        return ok;
    }

    codecTrace(FT02_CODEC_TRACE_CREATE_BEGIN, codecInternalLargest());
    FT02_LOGI("[MESH-VOICE-CODEC] create begin internal_free=%lu largest=%lu heap=%lu psram=%lu stack_hwm=%u\n",
              static_cast<unsigned long>(codecInternalFree()),
              static_cast<unsigned long>(codecInternalLargest()),
              static_cast<unsigned long>(codecFreeHeap()),
              static_cast<unsigned long>(codecFreePsram()),
              static_cast<unsigned>(codecStackHwm()));
    codecSerialFlush();
    const int decodeMode = codecModeForId(header.codec);
    struct CODEC2* codec = decodeMode >= 0 ? codec2_create(decodeMode) : nullptr;
    if(codec == nullptr || codec2_samples_per_frame(codec) != static_cast<int>(header.frameSamples) ||
       codec2_bytes_per_frame(codec) != static_cast<int>(header.frameBytes))
    {
        if(codec != nullptr) codec2_destroy(codec);
        freeScratch(scratch);
        return false;
    }

    durationMs = header.durationMs;
    const uint32_t targetSamples = static_cast<uint32_t>((static_cast<uint64_t>(durationMs) * CAPTURE_RATE + 500u) / 1000u);
    const uint32_t totalFrames = header.payloadBytes / header.frameBytes;
    const size_t codecSamples = header.frameSamples;
    const uint16_t codecRate = FT02_MeshVoiceSampleRateForCodec(header.codec);
    if(codecRate == 0u) { codec2_destroy(codec); freeScratch(scratch); return false; }
    const size_t capturePerFrame = (static_cast<size_t>(CAPTURE_RATE) * codecSamples) / codecRate;
    const uint32_t maxSamples = totalFrames * static_cast<uint32_t>(capturePerFrame);
    const uint32_t samplesToWrite = targetSamples < maxSamples ? targetSamples : maxSamples;
    const uint32_t dataBytes = samplesToWrite * sizeof(int16_t);

    FILE* out = FT02_StorageOpenWriteFile(wavPath, true);
    if(out == nullptr) { codec2_destroy(codec); freeScratch(scratch); return false; }
    ok = writeWavHeader(out, dataBytes);
    uint32_t writtenSamples = 0u;
    for(uint32_t frame = 0u; ok && frame < totalFrames && writtenSamples < samplesToWrite; ++frame)
    {
        codec2_decode(codec, scratch.codecPcm, scratch.encoded + static_cast<size_t>(frame) * header.frameBytes);
        upsampleFrame(scratch.codecPcm, codecSamples, scratch.playFrame, capturePerFrame);
        const uint32_t remain = samplesToWrite - writtenSamples;
        const size_t n = remain > capturePerFrame ? capturePerFrame : remain;
        ok = fwrite(scratch.playFrame, sizeof(int16_t), n, out) == n;
        writtenSamples += static_cast<uint32_t>(n);
        yield();
    }
    codec2_destroy(codec);
    if(ok) ok = writtenSamples == samplesToWrite && FT02_StorageSyncFile(out);
    if(fclose(out) != 0) ok = false;
    if(!ok) (void)FT02_StorageDeleteFile(wavPath);
    FT02_LOGI("[MESH-VOICE-CODEC] decode codec=%u %s duration=%lums bytes=%lu %s\n",
              static_cast<unsigned>(header.codec), lbvPath, static_cast<unsigned long>(durationMs),
              static_cast<unsigned long>(dataBytes), ok ? "OK" : "FAIL");
    freeScratch(scratch);
    (void)codecRuntimeHeadroom("decode-end");
    return ok;
}


void FT02_MeshVoiceCodecReportResetTrace()
{
#if defined(ARDUINO_ARCH_ESP32)
    if(g_ft02CodecTraceMagic == FT02_CODEC_TRACE_MAGIC && g_ft02CodecTracePhase != FT02_CODEC_TRACE_NONE)
    {
        Serial.printf("[VOICE-CODEC-RESET-TRACE] previous phase=%lu(%s) detail=%lu\n",
                      static_cast<unsigned long>(g_ft02CodecTracePhase),
                      codecTraceText(g_ft02CodecTracePhase),
                      static_cast<unsigned long>(g_ft02CodecTraceDetail));
        Serial.flush();
    }
#endif
}

void FT02_MeshVoiceCodecClearResetTrace()
{
#if defined(ARDUINO_ARCH_ESP32)
    g_ft02CodecTraceMagic = FT02_CODEC_TRACE_MAGIC;
    g_ft02CodecTracePhase = FT02_CODEC_TRACE_NONE;
    g_ft02CodecTraceDetail = 0u;
#endif
}
