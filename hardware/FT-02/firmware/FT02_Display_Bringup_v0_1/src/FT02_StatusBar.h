#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconRenderer.h"
#include "FT02_FontData.h"
#include "FT02_StatusFontData.h"

void FT02_DrawStatusBar(
    FT02Display& display
);

struct FT02StatusBarSnapshot
{
    char storageLine1[16];
    char storageLine2[16];
    char clockHHMM[6];
    char clockMMDD[6];
    char gnssLine1[16];
    char gnssLine2[16];
};

void FT02_GetStatusBarSnapshot(FT02StatusBarSnapshot& snapshot);


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

void FT02_DrawStatusBarLoRa(FT02Display& display, const char* line2);
