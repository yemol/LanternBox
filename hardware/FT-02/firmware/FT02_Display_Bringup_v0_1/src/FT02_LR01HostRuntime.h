#pragma once

#include <Arduino.h>


enum FT02CompassCalState : uint8_t
{
    FT02_COMPASS_CAL_IDLE = 0,
    FT02_COMPASS_CAL_RUNNING,
    FT02_COMPASS_CAL_READY,
    FT02_COMPASS_CAL_SAVED,
    FT02_COMPASS_CAL_CANCELED,
    FT02_COMPASS_CAL_FAILED,
    FT02_COMPASS_CAL_UNKNOWN
};

struct FT02CompassCalibrationState
{
    FT02CompassCalState state = FT02_COMPASS_CAL_UNKNOWN;
    uint8_t progress = 0;
    uint8_t quality = 0;
    uint32_t samples = 0;
    bool calibrated = false;
    int32_t minX = 0;
    int32_t maxX = 0;
    int32_t minY = 0;
    int32_t maxY = 0;
    int32_t minZ = 0;
    int32_t maxZ = 0;
    uint32_t lastUpdateMs = 0;
    uint32_t revision = 0;
    int32_t lastErrorCode = 0;
    char lastErrorMessage[48] = {};
};

struct FT02LR01State
{
    bool online = false;
    bool readySeen = false;
    uint8_t protocolVersion = 0;

    bool gnssReady = false;
    bool compassReady = false;
    bool loraReady = false;

    bool fix = false;
    uint8_t fixType = 0;
    int32_t latitudeE7 = 0;
    int32_t longitudeE7 = 0;
    int16_t altitudeDm = 0;
    uint8_t satellites = 0;       // A1 compatibility, equals satUsed
    uint8_t satUsed = 0;
    uint8_t satVisible = 0;
    uint16_t hdopX100 = 9999;
    uint16_t speedCms = 0;
    uint16_t headingX10 = 0;
    bool compassValid = false;
    uint8_t compassQuality = 0;
    uint32_t unixTime = 0;
    bool timeValid = false;

    char radioProfile[16] = "--";
    float frequencyMHz = 0.0f;
    uint32_t rxCount = 0;
    uint16_t nodeCount = 0;
    uint16_t pkiPeerCount = 0;
    uint32_t duplicateCount = 0;
    uint16_t txQueue = 0;

    uint32_t uptime = 0;
    uint32_t gnssBytes = 0;
    uint32_t heapBytes = 0;
    uint32_t psramBytes = 0;
    uint32_t rxErrors = 0;
    uint32_t uartErrors = 0;
    uint32_t radioResets = 0;

    uint32_t lastPongMs = 0;
    uint32_t lastNavMs = 0;
    uint32_t lastRadioMs = 0;
    uint32_t lastSystemMs = 0;
    uint32_t lastRxMs = 0;
    uint32_t lineCount = 0;
    uint32_t pingSequence = 0;

    uint32_t statusRequestSequence = 0;
    uint32_t lastStatusEndId = 0;
    uint32_t lastStatusCompleteMs = 0;
    uint16_t listedNodeCount = 0;
    uint32_t lastNodeListMs = 0;

    FT02CompassCalibrationState compassCalibration;
};

void FT02_LR01HostBegin();
void FT02_LR01HostPoll();

const FT02LR01State& FT02_LR01HostState();
bool FT02_LR01HostOnline();
bool FT02_LR01HostRadioReady();
uint32_t FT02_LR01HostRxLineCount();

bool FT02_LR01HostRequestStatus();
bool FT02_LR01HostRequestStatus(uint32_t requestId);
bool FT02_LR01HostRequestNodeInfo();
bool FT02_LR01HostRequestNodes();

// A2 lifecycle-aware send APIs. hostId is the Core-side unique correlation key.
bool FT02_LR01HostSendBroadcast(uint32_t hostId, const char* utf8Text);
bool FT02_LR01HostSendPrivate(uint32_t hostId, uint32_t nodeId, const char* utf8Text);

// A1-compatible helpers retained for call sites that do not need lifecycle IDs.
bool FT02_LR01HostSendBroadcast(const char* utf8Text);
bool FT02_LR01HostSendPrivate(uint32_t nodeId, const char* utf8Text);


bool FT02_LR01HostCompassCalRequestStatus();
bool FT02_LR01HostCompassCalStart();
bool FT02_LR01HostCompassCalSave();
bool FT02_LR01HostCompassCalCancel();
bool FT02_LR01HostCompassCalReset();
const char* FT02_LR01HostCompassCalStateText(FT02CompassCalState state);
