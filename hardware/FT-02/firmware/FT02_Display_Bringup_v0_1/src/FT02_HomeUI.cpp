#include "FT02_HomeUI.h"

#include "FT02_FontData.h"
#include "FT02_StatusFontData.h"
#include "FT02_IconRenderer.h"
#include "FT02_BottomIconData.h"
#include "FT02_BottomBar.h"
#include "FT02_StatusBar.h"

static void drawCentered(
    FT02Display& display,
    const FT02FontPack& font,
    const char* text,
    int baselineY
)
{
    int textW = FT02_TextWidthPack(font, text);

    FT02_DrawTextPack(
        display,
        font,
        text,
        (display.width() - textW) / 2,
        baselineY
    );
}

static void drawBookIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.drawRect(x, y, 42, 44, c);
    display.drawLine(x + 21, y, x + 21, y + 44, c);
    display.drawLine(x + 8, y + 11, x + 17, y + 11, c);
    display.drawLine(x + 25, y + 11, x + 34, y + 11, c);
    display.drawLine(x + 8, y + 21, x + 17, y + 21, c);
    display.drawLine(x + 25, y + 21, x + 34, y + 21, c);
    display.drawLine(x + 10, y + 44, x + 14, y + 38, c);
    display.drawLine(x + 14, y + 38, x + 18, y + 44, c);
}

static void drawMapIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.drawRect(x, y + 10, 48, 34, c);
    display.drawLine(x + 16, y + 10, x + 16, y + 44, c);
    display.drawLine(x + 32, y + 10, x + 32, y + 44, c);
    display.fillCircle(x + 34, y + 17, 5, c);
    display.drawLine(x + 34, y + 22, x + 29, y + 33, c);
    display.drawLine(x + 34, y + 22, x + 39, y + 33, c);
}

static void drawLogIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.drawRect(x + 8, y, 38, 46, c);
    display.fillRect(x, y + 7, 10, 4, c);
    display.fillRect(x, y + 20, 10, 4, c);
    display.fillRect(x, y + 33, 10, 4, c);
    display.drawLine(x + 17, y + 13, x + 38, y + 13, c);
    display.drawLine(x + 17, y + 24, x + 38, y + 24, c);
    display.drawLine(x + 17, y + 35, x + 32, y + 35, c);
}

static void drawSearchIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.drawCircle(x + 20, y + 20, 17, c);
    display.drawLine(x + 32, y + 32, x + 47, y + 47, c);
    display.drawLine(x + 33, y + 31, x + 48, y + 46, c);
}

static void drawDeviceIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.drawRect(x + 5, y + 8, 40, 32, c);
    display.drawLine(x + 10, y + 34, x + 18, y + 25, c);
    display.drawLine(x + 18, y + 25, x + 28, y + 31, c);
    display.drawLine(x + 28, y + 31, x + 40, y + 15, c);
}

static void drawCommIcon(
    FT02Display& display,
    int x,
    int y,
    bool invert
)
{
    uint16_t c = invert ? GxEPD_WHITE : GxEPD_BLACK;

    display.fillCircle(x + 24, y + 27, 4, c);
    display.drawCircle(x + 24, y + 27, 12, c);
    display.drawCircle(x + 24, y + 27, 22, c);
    if(!invert)
    {
        display.fillRect(x, y, 24, 54, GxEPD_WHITE);
    }
    display.drawLine(x + 24, y + 27, x + 48, y + 10, c);
    display.drawLine(x + 24, y + 27, x + 48, y + 44, c);
}

static void drawTile(
    FT02Display& display,
    int x,
    int y,
    int w,
    int h,
    const char* label,
    int iconType,
    bool selected
)
{
    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
    }
    else
    {
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, w - 4, h - 4, GxEPD_BLACK);
    }

    int iconX = x + 26;
    int iconY = y + 22;

    if(iconType == 0) drawBookIcon(display, iconX, iconY, selected);
    if(iconType == 1) drawMapIcon(display, iconX, iconY, selected);
    if(iconType == 2) drawLogIcon(display, iconX, iconY, selected);
    if(iconType == 3) drawSearchIcon(display, iconX, iconY, selected);
    if(iconType == 4) drawDeviceIcon(display, iconX, iconY, selected);
    if(iconType == 5) drawCommIcon(display, iconX, iconY, selected);

    if(selected)
    {
        FT02_DrawTextPackInvert(
            display,
            ft02_menu_24m,
            label,
            x + 112,
            y + 54
        );
    }
    else
    {
        FT02_DrawTextPack(
            display,
            ft02_menu_24m,
            label,
            x + 112,
            y + 54
        );
    }
}

void FT02_DrawHomeScreen(
    FT02Display& display
)
{
    int W = display.width();

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        // outer frame
        // outer frame removed

        // Shared status bar
        FT02_DrawStatusBar(display);

        // Hero title
        FT02_DrawTextPack(display, ft02_hero_40b, "离线智能 掌控全局", 34, 118);
        FT02_DrawTextPack(display, ft02_menu_24m, "FT-02 手持离线终端", 34, 162);
        FT02_DrawTextPack(display, ft02_ui_22r, "方向键选择 确认键进入", W - 310, 160);

        display.drawLine(24, 184, W - 24, 184, GxEPD_BLACK);

        // 2 x 3 cards
        int tileX = 32;
        int tileY = 202;
        int tileW = 238;
        int tileH = 86;
        int gapX = 24;
        int gapY = 24;

        drawTile(display, tileX, tileY, tileW, tileH, "知识库", 0, true);
        drawTile(display, tileX + tileW + gapX, tileY, tileW, tileH, "地图导航", 1, false);
        drawTile(display, tileX + (tileW + gapX) * 2, tileY, tileW, tileH, "日志记录", 2, false);

        drawTile(display, tileX, tileY + tileH + gapY, tileW, tileH, "定位搜索", 3, false);
        drawTile(display, tileX + tileW + gapX, tileY + tileH + gapY, tileW, tileH, "设备状态", 4, false);
        drawTile(display, tileX + (tileW + gapX) * 2, tileY + tileH + gapY, tileW, tileH, "通信管理", 5, false);

        // Footer
        // Bottom action bar
        const FT02BottomBarItem bottomItems[3] = {
            {
                nullptr,
                "编号：FT-02A"
            },
            {
                nullptr,
                "版本：v1.30"
            },
            {
                &ICON_BOTTOM_HELP,
                "帮助(H)"
            }
        };

        FT02_DrawBottomBar(
            display,
            bottomItems
        );
    }
    while(display.nextPage());
}
