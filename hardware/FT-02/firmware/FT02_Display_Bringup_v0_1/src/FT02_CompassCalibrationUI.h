#pragma once

#include "FT02_InputManager.h"
#include "FT02_StatusBar.h"

enum FT02CompassCalibrationUIAction : uint8_t
{
    FT02_COMPASS_CAL_UI_NONE = 0,
    FT02_COMPASS_CAL_UI_REDRAW,
    FT02_COMPASS_CAL_UI_EXIT
};

void FT02_CompassCalibrationUIOpen();
void FT02_DrawCompassCalibrationScreen(FT02Display& display);
FT02CompassCalibrationUIAction FT02_CompassCalibrationUIHandleInput(const FT02InputEvent& event);
uint32_t FT02_CompassCalibrationUIRevision();
