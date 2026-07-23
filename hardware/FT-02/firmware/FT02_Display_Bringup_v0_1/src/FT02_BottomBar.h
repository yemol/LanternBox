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
