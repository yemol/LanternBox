#pragma once
#include <Arduino.h>
#include "FT02_FontPackTypes.h"

// Global embedded font: full CJK Basic block + UI punctuation/symbols.
// Source font files are intentionally not distributed with this project.
extern const FT02FontPack ft02_cjk_24r;
extern const uint32_t FT02_CJK_24R_GLYPH_COUNT;
