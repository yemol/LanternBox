#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>

#include "FT02_FontPackTypes.h"

using FT02Display = GxEPD2_BW<
    GxEPD2_426_GDEQ0426T82,
    GxEPD2_426_GDEQ0426T82::HEIGHT
>;

uint32_t FT02_ReadCodepoint(
    const char* text,
    int* usedBytes
);

void FT02_DrawTextPack(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
);

int FT02_TextWidthPack(
    const FT02FontPack& font,
    const char* text
);

void FT02_DrawTextPackInvert(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
);
