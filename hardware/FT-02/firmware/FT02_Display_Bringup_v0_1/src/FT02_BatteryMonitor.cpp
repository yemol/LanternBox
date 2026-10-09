#include "FT02_BatteryMonitor.h"

#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "FT02_I2CBus.h"
#include "FT02_Log.h"

namespace
{
constexpr uint8_t MAX17048_ADDR = 0x36;
constexpr uint8_t REG_VCELL = 0x02;
constexpr uint8_t REG_SOC = 0x04;
constexpr uint8_t REG_MODE = 0x06;
constexpr uint8_t REG_VERSION = 0x08;
constexpr uint8_t REG_CRATE = 0x16;
constexpr uint8_t REG_CHIPID = 0x19;
constexpr uint32_t SAMPLE_INTERVAL_MS = 10000UL;
constexpr uint8_t IDENTITY_INVALIDATE_FAILURES = 3;
constexpr float RATE_ACTIVE_THRESHOLD = 0.5f;

FT02BatterySnapshot g_snapshot = {};
uint32_t g_lastAttemptMs = 0;
bool g_identityValid = false;
uint16_t g_cachedVersion = 0;
uint8_t g_cachedChipId = 0;
bool g_sleepPaused = false;
FT02BatteryBand g_lastLoggedBand = FT02_BATTERY_UNKNOWN;
bool g_failureReported = false;

bool readWordLocked(uint8_t reg, uint16_t& value)
{
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(reg);
    const uint8_t tx = Wire.endTransmission(false);
    if(tx != 0) return false;

    const uint8_t count = Wire.requestFrom(MAX17048_ADDR, static_cast<size_t>(2), true);
    if(count != 2 || Wire.available() < 2) return false;

    const uint8_t msb = static_cast<uint8_t>(Wire.read());
    const uint8_t lsb = static_cast<uint8_t>(Wire.read());
    value = static_cast<uint16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
    return true;
}

bool readByteLocked(uint8_t reg, uint8_t& value)
{
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(reg);
    const uint8_t tx = Wire.endTransmission(false);
    if(tx != 0) return false;

    const uint8_t count = Wire.requestFrom(MAX17048_ADDR, static_cast<size_t>(1), true);
    if(count != 1 || Wire.available() < 1) return false;
    value = static_cast<uint8_t>(Wire.read());
    return true;
}

bool publishMeasurement()
{
    uint16_t rawVcell = 0;
    uint16_t rawSoc = 0;
    uint16_t rawCrate = 0;
    uint16_t rawMode = 0;

    bool identityIoOk = true;
    bool identityReady = g_identityValid;

    if(!FT02_I2CBusLock(80)) return false;

    // VERSION and CHIPID are static identity registers. Read them on first
    // contact (or after repeated I2C failures), then cache them instead of
    // paying two extra transactions on every background battery sample.
    if(!g_identityValid)
    {
        uint16_t version = 0;
        uint8_t chipId = 0;
        identityIoOk =
            readWordLocked(REG_VERSION, version) &&
            readByteLocked(REG_CHIPID, chipId);

        identityReady =
            identityIoOk && ((version & 0xFFF0u) == 0x0010u);

        if(identityReady)
        {
            g_cachedVersion = version;
            g_cachedChipId = chipId;
            g_identityValid = true;
        }
    }

    const bool measurementOk = identityReady &&
        readWordLocked(REG_VCELL, rawVcell) &&
        readWordLocked(REG_SOC, rawSoc) &&
        readWordLocked(REG_CRATE, rawCrate) &&
        readWordLocked(REG_MODE, rawMode);

    FT02_I2CBusUnlock();

    const uint32_t now = millis();
    g_snapshot.lastReadMs = now;

    if(!identityReady)
    {
        // Preserve the previous behavior: a responding MAX17048 with an
        // unexpected VERSION is "present" but not a valid gauge sample.
        g_snapshot.monitorPresent = identityIoOk;
        g_snapshot.valid = false;
        g_snapshot.consecutiveFailures =
            g_snapshot.consecutiveFailures < 255 ? static_cast<uint8_t>(g_snapshot.consecutiveFailures + 1U) : 255U;
        if(g_snapshot.consecutiveFailures >= IDENTITY_INVALIDATE_FAILURES && !g_failureReported)
        {
            FT02_LOGW("[WARN][BAT] monitor identity failure count=%u\n",
                      static_cast<unsigned>(g_snapshot.consecutiveFailures));
            g_failureReported = true;
        }
        g_snapshot.generation++;
        return false;
    }

    if(!measurementOk)
    {
        g_snapshot.monitorPresent = false;
        g_snapshot.valid = false;
        g_snapshot.consecutiveFailures =
            g_snapshot.consecutiveFailures < 255 ? static_cast<uint8_t>(g_snapshot.consecutiveFailures + 1U) : 255U;

        // If the device/bus has really gone away, drop the cached identity so
        // the next recovery attempt re-validates VERSION/CHIPID instead of
        // trusting stale metadata forever.
        if(g_snapshot.consecutiveFailures >= IDENTITY_INVALIDATE_FAILURES)
        {
            g_identityValid = false;
            if(!g_failureReported)
            {
                FT02_LOGW("[WARN][BAT] monitor read failure count=%u\n",
                          static_cast<unsigned>(g_snapshot.consecutiveFailures));
                g_failureReported = true;
            }
        }

        g_snapshot.generation++;
        return false;
    }

    const float voltage = static_cast<float>(rawVcell) * 78.125f / 1000000.0f;
    float percent = static_cast<float>(rawSoc) / 256.0f;
    if(percent < 0.0f) percent = 0.0f;
    if(percent > 100.0f) percent = 100.0f;
    const float rate = static_cast<float>(static_cast<int16_t>(rawCrate)) * 0.208f;

    g_snapshot.monitorPresent = true;
    g_snapshot.valid = isfinite(voltage) && isfinite(percent) && isfinite(rate);
    g_snapshot.voltage = voltage;
    g_snapshot.percent = percent;
    g_snapshot.ratePercentPerHour = rate;
    g_snapshot.version = g_cachedVersion;
    g_snapshot.chipId = g_cachedChipId;
    g_snapshot.hibernating = (rawMode & 0x1000u) != 0u; // MODE.HibStat is bit 12 (MSB bit 4).
    g_snapshot.consecutiveFailures = 0;
    g_failureReported = false;
    g_snapshot.generation++;

    if(g_snapshot.valid)
    {
        FT02BatteryBand band = FT02_BATTERY_NORMAL;
        if(g_snapshot.percent <= static_cast<float>(FT02_BATTERY_CRITICAL_PERCENT)) band = FT02_BATTERY_CRITICAL;
        else if(g_snapshot.percent <= static_cast<float>(FT02_BATTERY_LOW_PERCENT)) band = FT02_BATTERY_LOW;
        if(band != g_lastLoggedBand)
        {
            if(band == FT02_BATTERY_CRITICAL)
                FT02_LOGW("[WARN][BAT] critical %.1f%% %.3fV\n", static_cast<double>(g_snapshot.percent), static_cast<double>(g_snapshot.voltage));
            else if(band == FT02_BATTERY_LOW)
                FT02_LOGW("[WARN][BAT] low %.1f%% %.3fV\n", static_cast<double>(g_snapshot.percent), static_cast<double>(g_snapshot.voltage));
            else
                FT02_LOGD("[BAT] normal %.1f%% %.3fV\n", static_cast<double>(g_snapshot.percent), static_cast<double>(g_snapshot.voltage));
            g_lastLoggedBand = band;
        }
    }

    return g_snapshot.valid;
}
}

void FT02_BatteryMonitorBegin()
{
    memset(&g_snapshot, 0, sizeof(g_snapshot));
    g_lastAttemptMs = 0;
    g_identityValid = false;
    g_cachedVersion = 0;
    g_cachedChipId = 0;
    g_sleepPaused = false;
    g_lastLoggedBand = FT02_BATTERY_UNKNOWN;
    g_failureReported = false;
    FT02_BatteryMonitorRefreshNow();
}

bool FT02_BatteryMonitorRefreshNow()
{
    g_lastAttemptMs = millis();
    const uint32_t before = g_snapshot.generation;
    publishMeasurement();
    return g_snapshot.generation != before;
}

bool FT02_BatteryMonitorPoll()
{
    if(g_sleepPaused) return false;

    const uint32_t now = millis();
    if(g_lastAttemptMs != 0 && (now - g_lastAttemptMs) < SAMPLE_INTERVAL_MS) return false;
    return FT02_BatteryMonitorRefreshNow();
}


void FT02_BatteryMonitorSleepPause()
{
    // The MAX17048 itself remains powered and keeps its ModelGauge state.
    // Sleep only pauses host-side background I2C polling so the shared bus is
    // owned exclusively by the wake-input path until a real system wake.
    g_sleepPaused = true;
}

bool FT02_BatteryMonitorWakeResume()
{
    // Re-enable host-side polling and take one immediate sample. This is both
    // the lifecycle handoff and the wake-integrity check for the shared I2C
    // bus/MAX17048 path. A failed read is reported to the caller but never
    // blocks the rest of wake recovery.
    g_sleepPaused = false;
    const bool published = FT02_BatteryMonitorRefreshNow();
    return published && g_snapshot.valid;
}

FT02BatterySnapshot FT02_BatterySnapshotCurrent()
{
    return g_snapshot;
}

FT02BatteryBand FT02_BatteryBandCurrent()
{
    if(!g_snapshot.valid) return FT02_BATTERY_UNKNOWN;
    if(g_snapshot.percent <= static_cast<float>(FT02_BATTERY_CRITICAL_PERCENT)) return FT02_BATTERY_CRITICAL;
    if(g_snapshot.percent <= static_cast<float>(FT02_BATTERY_LOW_PERCENT)) return FT02_BATTERY_LOW;
    return FT02_BATTERY_NORMAL;
}

bool FT02_BatteryIsCharging()
{
    return g_snapshot.valid && g_snapshot.ratePercentPerHour > RATE_ACTIVE_THRESHOLD;
}

void FT02_BatteryFormatStatus(char* line1, size_t line1Size, char* line2, size_t line2Size)
{
    if(line1 == nullptr || line1Size == 0 || line2 == nullptr || line2Size == 0) return;

    if(!g_snapshot.valid)
    {
        snprintf(line1, line1Size, "--%%");
        snprintf(line2, line2Size, "%s", g_snapshot.monitorPresent ? "异常" : "未检测");
        return;
    }

    int rounded = static_cast<int>(lroundf(g_snapshot.percent));
    if(rounded < 0) rounded = 0;
    if(rounded > 100) rounded = 100;
    snprintf(line1, line1Size, "%d%%", rounded);

    if(FT02_BatteryIsCharging())
        snprintf(line2, line2Size, "充电中");
    else
        snprintf(line2, line2Size, "供电中");
}
