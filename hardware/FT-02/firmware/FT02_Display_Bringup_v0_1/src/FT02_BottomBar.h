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

// Same footer geometry, but lets a page choose asymmetric three-column widths.
// split1Pct and split2Pct are the first/second divider positions in percent
// of screen width (for example 25/75 gives 25% / 50% / 25%).
void FT02_DrawBottomBarWithFontSplits(
    FT02Display& display,
    const FT02BottomBarItem items[3],
    const FT02FontPack& font,
    int split1Pct,
    int split2Pct
);
