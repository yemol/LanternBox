#pragma once

#include <Arduino.h>

enum FT02CoreCompassCalState : uint8_t
{
    FT02_CORE_COMPASS_CAL_IDLE = 0,
    FT02_CORE_COMPASS_CAL_RUNNING,
    FT02_CORE_COMPASS_CAL_READY,
    FT02_CORE_COMPASS_CAL_SAVED,
    FT02_CORE_COMPASS_CAL_CANCELED,
    FT02_CORE_COMPASS_CAL_FAILED,
    FT02_CORE_COMPASS_CAL_UNKNOWN
};

struct FT02CoreCompassCalibrationState
{
    FT02CoreCompassCalState state = FT02_CORE_COMPASS_CAL_UNKNOWN;
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
    uint32_t revision = 0;
    int32_t lastErrorCode = 0;
    char lastErrorMessage[48] = {};
};

struct FT02CoreCompassSnapshot
{
    bool hardwareKnown = false;
    bool ready = false;
    bool valid = false;
    uint16_t headingX10 = 0;
    uint8_t quality = 0;
    int16_t rawX = 0;
    int16_t rawY = 0;
    int16_t rawZ = 0;
    uint32_t lastUpdateMs = 0;
    uint32_t uiGeneration = 0;
    FT02CoreCompassCalibrationState calibration;
};

void FT02_CompassCoreBegin();
void FT02_CompassCorePoll();
FT02CoreCompassSnapshot FT02_CompassCoreSnapshotCurrent();

bool FT02_CompassCoreCalStart();
bool FT02_CompassCoreCalSave();
bool FT02_CompassCoreCalCancel();
bool FT02_CompassCoreCalReset();
const char* FT02_CompassCoreCalStateText(FT02CoreCompassCalState state);

// Sleep Power A3: put QMC5883L into standby while preserving calibration state.
bool FT02_CompassCoreSleepEnter();
bool FT02_CompassCoreSleepExit();
