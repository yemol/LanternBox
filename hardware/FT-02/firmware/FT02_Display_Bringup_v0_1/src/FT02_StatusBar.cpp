
#include "FT02_StatusBar.h"
#include "FT02_LoRaTransport.h"
#include "FT02_LoRaNodeRuntime.h"
#include "FT02_LoRaCommunicationRuntime.h"
#include "FT02_BatteryIcon.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include <string.h>
#include <stdio.h>

static char g_ft02StorageLine1[16] = "SD";
static char g_ft02StorageLine2[16] = "INIT";
static char g_ft02ClockHHMM[6] = "14:28";
static char g_ft02ClockMMDD[6] = "05/20";
static char g_ft02GnssLine1[16] = "GPS";
static char g_ft02GnssLine2[16] = "连接中";

void FT02_GetStatusBarSnapshot(FT02StatusBarSnapshot& snapshot)
{
    snprintf(snapshot.storageLine1, sizeof(snapshot.storageLine1), "%s", g_ft02StorageLine1);
    snprintf(snapshot.storageLine2, sizeof(snapshot.storageLine2), "%s", g_ft02StorageLine2);
    snprintf(snapshot.clockHHMM, sizeof(snapshot.clockHHMM), "%s", g_ft02ClockHHMM);
    snprintf(snapshot.clockMMDD, sizeof(snapshot.clockMMDD), "%s", g_ft02ClockMMDD);
    snprintf(snapshot.gnssLine1, sizeof(snapshot.gnssLine1), "%s", g_ft02GnssLine1);
    snprintf(snapshot.gnssLine2, sizeof(snapshot.gnssLine2), "%s", g_ft02GnssLine2);
}

void FT02_SetStatusBarStorageCache(const char* line1, const char* line2)
{
    snprintf(g_ft02StorageLine1, sizeof(g_ft02StorageLine1), "%s", line1 != nullptr ? line1 : "SD");
    snprintf(g_ft02StorageLine2, sizeof(g_ft02StorageLine2), "%s", line2 != nullptr ? line2 : "INIT");
}

void FT02_SetStatusBarClockCache(const char* hhmm, const char* mmdd)
{
    snprintf(g_ft02ClockHHMM, sizeof(g_ft02ClockHHMM), "%s", hhmm != nullptr ? hhmm : "--:--");
    snprintf(g_ft02ClockMMDD, sizeof(g_ft02ClockMMDD), "%s", mmdd != nullptr ? mmdd : "--/--");
}

void FT02_SetStatusBarGnssCache(const char* line1, const char* line2)
{
    snprintf(g_ft02GnssLine1, sizeof(g_ft02GnssLine1), "%s", line1 != nullptr ? line1 : "GPS");
    snprintf(g_ft02GnssLine2, sizeof(g_ft02GnssLine2), "%s", line2 != nullptr ? line2 : "连接中");
}


static void FT02_FormatLoRaLine1(char* out, size_t outSize)
{
    // Keep line 1 fixed-width. Unread count belongs on line 2 so the
    // compact LoRa block never grows past its right boundary.
    snprintf(out, outSize, "LoRa");
}

static void FT02_FormatLoRaLine2(char* out, size_t outSize, const char* connectionStatus)
{
    const uint16_t unread = FT02_LoRaCommunicationUnreadCount();

    // Unread messages own line 2 whenever present. This keeps line 1 fixed as
    // "LoRa" and prevents the notification badge from overflowing the block.
    // Cap presentation at 9+ so the width remains deterministic.
    if(unread > 0)
    {
        if(unread <= 9) snprintf(out, outSize, "%u条消息", static_cast<unsigned>(unread));
        else snprintf(out, outSize, "9+条消息");
        return;
    }

    snprintf(out, outSize, "%s", connectionStatus != nullptr ? connectionStatus : "连接中");
}

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

    // Full-page renders use the live cached clock. This avoids an immediate
    // second partial refresh after every page transition.
    FT02_DrawClockText(
        display,
        g_ft02ClockHHMM,
        g_ft02ClockMMDD
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

    char loraLine1[16];
    FT02_FormatLoRaLine1(loraLine1, sizeof(loraLine1));
    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        loraLine1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    const char* loraStatus = FT02_LoRaNodeRuntimeReady()
        ? "已连接"
        : (FT02_LoRaTransportLinkUp() ? "同步中" : "连接中");
    char loraLine2[20];
    FT02_FormatLoRaLine2(loraLine2, sizeof(loraLine2), loraStatus);

    // The full 20px CJK pack covers both connection states and the unread label.
    FT02_DrawTextPack(
        display,
        ft02_cjk_20r,
        loraLine2,
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
        g_ft02GnssLine1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_cjk_20r,
        g_ft02GnssLine2,
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
        g_ft02StorageLine1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        g_ft02StorageLine2,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );


    // Battery
    blockStart += FT02_BLOCK_WIDTH;

    FT02_DrawIconSize(
        display,
        ICON_STATUS_BATTERY,
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


static void FT02_DrawStatusLoRaBlockContent(
    FT02Display& display,
    const char* line2
)
{
    const int blockStart = FT02_BLOCK_START_X;

    FT02_DrawIconSize(
        display,
        ICON_STATUS_WIRELESS,
        blockStart + FT02_ICON_OFFSET_X,
        FT02_ICON_Y,
        FT02_ICON_SIZE,
        false
    );

    char loraLine1[16];
    FT02_FormatLoRaLine1(loraLine1, sizeof(loraLine1));
    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        loraLine1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    char loraLine2[20];
    FT02_FormatLoRaLine2(loraLine2, sizeof(loraLine2), line2);
    FT02_DrawTextPack(
        display,
        ft02_cjk_20r,
        loraLine2,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );
}

void FT02_DrawStatusBarLoRa(
    FT02Display& display,
    const char* line2
)
{
    static const int partialX = 212;
    static const int partialY = 6;
    static const int partialW = 160;
    static const int partialH = 61;

    display.setPartialWindow(partialX, partialY, partialW, partialH);
    display.firstPage();
    do
    {
        display.fillRect(partialX, partialY, partialW, partialH, GxEPD_WHITE);

        display.drawLine(
            FT02_BLOCK_START_X - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );
        display.drawLine(
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );

        FT02_DrawStatusLoRaBlockContent(display, line2);
    }
    while(display.nextPage());

    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display, "status-lora-partial");
}

static void FT02_DrawStatusGnssBlockContent(
    FT02Display& display,
    const char* line1,
    const char* line2
)
{
    const int blockStart = FT02_BLOCK_START_X + FT02_BLOCK_WIDTH;

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
        line1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_cjk_20r,
        line2,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );
}

void FT02_DrawStatusBarGnss(
    FT02Display& display,
    const char* line1,
    const char* line2
)
{
    FT02_SetStatusBarGnssCache(line1, line2);

    static const int partialX = 362;
    static const int partialY = 6;
    static const int partialW = 160;
    static const int partialH = 61;

    display.setPartialWindow(partialX, partialY, partialW, partialH);
    display.firstPage();

    do
    {
        display.fillRect(partialX, partialY, partialW, partialH, GxEPD_WHITE);

        display.drawLine(
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );

        display.drawLine(
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 2 - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 2 - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );

        FT02_DrawStatusGnssBlockContent(display, line1, line2);
    }
    while(display.nextPage());

    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display, "status-gnss-partial");
}

static void FT02_DrawStatusStorageBlockContent(
    FT02Display& display,
    const char* line1,
    const char* line2
)
{
    int blockStart = FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 2;

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
        line1,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE1_Y
    );

    FT02_DrawTextPack(
        display,
        ft02_status_22r,
        line2,
        blockStart + FT02_STATUS_TEXT_OFFSET_X,
        FT02_STATUS_TEXT_LINE2_Y
    );
}

void FT02_DrawStatusBarStorage(
    FT02Display& display,
    const char* line1,
    const char* line2
)
{
    FT02_SetStatusBarStorageCache(line1, line2);
    static const int partialX = 512;
    static const int partialY = 6;
    static const int partialW = 160;
    static const int partialH = 61;

    display.setPartialWindow(
        partialX,
        partialY,
        partialW,
        partialH
    );

    display.firstPage();

    do
    {
        display.fillRect(
            partialX,
            partialY,
            partialW,
            partialH,
            GxEPD_WHITE
        );

        display.drawLine(
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 2 - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 2 - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );

        display.drawLine(
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 3 - 10,
            FT02_STATUS_VLINE_TOP_Y,
            FT02_BLOCK_START_X + FT02_BLOCK_WIDTH * 3 - 10,
            FT02_STATUS_VLINE_BOTTOM_Y,
            GxEPD_BLACK
        );

        FT02_DrawStatusStorageBlockContent(
            display,
            line1,
            line2
        );
    }
    while(display.nextPage());

    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display, "status-storage-partial");
}

void FT02_DrawStatusBarClock(
    FT02Display& display,
    const char* hhmm,
    const char* mmdd
)
{
    FT02_SetStatusBarClockCache(hhmm, mmdd);

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
    FT02_EpdPowerOffAfterCommit(display, "status-clock-partial");
}
