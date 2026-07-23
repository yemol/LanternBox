
#include "FT02_StatusBar.h"
#include "FT02_BatteryIconReplace2.h"

static const int FT02_STATUS_LINE_Y = 73;
static const int FT02_STATUS_LINE_H = 3;

static const int FT02_TIME_X = 27;
static const int FT02_TIME_BASELINE_Y = 44;

static const int FT02_DATE_X = 132;
static const int FT02_DATE_BASELINE_Y = 44;

static const int FT02_BLOCK_START_X = 225;
static const int FT02_BLOCK_WIDTH = 150;

static const int FT02_ICON_OFFSET_X = 7;
static const int FT02_ICON_Y = 16;
static const int FT02_ICON_SIZE = 32;

static const int FT02_STATUS_TEXT_OFFSET_X = 52;
static const int FT02_STATUS_TEXT_LINE1_Y = 34;
static const int FT02_STATUS_TEXT_LINE2_Y = 58;

static const int FT02_STATUS_VLINE_TOP_Y = 6;
static const int FT02_STATUS_VLINE_BOTTOM_Y = 66;

// Partial refresh region for clock + date.
// Visual element positions are frozen; only the refresh window is widened to cover 05/20.
static const int FT02_CLOCK_PARTIAL_X = 16;
static const int FT02_CLOCK_PARTIAL_Y = 8;
static const int FT02_CLOCK_PARTIAL_W = 184;
static const int FT02_CLOCK_PARTIAL_H = 52;

static void FT02_DrawClockText(
    FT02Display& display,
    const char* hhmm,
    const char* mmdd
)
{
    FT02_DrawTextPack(
        display,
        ft02_title_32b,
        hhmm,
        FT02_TIME_X,
        FT02_TIME_BASELINE_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_ui_22r,
        mmdd,
        FT02_DATE_X,
        FT02_DATE_BASELINE_Y
    );
}

void FT02_DrawStatusBar(
    FT02Display& display
)
{
    int W = display.width();

    // Bottom divider: full width, 3px thick.
    display.fillRect(
        0,
        FT02_STATUS_LINE_Y,
        W,
        FT02_STATUS_LINE_H,
        GxEPD_BLACK
    );

    // Default boot display. Runtime clock/date will update this area afterward.
    FT02_DrawClockText(
        display,
        "14:28",
        "05/20"
    );

    int blockStart = FT02_BLOCK_START_X;

    // vertical separators
    for(int i = 0; i < 4; i++)
    {
        display.drawLine(
            blockStart + i * FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_TOP_Y,
            blockStart + i * FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );
    }

    // LoRa
    FT02_DrawIconSize(
        display,
        ICON_STATUS_WIRELESS,
        blockStart + FT02_ICON_OFFSET_X,
        FT02_ICON_Y,
        FT02_ICON_SIZE,
        false
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "LoRa",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "已连接",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );


    // GPS
    blockStart += FT02_BLOCK_WIDTH;

    FT02_DrawIconSize(
        display,
        ICON_STATUS_GPS,
        blockStart + FT02_ICON_OFFSET_X,
        FT02_ICON_Y,
        FT02_ICON_SIZE,
        false
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "GPS",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "未定位",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );


    // SD
    blockStart += FT02_BLOCK_WIDTH;

    FT02_DrawIconSize(
        display,
        ICON_STATUS_SD,
        blockStart + FT02_ICON_OFFSET_X,
        FT02_ICON_Y,
        FT02_ICON_SIZE,
        false
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "28G",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "SD剩余",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );


    // Battery
    blockStart += FT02_BLOCK_WIDTH;

    FT02_DrawIconSize(
        display,
        ICON_STATUS_BATTERY_REPLACE2,
        blockStart + FT02_ICON_OFFSET_X,
        FT02_ICON_Y,
        FT02_ICON_SIZE,
        false
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "86%",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        "电量",
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );
}

void FT02_DrawStatusBarClock(
    FT02Display& display,
    const char* hhmm,
    const char* mmdd
)
{
    display.setPartialWindow(
        FT02_CLOCK_PARTIAL_X,
        FT02_CLOCK_PARTIAL_Y,
        FT02_CLOCK_PARTIAL_W,
        FT02_CLOCK_PARTIAL_H
    );

    display.firstPage();

    do
    {
        display.fillRect(
            FT02_CLOCK_PARTIAL_X,
            FT02_CLOCK_PARTIAL_Y,
            FT02_CLOCK_PARTIAL_W,
            FT02_CLOCK_PARTIAL_H,
            GxEPD_WHITE
        );

        FT02_DrawClockText(
            display,
            hhmm,
            mmdd
        );
    }
    while(display.nextPage());

    display.setFullWindow();
}
