#pragma once

#include <Arduino.h>

enum FT02InputKey
{
    FT02_KEY_NONE = 0,

    FT02_KEY_UP,
    FT02_KEY_DOWN,
    FT02_KEY_LEFT,
    FT02_KEY_RIGHT,

    FT02_KEY_SELECT,
    FT02_KEY_BACK,
    FT02_KEY_HELP,
    FT02_KEY_IME_TOGGLE,

    FT02_KEY_CHAR
};

struct FT02InputEvent
{
    FT02InputKey key;

    // Exact byte returned by CardKB2. Keep this untouched for text-entry
    // surfaces and low-level diagnostics.
    char raw;

    // Lower-case UI command normalized across CardKB2 Aa/Sym states.
    // Example: Sym+D returns '^', but command remains 'd'. This prevents
    // sticky modifier state from disabling FT-02 navigation/hotkeys while
    // preserving raw punctuation for future text input. Zero means there is
    // no ASCII command representation (for example Fn direction bytes).
    char command;
};

void FT02_InputBegin(
    int sdaPin,
    int sclPin,
    uint8_t cardKbAddress
);

FT02InputEvent FT02_InputPoll();

// Low-noise capture mode keeps R/stop responsive while reducing CardKB2 I2C
// activity to 10 Hz, cutting digital-edge coupling into the analogue mic path.
void FT02_InputSetLowNoiseMode(bool enabled);

// Sleep-safe diagnostic gate. Disable before entering light sleep so the input
// driver never touches native USB CDC while the S3 USB clock is gated.
void FT02_InputSetDiagnosticsEnabled(bool enabled);

// Passive availability state maintained by the existing periodic CardKB2 probe.
bool FT02_InputCardKbAvailable();


// Sleep Power A4 / CardKB2 SCL2PHASE coordination.
// Normal key data remains the frozen A1.8 RAMREG protocol. Sleep coordination
// is out-of-band: SCL REQUEST -> RAMREG SLEEP_ARMED status -> SCL COMMIT.
bool FT02_InputRequestHostSleep();
bool FT02_InputSuspendBusForSleep();
bool FT02_InputSleepSclRtcReady();
bool FT02_InputSleepSclIsLow();
bool FT02_InputResumeBusAfterSleep(uint32_t timeoutMs = 500);
bool FT02_InputBusSuspended();

// Stabilized current CardKB activity byte. Because CardKB2 I2C mode does not
// expose physical key-up frames, short zero gaps between repeat events are
// ignored. Zero means the input stream has been quiet long enough to count as
// released for UI/sleep gates.
char FT02_InputCurrentRawKey();

const char* FT02_InputKeyName(
    FT02InputKey key
);
