#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconRenderer.h"
#include "FT02_FontData.h"
#include "FT02_StatusFontData.h"

void FT02_DrawStatusBar(
    FT02Display& display
);

void FT02_DrawStatusBarClock(
    FT02Display& display,
    const char* hhmm,
    const char* mmdd
);
