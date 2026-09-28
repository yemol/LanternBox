#include "FT02_SleepDiagnostics.h"

#include <Preferences.h>
#include <esp_system.h>

namespace
{
constexpr const char* FT02_SLEEP_DIAG_NS = "ft02slpdbg";

FT02SleepDiagnosticsSnapshot readSnapshot(Preferences& p)
{
    FT02SleepDiagnosticsSnapshot s;
    s.reason = static_cast<FT02SleepExitReason>(p.getUChar("reason", 0));
    s.sleepSessionActive = p.getBool("active", false);
    s.sleepSession = p.getUInt("session", 0);
    s.keyboardWakeCount = p.getUInt("wakecnt", 0);
    s.resetDuringSleepCount = p.getUInt("rstcnt", 0);
    s.lastRaw = p.getUChar("raw", 0);
    s.lastSlices = p.getUInt("slices", 0);
    s.lastDurationMs = p.getUInt("durms", 0);
    s.lastResetReason = p.getUChar("rstwhy", 0);
    return s;
}
}

void FT02_SleepDiagnosticsBegin()
{
    Preferences p;
    if(!p.begin(FT02_SLEEP_DIAG_NS, false)) return;

    // If setup() runs while the previous sleep session is still marked active,
    // preserve that reset as a Device Status health breadcrumb.
    if(p.getBool("active", false))
    {
        p.putBool("active", false);
        p.putUChar("reason", static_cast<uint8_t>(FT02_SLEEP_EXIT_RESET));
        p.putUChar("rstwhy", static_cast<uint8_t>(esp_reset_reason()));
        p.putUInt("rstcnt", p.getUInt("rstcnt", 0) + 1u);
    }

    p.end();
}

void FT02_SleepDiagnosticsMarkEnter(bool automatic)
{
    Preferences p;
    if(!p.begin(FT02_SLEEP_DIAG_NS, false)) return;

    p.putUInt("session", p.getUInt("session", 0) + 1u);
    p.putBool("active", true);
    p.putUChar("src", automatic ? 1u : 0u);
    p.end();
}

void FT02_SleepDiagnosticsMarkKeyboardWake(
    uint8_t raw,
    uint32_t slices,
    uint32_t durationMs
)
{
    Preferences p;
    if(!p.begin(FT02_SLEEP_DIAG_NS, false)) return;

    p.putBool("active", false);
    p.putUChar("reason", static_cast<uint8_t>(FT02_SLEEP_EXIT_KEYBOARD));
    p.putUChar("raw", raw);
    p.putUInt("slices", slices);
    p.putUInt("durms", durationMs);
    p.putUInt("wakecnt", p.getUInt("wakecnt", 0) + 1u);
    p.end();
}

FT02SleepDiagnosticsSnapshot FT02_SleepDiagnosticsSnapshotCurrent()
{
    Preferences p;
    if(!p.begin(FT02_SLEEP_DIAG_NS, true))
        return FT02SleepDiagnosticsSnapshot{};

    const FT02SleepDiagnosticsSnapshot s = readSnapshot(p);
    p.end();
    return s;
}
