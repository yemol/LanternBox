#pragma once

#include <stdint.h>

enum FT02MeshVoiceAudioState : uint8_t
{
    FT02_MESH_VOICE_AUDIO_IDLE = 0,
    FT02_MESH_VOICE_AUDIO_RECORDING,
    FT02_MESH_VOICE_AUDIO_SAVING,
    FT02_MESH_VOICE_AUDIO_ENCODING,
    FT02_MESH_VOICE_AUDIO_PLAYING,
    FT02_MESH_VOICE_AUDIO_ERROR
};

struct FT02MeshVoiceAudioStatus
{
    FT02MeshVoiceAudioState state;
    uint32_t revision;
    uint32_t destination;
    uint32_t voiceId;
    uint32_t elapsedMs;
    uint32_t durationMs;
    bool discardPending;
    char message[96];
};

void FT02_MeshVoiceAudioBegin();
void FT02_MeshVoiceAudioPoll();

// destination may be FT02_MESH_BROADCAST_NODE or a PKI-capable direct peer.
bool FT02_MeshVoiceAudioStartRecording(uint32_t destination);
// Called only after the partial START SPEAKING cue has physically completed.
// This defines t=0 for the user-visible 15 s voice window and the encoder trim.
void FT02_MeshVoiceAudioMarkUserStartCue();
bool FT02_MeshVoiceAudioStopRecording();
bool FT02_MeshVoiceAudioCancelRecording();

// Decode the selected .lbv to a temporary WAV and start the proven AudioLog
// playback path. The caller should redraw the page, then call ReleasePlaybackStart.
bool FT02_MeshVoiceAudioStartPlayback(const char* lbvPath);
void FT02_MeshVoiceAudioReleasePlaybackStart();
bool FT02_MeshVoiceAudioStopPlayback();

FT02MeshVoiceAudioStatus FT02_MeshVoiceAudioStatusCurrent();
bool FT02_MeshVoiceAudioIsRecording();
bool FT02_MeshVoiceAudioIsPlaying();
bool FT02_MeshVoiceAudioBusy();
