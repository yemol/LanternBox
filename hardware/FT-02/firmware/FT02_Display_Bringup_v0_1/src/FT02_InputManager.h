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

// Current raw CardKB byte observed by the last poll. Zero means released.
char FT02_InputCurrentRawKey();

const char* FT02_InputKeyName(
    FT02InputKey key
);
