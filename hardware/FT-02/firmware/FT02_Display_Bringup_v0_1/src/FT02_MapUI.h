#pragma once

#include "FT02_FontPackRenderer.h"

void FT02_MapUIOpen();

bool FT02_MapUIMove(
    int deltaX,
    int deltaY
);

void FT02_MapUIReload();

void FT02_DrawMapScreen(
    FT02Display& display
);
