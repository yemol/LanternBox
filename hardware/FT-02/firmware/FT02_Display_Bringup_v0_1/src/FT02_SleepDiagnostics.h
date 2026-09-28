#pragma once

#include <Arduino.h>

enum FT02SleepExitReason : uint8_t
{
    FT02_SLEEP_EXIT_NONE = 0,
    FT02_SLEEP_EXIT_KEYBOARD = 1,
    FT02_SLEEP_EXIT_RESET = 2
};

struct FT02SleepDiagnosticsSnapshot
{
    FT02SleepExitReason reason = FT02_SLEEP_EXIT_NONE;
    bool sleepSessionActive = false;
    uint32_t sleepSession = 0;
    uint32_t keyboardWakeCount = 0;
    uint32_t resetDuringSleepCount = 0;
    uint8_t lastRaw = 0;
    uint32_t lastSlices = 0;
    uint32_t lastDurationMs = 0;
    uint8_t lastResetReason = 0;
};

void FT02_SleepDiagnosticsBegin();
void FT02_SleepDiagnosticsMarkEnter(bool automatic);
void FT02_SleepDiagnosticsMarkKeyboardWake(
    uint8_t raw,
    uint32_t slices,
    uint32_t durationMs
);
FT02SleepDiagnosticsSnapshot FT02_SleepDiagnosticsSnapshotCurrent();
