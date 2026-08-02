#pragma once

#include <Arduino.h>

struct FT02GnssSnapshot
{
    bool started;
    bool serialDataSeen;
    bool nmeaSeen;
    bool communicationActive;
    bool fixValid;
    uint32_t currentBaud;
    uint32_t detectedBaud;
    uint32_t bytesReceived;
    uint32_t validSentences;
    uint32_t checksumErrors;
    uint32_t startAgeMs;
    uint32_t lastByteAgeMs;
    uint32_t lastSentenceAgeMs;
    uint32_t lastFixAgeMs;
    uint8_t fixType;
    uint8_t fixQuality;
    uint8_t satellites;
    double latitude;
    double longitude;
    float altitudeMeters;
    float hdop;
    float speedKmh;
    float courseDegrees;
    char utcTime[12];
    char utcDate[12];
    uint32_t uiGeneration;
};

void FT02_GnssBegin();
void FT02_GnssPoll();
void FT02_GnssReconnect();
FT02GnssSnapshot FT02_GnssSnapshotCurrent();
