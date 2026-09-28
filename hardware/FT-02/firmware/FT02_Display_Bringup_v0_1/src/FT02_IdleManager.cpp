#include "FT02_IdleManager.h"

#include "FT02_SystemSettings.h"

namespace
{
uint32_t g_lastActivityMs = 0u;
}

void FT02_IdleManagerBegin(uint32_t nowMs)
{
    g_lastActivityMs = nowMs;
}

void FT02_IdleManagerNoteActivity(uint32_t nowMs)
{
    g_lastActivityMs = nowMs;
}

uint32_t FT02_IdleManagerIdleMs(uint32_t nowMs)
{
    return static_cast<uint32_t>(nowMs - g_lastActivityMs);
}

bool FT02_IdleManagerShouldSleep(uint32_t nowMs)
{
    const uint32_t timeout = FT02_SystemSettingsAutoSleepTimeoutMs();
    if(timeout == 0u) return false;
    return FT02_IdleManagerIdleMs(nowMs) >= timeout;
}
