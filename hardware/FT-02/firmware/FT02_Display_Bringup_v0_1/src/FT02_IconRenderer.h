#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconData.h"

void FT02_DrawIcon(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    bool invert
);


void FT02_DrawIconScaled(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    int scale,
    bool invert
);


void FT02_DrawIconSize(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    int targetSize,
    bool invert
);
