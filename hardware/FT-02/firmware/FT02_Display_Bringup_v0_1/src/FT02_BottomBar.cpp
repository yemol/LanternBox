#include "FT02_BottomBar.h"
#include "FT02_BottomTextFontData.h"

// Bottom bar geometry is centralized here.
// Different pages pass different items; footer layout remains identical.
static const int FT02_BOTTOM_BAR_TOP_Y = 440;
static const int FT02_BOTTOM_BAR_LINE_H = 3;

static const int FT02_BOTTOM_DIVIDER_TOP_Y = 444;
static const int FT02_BOTTOM_DIVIDER_BOTTOM_Y = 476;

static const int FT02_BOTTOM_ICON_Y = 450;
static const int FT02_BOTTOM_ICON_SIZE = 24;

static const int FT02_BOTTOM_TEXT_BASELINE_Y = 470;

static const int FT02_BOTTOM_ICON_TEXT_GAP = 8;

static int FT02_CellStartX(
    int screenWidth,
    int index
)
{
    return (screenWidth * index) / 3;
}

static int FT02_CellEndX(
    int screenWidth,
    int index
)
{
    if(index >= 2)
    {
        return screenWidth;
    }

    return (screenWidth * (index + 1)) / 3;
}

void FT02_DrawBottomBar(
    FT02Display& display,
    const FT02BottomBarItem items[3]
)
{
    int W = display.width();

    // Top divider: full width, 3px thick.
    display.fillRect(
        0,
        FT02_BOTTOM_BAR_TOP_Y,
        W,
        FT02_BOTTOM_BAR_LINE_H,
        GxEPD_BLACK
    );

    // Cell dividers. Keep the same x positions across pages.
    display.drawLine(
        W / 3,
        FT02_BOTTOM_DIVIDER_TOP_Y,
        W / 3,
        FT02_BOTTOM_DIVIDER_BOTTOM_Y,
        GxEPD_BLACK
    );

    display.drawLine(
        W * 2 / 3,
        FT02_BOTTOM_DIVIDER_TOP_Y,
        W * 2 / 3,
        FT02_BOTTOM_DIVIDER_BOTTOM_Y,
        GxEPD_BLACK
    );

    for(int i = 0; i < 3; i++)
    {
        int cellStart = FT02_CellStartX(
            W,
            i
        );

        int cellEnd = FT02_CellEndX(
            W,
            i
        );

        int cellWidth = cellEnd - cellStart;

        int iconWidth = items[i].icon != nullptr
            ? FT02_BOTTOM_ICON_SIZE
            : 0;

        int textWidth = items[i].label != nullptr
            ? FT02_TextWidthPack(
                ft02_bottom_24m,
                items[i].label
            )
            : 0;

        int gap = iconWidth > 0 && textWidth > 0
            ? FT02_BOTTOM_ICON_TEXT_GAP
            : 0;

        int groupWidth = iconWidth + gap + textWidth;
        int groupX = cellStart + (cellWidth - groupWidth) / 2;

        if(items[i].icon != nullptr)
        {
            FT02_DrawIconSize(
                display,
                *items[i].icon,
                groupX,
                FT02_BOTTOM_ICON_Y,
                FT02_BOTTOM_ICON_SIZE,
                false
            );
        }

        if(items[i].label != nullptr)
        {
            FT02_DrawTextPack(
                display,
                ft02_bottom_24m,
                items[i].label,
                groupX + iconWidth + gap,
                FT02_BOTTOM_TEXT_BASELINE_Y
            );
        }
    }
}
