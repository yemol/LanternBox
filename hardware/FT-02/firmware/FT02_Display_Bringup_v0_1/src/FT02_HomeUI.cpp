#include "FT02_HomeUI.h"
#include "FT02_EpdLifecycle.h"

#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_BottomIconData.h"
#include "FT02_HomeCards.h"
#include "FT02_HomeContent.h"
#include "FT02_StatusBar.h"

static const FT02BottomBarItem FT02_HOME_BOTTOM_ITEMS[3] = {
    {
        nullptr,
        "编号：FT-02A"
    },
    {
        nullptr,
        FT02_FIRMWARE_VERSION_LABEL
    },
    {
        &ICON_BOTTOM_HELP,
        "帮助(H)"
    }
};

void FT02_DrawHomeScreen(
    FT02Display& display,
    int selectedCardIndex
)
{
    display.setFullWindow();

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        FT02_DrawStatusBar(display);
        FT02_DrawHomeHeader(display);
        FT02_DrawHomeCardGrid(display, selectedCardIndex);

        FT02_DrawBottomBar(
            display,
            FT02_HOME_BOTTOM_ITEMS
        );
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display, "home-full");
}
