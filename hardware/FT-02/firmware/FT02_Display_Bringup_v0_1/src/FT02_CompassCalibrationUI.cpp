#include "FT02_CompassCalibrationUI.h"

#include <stdio.h>
#include <string.h>

#include "FT02_BottomBar.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_CompassCore.h"
#include "FT02_StatusBar.h"

namespace
{
bool g_resetConfirm = false;
char g_notice[96] = {};

static const FT02BottomBarItem BOTTOM_NORMAL[3] = {
    {nullptr, "ENTER 开始/保存"},
    {nullptr, "DEL 取消 / R重置"},
    {nullptr, "B 返回"}
};

static const FT02BottomBarItem BOTTOM_RESET[3] = {
    {nullptr, "危险操作"},
    {nullptr, "ENTER 确认重置"},
    {nullptr, "B 取消"}
};

void setNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
}

const char* stateCn(FT02CoreCompassCalState state)
{
    switch(state)
    {
        case FT02_CORE_COMPASS_CAL_IDLE: return "空闲";
        case FT02_CORE_COMPASS_CAL_RUNNING: return "校准中";
        case FT02_CORE_COMPASS_CAL_READY: return "可保存";
        case FT02_CORE_COMPASS_CAL_SAVED: return "已保存";
        case FT02_CORE_COMPASS_CAL_CANCELED: return "已取消";
        case FT02_CORE_COMPASS_CAL_FAILED: return "失败";
        default: return "等待状态";
    }
}

const char* qualityCn(uint8_t q)
{
    switch(q)
    {
        case 3: return "高质量";
        case 2: return "可用";
        case 1: return "未校准/较差";
        default: return "无效";
    }
}

void drawHeader(FT02Display& display)
{
    FT02_DrawStatusBar(display);
    FT02_DrawTextPack(display, ft02_cjk_24b, "指南针  罗盘校准", 32, 116);
    display.drawLine(32, 132, 768, 132, GxEPD_BLACK);
}

void drawProgress(FT02Display& display, uint8_t progress)
{
    constexpr int x = 72;
    constexpr int y = 274;
    constexpr int w = 656;
    constexpr int h = 28;
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    const int fill = static_cast<int>((w - 4) * progress / 100U);
    if(fill > 0) display.fillRect(x + 2, y + 2, fill, h - 4, GxEPD_BLACK);

    char text[32];
    snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(progress));
    FT02_DrawTextPack(display, ft02_cjk_20r, text, 374, 328);
}

void drawResetConfirm(FT02Display& display)
{
    drawHeader(display);
    FT02_DrawTextPack(display, ft02_cjk_24b, "确认删除已保存的罗盘校准？", 200, 210);
    FT02_DrawTextPack(display, ft02_cjk_20r, "这会删除 Core NVS 中当前有效校准参数。", 178, 258);
    FT02_DrawTextPack(display, ft02_cjk_20r, "删除后 compass_q 将回到未校准状态，直到重新校准并保存。", 106, 296);
    FT02_DrawTextPack(display, ft02_cjk_20r, "ENTER 确认重置     B 取消", 244, 362);
    FT02_DrawBottomBarWithFont(display, BOTTOM_RESET, ft02_cjk_20r);
}
}

void FT02_CompassCalibrationUIOpen()
{
    g_resetConfirm = false;
    setNotice("");
    const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
    if(!compass.ready) setNotice("Core I2C罗盘未就绪");
}

uint32_t FT02_CompassCalibrationUIRevision()
{
    return FT02_CompassCoreSnapshotCurrent().calibration.revision;
}

void FT02_DrawCompassCalibrationScreen(FT02Display& display)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        if(g_resetConfirm)
        {
            drawResetConfirm(display);
            continue;
        }

        drawHeader(display);

        const FT02CoreCompassSnapshot compass = FT02_CompassCoreSnapshotCurrent();
        const FT02CoreCompassCalibrationState& cal = compass.calibration;

        char line[220];

        snprintf(line, sizeof(line), "Core I2C：%s    罗盘：%s    当前校准：%s",
                 compass.hardwareKnown ? "已初始化" : "未初始化",
                 compass.ready ? "正常" : "不可用",
                 cal.calibrated ? "已保存" : "未保存");
        FT02_DrawTextPack(display, ft02_cjk_20r, line, 48, 165);

        snprintf(line, sizeof(line), "状态：%s    质量：%s（%u）    样本：%lu",
                 stateCn(cal.state), qualityCn(cal.quality),
                 static_cast<unsigned>(cal.quality),
                 static_cast<unsigned long>(cal.samples));
        FT02_DrawTextPack(display, ft02_cjk_20r, line, 48, 201);

        if(cal.state == FT02_CORE_COMPASS_CAL_RUNNING)
            FT02_DrawTextPack(display, ft02_cjk_20r, "保持设备水平，缓慢旋转360°两到三圈；不要做8字、俯仰或滚转。", 148, 238);
        else if(cal.state == FT02_CORE_COMPASS_CAL_READY)
            FT02_DrawTextPack(display, ft02_cjk_20r, "覆盖已达标，按 ENTER 保存；若质量偏低，保持水平再补一圈360°。", 130, 238);
        else if(cal.calibrated)
            FT02_DrawTextPack(display, ft02_cjk_20r, "当前 heading 正在使用 Core 中已保存的校准参数。", 130, 238);
        else
            FT02_DrawTextPack(display, ft02_cjk_20r, "尚无已保存校准。按 ENTER 开始新的罗盘校准。", 138, 238);

        drawProgress(display, cal.progress);

        snprintf(line, sizeof(line), "X %ld..%ld   Y %ld..%ld   Z %ld..%ld",
                 static_cast<long>(cal.minX), static_cast<long>(cal.maxX),
                 static_cast<long>(cal.minY), static_cast<long>(cal.maxY),
                 static_cast<long>(cal.minZ), static_cast<long>(cal.maxZ));
        FT02_DrawTextPack(display, ft02_cjk_20r, line, 78, 365);

        if(cal.lastErrorCode != 0)
        {
            snprintf(line, sizeof(line), "错误 %ld：%s",
                     static_cast<long>(cal.lastErrorCode),
                     cal.lastErrorMessage);
            FT02_DrawTextPack(display, ft02_cjk_20r, line, 48, 405);
        }
        else if(g_notice[0])
        {
            FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 48, 405);
        }
        else
        {
            const char* action = cal.state == FT02_CORE_COMPASS_CAL_READY
                ? "ENTER 保存   DEL 取消   R 删除旧校准"
                : (cal.state == FT02_CORE_COMPASS_CAL_RUNNING
                    ? "校准进行中   DEL 取消   B可返回后台继续"
                    : "ENTER 开始   R 删除已保存校准");
            FT02_DrawTextPack(display, ft02_cjk_20r, action, 48, 405);
        }

        FT02_DrawBottomBarWithFont(display, BOTTOM_NORMAL, ft02_cjk_20r);
    }
    while(display.nextPage());

    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display);
}

FT02CompassCalibrationUIAction FT02_CompassCalibrationUIHandleInput(const FT02InputEvent& event)
{
    const uint8_t raw = static_cast<uint8_t>(event.raw);
    const FT02CoreCompassSnapshot compassNow = FT02_CompassCoreSnapshotCurrent();
    const FT02CoreCompassCalibrationState& cal = compassNow.calibration;

    if(g_resetConfirm)
    {
        if(event.key == FT02_KEY_SELECT)
        {
            const bool ok = FT02_CompassCoreCalReset();
            g_resetConfirm = false;
            setNotice(ok ? "已重置 Core 罗盘校准" : "重置失败");
            return FT02_COMPASS_CAL_UI_REDRAW;
        }
        if(event.key == FT02_KEY_BACK || event.command == 'b')
        {
            g_resetConfirm = false;
            setNotice("已取消重置");
            return FT02_COMPASS_CAL_UI_REDRAW;
        }
        return FT02_COMPASS_CAL_UI_NONE;
    }

    // CardKB2 DEL is normally mapped to BACK. On this page DEL means cancel
    // the active calibration session, while B/ESC remains page-back.
    if(raw == 0x08u || raw == 0x7Fu)
    {
        if(cal.state == FT02_CORE_COMPASS_CAL_RUNNING || cal.state == FT02_CORE_COMPASS_CAL_READY)
        {
            const bool ok = FT02_CompassCoreCalCancel();
            setNotice(ok ? "已取消校准" : "取消失败");
            return FT02_COMPASS_CAL_UI_REDRAW;
        }
        setNotice("当前没有进行中的校准");
        return FT02_COMPASS_CAL_UI_REDRAW;
    }

    if(event.key == FT02_KEY_BACK) return FT02_COMPASS_CAL_UI_EXIT;

    if(event.command == 'r')
    {
        g_resetConfirm = true;
        setNotice("");
        return FT02_COMPASS_CAL_UI_REDRAW;
    }

    if(event.key == FT02_KEY_SELECT)
    {
        bool ok = false;
        if(cal.state == FT02_CORE_COMPASS_CAL_READY)
        {
            ok = FT02_CompassCoreCalSave();
            setNotice(ok ? "已保存 Core 罗盘校准" : "保存失败");
        }
        else if(cal.state == FT02_CORE_COMPASS_CAL_RUNNING)
        {
            setNotice("校准仍在进行，请继续转动设备");
            ok = true;
        }
        else
        {
            ok = FT02_CompassCoreCalStart();
            setNotice(ok ? "已开始 Core 罗盘校准" : "开始校准失败");
        }
        return FT02_COMPASS_CAL_UI_REDRAW;
    }

    if(event.command == 's')
    {
        if(cal.state != FT02_CORE_COMPASS_CAL_READY)
        {
            setNotice("当前质量尚未达到可保存条件");
            return FT02_COMPASS_CAL_UI_REDRAW;
        }
        const bool ok = FT02_CompassCoreCalSave();
        setNotice(ok ? "已保存 Core 罗盘校准" : "保存失败");
        return FT02_COMPASS_CAL_UI_REDRAW;
    }

    return FT02_COMPASS_CAL_UI_NONE;
}
