#pragma once

#include <Arduino.h>

// Core-owned GNSS snapshot: ESP32-S3 UART1 RX=GPIO5, TX=GPIO6, fixed 115200 baud,
// 4096-byte RX buffer, TinyGPSPlus NMEA parsing. Sleep Power A5A does not
// rewrite navigation/output configuration; runtime OFF only requests the
// u-blox M10 software standby state and uses UART-RX edge wake.
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
    uint32_t bytesReceived;
    uint32_t startAgeMs;
    uint32_t lastFixAgeMs;
    uint32_t lastTimeSyncAgeMs;
    uint32_t timeSyncCount;
    uint32_t unixTime;
    uint8_t fixType;
    uint8_t fixQuality;
    uint8_t satellites;
    uint8_t satellitesVisible;
    double latitude;
    double longitude;
    float rawAltitudeMeters;
    float altitudeMeters;
    float hdop;
    float rawSpeedKmh;
    float speedKmh;
    float courseDegrees;
    char utcTime[12];
    char utcDate[12];
    char localTime[12];
    char localDate[12];
    uint32_t uiGeneration;
};

void FT02_GnssBegin();
void FT02_GnssPoll();
FT02GnssSnapshot FT02_GnssSnapshotCurrent();

// Indoor / Outdoor / Sleep policy controls the Core UART transport and GNSS data publication.
// The GNSS rail remains powered. Runtime OFF sends UBX-RXM-PMREQ software
// standby before UART close; host TX is then held HIGH to prevent false wake.
bool FT02_GnssSetRuntimeEnabled(bool enabled);
bool FT02_GnssRuntimeEnabled();
bool FT02_GnssHardwareKnown();
bool FT02_GnssHardwareReady();
