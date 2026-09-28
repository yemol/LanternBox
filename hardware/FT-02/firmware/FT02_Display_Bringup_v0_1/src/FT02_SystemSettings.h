#pragma once

#include <Arduino.h>

enum FT02BootModePolicy : uint8_t
{
    FT02_BOOT_MODE_REMEMBER = 0,
    FT02_BOOT_MODE_INDOOR,
    FT02_BOOT_MODE_OUTDOOR
};

enum FT02WakeReturnPolicy : uint8_t
{
    FT02_WAKE_RETURN_PREVIOUS = 0,
    FT02_WAKE_RETURN_HOME
};

void FT02_SystemSettingsBegin();

uint16_t FT02_SystemSettingsAutoSleepMinutes();
uint32_t FT02_SystemSettingsAutoSleepTimeoutMs();
void FT02_SystemSettingsCycleAutoSleep(int8_t direction);
const char* FT02_SystemSettingsAutoSleepLabel();

FT02BootModePolicy FT02_SystemSettingsBootModePolicy();
void FT02_SystemSettingsCycleBootMode(int8_t direction);
const char* FT02_SystemSettingsBootModeLabel();

FT02WakeReturnPolicy FT02_SystemSettingsWakeReturnPolicy();
void FT02_SystemSettingsCycleWakeReturn(int8_t direction);
const char* FT02_SystemSettingsWakeReturnLabel();
