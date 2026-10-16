#pragma once

#include <Arduino.h>
#include "FT02_Gnss.h"

constexpr uint16_t FT02_AUDIO_LOG_MAX_ENTRIES = 64;
constexpr uint8_t FT02_AUDIO_VOLUME_MIN_LEVEL = 0;
constexpr uint8_t FT02_AUDIO_VOLUME_MAX_LEVEL = 10;
constexpr uint8_t FT02_AUDIO_VOLUME_DEFAULT_LEVEL = 7;

enum FT02AudioLogState : uint8_t
{
    FT02_AUDIO_LOG_IDLE = 0,
    FT02_AUDIO_LOG_RECORDING,
    FT02_AUDIO_LOG_POST_ROLL,
    FT02_AUDIO_LOG_PLAYING,
    FT02_AUDIO_LOG_ERROR
};

// Audio hardware is powered only while a real consumer owns it.
// Keep the owner set explicit so future modules cannot accidentally leave
// ES8311/I2S running after their page has closed.
enum FT02AudioOwner : uint8_t
{
    FT02_AUDIO_OWNER_NONE = 0,
    FT02_AUDIO_OWNER_JOURNAL = 1u << 0,
    FT02_AUDIO_OWNER_SYSTEM_TEST = 1u << 1,
    FT02_AUDIO_OWNER_MESH_VOICE = 1u << 2
};


struct FT02AudioTransientResult
{
    bool ready;
    bool success;
    uint32_t durationMs;
    char filePath[112];
};

struct FT02AudioLogEntry
{
    bool valid;
    bool fileExists;
    bool gnssFix;
    uint32_t sequence;
    uint32_t samples;
    uint32_t durationMs;
    uint64_t sizeBytes;
    double latitude;
    double longitude;
    char audioId[48];
    char sessionId[48];
    char filePath[112];
    char filename[72];
    char deviceDate[16];
    char deviceTime[16];
};

struct FT02AudioLogStatus
{
    bool started;
    bool codecReady;
    bool i2sReady;
    bool storageReady;
    bool listLoaded;
    bool recording;
    bool stopRequested;
    bool playing;
    bool playbackStopRequested;
    bool lastOperationOk;
    uint16_t count;
    uint32_t generation;
    uint32_t recordingElapsedMs;
    uint32_t recordingSeconds;
    uint32_t playbackElapsedMs;
    uint32_t playbackDurationMs;
    uint8_t playbackVolumeLevel;
    uint32_t activeSequence;
    uint32_t lastDurationMs;
    uint32_t lastSamples;
    uint64_t lastSizeBytes;
    int32_t lastPeak;
    char activeAudioId[48];
    char activeFilePath[112];
    char message[112];
};

bool FT02_AudioLogBegin();
// Acquire/release the physical audio hardware. Begin() only initializes the
// journal data model and leaves ES8311/I2S asleep.
bool FT02_AudioLogAcquire(FT02AudioOwner owner);
void FT02_AudioLogRelease(FT02AudioOwner owner);
// Close idle audio hardware and verify the codec reached suspend before system sleep.
bool FT02_AudioLogPrepareForSleep();
bool FT02_AudioLogHardwareActive();
uint8_t FT02_AudioLogOwnerMask();
void FT02_AudioLogPoll();
void FT02_AudioLogNotifyKeyRelease();

FT02AudioLogStatus FT02_AudioLogStatusCurrent();
FT02AudioLogState FT02_AudioLogStateCurrent();
bool FT02_AudioLogIsCapturing();
bool FT02_AudioLogIsPlaying();
bool FT02_AudioLogIsBusy();

bool FT02_AudioLogReload();
bool FT02_AudioLogGetNewest(uint16_t newestIndex, FT02AudioLogEntry& output);
uint16_t FT02_AudioLogWrapIndex(uint16_t current, uint16_t count, int32_t step);

bool FT02_AudioLogStartRecording(const FT02GnssSnapshot& gnss);
// Mesh voice uses the exact same ES8311/I2S lifecycle as the journal, but
// records a temporary WAV without creating a journal/index entry.
bool FT02_AudioLogStartTransientRecording(const char* filePath, uint32_t maxDurationMs);
bool FT02_AudioLogRequestTransientStop();
FT02AudioTransientResult FT02_AudioLogTransientResultCurrent();
void FT02_AudioLogClearTransientResult();
void FT02_AudioLogClearTransientStopTrace();
bool FT02_AudioLogRequestStop();
bool FT02_AudioLogPlayNewest(uint16_t newestIndex);
bool FT02_AudioLogPlayWavPath(const char* filePath, const char* audioId = "mesh_voice");
void FT02_AudioLogReleasePlaybackStart();
bool FT02_AudioLogRequestPlaybackStop();
bool FT02_AudioLogAdjustPlaybackVolume(int8_t delta);
bool FT02_AudioLogSetPlaybackEpdGuard(bool enabled);
bool FT02_AudioLogDeleteNewest(uint16_t newestIndex);

const char* FT02_AudioLogIndexPath();
const char* FT02_AudioLogDirectory();
