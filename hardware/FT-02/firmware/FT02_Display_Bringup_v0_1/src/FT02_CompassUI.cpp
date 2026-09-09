#include "FT02_CompassUI.h"

#include <math.h>
#include <stdio.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_FontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_LR01HostRuntime.h"
#include "FT02_StatusBar.h"
#include "FT02_StatusFontData.h"

namespace
{
constexpr int BODY_X = 32;
constexpr int BODY_Y = 136;
constexpr int BODY_W = 736;
constexpr int BODY_H = 286;
constexpr int CX = 250;
constexpr int CY = 278;
constexpr int R_OUT = 126;
constexpr int R_IN = 118;
constexpr uint32_t REFRESH_MIN_MS = 900u;
constexpr float HEADING_STEP_DEG = 3.0f;
constexpr uint16_t PARTIAL_CLEAN_LIMIT = 60u;
constexpr float RAD_PER_DEG = 0.01745329251994329577f;

static const FT02BottomBarItem BOTTOM_ITEMS[3] = {
    {nullptr, "ENTER 校准"},
    {nullptr, "磁北航向"},
    {nullptr, "H帮助  B返回"}
};

uint32_t g_lastRefreshMs = 0;
float g_lastHeading = -1000.0f;
bool g_lastValid = false;
uint8_t g_lastQuality = 0xFFu;
uint16_t g_partialCount = 0;

float normalizeHeading(float h)
{
    while(h < 0.0f) h += 360.0f;
    while(h >= 360.0f) h -= 360.0f;
    return h;
}

float headingDelta(float a, float b)
{
    float d = fabsf(normalizeHeading(a) - normalizeHeading(b));
    return d > 180.0f ? 360.0f - d : d;
}

const char* directionCn(float heading)
{
    const int sector = static_cast<int>((normalizeHeading(heading) + 22.5f) / 45.0f) & 7;
    static const char* DIRS[8] = {
        "北", "东北", "东", "东南", "南", "西南", "西", "西北"
    };
    return DIRS[sector];
}

const char* qualityCn(const FT02LR01State& lr01)
{
    if(!lr01.compassReady || !lr01.compassValid) return "不可用";
    if(lr01.compassQuality >= 3) return "良好";
    if(lr01.compassQuality == 2) return "可用";
    if(lr01.compassCalibration.calibrated) return "较差";
    return "需校准";
}

void polarPoint(float angleDeg, int radius, int& x, int& y)
{
    const float rad = (angleDeg - 90.0f) * RAD_PER_DEG;
    x = CX + static_cast<int>(lroundf(cosf(rad) * radius));
    y = CY + static_cast<int>(lroundf(sinf(rad) * radius));
}

void drawCentered(FT02Display& display, const FT02FontPack& font, const char* text, int centerX, int baselineY)
{
    const int w = FT02_TextWidthPack(font, text);
    FT02_DrawTextPack(display, font, text, centerX - w / 2, baselineY);
}

void drawDialLabel(FT02Display& display, const char* text, float bearingDeg, int radius, float heading)
{
    int x, y;
    polarPoint(heading - bearingDeg, radius, x, y);
    drawCentered(display, ft02_status_22r, text, x, y + 8);
}

void drawCompassFace(FT02Display& display, float heading)
{
    const float h = normalizeHeading(heading);

    display.drawCircle(CX, CY, R_OUT, GxEPD_BLACK);
    display.drawCircle(CX, CY, R_OUT - 1, GxEPD_BLACK);
    display.drawCircle(CX, CY, R_IN, GxEPD_BLACK);

    // The device-forward pointer stays fixed at 12 o'clock. Rotate the dial
    // in the display direction verified on FT-02 so the current heading stays under the fixed pointer.
    for(int bearing = 0; bearing < 360; bearing += 5)
    {
        const bool cardinal = (bearing % 90) == 0;
        const bool major = (bearing % 30) == 0;
        const int r0 = cardinal ? R_OUT - 18 : (major ? R_OUT - 13 : R_OUT - 7);
        const float screenAngle = h - static_cast<float>(bearing);
        int x0, y0, x1, y1;
        polarPoint(screenAngle, r0, x0, y0);
        polarPoint(screenAngle, R_OUT - 3, x1, y1);
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
    }

    // Labels follow the corrected rotating dial while remaining upright for readability.
    drawDialLabel(display, "N",   0.0f, 96, h);
    drawDialLabel(display, "NE", 45.0f, 90, h);
    drawDialLabel(display, "E",  90.0f, 96, h);
    drawDialLabel(display, "SE",135.0f, 90, h);
    drawDialLabel(display, "S", 180.0f, 96, h);
    drawDialLabel(display, "SW",225.0f, 90, h);
    drawDialLabel(display, "W", 270.0f, 96, h);
    drawDialLabel(display, "NW",315.0f, 90, h);
}

void drawFixedHeadingPointer(FT02Display& display)
{
    constexpr float FORWARD_DEG = 0.0f;
    int tipX, tipY, tailX, tailY;
    polarPoint(FORWARD_DEG, 75, tipX, tipY);
    polarPoint(FORWARD_DEG + 180.0f, 42, tailX, tailY);

    display.drawLine(CX, CY, tipX, tipY, GxEPD_BLACK);
    display.drawLine(CX + 1, CY, tipX + 1, tipY, GxEPD_BLACK);
    display.drawLine(CX, CY + 1, tipX, tipY + 1, GxEPD_BLACK);
    display.drawLine(CX, CY, tailX, tailY, GxEPD_BLACK);

    int leftX, leftY, rightX, rightY;
    polarPoint(FORWARD_DEG - 7.0f, 60, leftX, leftY);
    polarPoint(FORWARD_DEG + 7.0f, 60, rightX, rightY);
    display.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, GxEPD_BLACK);
    display.fillCircle(CX, CY, 5, GxEPD_BLACK);
    display.drawCircle(CX, CY, 8, GxEPD_BLACK);
}

void drawBody(FT02Display& display)
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    const bool valid = FT02_LR01HostOnline() && lr01.compassReady && lr01.compassValid;
    const float heading = normalizeHeading(static_cast<float>(lr01.headingX10) / 10.0f);

    drawCompassFace(display, valid ? heading : 0.0f);
    if(valid) drawFixedHeadingPointer(display);
    else
    {
        display.drawLine(CX - 28, CY - 28, CX + 28, CY + 28, GxEPD_BLACK);
        display.drawLine(CX + 28, CY - 28, CX - 28, CY + 28, GxEPD_BLACK);
    }

    display.drawLine(430, 155, 430, 411, GxEPD_BLACK);

    FT02_DrawTextPack(display, ft02_cjk_24b, "航向", 474, 184);
    if(valid)
    {
        char angle[16];
        snprintf(angle, sizeof(angle), "%03u", static_cast<unsigned>(lroundf(heading)) % 360u);
        FT02_DrawTextPack(display, ft02_title_32b, angle, 474, 230);
        const int angleW = FT02_TextWidthPack(ft02_title_32b, angle);
        FT02_DrawTextPack(display, ft02_cjk_20r, "°", 474 + angleW + 4, 220);
        FT02_DrawTextPack(display, ft02_cjk_24b, directionCn(heading), 628, 226);
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_24b, "--", 474, 230);
        FT02_DrawTextPack(display, ft02_cjk_20r, "等待罗盘数据", 558, 226);
    }

    FT02_DrawTextPack(display, ft02_cjk_20r, "基准", 474, 278);
    FT02_DrawTextPack(display, ft02_cjk_20r, "磁北", 594, 278);

    FT02_DrawTextPack(display, ft02_cjk_20r, "状态", 474, 316);
    FT02_DrawTextPack(display, ft02_cjk_20r,
                      FT02_LR01HostOnline() ? (lr01.compassReady ? "正常" : "不可用") : "LR01离线",
                      594, 316);

    FT02_DrawTextPack(display, ft02_cjk_20r, "校准", 474, 354);
    FT02_DrawTextPack(display, ft02_cjk_20r, qualityCn(lr01), 594, 354);

    const char* hint = lr01.compassCalibration.calibrated
        ? "ENTER 可重新校准"
        : "建议先完成校准";
    FT02_DrawTextPack(display, ft02_cjk_20r, hint, 474, 401);
}

void snapshotDrawState()
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    g_lastValid = FT02_LR01HostOnline() && lr01.compassReady && lr01.compassValid;
    g_lastHeading = static_cast<float>(lr01.headingX10) / 10.0f;
    g_lastQuality = lr01.compassQuality;
    g_lastRefreshMs = millis();
}
}

void FT02_CompassUIOpen()
{
    g_lastRefreshMs = 0;
    g_lastHeading = -1000.0f;
    g_lastValid = false;
    g_lastQuality = 0xFFu;
    g_partialCount = 0;
    (void)FT02_LR01HostCompassCalRequestStatus();
}

void FT02_DrawCompassScreen(FT02Display& display)
{
    g_partialCount = 0;
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawTextPack(display, ft02_cjk_24b, "指南针", 32, 116);
        const char* sub = "电子罗盘  磁北";
        const int subW = FT02_TextWidthPack(ft02_cjk_20r, sub);
        FT02_DrawTextPack(display, ft02_cjk_20r, sub, display.width() - 32 - subW, 116);
        display.drawLine(32, 132, 768, 132, GxEPD_BLACK);
        drawBody(display);
        FT02_DrawBottomBarWithFont(display, BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractivePowerOffNow(display);
    snapshotDrawState();
}

void FT02_CompassUIService(FT02Display& display, uint32_t nowMs)
{
    const FT02LR01State& lr01 = FT02_LR01HostState();
    const bool valid = FT02_LR01HostOnline() && lr01.compassReady && lr01.compassValid;
    const float heading = static_cast<float>(lr01.headingX10) / 10.0f;

    const bool stateChanged = valid != g_lastValid || lr01.compassQuality != g_lastQuality;
    const bool headingChanged = valid && (g_lastHeading < -999.0f || headingDelta(heading, g_lastHeading) >= HEADING_STEP_DEG);
    if(!stateChanged && !headingChanged) return;
    if(g_lastRefreshMs != 0u && static_cast<uint32_t>(nowMs - g_lastRefreshMs) < REFRESH_MIN_MS) return;

    if(g_partialCount >= PARTIAL_CLEAN_LIMIT)
    {
        FT02_DrawCompassScreen(display);
        return;
    }

    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(BODY_X, BODY_Y, BODY_W, BODY_H);
    display.firstPage();
    do
    {
        display.fillRect(BODY_X, BODY_Y, BODY_W, BODY_H, GxEPD_WHITE);
        drawBody(display);
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    ++g_partialCount;
    snapshotDrawState();
    FT02_EpdInteractiveFinishPartial(display);
}
