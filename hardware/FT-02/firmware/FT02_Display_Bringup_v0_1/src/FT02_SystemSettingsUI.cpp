#include "FT02_SystemSettingsUI.h"

#include <stdio.h>

#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_FontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_StatusBar.h"
#include "FT02_SystemSettings.h"

namespace
{
uint8_t g_selectedRow = 0u;
constexpr uint8_t ROW_COUNT = 3u;

static const FT02BottomBarItem BOTTOM_ITEMS[3] = {
    {nullptr, "↑/↓ 选择"},
    {nullptr, "←/→ 修改"},
    {nullptr, "B 返回"}
};

const char* rowTitle(uint8_t row)
{
    switch(row)
    {
        case 0: return "室内自动休眠";
        case 1: return "开机模式";
        default: return "唤醒返回";
    }
}

const char* rowValue(uint8_t row)
{
    switch(row)
    {
        case 0: return FT02_SystemSettingsAutoSleepLabel();
        case 1: return FT02_SystemSettingsBootModeLabel();
        default: return FT02_SystemSettingsWakeReturnLabel();
    }
}

void changeSelected(int8_t direction)
{
    if(g_selectedRow == 0u) FT02_SystemSettingsCycleAutoSleep(direction);
    else if(g_selectedRow == 1u) FT02_SystemSettingsCycleBootMode(direction);
    else FT02_SystemSettingsCycleWakeReturn(direction);
}

void drawSettingRow(FT02Display& display, uint8_t row, int y)
{
    const bool selected = row == g_selectedRow;
    const int x = 42;
    const int w = display.width() - 84;
    const int h = 66;

    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackInvert(display, ft02_cjk_24b, rowTitle(row), x + 24, y + 42);
        const int valueW = FT02_TextWidthPack(ft02_cjk_24b, rowValue(row));
        FT02_DrawTextPackInvert(display, ft02_cjk_24b, rowValue(row), x + w - 24 - valueW, y + 42);
    }
    else
    {
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPack(display, ft02_cjk_24b, rowTitle(row), x + 24, y + 42);
        const int valueW = FT02_TextWidthPack(ft02_cjk_24b, rowValue(row));
        FT02_DrawTextPack(display, ft02_cjk_24b, rowValue(row), x + w - 24 - valueW, y + 42);
    }
}
}

void FT02_SystemSettingsUIOpen()
{
    g_selectedRow = 0u;
}

FT02SystemSettingsAction FT02_SystemSettingsUIHandleInput(const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK) return FT02_SETTINGS_ACTION_EXIT_HOME;
    if(event.key == FT02_KEY_HELP) return FT02_SETTINGS_ACTION_OPEN_HELP;

    if(event.key == FT02_KEY_UP)
    {
        g_selectedRow = g_selectedRow == 0u ? ROW_COUNT - 1u : g_selectedRow - 1u;
        return FT02_SETTINGS_ACTION_REDRAW;
    }
    if(event.key == FT02_KEY_DOWN)
    {
        g_selectedRow = static_cast<uint8_t>((g_selectedRow + 1u) % ROW_COUNT);
        return FT02_SETTINGS_ACTION_REDRAW;
    }
    if(event.key == FT02_KEY_LEFT)
    {
        changeSelected(-1);
        return FT02_SETTINGS_ACTION_REDRAW;
    }
    if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_SELECT)
    {
        changeSelected(1);
        return FT02_SETTINGS_ACTION_REDRAW;
    }
    return FT02_SETTINGS_ACTION_NONE;
}

void FT02_DrawSystemSettingsScreen(FT02Display& display)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);

        FT02_DrawTextPack(display, ft02_cjk_24b, "系统设置", 40, 120);
        display.fillRect(40, 142, display.width() - 80, 3, GxEPD_BLACK);

        drawSettingRow(display, 0u, 158);
        drawSettingRow(display, 1u, 232);
        drawSettingRow(display, 2u, 306);

        const char* wakeInfo = "唤醒：键盘已启用 · IMU后续支持";
        FT02_DrawTextPack(display, ft02_cjk_20r, wakeInfo, 48, 414);

        char versionLine[64];
        snprintf(versionLine, sizeof(versionLine), "FT-02A  %s", FT02_FIRMWARE_UI_VERSION);
        const int versionWidth = FT02_TextWidthPack(ft02_cjk_20r, versionLine);
        FT02_DrawTextPack(display, ft02_cjk_20r, versionLine, display.width() - 48 - versionWidth, 414);

        FT02_DrawBottomBarWithFont(display, BOTTOM_ITEMS, ft02_cjk_20r);
    }
    while(display.nextPage());

    FT02_EpdInteractivePowerOffNow(display);
    FT02_EpdInteractiveEndBusyCapture(display);
}
