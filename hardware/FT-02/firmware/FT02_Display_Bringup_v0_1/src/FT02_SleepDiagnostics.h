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

// Permanent sleep-health telemetry. Timing and battery boundary samples are held
// in RTC memory and never trigger I2C from the light-sleep hot path. A manual
// sleep starts a fresh measurement window; automatic re-sleep keeps the same
// window so awake gaps are not hidden.
struct FT02SleepTruthSnapshot
{
    bool valid = false;
    bool windowActive = false;
    uint32_t sessionCount = 0;
    uint32_t lightSleepEntryCount = 0;
    uint32_t lightSleepReturnCount = 0;
    uint32_t keyboardWakeCount = 0;
    uint32_t unexpectedReturnCount = 0;
    uint32_t resetDuringWindowCount = 0;
    int32_t lastSleepErr = 0;
    int32_t lastWakeCause = 0;
    uint64_t lastExt1Status = 0;
    uint64_t elapsedUs = 0;
    uint64_t lightSleepUs = 0;
    uint64_t awakeUs = 0;
    bool batterySampleValid = false;
    float batteryStartPercent = 0.0f;
    float batteryStartVoltage = 0.0f;
    float batteryWakePercent = 0.0f;
    float batteryWakeVoltage = 0.0f;
};

void FT02_SleepDiagnosticsBegin();
void FT02_SleepDiagnosticsMarkEnter(bool automatic);
void FT02_SleepDiagnosticsMarkKeyboardWake(
    uint8_t raw,
    uint32_t slices,
    uint32_t durationMs
);
FT02SleepDiagnosticsSnapshot FT02_SleepDiagnosticsSnapshotCurrent();

void FT02_SleepTruthSessionEnter(bool automatic);
void FT02_SleepTruthBeforeLightSleep();
void FT02_SleepTruthAfterLightSleep(
    int32_t sleepErr,
    int32_t wakeCause,
    uint64_t ext1Status
);
void FT02_SleepTruthSetBatteryStart(bool valid, float percent, float voltage);
void FT02_SleepTruthSetBatteryWake(bool valid, float percent, float voltage);
void FT02_SleepTruthMarkKeyboardWake();
void FT02_SleepTruthMarkUnexpectedReturn();
FT02SleepTruthSnapshot FT02_SleepTruthSnapshotCurrent();
