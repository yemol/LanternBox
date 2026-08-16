#pragma once

#include <Arduino.h>

struct FT02GnssSnapshot
{
    bool started;
    bool serialDataSeen;
    bool nmeaSeen;
    bool gsvSeen;
    bool communicationActive;
    bool fixValid;
    bool hasPosition;
    bool altitudeValid;
    bool speedValid;
    bool moving;
    bool courseValid;
    bool timeValid;
    bool systemTimeSynchronized;
    uint32_t currentBaud;
    uint32_t detectedBaud;
    uint32_t bytesReceived;
    uint32_t validSentences;
    uint32_t checksumErrors;
    uint32_t startAgeMs;
    uint32_t lastByteAgeMs;
    uint32_t lastSentenceAgeMs;
    uint32_t lastFixAgeMs;
    uint32_t lastTimeSyncAgeMs;
    uint32_t timeSyncCount;
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
void FT02_GnssReconnect();
FT02GnssSnapshot FT02_GnssSnapshotCurrent();
