#include "FT02_HomeUI.h"
#include "FT02_EpdLifecycle.h"
#include "FT02_EpdInteractiveRuntime.h"

#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_ModeManager.h"
#include "FT02_BottomIconData.h"
#include "FT02_HomeCards.h"
#include "FT02_HomeContent.h"
#include "FT02_StatusBar.h"

static void FT02_DrawHomeBottomBar(FT02Display& display)
{
    const FT02BottomBarItem items[3] = {
        {nullptr, "编号：FT-02A"},
        {nullptr, FT02_ModeHomeFooterLabel()},
        {&ICON_BOTTOM_HELP, "帮助(H)"}
    };

    FT02_DrawBottomBarWithFont(display, items, ft02_cjk_20r);
}

void FT02_DrawHomeScreen(
    FT02Display& display,
    int selectedCardIndex
)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        FT02_DrawStatusBar(display);
        FT02_DrawHomeHeader(display);
        FT02_DrawHomeCardGrid(display, selectedCardIndex);

        FT02_DrawHomeBottomBar(display);
    }
    while(display.nextPage());

    FT02_EpdInteractivePowerOffNow(display);
    FT02_EpdInteractiveEndBusyCapture(display);
}
