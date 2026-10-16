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
constexpr uint32_t FT02_SLEEP_TRUTH_VERSION = 0x00040000u;

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

constexpr uint8_t FT02_WAKE_BATTERY_SAMPLE_TARGET = 3u;

struct FT02WakeBatteryCollector
{
    bool active = false;
    uint8_t count = 0;
    int32_t permille[FT02_WAKE_BATTERY_SAMPLE_TARGET] = {};
    uint32_t mv[FT02_WAKE_BATTERY_SAMPLE_TARGET] = {};
};

FT02WakeBatteryCollector g_wakeBatteryCollector;

void clearWakeBatteryCollector()
{
    g_wakeBatteryCollector = FT02WakeBatteryCollector{};
}

template<typename T>
T median3(T a, T b, T c)
{
    if(a > b) { const T t = a; a = b; b = t; }
    if(b > c) { const T t = b; b = c; c = t; }
    if(a > b) { const T t = a; a = b; b = t; }
    return b;
}

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
    clearWakeBatteryCollector();
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
    (void)automatic;
    const uint64_t now = nowUs();

    // R1.4: every physical sleep entry is an independent measurement session.
    // Never carry a previous wake's awake gap into the next sleep result. This
    // keeps first/second/third sleep comparisons apples-to-apples.
    resetTruth(true, now);
    g_truth.sessionCount = 1u;
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

void FT02_SleepTruthBeginWakeBatterySettle()
{
    clearWakeBatteryCollector();
    if(!truthValid() || !g_truth.resultValid || g_truth.resultBatteryStartValid == 0u) return;

    // The immediate MAX17048 wake-resume read verifies I2C recovery only. It is
    // deliberately excluded from energy accounting because ModelGauge/SOC can
    // settle after the system load changes at wake. The normal 10 s background
    // poll supplies three later samples without increasing I2C traffic.
    g_wakeBatteryCollector.active = true;
}

void FT02_SleepTruthFeedWakeBatterySample(bool valid, float percent, float voltage)
{
    if(!g_wakeBatteryCollector.active || !truthValid() || !g_truth.resultValid ||
       !valid || !isfinite(percent) || !isfinite(voltage)) return;

    const uint8_t index = g_wakeBatteryCollector.count;
    if(index >= FT02_WAKE_BATTERY_SAMPLE_TARGET) return;

    g_wakeBatteryCollector.permille[index] = percentToPermille(percent);
    g_wakeBatteryCollector.mv[index] = voltageToMv(voltage);
    ++g_wakeBatteryCollector.count;

    if(g_wakeBatteryCollector.count < FT02_WAKE_BATTERY_SAMPLE_TARGET) return;

    const int32_t stablePermille = median3(
        g_wakeBatteryCollector.permille[0],
        g_wakeBatteryCollector.permille[1],
        g_wakeBatteryCollector.permille[2]);
    const uint32_t stableMv = median3(
        g_wakeBatteryCollector.mv[0],
        g_wakeBatteryCollector.mv[1],
        g_wakeBatteryCollector.mv[2]);

    g_truth.batteryWakeValid = 1u;
    g_truth.batteryWakePermille = stablePermille;
    g_truth.batteryWakeMv = stableMv;
    g_truth.resultBatteryWakeValid = 1u;
    g_truth.resultBatteryWakePermille = stablePermille;
    g_truth.resultBatteryWakeMv = stableMv;
    g_wakeBatteryCollector.active = false;
}

void FT02_SleepTruthMarkKeyboardWake()
{
    if(!truthValid() || !g_truth.windowActive) return;
    ++g_truth.keyboardWakeCount;
    captureTruthResult();
    // Close this physical sleep session immediately at the accepted wake. The
    // post-wake UI time is deliberately excluded from sleep efficiency/drain.
    g_truth.windowActive = 0u;
    g_truth.windowStartUs = 0u;
    g_truth.lightSleepStartUs = 0u;
}

void FT02_SleepTruthMarkUnexpectedReturn()
{
    if(!truthValid() || !g_truth.windowActive) return;
    ++g_truth.unexpectedReturnCount;
    captureTruthResult();
    g_truth.windowActive = 0u;
    g_truth.windowStartUs = 0u;
    g_truth.lightSleepStartUs = 0u;
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
        s.batteryStartSampleValid = g_truth.resultBatteryStartValid != 0u;
        s.batteryWakeSettling = g_wakeBatteryCollector.active;
        s.batteryWakeSampleCount = g_wakeBatteryCollector.count;
        s.batterySampleValid =
            s.batteryStartSampleValid && g_truth.resultBatteryWakeValid != 0u;
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
