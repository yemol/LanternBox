#include "FT02_MeshVoiceAudio.h"

#include "FT02_AudioLog.h"
#include "FT02_Log.h"
#include "FT02_MeshProtocol.h"
#include "FT02_MeshService.h"
#include "FT02_MeshVoiceProtocol.h"
#include "FT02_MeshVoiceCodec.h"
#include "FT02_MeshVoiceTransport.h"
#include "FT02_Storage.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace
{
// Hardware fail-safe only. The user-facing limit is enforced from the
// actual visible START SPEAKING cue, not from ADC start.
constexpr uint32_t MAX_RECORD_MS = FT02_VOICE_CAPTURE_RAW_MAX_MS;
constexpr const char* PLAYBACK_WAV = "/lanternbox/mesh/voice/.playback.wav";

FT02MeshVoiceAudioStatus g_status{};
char g_rawPath[112] = {};
char g_lbvPath[112] = {};
bool g_audioOwned = false;
bool g_playbackStartPending = false;
bool g_discardPending = false;
uint32_t g_encodeNotBeforeMs = 0u;
uint32_t g_captureTrimMs = FT02_VOICE_CAPTURE_PREROLL_MS;
bool g_userCueArmed = false;

void changed()
{
    ++g_status.revision;
    if(g_status.revision == 0u) g_status.revision = 1u;
}

void setMessage(const char* message)
{
    snprintf(g_status.message, sizeof(g_status.message), "%s", message != nullptr ? message : "");
}

void releaseOwner()
{
    if(!g_audioOwned) return;
    FT02_AudioLogRelease(FT02_AUDIO_OWNER_MESH_VOICE);
    g_audioOwned = false;
}

void cleanupCaptureFiles(bool keepLbv)
{
    if(g_rawPath[0] != '\0') (void)FT02_StorageDeleteFile(g_rawPath);
    if(!keepLbv && g_lbvPath[0] != '\0') (void)FT02_StorageDeleteFile(g_lbvPath);
    g_rawPath[0] = '\0';
    if(!keepLbv) g_lbvPath[0] = '\0';
}

void finishIdle(const char* message)
{
    g_status.state = FT02_MESH_VOICE_AUDIO_IDLE;
    g_status.elapsedMs = 0u;
    g_status.durationMs = 0u;
    g_status.discardPending = false;
    g_discardPending = false;
    g_encodeNotBeforeMs = 0u;
    g_captureTrimMs = FT02_VOICE_CAPTURE_PREROLL_MS;
    g_userCueArmed = false;
    setMessage(message);
    changed();
}

void fail(const char* message)
{
    cleanupCaptureFiles(false);
    releaseOwner();
    g_status.state = FT02_MESH_VOICE_AUDIO_ERROR;
    g_status.discardPending = false;
    g_discardPending = false;
    setMessage(message);
    changed();
    FT02_LOGW("[WARN][MESH-VOICE-AUDIO] %s\n", message != nullptr ? message : "error");
}
}

void FT02_MeshVoiceAudioBegin()
{
    FT02_MeshVoiceCodecReportResetTrace();
    memset(&g_status, 0, sizeof(g_status));
    g_status.state = FT02_MESH_VOICE_AUDIO_IDLE;
    g_status.revision = 1u;
    g_rawPath[0] = '\0';
    g_lbvPath[0] = '\0';
    g_audioOwned = false;
    g_playbackStartPending = false;
    g_discardPending = false;
    g_encodeNotBeforeMs = 0u;
    g_captureTrimMs = FT02_VOICE_CAPTURE_PREROLL_MS;
    g_userCueArmed = false;
    (void)FT02_StorageDeleteFile(PLAYBACK_WAV);
}

void FT02_MeshVoiceAudioPoll()
{
    if(g_status.state == FT02_MESH_VOICE_AUDIO_RECORDING ||
       g_status.state == FT02_MESH_VOICE_AUDIO_SAVING)
    {
        const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
        const uint32_t rawElapsedMs = audio.recordingElapsedMs > MAX_RECORD_MS ? MAX_RECORD_MS : audio.recordingElapsedMs;
        g_status.elapsedMs = (g_userCueArmed && rawElapsedMs > g_captureTrimMs)
            ? rawElapsedMs - g_captureTrimMs : 0u;
        if(g_status.elapsedMs > FT02_VOICE_TX_MAX_DURATION_MS)
            g_status.elapsedMs = FT02_VOICE_TX_MAX_DURATION_MS;

        // The 15 s limit begins when START SPEAKING is actually visible. The
        // AudioLog max is only a safety fence in case this service stalls.
        if(g_userCueArmed &&
           g_status.state == FT02_MESH_VOICE_AUDIO_RECORDING &&
           g_status.elapsedMs >= FT02_VOICE_TX_MAX_DURATION_MS)
        {
            if(FT02_AudioLogRequestTransientStop())
            {
                g_status.state = FT02_MESH_VOICE_AUDIO_SAVING;
                setMessage("已到15秒 · 正在保存语音");
                changed();
            }
        }

        // A cancel pressed during the first 600 ms cannot stop AudioLog yet.
        // Keep the discard intent and issue the stop as soon as the minimum
        // recording interval has elapsed.
        if(g_discardPending && g_status.state == FT02_MESH_VOICE_AUDIO_RECORDING &&
           rawElapsedMs >= 650u)
        {
            if(FT02_AudioLogRequestTransientStop())
            {
                g_status.state = FT02_MESH_VOICE_AUDIO_SAVING;
                setMessage("正在取消语音录制");
                changed();
            }
        }

        const FT02AudioTransientResult result = FT02_AudioLogTransientResultCurrent();
        if(!result.ready) return;

        FT02_LOGI("[MESH-VOICE-AUDIO] transient ready success=%u duration=%lu path=%s\n",
                  result.success ? 1u : 0u,
                  static_cast<unsigned long>(result.durationMs),
                  result.filePath);
        Serial.flush();

        // The audio task publishes transient ready only after it has fully
        // finalized the WAV and returned the shared AudioLog state to IDLE.
        // Clear/take the mailbox first, then release the physical audio owner.
        FT02_AudioLogClearTransientResult();
        FT02_LOGI_PRINTLN("[MESH-VOICE-AUDIO] release owner begin");
        Serial.flush();
        releaseOwner();
        FT02_LOGI_PRINTLN("[MESH-VOICE-AUDIO] release owner done");
        Serial.flush();

        if(g_discardPending)
        {
            if(result.filePath[0] != '\0') (void)FT02_StorageDeleteFile(result.filePath);
            cleanupCaptureFiles(false);
            finishIdle("语音录制已取消");
            return;
        }
        if(!result.success)
        {
            fail("语音录音保存失败");
            return;
        }

        // Do not start Codec2 in the same scheduling instant that tears down
        // ES8311/I2S. Persist the completed WAV path, publish ENCODING state,
        // then give both cores a short settle window. This makes the audio
        // producer/consumer boundary explicit and prevents a stop->encode
        // collision from sharing the same main-loop turn.
        snprintf(g_rawPath, sizeof(g_rawPath), "%s", result.filePath);
        g_status.state = FT02_MESH_VOICE_AUDIO_ENCODING;
        g_status.durationMs = result.durationMs;
        g_encodeNotBeforeMs = millis() + 300u;
        setMessage("录音已保存 · 正在准备压缩");
        changed();
        FT02_LOGI("[MESH-VOICE-AUDIO] encode deferred until=%lu raw=%s\n",
                  static_cast<unsigned long>(g_encodeNotBeforeMs),
                  g_rawPath);
        Serial.flush();
        return;
    }

    if(g_status.state == FT02_MESH_VOICE_AUDIO_ENCODING)
    {
        if(static_cast<int32_t>(millis() - g_encodeNotBeforeMs) < 0) return;

        uint32_t durationMs = 0u;
        uint32_t encodedBytes = 0u;
        FT02_LOGI("[MESH-VOICE-AUDIO] encode begin wav=%s lbv=%s\n",
                  g_rawPath, g_lbvPath);
        Serial.flush();

        const bool encoded = FT02_MeshVoiceEncodeWavToLbv(g_rawPath, g_lbvPath,
                                                           g_captureTrimMs,
                                                           durationMs, encodedBytes);
        (void)FT02_StorageDeleteFile(g_rawPath);
        g_rawPath[0] = '\0';
        if(!encoded)
        {
            FT02_AudioLogClearTransientStopTrace();
            fail("语音压缩失败");
            return;
        }

        g_status.durationMs = durationMs;
        if(!FT02_MeshVoiceTransportQueueFile(g_status.destination, g_lbvPath, g_status.voiceId))
        {
            FT02_AudioLogClearTransientStopTrace();
            fail("语音无法加入Mesh发送队列");
            return;
        }

        // The history record now owns the .lbv path. It remains on SD until
        // that message ages out of the 100-record history.
        g_lbvPath[0] = '\0';
        char message[96] = {};
        snprintf(message, sizeof(message), "语音已加入发送队列 · %lus · %luB",
                 static_cast<unsigned long>((durationMs + 500u) / 1000u),
                 static_cast<unsigned long>(encodedBytes));
        FT02_AudioLogClearTransientStopTrace();
        finishIdle(message);
        return;
    }

    if(g_status.state == FT02_MESH_VOICE_AUDIO_PLAYING)
    {
        const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
        g_status.elapsedMs = audio.playbackElapsedMs;
        g_status.durationMs = audio.playbackDurationMs;
        if(FT02_AudioLogIsPlaying()) return;

        (void)FT02_StorageDeleteFile(PLAYBACK_WAV);
        releaseOwner();
        g_playbackStartPending = false;
        finishIdle("语音播放结束");
    }
}

bool FT02_MeshVoiceAudioStartRecording(uint32_t destination)
{
    if(g_status.state != FT02_MESH_VOICE_AUDIO_IDLE && g_status.state != FT02_MESH_VOICE_AUDIO_ERROR) return false;
    if(FT02_AudioLogIsBusy() || FT02_MeshVoiceTransportBusy())
    {
        setMessage(FT02_MeshVoiceTransportBusy() ? "上一条语音仍在发送" : "音频设备正在使用");
        changed();
        return false;
    }
    const bool direct = destination != FT02_MESH_BROADCAST_NODE;
    if(direct && !FT02_MeshServiceCanSendPrivate(destination))
    {
        setMessage("目标节点没有可用PKI公钥");
        changed();
        return false;
    }
    if(!direct) destination = FT02_MESH_BROADCAST_NODE;

    const uint32_t voiceId = FT02_MeshServiceAllocatePacketId();
    if(voiceId == 0u) return false;
    snprintf(g_rawPath, sizeof(g_rawPath), "/lanternbox/mesh/voice/.record_%08lX.wav",
             static_cast<unsigned long>(voiceId));
    snprintf(g_lbvPath, sizeof(g_lbvPath), "/lanternbox/mesh/voice/tx_%08lX.lbv",
             static_cast<unsigned long>(voiceId));
    (void)FT02_StorageDeleteFile(g_rawPath);
    (void)FT02_StorageDeleteFile(g_lbvPath);
    g_captureTrimMs = FT02_VOICE_CAPTURE_PREROLL_MS;
    g_userCueArmed = false;

    FT02_LOGI("[MESH-VOICE-AUDIO] acquire begin dest=0x%08lX voice=0x%08lX\n",
              static_cast<unsigned long>(destination),
              static_cast<unsigned long>(voiceId));
    if(!FT02_AudioLogAcquire(FT02_AUDIO_OWNER_MESH_VOICE))
    {
        setMessage("无法启动Mesh语音硬件");
        changed();
        FT02_LOGW_PRINTLN("[WARN][MESH-VOICE-AUDIO] acquire failed");
        return false;
    }
    g_audioOwned = true;
    FT02_LOGI_PRINTLN("[MESH-VOICE-AUDIO] acquire OK; arming transient capture");
    FT02_AudioLogClearTransientResult();
    if(!FT02_AudioLogStartTransientRecording(g_rawPath, MAX_RECORD_MS))
    {
        releaseOwner();
        cleanupCaptureFiles(false);
        setMessage("无法开始语音录制");
        changed();
        FT02_LOGW_PRINTLN("[WARN][MESH-VOICE-AUDIO] transient capture start failed");
        return false;
    }

    FT02_LOGI_PRINTLN("[MESH-VOICE-AUDIO] transient capture started");
    g_status.state = FT02_MESH_VOICE_AUDIO_RECORDING;
    g_status.destination = destination;
    g_status.voiceId = voiceId;
    g_status.elapsedMs = 0u;
    g_status.durationMs = 0u;
    g_status.discardPending = false;
    g_discardPending = false;
    setMessage(direct ? "私聊语音录制中 · 最长15秒" : "广播语音录制中 · 最长15秒");
    changed();
    return true;
}

void FT02_MeshVoiceAudioMarkUserStartCue()
{
    if(g_status.state != FT02_MESH_VOICE_AUDIO_RECORDING) return;
    const FT02AudioLogStatus audio = FT02_AudioLogStatusCurrent();
    uint32_t trimMs = audio.recordingElapsedMs;
    if(trimMs < FT02_VOICE_CAPTURE_PREROLL_MS) trimMs = FT02_VOICE_CAPTURE_PREROLL_MS;
    if(trimMs > FT02_VOICE_CAPTURE_MAX_PREROLL_MS) trimMs = FT02_VOICE_CAPTURE_MAX_PREROLL_MS;
    g_captureTrimMs = trimMs;
    g_userCueArmed = true;
    g_status.elapsedMs = 0u;
    setMessage("录音中 · 最长15秒");
    changed();
    FT02_LOGI("[MESH-VOICE-AUDIO] user start cue armed trim=%lums raw_elapsed=%lums\n",
              static_cast<unsigned long>(g_captureTrimMs),
              static_cast<unsigned long>(audio.recordingElapsedMs));
}

bool FT02_MeshVoiceAudioStopRecording()
{
    if(g_status.state != FT02_MESH_VOICE_AUDIO_RECORDING) return false;
    if(!FT02_AudioLogRequestTransientStop())
    {
        setMessage("录音不足0.6秒，暂不能结束");
        changed();
        return false;
    }
    g_status.state = FT02_MESH_VOICE_AUDIO_SAVING;
    setMessage("正在保存并压缩语音");
    changed();
    return true;
}

bool FT02_MeshVoiceAudioCancelRecording()
{
    if(g_status.state != FT02_MESH_VOICE_AUDIO_RECORDING &&
       g_status.state != FT02_MESH_VOICE_AUDIO_SAVING) return false;
    g_discardPending = true;
    g_status.discardPending = true;
    if(g_status.state == FT02_MESH_VOICE_AUDIO_RECORDING && FT02_AudioLogRequestTransientStop())
        g_status.state = FT02_MESH_VOICE_AUDIO_SAVING;
    setMessage("正在取消语音录制");
    changed();
    return true;
}

bool FT02_MeshVoiceAudioStartPlayback(const char* lbvPath)
{
    if(lbvPath == nullptr || lbvPath[0] == '\0') return false;
    if(g_status.state != FT02_MESH_VOICE_AUDIO_IDLE && g_status.state != FT02_MESH_VOICE_AUDIO_ERROR) return false;
    if(FT02_AudioLogIsBusy()) return false;

    (void)FT02_StorageDeleteFile(PLAYBACK_WAV);
    uint32_t durationMs = 0u;
    if(!FT02_MeshVoiceDecodeLbvToWav(lbvPath, PLAYBACK_WAV, durationMs))
    {
        setMessage("语音文件校验或解码失败");
        changed();
        return false;
    }
    if(!FT02_AudioLogAcquire(FT02_AUDIO_OWNER_MESH_VOICE))
    {
        (void)FT02_StorageDeleteFile(PLAYBACK_WAV);
        setMessage("无法启动语音播放硬件");
        changed();
        return false;
    }
    g_audioOwned = true;
    if(!FT02_AudioLogPlayWavPath(PLAYBACK_WAV, "mesh_voice"))
    {
        (void)FT02_StorageDeleteFile(PLAYBACK_WAV);
        releaseOwner();
        setMessage("无法播放语音");
        changed();
        return false;
    }

    g_status.state = FT02_MESH_VOICE_AUDIO_PLAYING;
    g_status.elapsedMs = 0u;
    g_status.durationMs = durationMs;
    g_playbackStartPending = true;
    setMessage("语音播放中 · ENTER停止");
    changed();
    return true;
}

void FT02_MeshVoiceAudioReleasePlaybackStart()
{
    if(!g_playbackStartPending) return;
    g_playbackStartPending = false;
    FT02_AudioLogReleasePlaybackStart();
}

bool FT02_MeshVoiceAudioStopPlayback()
{
    if(g_status.state != FT02_MESH_VOICE_AUDIO_PLAYING) return false;
    return FT02_AudioLogRequestPlaybackStop();
}

FT02MeshVoiceAudioStatus FT02_MeshVoiceAudioStatusCurrent()
{
    return g_status;
}

bool FT02_MeshVoiceAudioIsRecording()
{
    return g_status.state == FT02_MESH_VOICE_AUDIO_RECORDING ||
           g_status.state == FT02_MESH_VOICE_AUDIO_SAVING ||
           g_status.state == FT02_MESH_VOICE_AUDIO_ENCODING;
}

bool FT02_MeshVoiceAudioIsPlaying()
{
    return g_status.state == FT02_MESH_VOICE_AUDIO_PLAYING;
}

bool FT02_MeshVoiceAudioBusy()
{
    return FT02_MeshVoiceAudioIsRecording() || FT02_MeshVoiceAudioIsPlaying();
}
