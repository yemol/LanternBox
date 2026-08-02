#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconRenderer.h"
#include "FT02_FontData.h"
#include "FT02_StatusFontData.h"

void FT02_DrawStatusBar(
    FT02Display& display
);


void FT02_SetStatusBarClockCache(
    const char* hhmm,
    const char* mmdd
);

void FT02_DrawStatusBarClock(
    FT02Display& display,
    const char* hhmm,
    const char* mmdd
);



void FT02_SetStatusBarStorageCache(
    const char* line1,
    const char* line2
);

void FT02_DrawStatusBarStorage(
    FT02Display& display,
    const char* line1,
    const char* line2
);

void FT02_SetStatusBarGnssCache(
    const char* line1,
    const char* line2
);

void FT02_DrawStatusBarGnss(
    FT02Display& display,
    const char* line1,
    const char* line2
);
