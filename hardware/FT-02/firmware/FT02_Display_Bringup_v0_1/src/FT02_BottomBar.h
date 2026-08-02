#pragma once

#include "FT02_FontPackRenderer.h"
#include "FT02_IconRenderer.h"
#include "FT02_FontData.h"
#include "FT02_IconData.h"

struct FT02BottomBarItem
{
    const FT02Icon* icon;
    const char* label;
};

void FT02_DrawBottomBar(
    FT02Display& display,
    const FT02BottomBarItem items[3]
);

// Reuses the exact same footer geometry while allowing a page-specific
// bitmap-font subset. Existing pages continue to use FT02_DrawBottomBar().
void FT02_DrawBottomBarWithFont(
    FT02Display& display,
    const FT02BottomBarItem items[3],
    const FT02FontPack& font
);
