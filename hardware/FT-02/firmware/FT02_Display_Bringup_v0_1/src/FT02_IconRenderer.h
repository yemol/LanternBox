#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconData.h"

void FT02_DrawIconSize(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    int targetSize,
    bool invert
);
