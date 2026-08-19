#include "FT02_HelpUI.h"
#include "FT02_EpdLifecycle.h"

#include "FT02_BottomBar.h"
#include "FT02_FontData.h"
#include "FT02_StatusBar.h"

static const int FT02_HELP_CONTENT_TOP = 156;
static const int FT02_HELP_PANEL_Y = 170;
static const int FT02_HELP_PANEL_H = 226;
static const int FT02_HELP_LEFT_X = 40;
static const int FT02_HELP_RIGHT_X = 420;
static const int FT02_HELP_PANEL_W = 340;

static const FT02BottomBarItem FT02_HELP_BOTTOM_ITEMS[3] = {
    {
        nullptr,
        "D/Z/X/C"
    },
    {
        nullptr,
        "ENTER 确认"
    },
    {
        nullptr,
        "B/ESC 返回"
    }
};

static void FT02_DrawHelpKey(
    FT02Display& display,
    const char* label,
    int x,
    int y,
    int w,
    int h
)
{
    display.fillRect(
        x,
        y,
        w,
        h,
        GxEPD_WHITE
    );

    display.drawRect(
        x,
        y,
        w,
        h,
        GxEPD_BLACK
    );

    display.drawRect(
        x + 2,
        y + 2,
        w - 4,
        h - 4,
        GxEPD_BLACK
    );

    int labelWidth = FT02_TextWidthPack(
        ft02_ui_22r,
        label
    );

    FT02_DrawTextPack(
        display,
        ft02_ui_22r,
        label,
        x + (w - labelWidth) / 2,
        y + 31
    );
}

static void FT02_DrawHelpLabelRow(
    FT02Display& display,
    const char* keyLabel,
    const char* actionLabel,
    int baselineY
)
{
    const int keyX = FT02_HELP_RIGHT_X + 24;
    const int keyY = baselineY - 34;
    const int keyW = 200;
    const int keyH = 44;

    FT02_DrawHelpKey(
        display,
        keyLabel,
        keyX,
        keyY,
        keyW,
        keyH
    );

    FT02_DrawTextPack(
        display,
        ft02_menu_28m,
        actionLabel,
        FT02_HELP_RIGHT_X + 244,
        baselineY
    );
}

static void FT02_DrawHelpContent(
    FT02Display& display
)
{
    int W = display.width();

    FT02_DrawTextPack(
        display,
        ft02_title_32b,
        "帮助",
        40,
        132
    );

    const char* returnHint = "B / ESC 返回";
    int returnHintWidth = FT02_TextWidthPack(
        ft02_ui_22r,
        returnHint
    );

    FT02_DrawTextPack(
        display,
        ft02_ui_22r,
        returnHint,
        W - 40 - returnHintWidth,
        132
    );

    display.fillRect(
        40,
        FT02_HELP_CONTENT_TOP,
        W - 80,
        3,
        GxEPD_BLACK
    );

    display.drawRect(
        FT02_HELP_LEFT_X,
        FT02_HELP_PANEL_Y,
        FT02_HELP_PANEL_W,
        FT02_HELP_PANEL_H,
        GxEPD_BLACK
    );

    display.drawRect(
        FT02_HELP_RIGHT_X,
        FT02_HELP_PANEL_Y,
        FT02_HELP_PANEL_W,
        FT02_HELP_PANEL_H,
        GxEPD_BLACK
    );

    FT02_DrawTextPack(
        display,
        ft02_menu_28m,
        "方向键",
        FT02_HELP_LEFT_X + 24,
        FT02_HELP_PANEL_Y + 42
    );

    const int keyW = 58;
    const int keyH = 48;
    const int centerX = FT02_HELP_LEFT_X + FT02_HELP_PANEL_W / 2;
    const int topY = FT02_HELP_PANEL_Y + 62;

    FT02_DrawHelpKey(
        display,
        "D",
        centerX - keyW / 2,
        topY,
        keyW,
        keyH
    );

    const int rowStartX = centerX - (keyW * 3 + 20) / 2;

    FT02_DrawHelpKey(
        display,
        "Z",
        rowStartX,
        topY + keyH + 10,
        keyW,
        keyH
    );

    FT02_DrawHelpKey(
        display,
        "X",
        rowStartX + keyW + 10,
        topY + keyH + 10,
        keyW,
        keyH
    );

    FT02_DrawHelpKey(
        display,
        "C",
        rowStartX + (keyW + 10) * 2,
        topY + keyH + 10,
        keyW,
        keyH
    );

    FT02_DrawTextPack(
        display,
        ft02_menu_28m,
        "常用快捷键",
        FT02_HELP_RIGHT_X + 24,
        FT02_HELP_PANEL_Y + 42
    );

    FT02_DrawHelpLabelRow(
        display,
        "ENTER / SPACE",
        "确认",
        FT02_HELP_PANEL_Y + 96
    );

    FT02_DrawHelpLabelRow(
        display,
        "H",
        "帮助",
        FT02_HELP_PANEL_Y + 154
    );

    FT02_DrawHelpLabelRow(
        display,
        "K",
        "状态页罗盘",
        FT02_HELP_PANEL_Y + 212
    );


}

void FT02_DrawHelpScreen(
    FT02Display& display
)
{
    display.setFullWindow();

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        FT02_DrawStatusBar(display);
        FT02_DrawHelpContent(display);

        FT02_DrawBottomBar(
            display,
            FT02_HELP_BOTTOM_ITEMS
        );
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display, "help-full");
}
