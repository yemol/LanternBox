#include "FT02_SleepUI.h"

#include <stdio.h>

#include "FT02_BatteryMonitor.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_FontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_ModeManager.h"
#include "FT02_SleepArtworkData.h"

namespace
{
int centeredX(const FT02FontPack& font, const char* text, int width)
{
    const int textWidth = FT02_TextWidthPack(font, text);
    return (width - textWidth) / 2;
}

void drawThickLine(FT02Display& display, int x1, int y1, int x2, int y2, int thickness = 2)
{
    if(thickness <= 1)
    {
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        return;
    }

    const int radius = thickness / 2;
    for(int d = -radius; d <= radius; ++d)
        display.drawLine(x1, y1 + d, x2, y2 + d, GxEPD_BLACK);
}

void drawTechFrame(FT02Display& display)
{
    (void)display;
}

void drawPine(FT02Display& display, int x, int baseY, int height)
{
    if(height < 18) height = 18;
    const int trunkTop = baseY - height;
    display.drawLine(x, trunkTop + 5, x, baseY, GxEPD_BLACK);

    const int w1 = height / 4;
    const int w2 = height / 3;
    const int w3 = (height * 2) / 5;
    display.fillTriangle(x, trunkTop, x - w1, trunkTop + height / 3, x + w1, trunkTop + height / 3, GxEPD_BLACK);
    display.fillTriangle(x, trunkTop + height / 5, x - w2, trunkTop + (height * 2) / 3, x + w2, trunkTop + (height * 2) / 3, GxEPD_BLACK);
    display.fillTriangle(x, trunkTop + (height * 2) / 5, x - w3, baseY, x + w3, baseY, GxEPD_BLACK);
}

void drawMountainLandscape(FT02Display& display)
{
    // A calm 1-bit outdoor ridge. The center is intentionally left open for
    // the official LanternBox mark, so the artwork remains readable on e-paper.

    // Full-bleed horizon details; no framing border so the scene can own the screen.
    display.drawCircle(624, 138, 45, GxEPD_BLACK);
    display.drawCircle(624, 138, 46, GxEPD_BLACK);
    drawThickLine(display, 662, 132, 736, 132, 1);
    drawThickLine(display, 684, 146, 780, 146, 1);
    drawThickLine(display, 28, 156, 104, 156, 1);
    drawThickLine(display, 52, 144, 146, 144, 1);

    // Left main ridge.
    drawThickLine(display, 26, 270, 104, 226, 2);
    drawThickLine(display, 104, 226, 170, 243, 2);
    drawThickLine(display, 170, 243, 276, 150, 3);
    drawThickLine(display, 276, 150, 330, 232, 2);
    display.fillTriangle(170, 243, 276, 150, 240, 213, GxEPD_BLACK);
    display.fillTriangle(200, 227, 276, 150, 252, 204, GxEPD_BLACK);
    display.fillTriangle(104, 226, 146, 201, 163, 238, GxEPD_BLACK);

    // Secondary inner facets create the approved sharp alpine look without gray.
    drawThickLine(display, 200, 226, 242, 192, 1);
    drawThickLine(display, 242, 192, 230, 218, 1);
    drawThickLine(display, 170, 243, 204, 221, 1);

    // Right main ridge.
    drawThickLine(display, 470, 238, 530, 179, 3);
    drawThickLine(display, 530, 179, 572, 215, 2);
    drawThickLine(display, 572, 215, 616, 187, 3);
    drawThickLine(display, 616, 187, 678, 240, 2);
    drawThickLine(display, 678, 240, 770, 226, 2);
    display.fillTriangle(470, 238, 530, 179, 506, 226, GxEPD_BLACK);
    display.fillTriangle(572, 215, 616, 187, 596, 226, GxEPD_BLACK);

    // Far ridges.
    drawThickLine(display, 18, 282, 70, 252, 1);
    drawThickLine(display, 70, 252, 108, 264, 1);
    drawThickLine(display, 108, 264, 142, 246, 1);
    drawThickLine(display, 142, 246, 188, 267, 1);
    drawThickLine(display, 188, 267, 222, 254, 1);

    drawThickLine(display, 622, 265, 662, 246, 1);
    drawThickLine(display, 662, 246, 700, 260, 1);
    drawThickLine(display, 700, 260, 730, 240, 1);
    drawThickLine(display, 730, 240, 784, 268, 1);

    // Pine clusters anchor the lower edges and reinforce the field-terminal theme.
    drawPine(display, 34, 286, 38);
    drawPine(display, 56, 286, 30);
    drawPine(display, 78, 286, 45);
    drawPine(display, 100, 286, 27);
    drawPine(display, 122, 286, 35);

    drawPine(display, 680, 286, 28);
    drawPine(display, 706, 286, 43);
    drawPine(display, 730, 286, 31);
    drawPine(display, 756, 286, 39);
}

void drawLayerIcon(FT02Display& display, int x, int y)
{
    for(int layer = 0; layer < 3; ++layer)
    {
        const int yy = y + layer * 6;
        display.drawLine(x, yy, x + 12, yy + 6, GxEPD_BLACK);
        display.drawLine(x + 12, yy + 6, x + 24, yy, GxEPD_BLACK);
        display.drawLine(x + 24, yy, x + 12, yy - 6, GxEPD_BLACK);
        display.drawLine(x + 12, yy - 6, x, yy, GxEPD_BLACK);
    }
}

void drawBatteryIcon(FT02Display& display, int x, int y, bool valid, float percent)
{
    display.drawRect(x, y, 36, 20, GxEPD_BLACK);
    display.fillRect(x + 36, y + 6, 4, 8, GxEPD_BLACK);

    int fill = 0;
    if(valid)
    {
        if(percent < 0.0f) percent = 0.0f;
        if(percent > 100.0f) percent = 100.0f;
        fill = static_cast<int>((percent * 30.0f) / 100.0f + 0.5f);
    }
    if(fill > 0) display.fillRect(x + 3, y + 3, fill, 14, GxEPD_BLACK);
}
}

void FT02_DrawSleepScreen(FT02Display& display)
{
    const FT02BatterySnapshot battery = FT02_BatterySnapshotCurrent();

    char batteryText[40];
    if(battery.valid)
        snprintf(batteryText, sizeof(batteryText), "入眠电量：%.0f%%", static_cast<double>(battery.percent));
    else
        snprintf(batteryText, sizeof(batteryText), "入眠电量：--%%");

    char previousMode[48];
    snprintf(previousMode, sizeof(previousMode), "上次模式：%s", FT02_ModeLabel(FT02_ModeAwakeMode()));

    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        // Borderless full-screen sleep artwork.
        display.drawBitmap(
            (display.width() - FT02_SLEEP_BRAND_CN_W) / 2,
            18,
            FT02_SLEEP_BRAND_CN,
            FT02_SLEEP_BRAND_CN_W,
            FT02_SLEEP_BRAND_CN_H,
            GxEPD_BLACK
        );

        const char* brandEn = "L A N T E R N B O X";
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            brandEn,
            centeredX(ft02_cjk_20r, brandEn, display.width()),
            74
        );

        drawMountainLandscape(display);

        // Official LanternBox / 壳中灯 icon rendered as a true 1-bit mark.
        display.drawBitmap(
            (display.width() - FT02_SLEEP_LOGO_W) / 2,
            82,
            FT02_SLEEP_LOGO,
            FT02_SLEEP_LOGO_W,
            FT02_SLEEP_LOGO_H,
            GxEPD_BLACK
        );

        display.drawBitmap(
            (display.width() - FT02_SLEEP_TITLE_W) / 2,
            270,
            FT02_SLEEP_TITLE,
            FT02_SLEEP_TITLE_W,
            FT02_SLEEP_TITLE_H,
            GxEPD_BLACK
        );

        const char* wakeHint = "按任意键唤醒";
        const int wakeW = FT02_TextWidthPack(ft02_cjk_20r, wakeHint);
        const int wakeX = (display.width() - wakeW) / 2;
        drawThickLine(display, wakeX - 94, 352, wakeX - 18, 352, 1);
        drawThickLine(display, wakeX + wakeW + 18, 352, wakeX + wakeW + 94, 352, 1);
        FT02_DrawTextPack(display, ft02_cjk_20r, wakeHint, wakeX, 359);

        // Full-width status/footer bands keep the page readable without a border.
        drawThickLine(display, 20, 382, 780, 382, 1);
        drawLayerIcon(display, 34, 403);
        FT02_DrawTextPack(display, ft02_cjk_20r, previousMode, 70, 412);

        drawThickLine(display, 404, 390, 404, 420, 1);
        drawBatteryIcon(display, 458, 394, battery.valid, battery.percent);
        FT02_DrawTextPack(display, ft02_cjk_20r, batteryText, 512, 412);

        drawThickLine(display, 20, 432, 780, 432, 1);

        const char* footerLeft = "FT-02 FIELD TERMINAL";
        const char* footerRight = "KEYBOARD WAKE READY";
        FT02_DrawTextPack(display, ft02_cjk_20r, footerLeft, 28, 458);
        const int rightWidth = FT02_TextWidthPack(ft02_cjk_20r, footerRight);
        FT02_DrawTextPack(display, ft02_cjk_20r, footerRight, 772 - rightWidth, 458);
    }
    while(display.nextPage());

    // The e-paper retains the approved sleep composition without panel power.
    FT02_EpdInteractivePowerOffNow(display);
    FT02_EpdInteractiveEndBusyCapture(display);
}
