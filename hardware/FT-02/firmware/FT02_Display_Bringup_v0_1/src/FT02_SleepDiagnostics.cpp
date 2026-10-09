#include "FT02_SleepDiagnostics.h"

#include <Preferences.h>
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_timer.h>

#include <math.h>
#include <string.h>

namespace
{
constexpr const char* FT02_SLEEP_DIAG_NS = "ft02slpdbg";
constexpr uint32_t FT02_SLEEP_TRUTH_MAGIC = 0x46543254u; // "FT2T"
constexpr uint32_t FT02_SLEEP_TRUTH_VERSION = 0x00020000u;

struct FT02SleepTruthRtc
{
    uint32_t magic;
    uint32_t version;
    uint32_t windowActive;
    uint32_t resultValid;

    uint32_t sessionCount;
    uint32_t lightSleepEntryCount;
    uint32_t lightSleepReturnCount;
    uint32_t keyboardWakeCount;
    uint32_t unexpectedReturnCount;
    uint32_t resetDuringWindowCount;

    int32_t lastSleepErr;
    int32_t lastWakeCause;
    uint64_t lastExt1Status;

    uint64_t windowStartUs;
    uint64_t lightSleepStartUs;
    uint64_t totalLightSleepUs;

    uint32_t batteryStartValid;
    int32_t batteryStartPermille;
    uint32_t batteryStartMv;
    uint32_t batteryWakeValid;
    int32_t batteryWakePermille;
    uint32_t batteryWakeMv;

    uint32_t resultSessionCount;
    uint32_t resultLightSleepEntryCount;
    uint32_t resultLightSleepReturnCount;
    uint32_t resultKeyboardWakeCount;
    uint32_t resultUnexpectedReturnCount;
    uint32_t resultResetDuringWindowCount;
    int32_t resultLastSleepErr;
    int32_t resultLastWakeCause;
    uint64_t resultLastExt1Status;
    uint64_t resultElapsedUs;
    uint64_t resultLightSleepUs;
    uint64_t resultAwakeUs;
    uint32_t resultBatteryStartValid;
    int32_t resultBatteryStartPermille;
    uint32_t resultBatteryStartMv;
    uint32_t resultBatteryWakeValid;
    int32_t resultBatteryWakePermille;
    uint32_t resultBatteryWakeMv;
};

RTC_NOINIT_ATTR FT02SleepTruthRtc g_truth;

uint64_t nowUs()
{
    const int64_t value = esp_timer_get_time();
    return value > 0 ? static_cast<uint64_t>(value) : 0u;
}

int32_t percentToPermille(float percent)
{
    if(!isfinite(percent)) return 0;
    if(percent < 0.0f) percent = 0.0f;
    if(percent > 100.0f) percent = 100.0f;
    return static_cast<int32_t>(lroundf(percent * 10.0f));
}

uint32_t voltageToMv(float voltage)
{
    if(!isfinite(voltage) || voltage < 0.0f) return 0u;
    return static_cast<uint32_t>(lroundf(voltage * 1000.0f));
}

void resetTruth(bool active, uint64_t startUs)
{
    memset(&g_truth, 0, sizeof(g_truth));
    g_truth.magic = FT02_SLEEP_TRUTH_MAGIC;
    g_truth.version = FT02_SLEEP_TRUTH_VERSION;
    g_truth.windowActive = active ? 1u : 0u;
    g_truth.windowStartUs = active ? startUs : 0u;
}

bool truthValid()
{
    return g_truth.magic == FT02_SLEEP_TRUTH_MAGIC &&
           g_truth.version == FT02_SLEEP_TRUTH_VERSION;
}

void captureTruthResult()
{
    if(!truthValid() || g_truth.windowStartUs == 0u) return;

    const uint64_t now = nowUs();
    const uint64_t elapsed = now >= g_truth.windowStartUs ? now - g_truth.windowStartUs : 0u;
    const uint64_t slept = g_truth.totalLightSleepUs <= elapsed ? g_truth.totalLightSleepUs : elapsed;

    g_truth.resultValid = 1u;
    g_truth.resultSessionCount = g_truth.sessionCount;
    g_truth.resultLightSleepEntryCount = g_truth.lightSleepEntryCount;
    g_truth.resultLightSleepReturnCount = g_truth.lightSleepReturnCount;
    g_truth.resultKeyboardWakeCount = g_truth.keyboardWakeCount;
    g_truth.resultUnexpectedReturnCount = g_truth.unexpectedReturnCount;
    g_truth.resultResetDuringWindowCount = g_truth.resetDuringWindowCount;
    g_truth.resultLastSleepErr = g_truth.lastSleepErr;
    g_truth.resultLastWakeCause = g_truth.lastWakeCause;
    g_truth.resultLastExt1Status = g_truth.lastExt1Status;
    g_truth.resultElapsedUs = elapsed;
    g_truth.resultLightSleepUs = slept;
    g_truth.resultAwakeUs = elapsed >= slept ? elapsed - slept : 0u;
    g_truth.resultBatteryStartValid = g_truth.batteryStartValid;
    g_truth.resultBatteryStartPermille = g_truth.batteryStartPermille;
    g_truth.resultBatteryStartMv = g_truth.batteryStartMv;
    g_truth.resultBatteryWakeValid = g_truth.batteryWakeValid;
    g_truth.resultBatteryWakePermille = g_truth.batteryWakePermille;
    g_truth.resultBatteryWakeMv = g_truth.batteryWakeMv;
}

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
    if(p.begin(FT02_SLEEP_DIAG_NS, false))
    {
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

    const esp_reset_reason_t resetReason = esp_reset_reason();
    if(!truthValid() || resetReason == ESP_RST_POWERON)
    {
        resetTruth(false, 0u);
        return;
    }

    // esp_timer restarts after a CPU reset, so an in-progress truth window can
    // no longer have trustworthy elapsed-time arithmetic. Preserve the fact
    // that a reset happened, but invalidate the timing window itself. The NVS
    // breadcrumb above retains the exact reset reason for Device Status.
    if(g_truth.windowActive)
    {
        ++g_truth.resetDuringWindowCount;
        g_truth.resultValid = 1u;
        g_truth.resultSessionCount = g_truth.sessionCount;
        g_truth.resultLightSleepEntryCount = g_truth.lightSleepEntryCount;
        g_truth.resultLightSleepReturnCount = g_truth.lightSleepReturnCount;
        g_truth.resultKeyboardWakeCount = g_truth.keyboardWakeCount;
        g_truth.resultUnexpectedReturnCount = g_truth.unexpectedReturnCount;
        g_truth.resultResetDuringWindowCount = g_truth.resetDuringWindowCount;
        g_truth.resultLastSleepErr = g_truth.lastSleepErr;
        g_truth.resultLastWakeCause = g_truth.lastWakeCause;
        g_truth.resultLastExt1Status = g_truth.lastExt1Status;
        g_truth.resultElapsedUs = 0u;
        g_truth.resultLightSleepUs = 0u;
        g_truth.resultAwakeUs = 0u;
        g_truth.resultBatteryStartValid = g_truth.batteryStartValid;
        g_truth.resultBatteryStartPermille = g_truth.batteryStartPermille;
        g_truth.resultBatteryStartMv = g_truth.batteryStartMv;
        g_truth.resultBatteryWakeValid = g_truth.batteryWakeValid;
        g_truth.resultBatteryWakePermille = g_truth.batteryWakePermille;
        g_truth.resultBatteryWakeMv = g_truth.batteryWakeMv;
        g_truth.windowActive = 0u;
        g_truth.windowStartUs = 0u;
        g_truth.lightSleepStartUs = 0u;
    }
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

void FT02_SleepTruthSessionEnter(bool automatic)
{
    const uint64_t now = nowUs();
    if(!truthValid()) resetTruth(false, 0u);

    // A manual sleep explicitly starts a fresh measurement window. Automatic
    // re-sleep continues an existing window so any awake gap after a spurious
    // wake is counted instead of being hidden by a new session.
    if(!automatic || !g_truth.windowActive || g_truth.windowStartUs == 0u)
    {
        resetTruth(true, now);
    }

    ++g_truth.sessionCount;
    g_truth.lightSleepStartUs = 0u;
}

void FT02_SleepTruthBeforeLightSleep()
{
    if(!truthValid() || !g_truth.windowActive) return;
    ++g_truth.lightSleepEntryCount;
    g_truth.lightSleepStartUs = nowUs();
}

void FT02_SleepTruthAfterLightSleep(
    int32_t sleepErr,
    int32_t wakeCause,
    uint64_t ext1Status
)
{
    if(!truthValid() || !g_truth.windowActive) return;

    const uint64_t now = nowUs();
    if(g_truth.lightSleepStartUs != 0u && now >= g_truth.lightSleepStartUs)
        g_truth.totalLightSleepUs += now - g_truth.lightSleepStartUs;

    g_truth.lightSleepStartUs = 0u;
    ++g_truth.lightSleepReturnCount;
    g_truth.lastSleepErr = sleepErr;
    g_truth.lastWakeCause = wakeCause;
    g_truth.lastExt1Status = ext1Status;
}

void FT02_SleepTruthSetBatteryStart(bool valid, float percent, float voltage)
{
    if(!truthValid() || !g_truth.windowActive || !valid ||
       !isfinite(percent) || !isfinite(voltage)) return;

    // Automatic re-sleep keeps the first sample of the manual measurement
    // window. This makes the displayed sleep drain span the whole session.
    if(g_truth.batteryStartValid == 0u)
    {
        g_truth.batteryStartValid = 1u;
        g_truth.batteryStartPermille = percentToPermille(percent);
        g_truth.batteryStartMv = voltageToMv(voltage);
    }
}

void FT02_SleepTruthSetBatteryWake(bool valid, float percent, float voltage)
{
    if(!truthValid() || !valid || !isfinite(percent) || !isfinite(voltage)) return;

    g_truth.batteryWakeValid = 1u;
    g_truth.batteryWakePermille = percentToPermille(percent);
    g_truth.batteryWakeMv = voltageToMv(voltage);

    // Keyboard wake captures the timing result before normal I2C is restored.
    // The wake battery sample arrives later from the existing BatteryMonitor
    // wake-resume transaction, so attach it to the already captured result.
    if(g_truth.resultValid)
    {
        g_truth.resultBatteryWakeValid = 1u;
        g_truth.resultBatteryWakePermille = g_truth.batteryWakePermille;
        g_truth.resultBatteryWakeMv = g_truth.batteryWakeMv;
    }
}

void FT02_SleepTruthMarkKeyboardWake()
{
    if(!truthValid() || !g_truth.windowActive) return;
    ++g_truth.keyboardWakeCount;
    captureTruthResult();
}

void FT02_SleepTruthMarkUnexpectedReturn()
{
    if(!truthValid() || !g_truth.windowActive) return;
    ++g_truth.unexpectedReturnCount;
    captureTruthResult();
}

FT02SleepTruthSnapshot FT02_SleepTruthSnapshotCurrent()
{
    FT02SleepTruthSnapshot s;
    if(!truthValid()) return s;

    s.windowActive = g_truth.windowActive != 0u;

    if(g_truth.resultValid)
    {
        s.valid = true;
        s.sessionCount = g_truth.resultSessionCount;
        s.lightSleepEntryCount = g_truth.resultLightSleepEntryCount;
        s.lightSleepReturnCount = g_truth.resultLightSleepReturnCount;
        s.keyboardWakeCount = g_truth.resultKeyboardWakeCount;
        s.unexpectedReturnCount = g_truth.resultUnexpectedReturnCount;
        s.resetDuringWindowCount = g_truth.resultResetDuringWindowCount;
        s.lastSleepErr = g_truth.resultLastSleepErr;
        s.lastWakeCause = g_truth.resultLastWakeCause;
        s.lastExt1Status = g_truth.resultLastExt1Status;
        s.elapsedUs = g_truth.resultElapsedUs;
        s.lightSleepUs = g_truth.resultLightSleepUs;
        s.awakeUs = g_truth.resultAwakeUs;
        s.batterySampleValid =
            g_truth.resultBatteryStartValid != 0u && g_truth.resultBatteryWakeValid != 0u;
        if(s.batterySampleValid)
        {
            s.batteryStartPercent = static_cast<float>(g_truth.resultBatteryStartPermille) / 10.0f;
            s.batteryStartVoltage = static_cast<float>(g_truth.resultBatteryStartMv) / 1000.0f;
            s.batteryWakePercent = static_cast<float>(g_truth.resultBatteryWakePermille) / 10.0f;
            s.batteryWakeVoltage = static_cast<float>(g_truth.resultBatteryWakeMv) / 1000.0f;
        }
        return s;
    }

    if(g_truth.windowActive && g_truth.windowStartUs != 0u)
    {
        const uint64_t now = nowUs();
        const uint64_t elapsed = now >= g_truth.windowStartUs ? now - g_truth.windowStartUs : 0u;
        const uint64_t slept = g_truth.totalLightSleepUs <= elapsed ? g_truth.totalLightSleepUs : elapsed;
        s.valid = true;
        s.sessionCount = g_truth.sessionCount;
        s.lightSleepEntryCount = g_truth.lightSleepEntryCount;
        s.lightSleepReturnCount = g_truth.lightSleepReturnCount;
        s.keyboardWakeCount = g_truth.keyboardWakeCount;
        s.unexpectedReturnCount = g_truth.unexpectedReturnCount;
        s.resetDuringWindowCount = g_truth.resetDuringWindowCount;
        s.lastSleepErr = g_truth.lastSleepErr;
        s.lastWakeCause = g_truth.lastWakeCause;
        s.lastExt1Status = g_truth.lastExt1Status;
        s.elapsedUs = elapsed;
        s.lightSleepUs = slept;
        s.awakeUs = elapsed >= slept ? elapsed - slept : 0u;
    }

    return s;
}
