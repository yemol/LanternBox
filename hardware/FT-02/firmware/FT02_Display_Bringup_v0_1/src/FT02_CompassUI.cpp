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
#include "FT02_CompassCore.h"
#include "FT02_StatusBar.h"
#include "FT02_StatusFontData.h"

namespace
{
// Compass UI A4: approved left/right layout.
// Left: instrument-style rotating compass dial. Right: detailed heading/status data.
// Presentation only. Heading now comes directly from the Core-owned K12+ QMC5883L path.
constexpr int BODY_X = 32;
constexpr int BODY_Y = 136;
constexpr int BODY_W = 736;
constexpr int BODY_H = 286;

constexpr int CX = 230;
constexpr int CY = 278;
constexpr int R_OUT = 106;
constexpr int R_DEG = 130;
constexpr int R_CARDINAL = 68;
constexpr int R_DIAGONAL = 62;
constexpr int DIVIDER_X = 430;

constexpr uint32_t REFRESH_MIN_MS = 900u;
constexpr float HEADING_STEP_DEG = 3.0f;
constexpr uint16_t PARTIAL_CLEAN_LIMIT = 60u;
constexpr float RAD_PER_DEG = 0.01745329251994329577f;

static const FT02BottomBarItem BOTTOM_ITEMS[3] = {
    {nullptr, "ENTER 校准"},
    {nullptr, "磁航向"},
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

const char* qualityCn(const FT02CoreCompassSnapshot& compass)
{
    if(!compass.ready || !compass.valid) return "不可用";
    if(compass.quality >= 3) return "良好";
    if(compass.quality == 2) return "可用";
    if(compass.calibration.calibrated) return "较差";
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

void drawRotatingText(FT02Display& display,
                      const FT02FontPack& font,
                      const char* text,
                      float worldBearingDeg,
                      int radius,
                      float heading,
                      int baselineAdjust = 7)
{
    int x, y;
    // Standard rotating-compass relationship. This is the mirror-fix baseline and must not be reversed.
    polarPoint(worldBearingDeg - heading, radius, x, y);
    drawCentered(display, font, text, x, y + baselineAdjust);
}

void drawDegreeScale(FT02Display& display, float heading)
{
    const float h = normalizeHeading(heading);

    // Dense 5-degree ring, with longer marks every 30 degrees and strongest marks at N/E/S/W.
    for(int bearing = 0; bearing < 360; bearing += 5)
    {
        const bool cardinal = (bearing % 90) == 0;
        const bool major = (bearing % 30) == 0;
        const int r0 = cardinal ? R_OUT - 19 : (major ? R_OUT - 14 : R_OUT - 7);
        const float screenAngle = static_cast<float>(bearing) - h;
        int x0, y0, x1, y1;
        polarPoint(screenAngle, r0, x0, y0);
        polarPoint(screenAngle, R_OUT, x1, y1);
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
        if(cardinal)
        {
            display.drawLine(x0 + 1, y0, x1 + 1, y1, GxEPD_BLACK);
        }
    }

    // Degree labels remain upright while their positions rotate with the dial.
    for(int bearing = 0; bearing < 360; bearing += 30)
    {
        char label[8];
        snprintf(label, sizeof(label), "%d", bearing);
        drawRotatingText(display, ft02_ui_22r, label,
                         static_cast<float>(bearing), R_DEG, h, 7);
    }
}

void drawDirectionLabels(FT02Display& display, float heading)
{
    const float h = normalizeHeading(heading);

    drawRotatingText(display, ft02_cjk_24b, "北",    0.0f, R_CARDINAL, h, 8);
    drawRotatingText(display, ft02_cjk_20r, "东北", 45.0f, R_DIAGONAL, h, 7);
    drawRotatingText(display, ft02_cjk_24b, "东",   90.0f, R_CARDINAL, h, 8);
    drawRotatingText(display, ft02_cjk_20r, "东南",135.0f, R_DIAGONAL, h, 7);
    drawRotatingText(display, ft02_cjk_24b, "南",  180.0f, R_CARDINAL, h, 8);
    drawRotatingText(display, ft02_cjk_20r, "西南",225.0f, R_DIAGONAL, h, 7);
    drawRotatingText(display, ft02_cjk_24b, "西",  270.0f, R_CARDINAL, h, 8);
    drawRotatingText(display, ft02_cjk_20r, "西北",315.0f, R_DIAGONAL, h, 7);
}

void drawCenterReticle(FT02Display& display)
{
    constexpr int HALF_W = 45;
    constexpr int HALF_H = 43;
    display.drawLine(CX - HALF_W, CY, CX + HALF_W, CY, GxEPD_BLACK);
    display.drawLine(CX, CY - HALF_H, CX, CY + HALF_H, GxEPD_BLACK);
    display.drawCircle(CX, CY, 6, GxEPD_BLACK);
}

void drawFixedTopPointer(FT02Display& display)
{
    // Fixed top pointer. The compass dial rotates underneath it.
    const int ringTopY = CY - R_OUT;
    display.fillTriangle(CX, ringTopY + 8,
                         CX - 8, ringTopY - 8,
                         CX + 8, ringTopY - 8,
                         GxEPD_BLACK);
    display.fillRect(CX - 2, ringTopY + 10, 5, 10, GxEPD_BLACK);
}

void drawCompassFace(FT02Display& display, bool valid, float heading)
{
    const float faceHeading = valid ? normalizeHeading(heading) : 0.0f;
    drawDegreeScale(display, faceHeading);
    drawDirectionLabels(display, faceHeading);
    drawCenterReticle(display);
    drawFixedTopPointer(display);

    if(!valid)
    {
        display.drawLine(CX - 22, CY - 22, CX + 22, CY + 22, GxEPD_BLACK);
        display.drawLine(CX + 22, CY - 22, CX - 22, CY + 22, GxEPD_BLACK);
    }
}

void drawRightPanel(FT02Display& display, const FT02CoreCompassSnapshot& compass, bool valid, float heading)
{
    display.drawLine(DIVIDER_X, 155, DIVIDER_X, 411, GxEPD_BLACK);

    FT02_DrawTextPack(display, ft02_cjk_24b, "航向", 474, 184);
    if(valid)
    {
        char angle[16];
        snprintf(angle, sizeof(angle), "%03u", static_cast<unsigned>(lroundf(normalizeHeading(heading))) % 360u);
        FT02_DrawTextPack(display, ft02_title_32b, angle, 474, 230);

        // Manual degree symbol avoids relying on font coverage.
        const int angleW = FT02_TextWidthPack(ft02_title_32b, angle);
        const int degCx = 474 + angleW + 7;
        const int degCy = 202;
        display.drawCircle(degCx, degCy, 4, GxEPD_BLACK);
        display.drawCircle(degCx, degCy, 5, GxEPD_BLACK);

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
                      compass.ready ? "正常" : "不可用",
                      594, 316);

    FT02_DrawTextPack(display, ft02_cjk_20r, "校准", 474, 354);
    FT02_DrawTextPack(display, ft02_cjk_20r, qualityCn(compass), 594, 354);

    const char* hint = compass.calibration.calibrated
        ? "ENTER 可重新校准"
        : "建议先完成校准";
    FT02_DrawTextPack(display, ft02_cjk_20r, hint, 474, 401);
}

void drawBody(FT02Display& display)
{
    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    const bool valid = compass.ready && compass.valid;
    const float heading = normalizeHeading(static_cast<float>(compass.headingX10) / 10.0f);

    drawCompassFace(display, valid, heading);
    drawRightPanel(display, compass, valid, heading);
}

void snapshotDrawState()
{
    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    g_lastValid = compass.ready && compass.valid;
    g_lastHeading = static_cast<float>(compass.headingX10) / 10.0f;
    g_lastQuality = compass.quality;
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
    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    const bool valid = compass.ready && compass.valid;
    const float heading = static_cast<float>(compass.headingX10) / 10.0f;

    const bool stateChanged = valid != g_lastValid || compass.quality != g_lastQuality;
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
