#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_Gnss.h"
#include "FT02_LocationRecorder.h"

void FT02_DrawLocationRecorderScreen(
    FT02Display& display,
    const FT02GnssSnapshot& gnss,
    const FT02LocationRecorderSnapshot& recorder
);

void FT02_DrawLocationRecorderMiddlePartial(
    FT02Display& display,
    const FT02GnssSnapshot& gnss,
    const FT02LocationRecorderSnapshot& recorder
);
