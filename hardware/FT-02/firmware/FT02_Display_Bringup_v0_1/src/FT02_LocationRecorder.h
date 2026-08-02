#pragma once

#include <Arduino.h>

#include "FT02_Gnss.h"

struct FT02LocationRecorderSnapshot
{
    bool sessionActive;
    bool autoTrackEnabled;
    bool originRecorded;
    bool storageReady;
    uint32_t sessionStartedMs;
    uint32_t durationSeconds;
    uint32_t pointCount;
    uint32_t failedPointCount;
    uint32_t writeFailureCount;
    uint32_t autoIntervalSeconds;
    uint32_t nextAutoSeconds;
    uint32_t uiGeneration;
    char sessionId[40];
    char lastAction[64];
};

void FT02_LocationRecorderBegin();
void FT02_LocationRecorderPoll();

bool FT02_LocationRecorderStartSession();
bool FT02_LocationRecorderStopSession(const char* note = "manual stop");
bool FT02_LocationRecorderToggleSession();
bool FT02_LocationRecorderRecordManualPoint();
bool FT02_LocationRecorderToggleAutoTrack();

FT02LocationRecorderSnapshot FT02_LocationRecorderSnapshotCurrent();
