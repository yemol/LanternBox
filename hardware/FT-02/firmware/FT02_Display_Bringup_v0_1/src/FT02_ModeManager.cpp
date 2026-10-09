#include "FT02_ModeManager.h"

#include <Preferences.h>

#include "FT02_GnssPowerControl.h"
#include "FT02_Log.h"
#include "FT02_SystemSettings.h"

namespace
{
constexpr const char* PREF_NAMESPACE = "ft02sys";
constexpr const char* PREF_MODE_AWAKE = "mode_awake";
constexpr const char* LEGACY_GNSS_KEY = "gnss_on";

FT02SystemMode g_currentMode = FT02_MODE_OUTDOOR;
FT02SystemMode g_lastAwakeMode = FT02_MODE_OUTDOOR;

void persistAwakeMode()
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, false)) return;
    preferences.putUChar(PREF_MODE_AWAKE, static_cast<uint8_t>(g_lastAwakeMode));
    preferences.end();
}

void applyGnssPolicy()
{
    const bool enable = g_currentMode == FT02_MODE_OUTDOOR;
    (void)FT02_GnssPowerSetEnabled(enable);
}

FT02SystemMode loadRememberedMode()
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, true)) return FT02_MODE_OUTDOOR;

    FT02SystemMode result = FT02_MODE_OUTDOOR;
    if(preferences.isKey(PREF_MODE_AWAKE))
    {
        const uint8_t stored = preferences.getUChar(PREF_MODE_AWAKE, FT02_MODE_OUTDOOR);
        if(stored == FT02_MODE_INDOOR || stored == FT02_MODE_OUTDOOR)
            result = static_cast<FT02SystemMode>(stored);
    }
    else if(preferences.isKey(LEGACY_GNSS_KEY))
    {
        // One-time migration from the old independent GNSS switch:
        // GNSS ON -> outdoor, GNSS OFF -> indoor.
        result = preferences.getUChar(LEGACY_GNSS_KEY, 1u) != 0u
            ? FT02_MODE_OUTDOOR
            : FT02_MODE_INDOOR;
    }
    preferences.end();
    return result;
}
}

void FT02_ModeManagerBegin()
{
    FT02SystemMode selected = loadRememberedMode();
    const FT02BootModePolicy boot = FT02_SystemSettingsBootModePolicy();
    if(boot == FT02_BOOT_MODE_INDOOR) selected = FT02_MODE_INDOOR;
    else if(boot == FT02_BOOT_MODE_OUTDOOR) selected = FT02_MODE_OUTDOOR;

    g_lastAwakeMode = selected;
    g_currentMode = selected;
    persistAwakeMode();
    applyGnssPolicy();
    FT02_LOGI("[MODE] %s\n", selected == FT02_MODE_INDOOR ? "indoor" : "outdoor");
}

FT02SystemMode FT02_ModeCurrent()
{
    return g_currentMode;
}

FT02SystemMode FT02_ModeAwakeMode()
{
    return g_currentMode == FT02_MODE_SLEEP ? g_lastAwakeMode : g_currentMode;
}

bool FT02_ModeSetAwake(FT02SystemMode mode)
{
    if(mode != FT02_MODE_INDOOR && mode != FT02_MODE_OUTDOOR) return false;
    const bool changed = g_lastAwakeMode != mode || g_currentMode != mode;
    g_lastAwakeMode = mode;
    g_currentMode = mode;
    persistAwakeMode();
    applyGnssPolicy();
    if(changed) FT02_LOGI("[MODE] %s\n", mode == FT02_MODE_INDOOR ? "indoor" : "outdoor");
    return changed;
}

bool FT02_ModeToggleIndoorOutdoor()
{
    const FT02SystemMode current = FT02_ModeAwakeMode();
    return FT02_ModeSetAwake(
        current == FT02_MODE_INDOOR ? FT02_MODE_OUTDOOR : FT02_MODE_INDOOR
    );
}

void FT02_ModeEnterSleep()
{
    if(g_currentMode == FT02_MODE_SLEEP) return;
    if(g_currentMode == FT02_MODE_INDOOR || g_currentMode == FT02_MODE_OUTDOOR)
        g_lastAwakeMode = g_currentMode;
    g_currentMode = FT02_MODE_SLEEP;
    applyGnssPolicy();
}

void FT02_ModeWake()
{
    if(g_currentMode != FT02_MODE_SLEEP) return;
    g_currentMode = g_lastAwakeMode;
    applyGnssPolicy();
}

bool FT02_ModeIsSleeping()
{
    return g_currentMode == FT02_MODE_SLEEP;
}

bool FT02_ModeGnssShouldBeEnabled()
{
    return g_currentMode == FT02_MODE_OUTDOOR;
}

bool FT02_ModeAutoSleepAllowed()
{
    // Automatic timeout sleep is an indoor-only policy. Outdoor operation
    // may be unattended for navigation/field use and must remain awake until
    // the user explicitly requests sleep. Manual sleep is handled separately.
    return g_currentMode == FT02_MODE_INDOOR;
}

const char* FT02_ModeLabel(FT02SystemMode mode)
{
    switch(mode)
    {
        case FT02_MODE_INDOOR: return "室内模式";
        case FT02_MODE_SLEEP: return "休眠模式";
        default: return "户外模式";
    }
}

const char* FT02_ModeCurrentLabel()
{
    return FT02_ModeLabel(g_currentMode);
}

const char* FT02_ModeHomeFooterLabel()
{
    return FT02_ModeAwakeMode() == FT02_MODE_INDOOR
        ? "室内模式(P)"
        : "户外模式(P)";
}
