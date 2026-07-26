#pragma once

#include "FT02_FontPackRenderer.h"

int FT02_HomeCardCount();

int FT02_ClampHomeCardIndex(
    int selectedCardIndex
);

int FT02_MoveHomeCardSelection(
    int selectedCardIndex,
    int deltaRow,
    int deltaCol
);

void FT02_DrawHomeCardGrid(
    FT02Display& display,
    int selectedCardIndex
);

void FT02_RedrawHomeCardSelection(
    FT02Display& display,
    int oldSelectedCardIndex,
    int newSelectedCardIndex
);
