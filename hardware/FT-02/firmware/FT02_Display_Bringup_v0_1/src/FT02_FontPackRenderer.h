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

bool FT02_HasGlyphPack(
    const FT02FontPack& font,
    uint32_t codepoint
);

void FT02_DrawTextPack(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
);

void FT02_DrawTextPackBold(
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

int FT02_CodepointAdvancePack(
    const FT02FontPack& font,
    uint32_t codepoint
);

void FT02_DrawTextPackClipped(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    int clipX,
    int clipY,
    int clipW,
    int clipH
);

void FT02_DrawTextPackInvertClipped(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY,
    int clipX,
    int clipY,
    int clipW,
    int clipH
);

void FT02_DrawTextPackInvert(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
);

void FT02_DrawTextPackInvertBold(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int x,
    int baselineY
);
