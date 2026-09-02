#pragma once

#include <Arduino.h>

#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

enum FT02WirelessAction : uint8_t
{
    FT02_WIRELESS_ACTION_NONE = 0,
    FT02_WIRELESS_ACTION_EXIT_TOOLS,
    FT02_WIRELESS_ACTION_REDRAW,
    FT02_WIRELESS_ACTION_PARTIAL_REDRAW,
    FT02_WIRELESS_ACTION_OPEN_HELP
};

void FT02_WirelessManagerBegin();
void FT02_WirelessManagerOpenWiFi();
void FT02_WirelessManagerCloseWiFi();
bool FT02_WirelessManagerPoll(uint32_t nowMs);

FT02WirelessAction FT02_WirelessManagerHandleWiFiInput(const FT02InputEvent& event);

void FT02_DrawWirelessWiFiScreen(FT02Display& display);
void FT02_DrawWirelessWiFiBodyPartial(FT02Display& display);
