#include "FT02_HomeContent.h"

#include "FT02_FontData.h"
#include "FT02_TitleV133FontData.h"

void FT02_DrawHomeHeader(
    FT02Display& display
)
{
    int W = display.width();

    FT02_DrawTextPack(
        display,
        ft02_title_v133_40b,
        "壳中灯智能随身终端",
        40,
        140
    );

    FT02_DrawTextPack(
        display,
        ft02_ui_22r,
        "方向键选择 确认键进入",
        W - 310,
        140
    );
}
