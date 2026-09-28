#pragma once

#include <Arduino.h>

#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

enum FT02SystemSettingsAction : uint8_t
{
    FT02_SETTINGS_ACTION_NONE = 0,
    FT02_SETTINGS_ACTION_REDRAW,
    FT02_SETTINGS_ACTION_EXIT_HOME,
    FT02_SETTINGS_ACTION_OPEN_HELP
};

void FT02_SystemSettingsUIOpen();
FT02SystemSettingsAction FT02_SystemSettingsUIHandleInput(const FT02InputEvent& event);
void FT02_DrawSystemSettingsScreen(FT02Display& display);
