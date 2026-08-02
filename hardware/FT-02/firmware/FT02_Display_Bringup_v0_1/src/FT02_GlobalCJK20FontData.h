#pragma once
#include <Arduino.h>
#include "FT02_FontPackTypes.h"

// Optimized global 20 px font: grayscale raster + threshold 170.
// Source font files are intentionally not distributed with this project.
extern const FT02FontPack ft02_cjk_20r;
extern const uint32_t FT02_CJK_20R_GLYPH_COUNT;
