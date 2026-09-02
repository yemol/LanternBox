#include "FT02_BatteryMonitor.h"

#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "FT02_I2CBus.h"

namespace
{
constexpr uint8_t MAX17048_ADDR = 0x36;
constexpr uint8_t REG_VCELL = 0x02;
constexpr uint8_t REG_SOC = 0x04;
constexpr uint8_t REG_MODE = 0x06;
constexpr uint8_t REG_VERSION = 0x08;
constexpr uint8_t REG_CRATE = 0x16;
constexpr uint8_t REG_CHIPID = 0x19;
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000UL;
constexpr float RATE_ACTIVE_THRESHOLD = 0.5f;

FT02BatterySnapshot g_snapshot = {};
uint32_t g_lastAttemptMs = 0;

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
    uint16_t version = 0;
    uint16_t rawVcell = 0;
    uint16_t rawSoc = 0;
    uint16_t rawCrate = 0;
    uint16_t rawMode = 0;
    uint8_t chipId = 0;

    if(!FT02_I2CBusLock(80)) return false;
    const bool ok =
        readWordLocked(REG_VERSION, version) &&
        readWordLocked(REG_VCELL, rawVcell) &&
        readWordLocked(REG_SOC, rawSoc) &&
        readWordLocked(REG_CRATE, rawCrate) &&
        readWordLocked(REG_MODE, rawMode) &&
        readByteLocked(REG_CHIPID, chipId);
    FT02_I2CBusUnlock();

    const uint32_t now = millis();
    g_snapshot.lastReadMs = now;

    // Adafruit's current MAX17048 driver treats VERSION 0x001x as ready.
    const bool versionReady = ok && ((version & 0xFFF0u) == 0x0010u);
    if(!versionReady)
    {
        g_snapshot.monitorPresent = ok;
        g_snapshot.valid = false;
        g_snapshot.consecutiveFailures =
            g_snapshot.consecutiveFailures < 255 ? static_cast<uint8_t>(g_snapshot.consecutiveFailures + 1U) : 255U;
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
    g_snapshot.version = version;
    g_snapshot.chipId = chipId;
    g_snapshot.hibernating = (rawMode & 0x1000u) != 0u; // MODE.HibStat is bit 12 (MSB bit 4).
    g_snapshot.consecutiveFailures = 0;
    g_snapshot.generation++;

    return g_snapshot.valid;
}
}

void FT02_BatteryMonitorBegin()
{
    memset(&g_snapshot, 0, sizeof(g_snapshot));
    g_lastAttemptMs = 0;
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
    const uint32_t now = millis();
    if(g_lastAttemptMs != 0 && (now - g_lastAttemptMs) < SAMPLE_INTERVAL_MS) return false;
    return FT02_BatteryMonitorRefreshNow();
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
