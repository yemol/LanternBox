#include "FT02_GnssPowerControl.h"

#include <Preferences.h>

#include "FT02_LR01HostRuntime.h"

namespace
{
constexpr const char* PREF_NAMESPACE = "ft02sys";
constexpr const char* PREF_KEY = "gnss_on";
constexpr uint32_t COMMAND_RETRY_MS = 3000u;
constexpr uint32_t WAKE_GRACE_MS = 12000u;

bool g_started = false;
bool g_desiredEnabled = true;
bool g_prevOnline = false;
uint32_t g_lastSeenPowerRevision = 0;
uint32_t g_lastCommandMs = 0;
uint32_t g_lastEnableRequestMs = 0;

void persistDesired()
{
    Preferences preferences;
    if(!preferences.begin(PREF_NAMESPACE, false)) return;
    preferences.putUChar(PREF_KEY, g_desiredEnabled ? 1u : 0u);
    preferences.end();
}

bool sendDesired()
{
    if(!FT02_LR01HostOnline()) return false;
    const bool ok = g_desiredEnabled
        ? FT02_LR01HostGnssSoftwareOn()
        : FT02_LR01HostGnssSoftwareOff();
    if(ok) g_lastCommandMs = millis();
    return ok;
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
    g_prevOnline = false;
    g_lastSeenPowerRevision = 0;
    g_lastCommandMs = 0;
    g_lastEnableRequestMs = 0;
}

void FT02_GnssPowerControlPoll()
{
    if(!g_started) return;

    const FT02LR01State& lr01 = FT02_LR01HostState();
    const bool online = FT02_LR01HostOnline();
    const uint32_t now = millis();

    if(!online)
    {
        g_prevOnline = false;
        return;
    }

    if(!g_prevOnline)
    {
        g_prevOnline = true;
        g_lastCommandMs = 0;
        if(g_desiredEnabled) g_lastEnableRequestMs = now;

        // The deployed LR01 does not support the legacy GNSS status query.
        // Reapply the desired state on reconnect and rely on any available
        // GNSS_POWER_SET / SYSTEM / NAV telemetry from the current firmware.
        (void)sendDesired();
        return;
    }

    if(lr01.gnssPowerRevision != g_lastSeenPowerRevision)
    {
        g_lastSeenPowerRevision = lr01.gnssPowerRevision;
        if(lr01.gnssPowerKnown && lr01.gnssPowerEnabled != g_desiredEnabled)
        {
            if(g_lastCommandMs == 0u || static_cast<uint32_t>(now - g_lastCommandMs) >= COMMAND_RETRY_MS)
                (void)sendDesired();
        }
        return;
    }

    if(!lr01.gnssPowerKnown)
    {
        // Command-only compatibility fallback. Keep applying the idempotent
        // desired state until LR01 acknowledges with GNSS_POWER_SET.
        if(g_lastCommandMs == 0u || static_cast<uint32_t>(now - g_lastCommandMs) >= COMMAND_RETRY_MS)
            (void)sendDesired();
        return;
    }

    if(lr01.gnssPowerEnabled != g_desiredEnabled &&
       (g_lastCommandMs == 0u || static_cast<uint32_t>(now - g_lastCommandMs) >= COMMAND_RETRY_MS))
    {
        (void)sendDesired();
    }
}

bool FT02_GnssPowerDesiredEnabled()
{
    return g_desiredEnabled;
}

bool FT02_GnssPowerActualKnown()
{
    return FT02_LR01HostState().gnssPowerKnown;
}

bool FT02_GnssPowerActualEnabled()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    return lr01.gnssPowerKnown ? lr01.gnssPowerEnabled : g_desiredEnabled;
}

bool FT02_GnssPowerSetEnabled(bool enabled)
{
    if(!g_started) FT02_GnssPowerControlBegin();
    const bool changed = g_desiredEnabled != enabled;
    if(changed)
    {
        g_desiredEnabled = enabled;
        persistDesired();
    }

    // A stale GNSS snapshot is expected immediately after indoor -> outdoor.
    // Track a bounded hardware wake interval so the status bar reports
    // "唤醒中" instead of briefly claiming the receiver is offline.
    if(enabled && (changed || !FT02_LR01HostState().gnssPowerStream))
        g_lastEnableRequestMs = millis();
    else if(!enabled)
        g_lastEnableRequestMs = 0u;

    if(!FT02_LR01HostOnline()) return false;
    return sendDesired();
}

bool FT02_GnssPowerToggle()
{
    return FT02_GnssPowerSetEnabled(!g_desiredEnabled);
}

bool FT02_GnssPowerWakeTransitionActive()
{
    if(!g_desiredEnabled || g_lastEnableRequestMs == 0u) return false;

    const FT02LR01State& lr01 = FT02_LR01HostState();
    if(lr01.gnssPowerKnown && lr01.gnssPowerEnabled && lr01.gnssPowerStream)
    {
        g_lastEnableRequestMs = 0u;
        return false;
    }

    return static_cast<uint32_t>(millis() - g_lastEnableRequestMs) < WAKE_GRACE_MS;
}

const char* FT02_GnssPowerDisplayText()
{
    if(!g_desiredEnabled) return "已关闭";
    if(!FT02_LR01HostOnline()) return "等待LR01";
    if(FT02_GnssPowerWakeTransitionActive()) return "唤醒中";
    const FT02LR01State& lr01 = FT02_LR01HostState();
    if(lr01.gnssPowerKnown && !lr01.gnssPowerEnabled) return "唤醒中";
    return "已开启";
}
