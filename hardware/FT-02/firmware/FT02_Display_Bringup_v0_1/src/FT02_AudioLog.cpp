#include "FT02_AudioLog.h"

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_WM8960_Arduino_Library.h>
#include <driver/i2s_std.h>
#include <esp_err.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "FT02_Storage.h"

namespace
{
constexpr int FT02_AUDIO_BCLK_PIN = 14;
constexpr int FT02_AUDIO_WS_PIN = 15;
constexpr int FT02_AUDIO_DOUT_PIN = 16;
constexpr int FT02_AUDIO_DIN_PIN = 17;

constexpr uint32_t FT02_AUDIO_SAMPLE_RATE = 44100;
constexpr uint16_t FT02_AUDIO_BITS_PER_SAMPLE = 16;
constexpr uint16_t FT02_AUDIO_RECORD_CHANNELS = 1;
constexpr uint32_t FT02_AUDIO_POST_ROLL_MS = 250;
constexpr uint32_t FT02_AUDIO_MIN_STOP_MS = 600;
constexpr uint32_t FT02_AUDIO_MAX_SECONDS = 600;
constexpr size_t FT02_AUDIO_FRAMES_PER_BLOCK = 512;
constexpr float FT02_AUDIO_SPEAKER_DB = 0.0f;
constexpr float FT02_AUDIO_MIC_PGA_DB = 30.0f;
constexpr float FT02_AUDIO_ADC_DIGITAL_DB = 6.0f;
constexpr float FT02_AUDIO_PLAYBACK_TARGET_RMS = 6500.0f;
constexpr float FT02_AUDIO_PLAYBACK_MAX_GAIN = 4.0f;
constexpr float FT02_AUDIO_LIMIT_THRESHOLD = 22000.0f;
constexpr float FT02_AUDIO_LIMIT_RATIO = 6.0f;
constexpr size_t FT02_AUDIO_INDEX_LINE_BYTES = 1024;
constexpr uint8_t FT02_AUDIO_PLAYBACK_BLOCKS_PER_POLL = 4;
constexpr uint8_t FT02_AUDIO_PLAYBACK_TAIL_BLOCKS = 8;
constexpr uint32_t FT02_AUDIO_PLAYBACK_TAIL_WAIT_MS = 120;
constexpr const char* FT02_AUDIO_PREF_NAMESPACE = "ft02audio";
constexpr const char* FT02_AUDIO_PREF_VOLUME_KEY = "volume";

constexpr float FT02_AUDIO_VOLUME_GAINS[FT02_AUDIO_VOLUME_MAX_LEVEL + 1] = {
    0.0f, 0.20f, 0.28f, 0.40f, 0.56f, 0.70f,
    0.85f, 1.00f, 1.20f, 1.45f, 1.70f
};

constexpr const char* FT02_AUDIO_DIR = "/lanternbox/audio";
constexpr const char* FT02_AUDIO_INDEX = "/lanternbox/audio/audio_index.jsonl";
constexpr const char* FT02_AUDIO_SEQUENCE_STATE = "/lanternbox/audio/audio_sequence.txt";
constexpr const char* FT02_AUDIO_SEQUENCE_TEMP = "/lanternbox/audio/audio_sequence.tmp";
constexpr const char* FT02_AUDIO_SEQUENCE_BACKUP = "/lanternbox/audio/audio_sequence.bak";
constexpr uint32_t FT02_AUDIO_SEQUENCE_MIN = 1u;
constexpr uint32_t FT02_AUDIO_SEQUENCE_MAX = 999999999u;

WM8960 g_ft02AudioCodec;
i2s_chan_handle_t g_ft02AudioTx = nullptr;
i2s_chan_handle_t g_ft02AudioRx = nullptr;

int16_t g_ft02AudioStereoBuffer[FT02_AUDIO_FRAMES_PER_BLOCK * 2];
int16_t g_ft02AudioPlayBuffer[FT02_AUDIO_FRAMES_PER_BLOCK * 2];
int16_t g_ft02AudioMonoBuffer[FT02_AUDIO_FRAMES_PER_BLOCK];
int16_t g_ft02AudioZeroBuffer[FT02_AUDIO_FRAMES_PER_BLOCK * 2] = {};

FT02AudioLogEntry g_ft02AudioEntries[FT02_AUDIO_LOG_MAX_ENTRIES] = {};
FT02AudioLogStatus g_ft02AudioStatus = {};
volatile FT02AudioLogState g_ft02AudioState = FT02_AUDIO_LOG_IDLE;
TaskHandle_t g_ft02AudioServiceTaskHandle = nullptr;

struct FT02AudioRecordingSession
{
    FILE* file = nullptr;
    bool success = true;
    bool stopArmed = false;
    uint32_t startMs = 0;
    uint32_t stopDeadlineMs = 0;
    uint32_t sequence = 0;
    uint32_t recordedFrames = 0;
    uint32_t dataBytes = 0;
    int32_t peak = 0;
    int16_t minSample = INT16_MAX;
    int16_t maxSample = INT16_MIN;
    int64_t sum = 0;
    uint64_t sumSq = 0;
    uint32_t clipCount = 0;
    uint32_t zeroCount = 0;
    uint32_t nextProgressMs = 1000;
    uint32_t secondFrames = 0;
    int32_t secondPeak = 0;
    uint64_t secondSumSq = 0;
    bool gnssFix = false;
    double latitude = 0.0;
    double longitude = 0.0;
    char audioId[48] = {};
    char sessionId[48] = {};
    char filePath[112] = {};
    char filename[72] = {};
    char deviceDate[16] = {};
    char deviceTime[16] = {};
};

FT02AudioRecordingSession g_ft02AudioRecording;
uint32_t g_ft02AudioRestartNotBeforeMs = 0;
uint32_t g_ft02AudioNextSequence = FT02_AUDIO_SEQUENCE_MIN;
uint32_t g_ft02AudioMaxSequenceSeen = 0;

struct FT02WavInfo
{
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t sampleRate = 0;
    uint32_t dataOffset = 0;
    uint32_t dataBytes = 0;
};

struct FT02AudioPlaybackSession
{
    FILE* file = nullptr;
    FT02WavInfo info = {};
    bool useRight = false;
    bool success = true;
    bool stopRequested = false;
    bool startReleased = false;
    bool volumeDirty = false;
    bool draining = false;
    uint8_t tailBlocksWritten = 0;
    uint32_t tailWaitDeadlineMs = 0;
    uint32_t bytesRemaining = 0;
    uint32_t playedFrames = 0;
    uint32_t totalFrames = 0;
    float normalizationGain = 1.0f;
    char audioId[48] = {};
    char filePath[112] = {};
};

FT02AudioPlaybackSession g_ft02AudioPlayback;

static uint8_t FT02_AudioLoadVolumeLevel()
{
    Preferences preferences;
    if(!preferences.begin(FT02_AUDIO_PREF_NAMESPACE, true))
    {
        return FT02_AUDIO_VOLUME_DEFAULT_LEVEL;
    }
    const uint8_t stored = preferences.getUChar(
        FT02_AUDIO_PREF_VOLUME_KEY,
        FT02_AUDIO_VOLUME_DEFAULT_LEVEL
    );
    preferences.end();
    return stored <= FT02_AUDIO_VOLUME_MAX_LEVEL
        ? stored
        : FT02_AUDIO_VOLUME_DEFAULT_LEVEL;
}

static void FT02_AudioSaveVolumeLevel(uint8_t level)
{
    Preferences preferences;
    if(!preferences.begin(FT02_AUDIO_PREF_NAMESPACE, false)) return;
    preferences.putUChar(FT02_AUDIO_PREF_VOLUME_KEY, level);
    preferences.end();
}

static float FT02_AudioUserVolumeGain(uint8_t level)
{
    if(level > FT02_AUDIO_VOLUME_MAX_LEVEL)
    {
        level = FT02_AUDIO_VOLUME_MAX_LEVEL;
    }
    return FT02_AUDIO_VOLUME_GAINS[level];
}

static void FT02_AudioSetMessage(const char* message, bool ok)
{
    snprintf(
        g_ft02AudioStatus.message,
        sizeof(g_ft02AudioStatus.message),
        "%s",
        message != nullptr ? message : ""
    );
    g_ft02AudioStatus.lastOperationOk = ok;
    Serial.print("[AUDIO-LOG] ");
    Serial.println(g_ft02AudioStatus.message);
}

static void FT02_AudioChanged()
{
    g_ft02AudioStatus.generation++;
}

static bool FT02_AudioTimeReached(uint32_t now, uint32_t deadline)
{
    return static_cast<int32_t>(now - deadline) >= 0;
}

static void FT02_AudioWriteLe16(uint8_t* output, uint16_t value)
{
    output[0] = static_cast<uint8_t>(value & 0xFFu);
    output[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

static void FT02_AudioWriteLe32(uint8_t* output, uint32_t value)
{
    output[0] = static_cast<uint8_t>(value & 0xFFu);
    output[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    output[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    output[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

static uint16_t FT02_AudioReadLe16(const uint8_t* input)
{
    return static_cast<uint16_t>(input[0]) |
           (static_cast<uint16_t>(input[1]) << 8);
}

static uint32_t FT02_AudioReadLe32(const uint8_t* input)
{
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8) |
           (static_cast<uint32_t>(input[2]) << 16) |
           (static_cast<uint32_t>(input[3]) << 24);
}

static bool FT02_AudioWriteWavHeader(FILE* file, uint32_t dataBytes)
{
    if(file == nullptr) return false;

    uint8_t header[44] = {};
    memcpy(header + 0, "RIFF", 4);
    FT02_AudioWriteLe32(header + 4, 36u + dataBytes);
    memcpy(header + 8, "WAVE", 4);
    memcpy(header + 12, "fmt ", 4);
    FT02_AudioWriteLe32(header + 16, 16u);
    FT02_AudioWriteLe16(header + 20, 1u);
    FT02_AudioWriteLe16(header + 22, FT02_AUDIO_RECORD_CHANNELS);
    FT02_AudioWriteLe32(header + 24, FT02_AUDIO_SAMPLE_RATE);
    const uint16_t blockAlign = FT02_AUDIO_RECORD_CHANNELS * (FT02_AUDIO_BITS_PER_SAMPLE / 8u);
    FT02_AudioWriteLe32(header + 28, FT02_AUDIO_SAMPLE_RATE * blockAlign);
    FT02_AudioWriteLe16(header + 32, blockAlign);
    FT02_AudioWriteLe16(header + 34, FT02_AUDIO_BITS_PER_SAMPLE);
    memcpy(header + 36, "data", 4);
    FT02_AudioWriteLe32(header + 40, dataBytes);
    return fwrite(header, 1, sizeof(header), file) == sizeof(header);
}

static bool FT02_AudioFinalizeWav(FILE* file, uint32_t dataBytes)
{
    if(file == nullptr || fseek(file, 0, SEEK_SET) != 0) return false;
    return FT02_AudioWriteWavHeader(file, dataBytes) && FT02_StorageSyncFile(file);
}

static bool FT02_AudioReadWavInfo(FILE* file, FT02WavInfo& info)
{
    if(file == nullptr || fseek(file, 0, SEEK_SET) != 0) return false;

    uint8_t riff[12];
    if(fread(riff, 1, sizeof(riff), file) != sizeof(riff)) return false;
    if(memcmp(riff, "RIFF", 4) != 0 || memcmp(riff + 8, "WAVE", 4) != 0) return false;

    bool haveFormat = false;
    bool haveData = false;
    uint16_t audioFormat = 0;

    while(!haveData)
    {
        uint8_t chunkHeader[8];
        if(fread(chunkHeader, 1, sizeof(chunkHeader), file) != sizeof(chunkHeader)) break;
        const uint32_t chunkSize = FT02_AudioReadLe32(chunkHeader + 4);
        const long chunkDataPosition = ftell(file);
        if(chunkDataPosition < 0) return false;

        if(memcmp(chunkHeader, "fmt ", 4) == 0)
        {
            if(chunkSize < 16u) return false;
            uint8_t format[16];
            if(fread(format, 1, sizeof(format), file) != sizeof(format)) return false;
            audioFormat = FT02_AudioReadLe16(format + 0);
            info.channels = FT02_AudioReadLe16(format + 2);
            info.sampleRate = FT02_AudioReadLe32(format + 4);
            info.bitsPerSample = FT02_AudioReadLe16(format + 14);
            haveFormat = true;
        }
        else if(memcmp(chunkHeader, "data", 4) == 0)
        {
            info.dataOffset = static_cast<uint32_t>(chunkDataPosition);
            info.dataBytes = chunkSize;
            haveData = true;
            break;
        }

        const uint32_t paddedSize = chunkSize + (chunkSize & 1u);
        if(fseek(file, chunkDataPosition + static_cast<long>(paddedSize), SEEK_SET) != 0)
        {
            return false;
        }
    }

    return haveFormat && haveData && audioFormat == 1u &&
           (info.channels == 1u || info.channels == 2u) &&
           info.bitsPerSample == 16u &&
           info.sampleRate == FT02_AUDIO_SAMPLE_RATE;
}

static int16_t FT02_AudioApplyPlaybackGain(int16_t sample, float gain)
{
    float value = static_cast<float>(sample) * gain;
    const float magnitude = fabsf(value);
    if(magnitude > FT02_AUDIO_LIMIT_THRESHOLD)
    {
        const float limited = FT02_AUDIO_LIMIT_THRESHOLD +
            (magnitude - FT02_AUDIO_LIMIT_THRESHOLD) / FT02_AUDIO_LIMIT_RATIO;
        value = value < 0.0f ? -limited : limited;
    }
    if(value > 32767.0f) value = 32767.0f;
    if(value < -32768.0f) value = -32768.0f;
    return static_cast<int16_t>(lrintf(value));
}

static void FT02_AudioLogEspError(const char* label, esp_err_t error)
{
    Serial.print(label);
    Serial.print(" error=");
    Serial.print(static_cast<int>(error));
    Serial.print(" ");
    Serial.println(esp_err_to_name(error));
}

static bool FT02_AudioInitCodec()
{
    if(!g_ft02AudioCodec.begin(Wire))
    {
        FT02_AudioSetMessage("WM8960未响应", false);
        return false;
    }

    bool ok = true;
    auto step = [&ok](const char* name, bool result)
    {
        Serial.printf("[AUDIO-CODEC] %-34s %s\n", name, result ? "OK" : "FAIL");
        if(!result) ok = false;
        return result;
    };

    step("enableVREF", g_ft02AudioCodec.enableVREF());
    step("enableVMID", g_ft02AudioCodec.enableVMID());
    delay(100);
    step("enableLMIC", g_ft02AudioCodec.enableLMIC());
    step("disableMicBias(onboard mic)", g_ft02AudioCodec.disableMicBias());
    delay(80);
    Serial.println("[AUDIO-CODEC] PGAL input VMID                  OK (reset default)");
    step("connectLMN1", g_ft02AudioCodec.connectLMN1());
    step("disableLINMUTE", g_ft02AudioCodec.disableLINMUTE());
    step("setLINVOLDB(+30dB)", g_ft02AudioCodec.setLINVOLDB(FT02_AUDIO_MIC_PGA_DB));
    step("setLMICBOOST(0dB)", g_ft02AudioCodec.setLMICBOOST(WM8960_MIC_BOOST_GAIN_0DB));
    step("connectLMIC2B", g_ft02AudioCodec.connectLMIC2B());
    step("enableAINL", g_ft02AudioCodec.enableAINL());

    step("disableLB2LO", g_ft02AudioCodec.disableLB2LO());
    step("disableRB2RO", g_ft02AudioCodec.disableRB2RO());
    step("enableLD2LO", g_ft02AudioCodec.enableLD2LO());
    step("enableRD2RO", g_ft02AudioCodec.enableRD2RO());
    step("setLB2LOVOL(-21dB)", g_ft02AudioCodec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB));
    step("setRB2ROVOL(-21dB)", g_ft02AudioCodec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB));
    step("enableLOMIX", g_ft02AudioCodec.enableLOMIX());
    step("enableROMIX", g_ft02AudioCodec.enableROMIX());

    step("enablePLL", g_ft02AudioCodec.enablePLL());
    step("setPLLPRESCALE(/2)", g_ft02AudioCodec.setPLLPRESCALE(WM8960_PLLPRESCALE_DIV_2));
    step("setSMD(fractional)", g_ft02AudioCodec.setSMD(WM8960_PLL_MODE_FRACTIONAL));
    step("setCLKSEL(PLL)", g_ft02AudioCodec.setCLKSEL(WM8960_CLKSEL_PLL));
    step("setSYSCLKDIV(/2)", g_ft02AudioCodec.setSYSCLKDIV(WM8960_SYSCLK_DIV_BY_2));
    step("setBCLKDIV(4)", g_ft02AudioCodec.setBCLKDIV(4));
    step("setDCLKDIV(16)", g_ft02AudioCodec.setDCLKDIV(WM8960_DCLKDIV_16));
    step("setPLLN(7)", g_ft02AudioCodec.setPLLN(7));
    step("setPLLK(86,C2,26)", g_ft02AudioCodec.setPLLK(0x86, 0xC2, 0x26));
    step("setWL(16-bit)", g_ft02AudioCodec.setWL(WM8960_WL_16BIT));
    step("enablePeripheralMode", g_ft02AudioCodec.enablePeripheralMode());
    step(
        "audioIF(I2S/16/slave)",
        g_ft02AudioCodec.writeRegister(WM8960_REG_AUDIO_INTERFACE_1, 0x0002)
    );

    step("enableAdcLeft", g_ft02AudioCodec.enableAdcLeft());
    step("disableAdcRight", g_ft02AudioCodec.disableAdcRight());
    step(
        "adcLeftDigital(+6dB)",
        g_ft02AudioCodec.setAdcLeftDigitalVolumeDB(FT02_AUDIO_ADC_DIGITAL_DB)
    );
    step("disableALC(fixed gain)", g_ft02AudioCodec.disableAlc());
    step("disablePeakLimiter", g_ft02AudioCodec.disablePeakLimiter());
    step("disableNoiseGate", g_ft02AudioCodec.disableNoiseGate());

    step("enableDacLeft", g_ft02AudioCodec.enableDacLeft());
    step("enableDacRight", g_ft02AudioCodec.enableDacRight());
    step("disableLoopBack", g_ft02AudioCodec.disableLoopBack());
    step("disableDacMute", g_ft02AudioCodec.disableDacMute());
    step("enableSpeakers", g_ft02AudioCodec.enableSpeakers());
    step("disableLeftSpeaker", g_ft02AudioCodec.disableLeftSpeaker());
    step("enableRightSpeaker", g_ft02AudioCodec.enableRightSpeaker());
    step("speakerVolume(0dB)", g_ft02AudioCodec.setSpeakerVolumeDB(FT02_AUDIO_SPEAKER_DB));

    if(!ok)
    {
        FT02_AudioSetMessage("WM8960配置失败", false);
        return false;
    }
    Serial.println("[AUDIO-CODEC] complete                         OK");
    return true;
}

static bool FT02_AudioInitI2s()
{
    i2s_chan_config_t channelConfig = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    channelConfig.dma_desc_num = 8;
    channelConfig.dma_frame_num = 256;

    esp_err_t error = i2s_new_channel(&channelConfig, &g_ft02AudioTx, &g_ft02AudioRx);
    if(error != ESP_OK)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] new channel", error);
        return false;
    }

    i2s_std_config_t config = {};
    config.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(FT02_AUDIO_SAMPLE_RATE);
    config.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO
    );
    config.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    config.gpio_cfg.bclk = static_cast<gpio_num_t>(FT02_AUDIO_BCLK_PIN);
    config.gpio_cfg.ws = static_cast<gpio_num_t>(FT02_AUDIO_WS_PIN);
    config.gpio_cfg.dout = static_cast<gpio_num_t>(FT02_AUDIO_DOUT_PIN);
    config.gpio_cfg.din = static_cast<gpio_num_t>(FT02_AUDIO_DIN_PIN);
    config.gpio_cfg.invert_flags.mclk_inv = false;
    config.gpio_cfg.invert_flags.bclk_inv = false;
    config.gpio_cfg.invert_flags.ws_inv = false;

    error = i2s_channel_init_std_mode(g_ft02AudioTx, &config);
    if(error != ESP_OK)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] init TX", error);
        return false;
    }
    error = i2s_channel_init_std_mode(g_ft02AudioRx, &config);
    if(error != ESP_OK)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] init RX", error);
        return false;
    }
    error = i2s_channel_enable(g_ft02AudioTx);
    if(error != ESP_OK)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] enable TX", error);
        return false;
    }
    error = i2s_channel_enable(g_ft02AudioRx);
    if(error != ESP_OK)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] enable RX", error);
        return false;
    }

    size_t written = 0;
    (void)i2s_channel_write(
        g_ft02AudioTx,
        g_ft02AudioZeroBuffer,
        sizeof(g_ft02AudioZeroBuffer),
        &written,
        pdMS_TO_TICKS(200)
    );
    return true;
}

static void FT02_AudioQueueSilence(uint32_t timeoutMs = 0)
{
    if(g_ft02AudioTx == nullptr) return;
    size_t written = 0;
    const esp_err_t error = i2s_channel_write(
        g_ft02AudioTx,
        g_ft02AudioZeroBuffer,
        sizeof(g_ft02AudioZeroBuffer),
        &written,
        pdMS_TO_TICKS(timeoutMs)
    );
    if(error != ESP_OK && error != ESP_ERR_TIMEOUT)
    {
        FT02_AudioLogEspError("[AUDIO-I2S] silence", error);
    }
}

static void FT02_AudioDrainIdleRx()
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_IDLE || !g_ft02AudioStatus.i2sReady) return;
    for(uint8_t batch = 0; batch < 2; batch++)
    {
        size_t bytesRead = 0;
        const esp_err_t error = i2s_channel_read(
            g_ft02AudioRx,
            g_ft02AudioStereoBuffer,
            sizeof(g_ft02AudioStereoBuffer),
            &bytesRead,
            0
        );
        if(error == ESP_ERR_TIMEOUT || bytesRead == 0) break;
        if(error != ESP_OK)
        {
            FT02_AudioLogEspError("[AUDIO-IDLE] drain", error);
            break;
        }
        FT02_AudioQueueSilence(0);
    }
}

static const char* FT02_AudioFindJsonValue(const char* line, const char* key)
{
    if(line == nullptr || key == nullptr) return nullptr;
    char pattern[64];
    const int written = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if(written <= 0 || static_cast<size_t>(written) >= sizeof(pattern)) return nullptr;
    const char* cursor = strstr(line, pattern);
    if(cursor == nullptr) return nullptr;
    cursor += strlen(pattern);
    while(*cursor == ' ' || *cursor == '\t') cursor++;
    if(*cursor != ':') return nullptr;
    cursor++;
    while(*cursor == ' ' || *cursor == '\t') cursor++;
    return cursor;
}

static bool FT02_AudioJsonString(
    const char* line,
    const char* key,
    char* output,
    size_t outputSize
)
{
    if(output == nullptr || outputSize == 0) return false;
    output[0] = '\0';
    const char* cursor = FT02_AudioFindJsonValue(line, key);
    if(cursor == nullptr || *cursor != '"') return false;
    cursor++;
    size_t used = 0;
    bool escaped = false;
    while(*cursor != '\0')
    {
        const char value = *cursor++;
        if(escaped)
        {
            if(used + 1 < outputSize) output[used++] = value;
            escaped = false;
            continue;
        }
        if(value == '\\')
        {
            escaped = true;
            continue;
        }
        if(value == '"')
        {
            output[used] = '\0';
            return true;
        }
        if(used + 1 < outputSize) output[used++] = value;
    }
    output[0] = '\0';
    return false;
}

static bool FT02_AudioJsonBool(const char* line, const char* key, bool& output)
{
    const char* cursor = FT02_AudioFindJsonValue(line, key);
    if(cursor == nullptr) return false;
    if(strncmp(cursor, "true", 4) == 0)
    {
        output = true;
        return true;
    }
    if(strncmp(cursor, "false", 5) == 0)
    {
        output = false;
        return true;
    }
    return false;
}

static bool FT02_AudioJsonU64(const char* line, const char* key, uint64_t& output)
{
    const char* cursor = FT02_AudioFindJsonValue(line, key);
    if(cursor == nullptr) return false;
    char* end = nullptr;
    const unsigned long long value = strtoull(cursor, &end, 10);
    if(end == cursor) return false;
    output = static_cast<uint64_t>(value);
    return true;
}

static bool FT02_AudioJsonDouble(const char* line, const char* key, double& output)
{
    const char* cursor = FT02_AudioFindJsonValue(line, key);
    if(cursor == nullptr) return false;
    char* end = nullptr;
    const double value = strtod(cursor, &end);
    if(end == cursor) return false;
    output = value;
    return true;
}

static bool FT02_AudioSequenceFromText(const char* text, uint32_t& output)
{
    output = 0;
    if(text == nullptr || text[0] == '\0') return false;

    static const char* const prefixes[] = {
        "AUD_FT02A_",
        "FT02A-AUD-"
    };
    for(const char* prefix : prefixes)
    {
        const char* cursor = strstr(text, prefix);
        if(cursor == nullptr) continue;
        cursor += strlen(prefix);
        if(!isdigit(static_cast<unsigned char>(*cursor))) continue;

        char* end = nullptr;
        const unsigned long value = strtoul(cursor, &end, 10);
        if(end == cursor || value < FT02_AUDIO_SEQUENCE_MIN ||
           value > FT02_AUDIO_SEQUENCE_MAX)
        {
            continue;
        }
        output = static_cast<uint32_t>(value);
        return true;
    }
    return false;
}

static void FT02_AudioTrackSequence(uint32_t sequence)
{
    if(sequence >= FT02_AUDIO_SEQUENCE_MIN && sequence <= FT02_AUDIO_SEQUENCE_MAX &&
       sequence > g_ft02AudioMaxSequenceSeen)
    {
        g_ft02AudioMaxSequenceSeen = sequence;
    }
}

static bool FT02_AudioReadSequenceState(const char* path, uint32_t& nextSequence)
{
    nextSequence = 0;
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;

    char text[32] = {};
    const bool readOk = fgets(text, sizeof(text), file) != nullptr;
    const bool closeOk = fclose(file) == 0;
    if(!readOk || !closeOk) return false;

    char* end = nullptr;
    const unsigned long value = strtoul(text, &end, 10);
    if(end == text || value < FT02_AUDIO_SEQUENCE_MIN ||
       value > FT02_AUDIO_SEQUENCE_MAX)
    {
        return false;
    }
    nextSequence = static_cast<uint32_t>(value);
    return true;
}

static bool FT02_AudioWriteSequenceState(uint32_t nextSequence)
{
    if(nextSequence < FT02_AUDIO_SEQUENCE_MIN ||
       nextSequence > FT02_AUDIO_SEQUENCE_MAX)
    {
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_TEMP);
    FILE* file = FT02_StorageOpenWriteFile(FT02_AUDIO_SEQUENCE_TEMP, true);
    if(file == nullptr) return false;

    const int written = fprintf(
        file,
        "%lu\n",
        static_cast<unsigned long>(nextSequence)
    );
    bool ok = written > 0;
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_BACKUP);
    const bool hadState = FT02_StorageFileExists(FT02_AUDIO_SEQUENCE_STATE);
    if(hadState &&
       !FT02_StorageRenameFile(FT02_AUDIO_SEQUENCE_STATE, FT02_AUDIO_SEQUENCE_BACKUP))
    {
        (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_TEMP);
        return false;
    }

    if(!FT02_StorageRenameFile(FT02_AUDIO_SEQUENCE_TEMP, FT02_AUDIO_SEQUENCE_STATE))
    {
        if(hadState)
        {
            (void)FT02_StorageRenameFile(
                FT02_AUDIO_SEQUENCE_BACKUP,
                FT02_AUDIO_SEQUENCE_STATE
            );
        }
        (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_AUDIO_SEQUENCE_BACKUP);
    return true;
}

static void FT02_AudioBuildSequencePath(
    uint32_t sequence,
    char* output,
    size_t outputSize
)
{
    snprintf(
        output,
        outputSize,
        "%s/AUD_FT02A_%06lu.wav",
        FT02_AUDIO_DIR,
        static_cast<unsigned long>(sequence)
    );
}

static bool FT02_AudioInitializeSequence()
{
    uint32_t storedNext = 0;
    const bool primaryStateValid = FT02_AudioReadSequenceState(
        FT02_AUDIO_SEQUENCE_STATE,
        storedNext
    );
    bool recoveredFromBackup = false;
    bool stateValid = primaryStateValid;
    if(!stateValid)
    {
        stateValid = FT02_AudioReadSequenceState(
            FT02_AUDIO_SEQUENCE_BACKUP,
            storedNext
        );
        recoveredFromBackup = stateValid;
    }

    uint32_t candidate = g_ft02AudioNextSequence;
    if(candidate < FT02_AUDIO_SEQUENCE_MIN) candidate = FT02_AUDIO_SEQUENCE_MIN;
    if(g_ft02AudioMaxSequenceSeen < FT02_AUDIO_SEQUENCE_MAX &&
       candidate <= g_ft02AudioMaxSequenceSeen)
    {
        candidate = g_ft02AudioMaxSequenceSeen + 1u;
    }
    if(stateValid && storedNext > candidate) candidate = storedNext;

    char path[112];
    while(candidate <= FT02_AUDIO_SEQUENCE_MAX)
    {
        FT02_AudioBuildSequencePath(candidate, path, sizeof(path));
        if(!FT02_StorageFileExists(path)) break;
        candidate++;
    }
    if(candidate > FT02_AUDIO_SEQUENCE_MAX) return false;

    g_ft02AudioNextSequence = candidate;
    if(!primaryStateValid || recoveredFromBackup || storedNext != candidate)
    {
        if(!FT02_AudioWriteSequenceState(candidate))
        {
            Serial.println("[AUDIO-ID] failed to initialize sequence state");
            return false;
        }
    }

    Serial.printf(
        "[AUDIO-ID] next=%lu maxSeen=%lu source=%s\n",
        static_cast<unsigned long>(g_ft02AudioNextSequence),
        static_cast<unsigned long>(g_ft02AudioMaxSequenceSeen),
        recoveredFromBackup
            ? "backup-recovery"
            : (stateValid ? "state/index" : "index/rebuild")
    );
    return true;
}

static bool FT02_AudioReserveSequence(uint32_t& sequence)
{
    sequence = g_ft02AudioNextSequence;
    char path[112];
    while(sequence <= FT02_AUDIO_SEQUENCE_MAX)
    {
        FT02_AudioBuildSequencePath(sequence, path, sizeof(path));
        if(!FT02_StorageFileExists(path)) break;
        sequence++;
    }
    if(sequence >= FT02_AUDIO_SEQUENCE_MAX) return false;

    const uint32_t nextSequence = sequence + 1u;
    if(!FT02_AudioWriteSequenceState(nextSequence))
    {
        Serial.println("[AUDIO-ID] sequence reservation failed");
        return false;
    }

    g_ft02AudioNextSequence = nextSequence;
    FT02_AudioTrackSequence(sequence);
    Serial.printf(
        "[AUDIO-ID] reserved=%lu next=%lu\n",
        static_cast<unsigned long>(sequence),
        static_cast<unsigned long>(nextSequence)
    );
    return true;
}

static int FT02_AudioFindEntry(const char* audioId)
{
    if(audioId == nullptr || audioId[0] == '\0') return -1;
    for(uint16_t i = 0; i < g_ft02AudioStatus.count; i++)
    {
        if(strcmp(g_ft02AudioEntries[i].audioId, audioId) == 0)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

static void FT02_AudioRemoveEntryAt(uint16_t index)
{
    if(index >= g_ft02AudioStatus.count) return;
    if(index + 1u < g_ft02AudioStatus.count)
    {
        memmove(
            &g_ft02AudioEntries[index],
            &g_ft02AudioEntries[index + 1u],
            sizeof(FT02AudioLogEntry) * (g_ft02AudioStatus.count - index - 1u)
        );
    }
    g_ft02AudioStatus.count--;
    memset(&g_ft02AudioEntries[g_ft02AudioStatus.count], 0, sizeof(FT02AudioLogEntry));
}

static FT02AudioLogEntry* FT02_AudioCreateOrReplaceEntry(const char* audioId)
{
    const int existing = FT02_AudioFindEntry(audioId);
    if(existing >= 0)
    {
        FT02AudioLogEntry& entry = g_ft02AudioEntries[existing];
        const uint32_t preservedSequence = entry.sequence;
        memset(&entry, 0, sizeof(entry));
        entry.valid = true;
        entry.sequence = preservedSequence;
        snprintf(entry.audioId, sizeof(entry.audioId), "%s", audioId);
        return &entry;
    }

    uint16_t index = g_ft02AudioStatus.count;
    if(g_ft02AudioStatus.count >= FT02_AUDIO_LOG_MAX_ENTRIES)
    {
        memmove(
            &g_ft02AudioEntries[0],
            &g_ft02AudioEntries[1],
            sizeof(FT02AudioLogEntry) * (FT02_AUDIO_LOG_MAX_ENTRIES - 1u)
        );
        index = FT02_AUDIO_LOG_MAX_ENTRIES - 1u;
    }
    else
    {
        g_ft02AudioStatus.count++;
    }

    FT02AudioLogEntry& entry = g_ft02AudioEntries[index];
    memset(&entry, 0, sizeof(entry));
    entry.valid = true;
    snprintf(entry.audioId, sizeof(entry.audioId), "%s", audioId);
    return &entry;
}

static void FT02_AudioParseIndexLine(const char* line)
{
    char audioId[48];
    if(!FT02_AudioJsonString(line, "audio_id", audioId, sizeof(audioId))) return;

    uint32_t sequence = 0;
    uint64_t sequenceValue = 0;
    if(FT02_AudioJsonU64(line, "sequence", sequenceValue) &&
       sequenceValue >= FT02_AUDIO_SEQUENCE_MIN &&
       sequenceValue <= FT02_AUDIO_SEQUENCE_MAX)
    {
        sequence = static_cast<uint32_t>(sequenceValue);
    }
    else
    {
        (void)FT02_AudioSequenceFromText(audioId, sequence);
    }

    bool deleted = false;
    char type[32] = {};
    (void)FT02_AudioJsonString(line, "type", type, sizeof(type));
    (void)FT02_AudioJsonBool(line, "deleted", deleted);
    if(deleted || strcmp(type, "audio_delete") == 0)
    {
        FT02_AudioTrackSequence(sequence);
        const int index = FT02_AudioFindEntry(audioId);
        if(index >= 0) FT02_AudioRemoveEntryAt(static_cast<uint16_t>(index));
        return;
    }

    const int existingIndex = FT02_AudioFindEntry(audioId);
    if(sequence == 0 && existingIndex >= 0)
    {
        sequence = g_ft02AudioEntries[existingIndex].sequence;
    }
    if(sequence == 0)
    {
        if(g_ft02AudioMaxSequenceSeen >= FT02_AUDIO_SEQUENCE_MAX) return;
        sequence = g_ft02AudioMaxSequenceSeen + 1u;
    }
    FT02_AudioTrackSequence(sequence);

    FT02AudioLogEntry* entry = FT02_AudioCreateOrReplaceEntry(audioId);
    if(entry == nullptr) return;
    entry->sequence = sequence;

    (void)FT02_AudioJsonString(line, "session_id", entry->sessionId, sizeof(entry->sessionId));
    if(!FT02_AudioJsonString(line, "file", entry->filePath, sizeof(entry->filePath)))
    {
        (void)FT02_AudioJsonString(line, "path", entry->filePath, sizeof(entry->filePath));
    }
    (void)FT02_AudioJsonString(line, "filename", entry->filename, sizeof(entry->filename));
    (void)FT02_AudioJsonString(line, "device_date", entry->deviceDate, sizeof(entry->deviceDate));
    (void)FT02_AudioJsonString(line, "device_time", entry->deviceTime, sizeof(entry->deviceTime));

    uint64_t value = 0;
    if(FT02_AudioJsonU64(line, "samples", value)) entry->samples = static_cast<uint32_t>(value);
    if(FT02_AudioJsonU64(line, "duration_ms", value)) entry->durationMs = static_cast<uint32_t>(value);
    else
    {
        double durationSeconds = 0.0;
        if(FT02_AudioJsonDouble(line, "duration_sec", durationSeconds))
        {
            entry->durationMs = static_cast<uint32_t>(durationSeconds * 1000.0 + 0.5);
        }
    }
    if(FT02_AudioJsonU64(line, "size", value)) entry->sizeBytes = value;
    (void)FT02_AudioJsonDouble(line, "lat", entry->latitude);
    (void)FT02_AudioJsonDouble(line, "lon", entry->longitude);
    (void)FT02_AudioJsonBool(line, "gnss_fix", entry->gnssFix);

    if(entry->filename[0] == '\0' && entry->filePath[0] != '\0')
    {
        const char* slash = strrchr(entry->filePath, '/');
        snprintf(
            entry->filename,
            sizeof(entry->filename),
            "%s",
            slash != nullptr ? slash + 1 : entry->filePath
        );
    }
    entry->fileExists = entry->filePath[0] != '\0' && FT02_StorageFileExists(entry->filePath);
    if(entry->sizeBytes == 0 && entry->fileExists)
    {
        (void)FT02_StorageFileSize(entry->filePath, entry->sizeBytes);
    }
}

static void FT02_AudioDateTime(
    const FT02GnssSnapshot& gnss,
    char* dateOutput,
    size_t dateSize,
    char* timeOutput,
    size_t timeSize
)
{
    if(gnss.timeValid && strlen(gnss.localDate) >= 10 && strlen(gnss.localTime) >= 8)
    {
        snprintf(dateOutput, dateSize, "%.10s", gnss.localDate);
        snprintf(timeOutput, timeSize, "%.8s", gnss.localTime);
        return;
    }

    const time_t now = time(nullptr);
    if(now > 1609459200)
    {
        struct tm localTime = {};
        localtime_r(&now, &localTime);
        strftime(dateOutput, dateSize, "%Y-%m-%d", &localTime);
        strftime(timeOutput, timeSize, "%H:%M:%S", &localTime);
        return;
    }

    snprintf(dateOutput, dateSize, "0000-00-00");
    const uint32_t seconds = millis() / 1000u;
    snprintf(
        timeOutput,
        timeSize,
        "%02lu:%02lu:%02lu",
        static_cast<unsigned long>((seconds / 3600u) % 24u),
        static_cast<unsigned long>((seconds / 60u) % 60u),
        static_cast<unsigned long>(seconds % 60u)
    );
}

static bool FT02_AudioBuildIdentity(const FT02GnssSnapshot& gnss)
{
    FT02_AudioDateTime(
        gnss,
        g_ft02AudioRecording.deviceDate,
        sizeof(g_ft02AudioRecording.deviceDate),
        g_ft02AudioRecording.deviceTime,
        sizeof(g_ft02AudioRecording.deviceTime)
    );

    uint32_t sequence = 0;
    if(!FT02_AudioReserveSequence(sequence)) return false;
    g_ft02AudioRecording.sequence = sequence;

    snprintf(
        g_ft02AudioRecording.audioId,
        sizeof(g_ft02AudioRecording.audioId),
        "AUD_FT02A_%06lu",
        static_cast<unsigned long>(sequence)
    );
    snprintf(
        g_ft02AudioRecording.sessionId,
        sizeof(g_ft02AudioRecording.sessionId),
        "%s",
        g_ft02AudioRecording.audioId
    );
    snprintf(
        g_ft02AudioRecording.filename,
        sizeof(g_ft02AudioRecording.filename),
        "%s.wav",
        g_ft02AudioRecording.audioId
    );
    snprintf(
        g_ft02AudioRecording.filePath,
        sizeof(g_ft02AudioRecording.filePath),
        "%s/%s",
        FT02_AUDIO_DIR,
        g_ft02AudioRecording.filename
    );
    return true;
}

static bool FT02_AudioAppendIndexRecord(
    const FT02AudioRecordingSession& recording,
    uint32_t durationMs,
    uint64_t sizeBytes
)
{
    const bool timeValid = strcmp(recording.deviceDate, "0000-00-00") != 0;
    char line[1024];
    const int written = snprintf(
        line,
        sizeof(line),
        "{\"type\":\"audio_log\",\"record_id\":\"%s\","
        "\"audio_id\":\"%s\",\"session_id\":\"%s\",\"sequence\":%lu,"
        "\"device_id\":\"FT-02A\",\"file\":\"%s\",\"path\":\"%s\","
        "\"filename\":\"%s\",\"samples\":%lu,\"duration_sec\":%.3f,"
        "\"duration_ms\":%lu,\"size\":%llu,\"dropped_chunks\":0,"
        "\"sample_rate\":%lu,\"channels\":1,\"bits_per_sample\":16,"
        "\"device_date\":\"%s\",\"device_time\":\"%s\",\"time_valid\":%s,"
        "\"gnss_fix\":%s,\"lat\":%.7f,\"lon\":%.7f,\"sync_state\":\"pending\"}",
        recording.audioId,
        recording.audioId,
        recording.sessionId,
        static_cast<unsigned long>(recording.sequence),
        recording.filePath,
        recording.filePath,
        recording.filename,
        static_cast<unsigned long>(recording.recordedFrames),
        static_cast<double>(durationMs) / 1000.0,
        static_cast<unsigned long>(durationMs),
        static_cast<unsigned long long>(sizeBytes),
        static_cast<unsigned long>(FT02_AUDIO_SAMPLE_RATE),
        recording.deviceDate,
        recording.deviceTime,
        timeValid ? "true" : "false",
        recording.gnssFix ? "true" : "false",
        recording.latitude,
        recording.longitude
    );
    return written > 0 && static_cast<size_t>(written) < sizeof(line) &&
           FT02_StorageAppendLine(FT02_AUDIO_INDEX, line);
}

static bool FT02_AudioAppendDeleteRecord(const FT02AudioLogEntry& entry)
{
    char line[384];
    const int written = snprintf(
        line,
        sizeof(line),
        "{\"type\":\"audio_delete\",\"record_id\":\"%s\","
        "\"audio_id\":\"%s\",\"session_id\":\"%s\",\"sequence\":%lu,"
        "\"device_id\":\"FT-02A\",\"file\":\"%s\",\"deleted\":true}",
        entry.audioId,
        entry.audioId,
        entry.sessionId,
        static_cast<unsigned long>(entry.sequence),
        entry.filePath
    );
    return written > 0 && static_cast<size_t>(written) < sizeof(line) &&
           FT02_StorageAppendLine(FT02_AUDIO_INDEX, line);
}

static bool FT02_AudioAppendRecordingSamples(const int16_t* samples, size_t frames)
{
    if(g_ft02AudioRecording.file == nullptr || samples == nullptr || frames == 0)
    {
        return frames == 0;
    }

    const size_t bytes = frames * sizeof(int16_t);
    if(fwrite(samples, 1, bytes, g_ft02AudioRecording.file) != bytes)
    {
        Serial.println("[AUDIO-REC] SD write failed");
        g_ft02AudioRecording.success = false;
        return false;
    }

    for(size_t i = 0; i < frames; i++)
    {
        const int16_t sample = samples[i];
        const int32_t sample32 = static_cast<int32_t>(sample);
        const int32_t magnitude = sample32 < 0 ? -sample32 : sample32;
        if(magnitude > g_ft02AudioRecording.peak) g_ft02AudioRecording.peak = magnitude;
        if(sample < g_ft02AudioRecording.minSample) g_ft02AudioRecording.minSample = sample;
        if(sample > g_ft02AudioRecording.maxSample) g_ft02AudioRecording.maxSample = sample;
        g_ft02AudioRecording.sum += sample32;
        g_ft02AudioRecording.sumSq += static_cast<uint64_t>(
            static_cast<int64_t>(sample32) * static_cast<int64_t>(sample32)
        );
        if(sample >= 32760 || sample <= -32760) g_ft02AudioRecording.clipCount++;
        if(sample == 0) g_ft02AudioRecording.zeroCount++;
        if(magnitude > g_ft02AudioRecording.secondPeak) g_ft02AudioRecording.secondPeak = magnitude;
        g_ft02AudioRecording.secondSumSq += static_cast<uint64_t>(
            static_cast<int64_t>(sample32) * static_cast<int64_t>(sample32)
        );
        g_ft02AudioRecording.secondFrames++;
    }

    g_ft02AudioRecording.recordedFrames += static_cast<uint32_t>(frames);
    g_ft02AudioRecording.dataBytes += static_cast<uint32_t>(bytes);
    return true;
}

static void FT02_AudioFinishRecording(const char* reason)
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_RECORDING &&
       g_ft02AudioState != FT02_AUDIO_LOG_POST_ROLL)
    {
        return;
    }

    bool success = g_ft02AudioRecording.success;
    if(g_ft02AudioRecording.file != nullptr)
    {
        if(success)
        {
            success = FT02_AudioFinalizeWav(
                g_ft02AudioRecording.file,
                g_ft02AudioRecording.dataBytes
            );
        }
        fclose(g_ft02AudioRecording.file);
        g_ft02AudioRecording.file = nullptr;
    }

    const uint32_t durationMs = static_cast<uint32_t>(
        (static_cast<uint64_t>(g_ft02AudioRecording.recordedFrames) * 1000u) /
        FT02_AUDIO_SAMPLE_RATE
    );
    uint64_t fileSize = 0;
    if(success)
    {
        success = FT02_StorageFileSize(g_ft02AudioRecording.filePath, fileSize);
    }
    if(success)
    {
        success = FT02_AudioAppendIndexRecord(g_ft02AudioRecording, durationMs, fileSize);
    }

    const uint64_t samples = g_ft02AudioRecording.recordedFrames > 0
        ? g_ft02AudioRecording.recordedFrames
        : 1;
    const double rms = sqrt(
        static_cast<double>(g_ft02AudioRecording.sumSq) /
        static_cast<double>(samples)
    );
    const double clipPercent = 100.0 *
        static_cast<double>(g_ft02AudioRecording.clipCount) /
        static_cast<double>(samples);

    Serial.printf(
        "[AUDIO-REC] complete success=%s reason=%s id=%s file=%s duration=%lu.%03lus samples=%lu size=%llu peak=%ld rms=%.1f clip=%.3f%%\n",
        success ? "yes" : "no",
        reason != nullptr ? reason : "unknown",
        g_ft02AudioRecording.audioId,
        g_ft02AudioRecording.filePath,
        static_cast<unsigned long>(durationMs / 1000u),
        static_cast<unsigned long>(durationMs % 1000u),
        static_cast<unsigned long>(g_ft02AudioRecording.recordedFrames),
        static_cast<unsigned long long>(fileSize),
        static_cast<long>(g_ft02AudioRecording.peak),
        rms,
        clipPercent
    );

    g_ft02AudioStatus.recording = false;
    g_ft02AudioStatus.stopRequested = false;
    g_ft02AudioStatus.recordingElapsedMs = durationMs;
    g_ft02AudioStatus.recordingSeconds = durationMs / 1000u;
    g_ft02AudioStatus.lastDurationMs = durationMs;
    g_ft02AudioStatus.lastSamples = g_ft02AudioRecording.recordedFrames;
    g_ft02AudioStatus.lastSizeBytes = fileSize;
    g_ft02AudioStatus.lastPeak = g_ft02AudioRecording.peak;
    g_ft02AudioStatus.activeSequence = g_ft02AudioRecording.sequence;
    snprintf(
        g_ft02AudioStatus.activeAudioId,
        sizeof(g_ft02AudioStatus.activeAudioId),
        "%s",
        g_ft02AudioRecording.audioId
    );
    snprintf(
        g_ft02AudioStatus.activeFilePath,
        sizeof(g_ft02AudioStatus.activeFilePath),
        "%s",
        g_ft02AudioRecording.filePath
    );

    // Complete all file/index work before publishing IDLE to the UI task.
    FT02_AudioQueueSilence(50);
    g_ft02AudioRecording = FT02AudioRecordingSession{};
    (void)FT02_AudioLogReload();
    g_ft02AudioRestartNotBeforeMs = millis() + 500u;
    FT02_AudioSetMessage(success ? "录音已保存" : "录音保存失败", success);
    g_ft02AudioState = FT02_AUDIO_LOG_IDLE;
    FT02_AudioChanged();
}

static void FT02_AudioServiceRecording()
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_RECORDING &&
       g_ft02AudioState != FT02_AUDIO_LOG_POST_ROLL)
    {
        return;
    }

    const uint32_t beforeRead = millis();
    if(g_ft02AudioState == FT02_AUDIO_LOG_RECORDING &&
       beforeRead - g_ft02AudioRecording.startMs >= FT02_AUDIO_MAX_SECONDS * 1000u)
    {
        g_ft02AudioState = FT02_AUDIO_LOG_POST_ROLL;
        g_ft02AudioRecording.stopDeadlineMs = beforeRead + FT02_AUDIO_POST_ROLL_MS;
        g_ft02AudioStatus.stopRequested = true;
        Serial.println("[AUDIO-REC] maximum duration reached; stopping");
    }

    for(uint8_t batch = 0; batch < 4; batch++)
    {
        size_t bytesRead = 0;
        const esp_err_t error = i2s_channel_read(
            g_ft02AudioRx,
            g_ft02AudioStereoBuffer,
            sizeof(g_ft02AudioStereoBuffer),
            &bytesRead,
            pdMS_TO_TICKS(8)
        );
        if(error == ESP_ERR_TIMEOUT || bytesRead == 0) break;
        if(error != ESP_OK)
        {
            FT02_AudioLogEspError("[AUDIO-REC] read", error);
            g_ft02AudioRecording.success = false;
            break;
        }

        bytesRead -= bytesRead % (2u * sizeof(int16_t));
        const size_t frames = bytesRead / (2u * sizeof(int16_t));
        for(size_t i = 0; i < frames; i++)
        {
            g_ft02AudioMonoBuffer[i] = g_ft02AudioStereoBuffer[i * 2u];
        }
        if(!FT02_AudioAppendRecordingSamples(g_ft02AudioMonoBuffer, frames)) break;
        FT02_AudioQueueSilence(0);
    }

    if(!g_ft02AudioRecording.success)
    {
        FT02_AudioFinishRecording("io_error");
        return;
    }

    const uint32_t elapsedMs = millis() - g_ft02AudioRecording.startMs;
    g_ft02AudioStatus.recordingElapsedMs = elapsedMs;
    g_ft02AudioStatus.recordingSeconds = elapsedMs / 1000u;
    while(elapsedMs >= g_ft02AudioRecording.nextProgressMs)
    {
        const double secondRms = g_ft02AudioRecording.secondFrames > 0
            ? sqrt(
                static_cast<double>(g_ft02AudioRecording.secondSumSq) /
                static_cast<double>(g_ft02AudioRecording.secondFrames)
            )
            : 0.0;
        Serial.printf(
            "[AUDIO-REC] elapsed=%lu.%01lus bytes=%lu peak=%ld rms=%.1f state=%s\n",
            static_cast<unsigned long>(g_ft02AudioRecording.nextProgressMs / 1000u),
            static_cast<unsigned long>((g_ft02AudioRecording.nextProgressMs % 1000u) / 100u),
            static_cast<unsigned long>(g_ft02AudioRecording.dataBytes),
            static_cast<long>(g_ft02AudioRecording.secondPeak),
            secondRms,
            g_ft02AudioState == FT02_AUDIO_LOG_POST_ROLL ? "TAIL" : "REC"
        );
        g_ft02AudioRecording.nextProgressMs += 1000u;
        g_ft02AudioRecording.secondFrames = 0;
        g_ft02AudioRecording.secondPeak = 0;
        g_ft02AudioRecording.secondSumSq = 0;
    }

    if(g_ft02AudioState == FT02_AUDIO_LOG_POST_ROLL &&
       FT02_AudioTimeReached(millis(), g_ft02AudioRecording.stopDeadlineMs))
    {
        FT02_AudioFinishRecording("user_stop");
    }
}

static void FT02_AudioBeginPlaybackDrain()
{
    if(g_ft02AudioPlayback.file != nullptr)
    {
        fclose(g_ft02AudioPlayback.file);
        g_ft02AudioPlayback.file = nullptr;
    }
    g_ft02AudioPlayback.draining = true;
    g_ft02AudioPlayback.tailBlocksWritten = 0;
    g_ft02AudioPlayback.tailWaitDeadlineMs = 0;
}

static void FT02_AudioFinishPlayback()
{
    if(g_ft02AudioPlayback.file != nullptr)
    {
        fclose(g_ft02AudioPlayback.file);
        g_ft02AudioPlayback.file = nullptr;
    }

    const bool stopped = g_ft02AudioPlayback.stopRequested;
    const bool success = g_ft02AudioPlayback.success;
    if(g_ft02AudioPlayback.volumeDirty)
    {
        FT02_AudioSaveVolumeLevel(g_ft02AudioStatus.playbackVolumeLevel);
    }
    Serial.printf(
        "[AUDIO-PLAY] complete success=%s stopped=%s frames=%lu/%lu volume=%u/10\n",
        success ? "yes" : "no",
        stopped ? "yes" : "no",
        static_cast<unsigned long>(g_ft02AudioPlayback.playedFrames),
        static_cast<unsigned long>(g_ft02AudioPlayback.totalFrames),
        static_cast<unsigned int>(g_ft02AudioStatus.playbackVolumeLevel)
    );

    g_ft02AudioStatus.playing = false;
    g_ft02AudioStatus.playbackStopRequested = false;
    if(success)
    {
        FT02_AudioSetMessage(stopped ? "播放已停止" : "播放完成", true);
    }
    else
    {
        FT02_AudioSetMessage("播放失败", false);
    }
    g_ft02AudioPlayback = FT02AudioPlaybackSession{};
    g_ft02AudioState = FT02_AUDIO_LOG_IDLE;
    FT02_AudioChanged();
}

static void FT02_AudioServicePlayback()
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_PLAYING) return;

    if(!g_ft02AudioPlayback.startReleased) return;

    if(g_ft02AudioPlayback.stopRequested && !g_ft02AudioPlayback.draining)
    {
        FT02_AudioBeginPlaybackDrain();
    }

    if(!g_ft02AudioPlayback.draining)
    {
        for(uint8_t batch = 0; batch < FT02_AUDIO_PLAYBACK_BLOCKS_PER_POLL; batch++)
        {
            if(g_ft02AudioPlayback.bytesRemaining == 0)
            {
                FT02_AudioBeginPlaybackDrain();
                break;
            }

            const size_t maxInputBytes = g_ft02AudioPlayback.info.channels == 2
                ? sizeof(g_ft02AudioStereoBuffer)
                : FT02_AUDIO_FRAMES_PER_BLOCK * sizeof(int16_t);
            const size_t bytesWanted = g_ft02AudioPlayback.bytesRemaining < maxInputBytes
                ? static_cast<size_t>(g_ft02AudioPlayback.bytesRemaining)
                : maxInputBytes;
            const size_t bytesRead = fread(
                g_ft02AudioStereoBuffer,
                1,
                bytesWanted,
                g_ft02AudioPlayback.file
            );
            if(bytesRead == 0)
            {
                if(g_ft02AudioPlayback.bytesRemaining > 0)
                {
                    g_ft02AudioPlayback.success = false;
                }
                FT02_AudioBeginPlaybackDrain();
                break;
            }

            const float effectiveGain = g_ft02AudioPlayback.normalizationGain *
                FT02_AudioUserVolumeGain(g_ft02AudioStatus.playbackVolumeLevel);
            size_t frames = 0;
            if(g_ft02AudioPlayback.info.channels == 2)
            {
                frames = bytesRead / (2u * sizeof(int16_t));
                for(size_t i = 0; i < frames; i++)
                {
                    const int16_t sample = g_ft02AudioStereoBuffer[
                        i * 2u + (g_ft02AudioPlayback.useRight ? 1u : 0u)
                    ];
                    const int16_t amplified = FT02_AudioApplyPlaybackGain(sample, effectiveGain);
                    g_ft02AudioPlayBuffer[i * 2u] = amplified;
                    g_ft02AudioPlayBuffer[i * 2u + 1u] = amplified;
                }
            }
            else
            {
                frames = bytesRead / sizeof(int16_t);
                for(size_t i = 0; i < frames; i++)
                {
                    const int16_t amplified = FT02_AudioApplyPlaybackGain(
                        g_ft02AudioStereoBuffer[i],
                        effectiveGain
                    );
                    g_ft02AudioPlayBuffer[i * 2u] = amplified;
                    g_ft02AudioPlayBuffer[i * 2u + 1u] = amplified;
                }
            }

            size_t written = 0;
            const esp_err_t error = i2s_channel_write(
                g_ft02AudioTx,
                g_ft02AudioPlayBuffer,
                frames * 2u * sizeof(int16_t),
                &written,
                pdMS_TO_TICKS(30)
            );
            if(error == ESP_ERR_TIMEOUT)
            {
                // Rewind this block and retry on the next poll instead of losing audio.
                fseek(g_ft02AudioPlayback.file, -static_cast<long>(bytesRead), SEEK_CUR);
                break;
            }
            if(error != ESP_OK || written != frames * 2u * sizeof(int16_t))
            {
                FT02_AudioLogEspError("[AUDIO-PLAY] write", error);
                g_ft02AudioPlayback.success = false;
                FT02_AudioBeginPlaybackDrain();
                break;
            }

            g_ft02AudioPlayback.playedFrames += static_cast<uint32_t>(frames);
            g_ft02AudioPlayback.bytesRemaining -= static_cast<uint32_t>(bytesRead);
            g_ft02AudioStatus.playbackElapsedMs = static_cast<uint32_t>(
                (static_cast<uint64_t>(g_ft02AudioPlayback.playedFrames) * 1000u) /
                FT02_AUDIO_SAMPLE_RATE
            );
            if(g_ft02AudioStatus.playbackElapsedMs > g_ft02AudioStatus.playbackDurationMs)
            {
                g_ft02AudioStatus.playbackElapsedMs = g_ft02AudioStatus.playbackDurationMs;
            }
        }
    }

    if(!g_ft02AudioPlayback.draining) return;

    if(g_ft02AudioPlayback.tailBlocksWritten < FT02_AUDIO_PLAYBACK_TAIL_BLOCKS)
    {
        size_t written = 0;
        const esp_err_t error = i2s_channel_write(
            g_ft02AudioTx,
            g_ft02AudioZeroBuffer,
            sizeof(g_ft02AudioZeroBuffer),
            &written,
            pdMS_TO_TICKS(30)
        );
        if(error == ESP_ERR_TIMEOUT) return;
        if(error != ESP_OK || written != sizeof(g_ft02AudioZeroBuffer))
        {
            FT02_AudioLogEspError("[AUDIO-PLAY] tail", error);
            g_ft02AudioPlayback.success = false;
            g_ft02AudioPlayback.tailBlocksWritten = FT02_AUDIO_PLAYBACK_TAIL_BLOCKS;
        }
        else
        {
            g_ft02AudioPlayback.tailBlocksWritten++;
        }
        return;
    }

    if(g_ft02AudioPlayback.tailWaitDeadlineMs == 0)
    {
        g_ft02AudioPlayback.tailWaitDeadlineMs = millis() + FT02_AUDIO_PLAYBACK_TAIL_WAIT_MS;
        return;
    }
    if(FT02_AudioTimeReached(millis(), g_ft02AudioPlayback.tailWaitDeadlineMs))
    {
        FT02_AudioFinishPlayback();
    }
}

static void FT02_AudioServiceTask(void*)
{
    for(;;)
    {
        const FT02AudioLogState state = g_ft02AudioState;
        if(state == FT02_AUDIO_LOG_IDLE)
        {
            FT02_AudioDrainIdleRx();
        }
        else if(state == FT02_AUDIO_LOG_RECORDING || state == FT02_AUDIO_LOG_POST_ROLL)
        {
            FT02_AudioServiceRecording();
        }
        else if(state == FT02_AUDIO_LOG_PLAYING)
        {
            FT02_AudioServicePlayback();
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
}

bool FT02_AudioLogBegin()
{
    memset(&g_ft02AudioStatus, 0, sizeof(g_ft02AudioStatus));
    memset(g_ft02AudioEntries, 0, sizeof(g_ft02AudioEntries));
    g_ft02AudioState = FT02_AUDIO_LOG_IDLE;
    g_ft02AudioStatus.started = true;
    g_ft02AudioStatus.storageReady = FT02_StorageIsReady();
    g_ft02AudioStatus.playbackVolumeLevel = FT02_AudioLoadVolumeLevel();

    // Waveshare board supplies its own 24 MHz MCLK. GPIO13 is fully released.
    g_ft02AudioStatus.codecReady = FT02_AudioInitCodec();
    if(g_ft02AudioStatus.codecReady)
    {
        g_ft02AudioStatus.i2sReady = FT02_AudioInitI2s();
    }
    const bool logStoreReady = FT02_AudioLogReload();

    bool ready = g_ft02AudioStatus.codecReady &&
                 g_ft02AudioStatus.i2sReady &&
                 g_ft02AudioStatus.storageReady &&
                 logStoreReady;
    FT02_AudioSetMessage(
        ready
            ? "语音日志就绪"
            : (logStoreReady ? "语音模块未就绪" : "录音编号状态异常"),
        ready
    );
    if(g_ft02AudioStatus.i2sReady && g_ft02AudioServiceTaskHandle == nullptr)
    {
        const BaseType_t taskCreated = xTaskCreatePinnedToCore(
            FT02_AudioServiceTask,
            "ft02-audio",
            8192,
            nullptr,
            4,
            &g_ft02AudioServiceTaskHandle,
            0
        );
        if(taskCreated != pdPASS)
        {
            g_ft02AudioServiceTaskHandle = nullptr;
            g_ft02AudioStatus.i2sReady = false;
            FT02_AudioSetMessage("音频后台任务启动失败", false);
        }
    }
    ready = g_ft02AudioStatus.codecReady &&
            g_ft02AudioStatus.i2sReady &&
            g_ft02AudioStatus.storageReady &&
            logStoreReady &&
            g_ft02AudioServiceTaskHandle != nullptr;

    Serial.printf(
        "[AUDIO-LOG] begin codec=%s i2s=%s storage=%s count=%u nextId=%lu volume=%u/10 task=%s pins=BCLK%d WS%d DOUT%d DIN%d\n",
        g_ft02AudioStatus.codecReady ? "OK" : "FAIL",
        g_ft02AudioStatus.i2sReady ? "OK" : "FAIL",
        g_ft02AudioStatus.storageReady ? "OK" : "FAIL",
        static_cast<unsigned int>(g_ft02AudioStatus.count),
        static_cast<unsigned long>(g_ft02AudioNextSequence),
        static_cast<unsigned int>(g_ft02AudioStatus.playbackVolumeLevel),
        g_ft02AudioServiceTaskHandle != nullptr ? "OK" : "FAIL",
        FT02_AUDIO_BCLK_PIN,
        FT02_AUDIO_WS_PIN,
        FT02_AUDIO_DOUT_PIN,
        FT02_AUDIO_DIN_PIN
    );
    return ready;
}

void FT02_AudioLogPoll()
{
    // I2S/SD streaming runs in the dedicated audio task so e-paper updates do
    // not cut recording or starve playback. The main loop keeps this hook for
    // API compatibility and future non-streaming maintenance.
}

void FT02_AudioLogNotifyKeyRelease()
{
    if(g_ft02AudioState == FT02_AUDIO_LOG_RECORDING)
    {
        g_ft02AudioRecording.stopArmed = true;
    }
}

FT02AudioLogStatus FT02_AudioLogStatusCurrent()
{
    return g_ft02AudioStatus;
}

FT02AudioLogState FT02_AudioLogStateCurrent()
{
    return g_ft02AudioState;
}

bool FT02_AudioLogIsCapturing()
{
    return g_ft02AudioState == FT02_AUDIO_LOG_RECORDING ||
           g_ft02AudioState == FT02_AUDIO_LOG_POST_ROLL;
}

bool FT02_AudioLogIsPlaying()
{
    return g_ft02AudioState == FT02_AUDIO_LOG_PLAYING;
}

bool FT02_AudioLogIsBusy()
{
    return FT02_AudioLogIsCapturing() || FT02_AudioLogIsPlaying();
}

bool FT02_AudioLogReload()
{
    memset(g_ft02AudioEntries, 0, sizeof(g_ft02AudioEntries));
    g_ft02AudioStatus.count = 0;
    g_ft02AudioMaxSequenceSeen = 0;
    g_ft02AudioStatus.storageReady = FT02_StorageIsReady();
    g_ft02AudioStatus.listLoaded = false;

    if(!g_ft02AudioStatus.storageReady)
    {
        FT02_AudioSetMessage("SD未就绪", false);
        return false;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_AUDIO_INDEX);
    if(file == nullptr)
    {
        const bool sequenceReady = FT02_AudioInitializeSequence();
        g_ft02AudioStatus.listLoaded = true;
        FT02_AudioSetMessage(
            sequenceReady ? "暂无语音日志" : "录音编号状态异常",
            sequenceReady
        );
        return sequenceReady;
    }

    char line[FT02_AUDIO_INDEX_LINE_BYTES];
    while(fgets(line, sizeof(line), file) != nullptr)
    {
        FT02_AudioParseIndexLine(line);
        yield();
    }
    fclose(file);

    const bool sequenceReady = FT02_AudioInitializeSequence();
    g_ft02AudioStatus.listLoaded = true;
    FT02_AudioSetMessage(
        sequenceReady
            ? (g_ft02AudioStatus.count > 0 ? "语音日志已载入" : "暂无语音日志")
            : "录音编号状态异常",
        sequenceReady
    );
    return sequenceReady;
}

bool FT02_AudioLogGetNewest(uint16_t newestIndex, FT02AudioLogEntry& output)
{
    if(newestIndex >= g_ft02AudioStatus.count) return false;
    const uint16_t index = static_cast<uint16_t>(
        g_ft02AudioStatus.count - 1u - newestIndex
    );
    output = g_ft02AudioEntries[index];
    return output.valid;
}

uint16_t FT02_AudioLogWrapIndex(uint16_t current, uint16_t count, int32_t step)
{
    if(count == 0) return 0;
    int32_t value = static_cast<int32_t>(current) + step;
    while(value < 0) value += count;
    while(value >= count) value -= count;
    return static_cast<uint16_t>(value);
}

bool FT02_AudioLogStartRecording(const FT02GnssSnapshot& gnss)
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_IDLE) return false;
    if(!g_ft02AudioStatus.codecReady ||
       !g_ft02AudioStatus.i2sReady ||
       !g_ft02AudioStatus.storageReady)
    {
        FT02_AudioSetMessage("无法录音：音频或SD未就绪", false);
        FT02_AudioChanged();
        return false;
    }
    if(!FT02_AudioTimeReached(millis(), g_ft02AudioRestartNotBeforeMs))
    {
        FT02_AudioSetMessage("请稍候再开始录音", false);
        return false;
    }

    g_ft02AudioRecording = FT02AudioRecordingSession{};

    // Bind one deterministic location snapshot to the voice log at the exact
    // moment recording starts. Never carry a stale last-known coordinate into
    // a new log after GNSS fix has been lost.
    const bool gnssPositionValid =
        gnss.fixValid &&
        gnss.hasPosition &&
        isfinite(gnss.latitude) &&
        isfinite(gnss.longitude) &&
        fabs(gnss.latitude) <= 90.0 &&
        fabs(gnss.longitude) <= 180.0 &&
        !(gnss.latitude == 0.0 && gnss.longitude == 0.0);

    g_ft02AudioRecording.gnssFix = gnssPositionValid;
    if(gnssPositionValid)
    {
        g_ft02AudioRecording.latitude = gnss.latitude;
        g_ft02AudioRecording.longitude = gnss.longitude;
        Serial.printf(
            "[AUDIO-REC] GNSS captured lat=%.7f lon=%.7f\n",
            g_ft02AudioRecording.latitude,
            g_ft02AudioRecording.longitude
        );
    }
    else
    {
        g_ft02AudioRecording.latitude = 0.0;
        g_ft02AudioRecording.longitude = 0.0;
        Serial.println("[AUDIO-REC] GNSS not captured: no current valid fix");
    }

    if(!FT02_AudioBuildIdentity(gnss))
    {
        FT02_AudioSetMessage("无法分配录音编号", false);
        FT02_AudioChanged();
        return false;
    }

    // Clear samples collected during the e-paper recording indicator update.
    for(uint8_t batch = 0; batch < 8; batch++)
    {
        size_t discarded = 0;
        const esp_err_t error = i2s_channel_read(
            g_ft02AudioRx,
            g_ft02AudioStereoBuffer,
            sizeof(g_ft02AudioStereoBuffer),
            &discarded,
            0
        );
        if(error == ESP_ERR_TIMEOUT || discarded == 0) break;
        if(error != ESP_OK)
        {
            FT02_AudioLogEspError("[AUDIO-REC] pre-start drain", error);
            break;
        }
    }

    FILE* file = FT02_StorageOpenWriteFile(g_ft02AudioRecording.filePath, true);
    if(file == nullptr || !FT02_AudioWriteWavHeader(file, 0))
    {
        if(file != nullptr) fclose(file);
        FT02_AudioSetMessage("无法创建WAV文件", false);
        FT02_AudioChanged();
        return false;
    }

    g_ft02AudioRecording.file = file;
    g_ft02AudioRecording.startMs = millis();
    g_ft02AudioRecording.stopArmed = false;
    g_ft02AudioState = FT02_AUDIO_LOG_RECORDING;
    g_ft02AudioStatus.recording = true;
    g_ft02AudioStatus.stopRequested = false;
    g_ft02AudioStatus.recordingElapsedMs = 0;
    g_ft02AudioStatus.recordingSeconds = 0;
    g_ft02AudioStatus.activeSequence = g_ft02AudioRecording.sequence;
    snprintf(
        g_ft02AudioStatus.activeAudioId,
        sizeof(g_ft02AudioStatus.activeAudioId),
        "%s",
        g_ft02AudioRecording.audioId
    );
    snprintf(
        g_ft02AudioStatus.activeFilePath,
        sizeof(g_ft02AudioStatus.activeFilePath),
        "%s",
        g_ft02AudioRecording.filePath
    );
    FT02_AudioSetMessage("录音中，再按R结束", true);
    FT02_AudioChanged();

    Serial.printf(
        "[AUDIO-REC] begin id=%s path=%s rate=%lu channels=1 postRoll=%lums max=%lus\n",
        g_ft02AudioRecording.audioId,
        g_ft02AudioRecording.filePath,
        static_cast<unsigned long>(FT02_AUDIO_SAMPLE_RATE),
        static_cast<unsigned long>(FT02_AUDIO_POST_ROLL_MS),
        static_cast<unsigned long>(FT02_AUDIO_MAX_SECONDS)
    );
    return true;
}

bool FT02_AudioLogRequestStop()
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_RECORDING) return false;
    const uint32_t now = millis();
    const uint32_t elapsed = now - g_ft02AudioRecording.startMs;
    if(!g_ft02AudioRecording.stopArmed || elapsed < FT02_AUDIO_MIN_STOP_MS)
    {
        Serial.println("[AUDIO-REC] stop ignored until R release/minimum duration");
        return false;
    }

    g_ft02AudioState = FT02_AUDIO_LOG_POST_ROLL;
    g_ft02AudioRecording.stopDeadlineMs = now + FT02_AUDIO_POST_ROLL_MS;
    g_ft02AudioStatus.stopRequested = true;
    FT02_AudioSetMessage("正在保存尾音", true);
    Serial.printf(
        "[AUDIO-REC] stop requested elapsed=%lu.%03lus tail=%lums\n",
        static_cast<unsigned long>(elapsed / 1000u),
        static_cast<unsigned long>(elapsed % 1000u),
        static_cast<unsigned long>(FT02_AUDIO_POST_ROLL_MS)
    );
    return true;
}

bool FT02_AudioLogPlayNewest(uint16_t newestIndex)
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_IDLE) return false;
    FT02AudioLogEntry entry;
    if(!FT02_AudioLogGetNewest(newestIndex, entry) || !entry.fileExists)
    {
        FT02_AudioSetMessage("录音文件不存在", false);
        return false;
    }
    if(!g_ft02AudioStatus.codecReady || !g_ft02AudioStatus.i2sReady)
    {
        FT02_AudioSetMessage("播放模块未就绪", false);
        return false;
    }

    FILE* file = FT02_StorageOpenReadFile(entry.filePath);
    if(file == nullptr)
    {
        FT02_AudioSetMessage("无法打开录音文件", false);
        return false;
    }

    FT02WavInfo info;
    if(!FT02_AudioReadWavInfo(file, info))
    {
        fclose(file);
        FT02_AudioSetMessage("WAV格式不支持", false);
        return false;
    }

    if(fseek(file, static_cast<long>(info.dataOffset), SEEK_SET) != 0)
    {
        fclose(file);
        FT02_AudioSetMessage("WAV定位失败", false);
        return false;
    }

    uint64_t sumSqLeft = 0;
    uint64_t sumSqRight = 0;
    uint32_t bytesRemaining = info.dataBytes;
    while(bytesRemaining > 0)
    {
        const size_t bytesWanted = bytesRemaining < sizeof(g_ft02AudioStereoBuffer)
            ? static_cast<size_t>(bytesRemaining)
            : sizeof(g_ft02AudioStereoBuffer);
        const size_t bytesRead = fread(g_ft02AudioStereoBuffer, 1, bytesWanted, file);
        if(bytesRead == 0) break;

        if(info.channels == 2)
        {
            const size_t frames = bytesRead / (2u * sizeof(int16_t));
            for(size_t i = 0; i < frames; i++)
            {
                const int32_t left = g_ft02AudioStereoBuffer[i * 2u];
                const int32_t right = g_ft02AudioStereoBuffer[i * 2u + 1u];
                sumSqLeft += static_cast<uint64_t>(static_cast<int64_t>(left) * left);
                sumSqRight += static_cast<uint64_t>(static_cast<int64_t>(right) * right);
            }
        }
        else
        {
            const size_t frames = bytesRead / sizeof(int16_t);
            for(size_t i = 0; i < frames; i++)
            {
                const int32_t sample = g_ft02AudioStereoBuffer[i];
                sumSqLeft += static_cast<uint64_t>(static_cast<int64_t>(sample) * sample);
            }
        }
        bytesRemaining -= static_cast<uint32_t>(bytesRead);
        yield();
    }

    const bool useRight = info.channels == 2 && sumSqRight > sumSqLeft;
    const uint32_t frameCount = info.channels > 0
        ? info.dataBytes / (static_cast<uint32_t>(info.channels) * sizeof(int16_t))
        : 0;
    const uint64_t selectedSumSq = useRight ? sumSqRight : sumSqLeft;
    const float selectedRms = frameCount > 0
        ? static_cast<float>(sqrt(
            static_cast<double>(selectedSumSq) /
            static_cast<double>(frameCount)
        ))
        : 0.0f;
    float normalizationGain = 1.0f;
    if(selectedRms > 1.0f && selectedRms < FT02_AUDIO_PLAYBACK_TARGET_RMS)
    {
        normalizationGain = FT02_AUDIO_PLAYBACK_TARGET_RMS / selectedRms;
        if(normalizationGain > FT02_AUDIO_PLAYBACK_MAX_GAIN)
        {
            normalizationGain = FT02_AUDIO_PLAYBACK_MAX_GAIN;
        }
    }

    if(fseek(file, static_cast<long>(info.dataOffset), SEEK_SET) != 0)
    {
        fclose(file);
        FT02_AudioSetMessage("WAV回卷失败", false);
        return false;
    }

    g_ft02AudioPlayback = FT02AudioPlaybackSession{};
    g_ft02AudioPlayback.file = file;
    g_ft02AudioPlayback.info = info;
    g_ft02AudioPlayback.useRight = useRight;
    g_ft02AudioPlayback.bytesRemaining = info.dataBytes;
    g_ft02AudioPlayback.totalFrames = frameCount;
    g_ft02AudioPlayback.normalizationGain = normalizationGain;
    snprintf(
        g_ft02AudioPlayback.audioId,
        sizeof(g_ft02AudioPlayback.audioId),
        "%s",
        entry.audioId
    );
    snprintf(
        g_ft02AudioPlayback.filePath,
        sizeof(g_ft02AudioPlayback.filePath),
        "%s",
        entry.filePath
    );

    g_ft02AudioState = FT02_AUDIO_LOG_PLAYING;
    g_ft02AudioStatus.playing = true;
    g_ft02AudioStatus.playbackStopRequested = false;
    g_ft02AudioStatus.playbackElapsedMs = 0;
    g_ft02AudioStatus.playbackDurationMs = static_cast<uint32_t>(
        (static_cast<uint64_t>(frameCount) * 1000u) / FT02_AUDIO_SAMPLE_RATE
    );
    g_ft02AudioStatus.activeSequence = entry.sequence;
    snprintf(
        g_ft02AudioStatus.activeAudioId,
        sizeof(g_ft02AudioStatus.activeAudioId),
        "%s",
        entry.audioId
    );
    snprintf(
        g_ft02AudioStatus.activeFilePath,
        sizeof(g_ft02AudioStatus.activeFilePath),
        "%s",
        entry.filePath
    );
    FT02_AudioSetMessage("正在播放，B或ENTER停止", true);
    FT02_AudioChanged();
    Serial.printf(
        "[AUDIO-PLAY] begin id=%s path=%s channels=%u bytes=%lu duration=%lu.%03lus rms=%.1f normalize=%.2fx volume=%u/10 userGain=%.2fx\n",
        entry.audioId,
        entry.filePath,
        info.channels,
        static_cast<unsigned long>(info.dataBytes),
        static_cast<unsigned long>(g_ft02AudioStatus.playbackDurationMs / 1000u),
        static_cast<unsigned long>(g_ft02AudioStatus.playbackDurationMs % 1000u),
        selectedRms,
        normalizationGain,
        static_cast<unsigned int>(g_ft02AudioStatus.playbackVolumeLevel),
        FT02_AudioUserVolumeGain(g_ft02AudioStatus.playbackVolumeLevel)
    );
    return true;
}


void FT02_AudioLogReleasePlaybackStart()
{
    if(g_ft02AudioState == FT02_AUDIO_LOG_PLAYING)
    {
        g_ft02AudioPlayback.startReleased = true;
        Serial.println("[AUDIO-PLAY] UI committed; streaming released");
    }
}

bool FT02_AudioLogRequestPlaybackStop()
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_PLAYING) return false;
    if(g_ft02AudioPlayback.stopRequested) return true;
    g_ft02AudioPlayback.stopRequested = true;
    g_ft02AudioStatus.playbackStopRequested = true;
    FT02_AudioSetMessage("正在停止播放", true);
    Serial.printf(
        "[AUDIO-PLAY] stop requested at %lu.%03lus\n",
        static_cast<unsigned long>(g_ft02AudioStatus.playbackElapsedMs / 1000u),
        static_cast<unsigned long>(g_ft02AudioStatus.playbackElapsedMs % 1000u)
    );
    return true;
}

bool FT02_AudioLogAdjustPlaybackVolume(int8_t delta)
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_PLAYING || delta == 0) return false;
    int16_t next = static_cast<int16_t>(g_ft02AudioStatus.playbackVolumeLevel) + delta;
    if(next < FT02_AUDIO_VOLUME_MIN_LEVEL) next = FT02_AUDIO_VOLUME_MIN_LEVEL;
    if(next > FT02_AUDIO_VOLUME_MAX_LEVEL) next = FT02_AUDIO_VOLUME_MAX_LEVEL;
    if(next == g_ft02AudioStatus.playbackVolumeLevel) return false;

    g_ft02AudioStatus.playbackVolumeLevel = static_cast<uint8_t>(next);
    g_ft02AudioPlayback.volumeDirty = true;
    Serial.printf(
        "[AUDIO-PLAY] volume=%u/10 gain=%.2fx persist=on_finish\n",
        static_cast<unsigned int>(g_ft02AudioStatus.playbackVolumeLevel),
        FT02_AudioUserVolumeGain(g_ft02AudioStatus.playbackVolumeLevel)
    );
    return true;
}

bool FT02_AudioLogDeleteNewest(uint16_t newestIndex)
{
    if(g_ft02AudioState != FT02_AUDIO_LOG_IDLE) return false;
    FT02AudioLogEntry entry;
    if(!FT02_AudioLogGetNewest(newestIndex, entry)) return false;

    bool fileDeleted = true;
    if(entry.fileExists)
    {
        fileDeleted = FT02_StorageDeleteFile(entry.filePath);
    }
    const bool indexUpdated = FT02_AudioAppendDeleteRecord(entry);
    const bool success = fileDeleted && indexUpdated;
    FT02_AudioSetMessage(success ? "语音日志已删除" : "删除失败", success);
    (void)FT02_AudioLogReload();
    FT02_AudioChanged();
    return success;
}

const char* FT02_AudioLogIndexPath()
{
    return FT02_AUDIO_INDEX;
}

const char* FT02_AudioLogDirectory()
{
    return FT02_AUDIO_DIR;
}
