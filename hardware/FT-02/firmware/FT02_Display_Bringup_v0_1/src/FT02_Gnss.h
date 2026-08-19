#pragma once

#include <Arduino.h>

// Application-facing navigation snapshot. Despite the historical filename,
// this is NOT a GNSS hardware driver. In production all navigation data comes
// exclusively from FT02_LR01HostRuntime NAV_STATE/SYSTEM_STATE.
struct FT02GnssSnapshot
{
    bool started;
    bool communicationActive;
    bool fixValid;
    bool hasPosition;
    bool altitudeValid;
    bool speedValid;
    bool moving;
    bool courseValid;
    bool timeValid;
    bool systemTimeSynchronized;
    uint32_t bytesReceived;       // LR01 SYSTEM_STATE gnss_bytes
    uint32_t startAgeMs;
    uint32_t lastFixAgeMs;
    uint32_t lastTimeSyncAgeMs;
    uint32_t timeSyncCount;
    uint8_t fixType;
    uint8_t fixQuality;           // compatibility: mirrors fixType
    uint8_t satellites;
    uint8_t satellitesVisible;
    double latitude;
    double longitude;
    float rawAltitudeMeters;      // compatibility: equals altitudeMeters
    float altitudeMeters;
    float hdop;
    float rawSpeedKmh;            // compatibility: equals speedKmh
    float speedKmh;
    float courseDegrees;          // LR01 heading/compass value
    char utcTime[12];
    char utcDate[12];
    char localTime[12];
    char localDate[12];
    uint32_t uiGeneration;
};

// Compatibility lifecycle. These functions never open/configure a GNSS UART.
void FT02_GnssBegin();
void FT02_GnssPoll();
FT02GnssSnapshot FT02_GnssSnapshotCurrent();

// Sole production ingress for navigation state: called by LR01 Host Runtime.
void FT02_GnssApplyLR01State(bool gnssReady,
                             bool fix,
                             uint8_t fixType,
                             int32_t latitudeE7,
                             int32_t longitudeE7,
                             int16_t altitudeDm,
                             uint8_t satellitesUsed,
                             uint8_t satellitesVisible,
                             uint16_t hdopX100,
                             uint16_t speedCms,
                             uint16_t headingX10,
                             bool compassValid,
                             uint8_t compassQuality,
                             uint32_t gnssBytes,
                             uint32_t unixTime,
                             bool timeValid);
