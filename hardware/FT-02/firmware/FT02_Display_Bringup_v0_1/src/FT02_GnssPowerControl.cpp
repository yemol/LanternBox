#include "FT02_GnssPowerControl.h"

#include <Preferences.h>

#include "FT02_Gnss.h"

namespace
{
constexpr const char* PREF_NAMESPACE = "ft02sys";
constexpr const char* PREF_KEY = "gnss_on"; // retained for backward-compatible settings migration
constexpr uint32_t WAKE_GRACE_MS = 60000u;

bool g_started = false;
bool g_desiredEnabled = true;
uint32_t g_lastEnableRequestMs = 0;

void persistDesired()
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, false)) return;
    preferences.putUChar(PREF_KEY, g_desiredEnabled ? 1u : 0u);
    preferences.end();
}
}

void FT02_GnssPowerControlBegin()
{
    Preferences preferences;
    if(preferences.begin(PREF_NAMESPACE, true))
    {
        g_desiredEnabled = preferences.getUChar(PREF_KEY, 1u) != 0u;
        preferences.end();
    }
    else
    {
        g_desiredEnabled = true;
    }

    g_started = true;
    g_lastEnableRequestMs = g_desiredEnabled ? millis() : 0u;
    (void)FT02_GnssSetRuntimeEnabled(g_desiredEnabled);
}

void FT02_GnssPowerControlPoll()
{
    if(!g_started) return;
    // Hardware discovery/recovery and PVT ingest are serviced by FT02_GnssPoll.
    if(g_desiredEnabled && FT02_GnssSnapshotCurrent().communicationActive)
        g_lastEnableRequestMs = 0u;
}

bool FT02_GnssPowerDesiredEnabled() { return g_desiredEnabled; }
bool FT02_GnssPowerActualKnown() { return FT02_GnssHardwareKnown(); }
bool FT02_GnssPowerActualEnabled() { return g_desiredEnabled && FT02_GnssHardwareReady(); }

bool FT02_GnssPowerSetEnabled(bool enabled)
{
    if(!g_started) FT02_GnssPowerControlBegin();
    const bool changed = g_desiredEnabled != enabled;
    if(changed)
    {
        g_desiredEnabled = enabled;
        persistDesired();
    }

    if(enabled && changed) g_lastEnableRequestMs = millis();
    else if(!enabled) g_lastEnableRequestMs = 0u;

    return FT02_GnssSetRuntimeEnabled(enabled);
}

bool FT02_GnssPowerToggle()
{
    return FT02_GnssPowerSetEnabled(!g_desiredEnabled);
}

bool FT02_GnssPowerWakeTransitionActive()
{
    if(!g_desiredEnabled || g_lastEnableRequestMs == 0u) return false;
    if(FT02_GnssSnapshotCurrent().communicationActive)
    {
        g_lastEnableRequestMs = 0u;
        return false;
    }
    return static_cast<uint32_t>(millis() - g_lastEnableRequestMs) < WAKE_GRACE_MS;
}

const char* FT02_GnssPowerDisplayText()
{
    if(!g_desiredEnabled) return "已关闭";
    if(FT02_GnssPowerWakeTransitionActive()) return "唤醒中";
    if(!FT02_GnssHardwareReady()) return "连接中";
    return "已开启";
}
