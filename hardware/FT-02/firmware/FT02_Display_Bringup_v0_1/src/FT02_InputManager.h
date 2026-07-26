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
    char raw;
};

void FT02_InputBegin(
    int sdaPin,
    int sclPin,
    uint8_t cardKbAddress
);

FT02InputEvent FT02_InputPoll();

const char* FT02_InputKeyName(
    FT02InputKey key
);
