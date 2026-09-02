#pragma once

#include <Arduino.h>

struct FT02BatterySnapshot
{
    bool monitorPresent;
    bool valid;
    float voltage;
    float percent;
    float ratePercentPerHour;
    uint16_t version;
    uint8_t chipId;
    bool hibernating;
    uint32_t generation;
    uint32_t lastReadMs;
    uint8_t consecutiveFailures;
};

enum FT02BatteryBand : uint8_t
{
    FT02_BATTERY_UNKNOWN = 0,
    FT02_BATTERY_NORMAL,
    FT02_BATTERY_LOW,
    FT02_BATTERY_CRITICAL
};

// MAX17048 lives on the shared FT-02 I2C bus (SDA47/SCL21), default 0x36.
// The driver deliberately does not reset or QuickStart the gauge at boot: doing
// so under load can disturb the ModelGauge estimate. The device's default
// automatic hibernation policy is preserved.
void FT02_BatteryMonitorBegin();

// Poll only when the sample interval is due. Returns true when the
// published snapshot generation changed.
bool FT02_BatteryMonitorPoll();

// Force one immediate measurement transaction. Intended for self-test/UI.
bool FT02_BatteryMonitorRefreshNow();

FT02BatterySnapshot FT02_BatterySnapshotCurrent();
FT02BatteryBand FT02_BatteryBandCurrent();

bool FT02_BatteryIsCharging();

// Build compact status-bar strings. line1 is percentage, line2 is state.
void FT02_BatteryFormatStatus(char* line1, size_t line1Size, char* line2, size_t line2Size);

// Software policy thresholds. They are display/warning thresholds only; the
// MAX17048 is a fuel gauge and cannot physically disconnect the battery.
constexpr uint8_t FT02_BATTERY_LOW_PERCENT = 20;
constexpr uint8_t FT02_BATTERY_CRITICAL_PERCENT = 10;
