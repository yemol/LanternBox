#pragma once

#include <Arduino.h>

enum FT02SystemMode : uint8_t
{
    FT02_MODE_INDOOR = 0,
    FT02_MODE_OUTDOOR,
    FT02_MODE_SLEEP
};

void FT02_ModeManagerBegin();

FT02SystemMode FT02_ModeCurrent();
FT02SystemMode FT02_ModeAwakeMode();

bool FT02_ModeSetAwake(FT02SystemMode mode);
bool FT02_ModeToggleIndoorOutdoor();
void FT02_ModeEnterSleep();
void FT02_ModeWake();

bool FT02_ModeIsSleeping();
bool FT02_ModeGnssShouldBeEnabled();
bool FT02_ModeAutoSleepAllowed();

const char* FT02_ModeLabel(FT02SystemMode mode);
const char* FT02_ModeCurrentLabel();
const char* FT02_ModeHomeFooterLabel();
