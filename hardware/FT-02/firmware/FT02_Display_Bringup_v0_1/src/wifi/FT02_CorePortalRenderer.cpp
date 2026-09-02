#include "FT02_CorePortalRenderer.h"

#include <stdio.h>

#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"

namespace
{
static constexpr int FT02_PORTAL_CARD_X = 48;
static constexpr int FT02_PORTAL_CARD_Y = 156;
static constexpr int FT02_PORTAL_CARD_W = 328;
static constexpr int FT02_PORTAL_CARD_H = 82;
static constexpr int FT02_PORTAL_CARD_GAP_X = 36;
static constexpr int FT02_PORTAL_CARD_GAP_Y = 24;

static void drawCard(FT02Display& display, const FT02CorePortalItem& item, uint8_t index, bool selected)
{
    const int col = index % 2;
    const int row = index / 2;
    const int x = FT02_PORTAL_CARD_X + col * (FT02_PORTAL_CARD_W + FT02_PORTAL_CARD_GAP_X);
    const int y = FT02_PORTAL_CARD_Y + row * (FT02_PORTAL_CARD_H + FT02_PORTAL_CARD_GAP_Y);

    if(selected)
    {
        display.fillRect(x, y, FT02_PORTAL_CARD_W, FT02_PORTAL_CARD_H, GxEPD_BLACK);
    }
    else
    {
        display.fillRect(x, y, FT02_PORTAL_CARD_W, FT02_PORTAL_CARD_H, GxEPD_WHITE);
        display.drawRect(x, y, FT02_PORTAL_CARD_W, FT02_PORTAL_CARD_H, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, FT02_PORTAL_CARD_W - 4, FT02_PORTAL_CARD_H - 4, GxEPD_BLACK);
    }

    const int labelW = FT02_TextWidthPack(ft02_cjk_24b, item.label);
    const int labelX = x + (FT02_PORTAL_CARD_W - labelW) / 2;

    if(selected)
    {
        FT02_DrawTextPackInvert(display, ft02_cjk_24b, item.label, labelX, y + 52);
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_24b, item.label, labelX, y + 52);
    }
}
}

void FT02_CorePortalRendererDraw(FT02Display& display, const FT02CorePortalPage& page, int topY)
{
    (void)topY;
    const uint8_t count = page.itemCount < 4 ? page.itemCount : 4;
    for(uint8_t i = 0; i < count; ++i)
    {
        drawCard(display, page.items[i], i, i == page.selected);
    }
}
