#include "FT02_GnssPowerControl.h"

#include <Preferences.h>

#include "FT02_LR01HostRuntime.h"

namespace
{
constexpr const char* PREF_NAMESPACE = "ft02sys";
constexpr const char* PREF_KEY = "gnss_on";
constexpr uint32_t QUERY_RETRY_MS = 2500u;
constexpr uint32_t COMMAND_RETRY_MS = 3000u;

bool g_started = false;
bool g_desiredEnabled = true;
bool g_prevOnline = false;
uint32_t g_lastSeenPowerRevision = 0;
uint32_t g_lastQueryMs = 0;
uint32_t g_lastCommandMs = 0;

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
    g_lastQueryMs = 0;
    g_lastCommandMs = 0;

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
        g_lastQueryMs = 0;
        g_lastCommandMs = 0;
        (void)FT02_LR01HostGnssPowerRequestStatus();
        g_lastQueryMs = now;

        // OFF must be actively restored after every LR01 reboot because the
        // receiver itself powers up active. ON can wait for the status reply.
        if(!g_desiredEnabled) (void)sendDesired();
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
        if(g_lastQueryMs == 0u || static_cast<uint32_t>(now - g_lastQueryMs) >= QUERY_RETRY_MS)
        {
            (void)FT02_LR01HostGnssPowerRequestStatus();
            g_lastQueryMs = now;
        }
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
    if(g_desiredEnabled != enabled)
    {
        g_desiredEnabled = enabled;
        persistDesired();
    }

    if(!FT02_LR01HostOnline()) return false;
    const bool ok = sendDesired();
    if(ok) (void)FT02_LR01HostGnssPowerRequestStatus();
    return ok;
}

bool FT02_GnssPowerToggle()
{
    return FT02_GnssPowerSetEnabled(!g_desiredEnabled);
}

const char* FT02_GnssPowerDisplayText()
{
    if(!g_desiredEnabled) return "已关闭";
    if(!FT02_LR01HostOnline()) return "等待LR01";
    const FT02LR01State& lr01 = FT02_LR01HostState();
    if(lr01.gnssPowerKnown && !lr01.gnssPowerEnabled) return "唤醒中";
    return "已开启";
}
