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
bool FT02_AudioLogRequestStop();
bool FT02_AudioLogPlayNewest(uint16_t newestIndex);
void FT02_AudioLogReleasePlaybackStart();
bool FT02_AudioLogRequestPlaybackStop();
bool FT02_AudioLogAdjustPlaybackVolume(int8_t delta);
bool FT02_AudioLogDeleteNewest(uint16_t newestIndex);

const char* FT02_AudioLogIndexPath();
const char* FT02_AudioLogDirectory();
