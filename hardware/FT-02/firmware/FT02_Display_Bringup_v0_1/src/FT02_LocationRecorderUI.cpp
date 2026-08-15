#include "FT02_LocationRecorderUI.h"

#include <stdio.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_StatusBar.h"

namespace
{
static const FT02BottomBarItem FT02_RECORDER_BOTTOM_ITEMS[3] = {
    {nullptr, "ENTER 开始/停止"},
    {nullptr, "P 记点  A 自动"},
    {nullptr, "L列表 R重连 B返回"}
};

static const int FT02_RECORDER_PARTIAL_X = 24;
static const int FT02_RECORDER_PARTIAL_Y = 78;
static const int FT02_RECORDER_PARTIAL_W = 752;
static const int FT02_RECORDER_PARTIAL_H = 350;
static const uint16_t FT02_RECORDER_PARTIAL_LIMIT = 120;
static uint16_t g_ft02RecorderPartialCount = 0;

static void FT02_RecorderDrawLabelValue(
    FT02Display& display,
    const char* label,
    const char* value,
    int x,
    int baselineY,
    int valueX
)
{
    FT02_DrawTextPack(display, ft02_cjk_20r, label, x, baselineY);
    FT02_DrawTextPack(display, ft02_cjk_20r, value, valueX, baselineY);
}

static const char* FT02_RecorderGnssState(const FT02GnssSnapshot& gnss)
{
    if(gnss.communicationActive)
    {
        if(!gnss.fixValid) return "搜索中";
        if(gnss.fixType >= 3) return "3D定位";
        if(gnss.fixType == 2) return "2D定位";
        return "已定位";
    }
    if(gnss.nmeaSeen) return "通讯超时";
    if(gnss.serialDataSeen) return "数据异常";
    return "连接中";
}

static void FT02_RecorderFormatDuration(uint32_t seconds, char* output, size_t outputSize)
{
    const uint32_t hours = seconds / 3600UL;
    const uint32_t minutes = (seconds / 60UL) % 60UL;
    const uint32_t remain = seconds % 60UL;
    snprintf(
        output,
        outputSize,
        "%02lu:%02lu:%02lu",
        (unsigned long)hours,
        (unsigned long)minutes,
        (unsigned long)remain
    );
}

static void FT02_DrawRecorderContent(
    FT02Display& display,
    const FT02GnssSnapshot& gnss,
    const FT02LocationRecorderSnapshot& recorder
)
{
    FT02_DrawTextPack(display, ft02_cjk_24b, "定位记录", 32, 119);

    const char* hint = recorder.sessionActive
        ? (recorder.autoTrackEnabled ? "路径与自动轨迹记录中" : "路径记录中")
        : (gnss.communicationActive ? "GNSS 通讯正常" : "等待 GNSS 数据");
    const int hintWidth = FT02_TextWidthPack(ft02_cjk_20r, hint);
    FT02_DrawTextPack(display, ft02_cjk_20r, hint, display.width() - 32 - hintWidth, 117);

    display.drawRect(32, 140, 350, 270, GxEPD_BLACK);
    display.drawRect(398, 140, 370, 270, GxEPD_BLACK);

    FT02_DrawTextPack(display, ft02_cjk_24b, "记录状态", 52, 176);
    FT02_DrawTextPack(display, ft02_cjk_24b, "定位信息", 418, 176);

    char value[80];

    FT02_RecorderDrawLabelValue(
        display,
        "状态",
        recorder.sessionActive ? "记录中" : "未开始",
        52,
        210,
        122
    );

    const char* sessionText = recorder.sessionActive ? recorder.sessionId : "--";
    FT02_RecorderDrawLabelValue(display, "行程", sessionText, 52, 240, 122);

    FT02_RecorderFormatDuration(recorder.durationSeconds, value, sizeof(value));
    FT02_RecorderDrawLabelValue(display, "时长", value, 52, 270, 122);

    snprintf(
        value,
        sizeof(value),
        "%lu  失败 %lu",
        (unsigned long)recorder.pointCount,
        (unsigned long)recorder.failedPointCount
    );
    FT02_RecorderDrawLabelValue(display, "点数", value, 52, 300, 122);

    if(recorder.autoTrackEnabled)
    {
        if(gnss.fixValid)
        {
            snprintf(value, sizeof(value), "开启  下次%lus", (unsigned long)recorder.nextAutoSeconds);
        }
        else
        {
            snprintf(value, sizeof(value), "开启  等待定位");
        }
    }
    else
    {
        snprintf(value, sizeof(value), "关闭  间隔30s");
    }
    FT02_RecorderDrawLabelValue(display, "自动", value, 52, 330, 122);

    const char* originText = recorder.originRecorded
        ? "已记录"
        : (recorder.sessionActive ? "等待定位" : "--");
    FT02_RecorderDrawLabelValue(
        display,
        "起点",
        originText,
        52,
        360,
        122
    );

    FT02_DrawTextPack(display, ft02_cjk_20r, recorder.lastAction, 52, 394);

    FT02_RecorderDrawLabelValue(display, "GNSS", FT02_RecorderGnssState(gnss), 418, 210, 500);

    snprintf(value, sizeof(value), "%u", (unsigned int)gnss.satellites);
    FT02_RecorderDrawLabelValue(display, "卫星", value, 418, 240, 500);

    if(gnss.hdop > 0.0f) snprintf(value, sizeof(value), "%.1f", gnss.hdop);
    else snprintf(value, sizeof(value), "--");
    FT02_RecorderDrawLabelValue(display, "HDOP", value, 418, 270, 500);

    if(gnss.fixValid) snprintf(value, sizeof(value), "%.6f", gnss.latitude);
    else snprintf(value, sizeof(value), "--");
    FT02_RecorderDrawLabelValue(display, "纬度", value, 418, 300, 500);

    if(gnss.fixValid) snprintf(value, sizeof(value), "%.6f", gnss.longitude);
    else snprintf(value, sizeof(value), "--");
    FT02_RecorderDrawLabelValue(display, "经度", value, 418, 330, 500);

    if(gnss.fixValid && gnss.altitudeValid)
        snprintf(value, sizeof(value), "约%.0fm", gnss.altitudeMeters);
    else
        snprintf(value, sizeof(value), "--");
    FT02_RecorderDrawLabelValue(display, "海拔", value, 418, 360, 500);

    if(gnss.fixValid && gnss.speedValid)
    {
        if(gnss.courseValid)
            snprintf(value, sizeof(value), "%.1fkm/h  %.0f°", gnss.speedKmh, gnss.courseDegrees);
        else
            snprintf(value, sizeof(value), "%.1fkm/h  --", gnss.speedKmh);
    }
    else
    {
        snprintf(value, sizeof(value), "--");
    }
    FT02_RecorderDrawLabelValue(display, "速度", value, 418, 390, 500);
}
}

void FT02_DrawLocationRecorderScreen(
    FT02Display& display,
    const FT02GnssSnapshot& gnss,
    const FT02LocationRecorderSnapshot& recorder
)
{
    g_ft02RecorderPartialCount = 0;
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_DrawRecorderContent(display, gnss, recorder);
        FT02_DrawBottomBarWithFont(display, FT02_RECORDER_BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display, "location-recorder-full");
}

void FT02_DrawLocationRecorderMiddlePartial(
    FT02Display& display,
    const FT02GnssSnapshot& gnss,
    const FT02LocationRecorderSnapshot& recorder
)
{
    // Periodically perform one clean full refresh to prevent long-running
    // partial-update ghosting. At the normal 30-second recorder UI cadence this is
    // approximately once every hour.
    if(g_ft02RecorderPartialCount >= FT02_RECORDER_PARTIAL_LIMIT)
    {
        Serial.println("[RECORDER-UI] partial budget reached; clean full refresh");
        FT02_DrawLocationRecorderScreen(display, gnss, recorder);
        return;
    }

    display.setPartialWindow(
        FT02_RECORDER_PARTIAL_X,
        FT02_RECORDER_PARTIAL_Y,
        FT02_RECORDER_PARTIAL_W,
        FT02_RECORDER_PARTIAL_H
    );
    display.firstPage();

    do
    {
        display.fillRect(
            FT02_RECORDER_PARTIAL_X,
            FT02_RECORDER_PARTIAL_Y,
            FT02_RECORDER_PARTIAL_W,
            FT02_RECORDER_PARTIAL_H,
            GxEPD_WHITE
        );
        FT02_DrawRecorderContent(display, gnss, recorder);
    }
    while(display.nextPage());

    display.setFullWindow();
    g_ft02RecorderPartialCount++;
    FT02_EpdPowerOffAfterCommit(display, "location-recorder-middle-partial");
}
