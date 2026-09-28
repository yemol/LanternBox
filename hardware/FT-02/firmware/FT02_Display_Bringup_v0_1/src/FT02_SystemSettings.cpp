#include "FT02_SystemSettings.h"

#include <Preferences.h>

namespace
{
constexpr const char* PREF_NAMESPACE = "ft02sys";
constexpr const char* PREF_SLEEP_MIN = "slp_min";
constexpr const char* PREF_BOOT_MODE = "boot_mode";
constexpr const char* PREF_WAKE_RETURN = "wake_ret";

uint16_t g_autoSleepMinutes = 15u;
FT02BootModePolicy g_bootMode = FT02_BOOT_MODE_REMEMBER;
FT02WakeReturnPolicy g_wakeReturn = FT02_WAKE_RETURN_PREVIOUS;

const uint16_t AUTO_SLEEP_VALUES[] = {5u, 15u, 30u, 0u};
constexpr uint8_t AUTO_SLEEP_COUNT = sizeof(AUTO_SLEEP_VALUES) / sizeof(AUTO_SLEEP_VALUES[0]);

uint8_t autoSleepIndex(uint16_t minutes)
{
    for(uint8_t i = 0; i < AUTO_SLEEP_COUNT; ++i)
    {
        if(AUTO_SLEEP_VALUES[i] == minutes) return i;
    }
    return 1u; // 15 minutes
}

uint8_t wrapIndex(int value, int count)
{
    while(value < 0) value += count;
    while(value >= count) value -= count;
    return static_cast<uint8_t>(value);
}

void persistUShort(const char* key, uint16_t value)
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, false)) return;
    preferences.putUShort(key, value);
    preferences.end();
}

void persistUChar(const char* key, uint8_t value)
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, false)) return;
    preferences.putUChar(key, value);
    preferences.end();
}
}

void FT02_SystemSettingsBegin()
{
    Preferences preferences;
    if(preferences.begin(PREF_NAMESPACE, true))
    {
        const uint16_t storedSleep = preferences.getUShort(PREF_SLEEP_MIN, 15u);
        g_autoSleepMinutes = AUTO_SLEEP_VALUES[autoSleepIndex(storedSleep)];

        const uint8_t storedBoot = preferences.getUChar(PREF_BOOT_MODE, FT02_BOOT_MODE_REMEMBER);
        g_bootMode = storedBoot <= FT02_BOOT_MODE_OUTDOOR
            ? static_cast<FT02BootModePolicy>(storedBoot)
            : FT02_BOOT_MODE_REMEMBER;

        const uint8_t storedWake = preferences.getUChar(PREF_WAKE_RETURN, FT02_WAKE_RETURN_PREVIOUS);
        g_wakeReturn = storedWake <= FT02_WAKE_RETURN_HOME
            ? static_cast<FT02WakeReturnPolicy>(storedWake)
            : FT02_WAKE_RETURN_PREVIOUS;

        preferences.end();
    }
}

uint16_t FT02_SystemSettingsAutoSleepMinutes()
{
    return g_autoSleepMinutes;
}

uint32_t FT02_SystemSettingsAutoSleepTimeoutMs()
{
    if(g_autoSleepMinutes == 0u) return 0u;
    return static_cast<uint32_t>(g_autoSleepMinutes) * 60u * 1000u;
}

void FT02_SystemSettingsCycleAutoSleep(int8_t direction)
{
    const uint8_t current = autoSleepIndex(g_autoSleepMinutes);
    const uint8_t next = wrapIndex(static_cast<int>(current) + (direction < 0 ? -1 : 1), AUTO_SLEEP_COUNT);
    g_autoSleepMinutes = AUTO_SLEEP_VALUES[next];
    persistUShort(PREF_SLEEP_MIN, g_autoSleepMinutes);
}

const char* FT02_SystemSettingsAutoSleepLabel()
{
    switch(g_autoSleepMinutes)
    {
        case 5u: return "5分钟";
        case 30u: return "30分钟";
        case 0u: return "永不休眠";
        default: return "15分钟";
    }
}

FT02BootModePolicy FT02_SystemSettingsBootModePolicy()
{
    return g_bootMode;
}

void FT02_SystemSettingsCycleBootMode(int8_t direction)
{
    const uint8_t next = wrapIndex(
        static_cast<int>(g_bootMode) + (direction < 0 ? -1 : 1),
        3
    );
    g_bootMode = static_cast<FT02BootModePolicy>(next);
    persistUChar(PREF_BOOT_MODE, next);
}

const char* FT02_SystemSettingsBootModeLabel()
{
    switch(g_bootMode)
    {
        case FT02_BOOT_MODE_INDOOR: return "室内模式";
        case FT02_BOOT_MODE_OUTDOOR: return "户外模式";
        default: return "记住上次";
    }
}

FT02WakeReturnPolicy FT02_SystemSettingsWakeReturnPolicy()
{
    return g_wakeReturn;
}

void FT02_SystemSettingsCycleWakeReturn(int8_t direction)
{
    const uint8_t next = wrapIndex(
        static_cast<int>(g_wakeReturn) + (direction < 0 ? -1 : 1),
        2
    );
    g_wakeReturn = static_cast<FT02WakeReturnPolicy>(next);
    persistUChar(PREF_WAKE_RETURN, next);
}

const char* FT02_SystemSettingsWakeReturnLabel()
{
    return g_wakeReturn == FT02_WAKE_RETURN_HOME ? "首页" : "原页面";
}
