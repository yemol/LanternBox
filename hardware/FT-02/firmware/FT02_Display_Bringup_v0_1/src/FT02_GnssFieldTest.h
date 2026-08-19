#pragma once
#include <Arduino.h>

// Historical API name retained for UI compatibility. This is now an LR01
// navigation-state logger; it never receives raw GNSS bytes or sentences.
struct FT02GnssFieldTestSnapshot
{
    bool active;
    bool storageReady;
    uint32_t startedMs;
    uint32_t durationSeconds;
    uint32_t rawSentenceCount;      // always 0; compatibility field
    uint32_t snapshotCount;
    uint32_t invalidSentenceCount;  // always 0; compatibility field
    uint32_t writeErrorCount;
    uint32_t uiGeneration;
    char path[96];
    char lastAction[64];
};
void FT02_GnssFieldTestBegin();
void FT02_GnssFieldTestPoll();
bool FT02_GnssFieldTestStart();
void FT02_GnssFieldTestStop();
void FT02_GnssFieldTestToggle();
FT02GnssFieldTestSnapshot FT02_GnssFieldTestSnapshotCurrent();
