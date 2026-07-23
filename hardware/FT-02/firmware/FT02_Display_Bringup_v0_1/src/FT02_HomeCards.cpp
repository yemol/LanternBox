#include "FT02_HomeCards.h"

#include "FT02_FontData.h"
#include "FT02_HomeCardIconData.h"

struct FT02HomeCard
{
    const char* label;
    const FT02Icon* icon;
};

static const int FT02_HOME_CARD_X = 32;
static const int FT02_HOME_CARD_Y = 170;
static const int FT02_HOME_CARD_W = 230;
static const int FT02_HOME_CARD_H = 100;
static const int FT02_HOME_CARD_GAP_X = 24;
static const int FT02_HOME_CARD_GAP_Y = 24;

static const int FT02_HOME_CARD_ICON_OFFSET_X = 26;
static const int FT02_HOME_CARD_ICON_OFFSET_Y = 22;

static const int FT02_HOME_CARD_TEXT_OFFSET_X = 90;
static const int FT02_HOME_CARD_TEXT_BASELINE_OFFSET_Y = 60;

static const int FT02_HOME_SELECTED_CARD_INDEX = 0;

static const int FT02_HOME_CURRENT_PAGE = 0;
static const int FT02_HOME_TOTAL_PAGES = 2;
static const int FT02_HOME_PAGE_INDICATOR_Y = 420;
static const int FT02_HOME_PAGE_DOT_RADIUS = 7;
static const int FT02_HOME_PAGE_DOT_GAP = 26;

static const FT02HomeCard FT02_HOME_CARDS[] = {
    {
        "知识库",
        &ICON_HOME_CARD_KNOWLEDGE
    },
    {
        "地图导航",
        &ICON_HOME_CARD_MAP
    },
    {
        "日志记录",
        &ICON_HOME_CARD_LOG
    },
    {
        "定位搜索",
        &ICON_HOME_CARD_LOCATION
    },
    {
        "设备状态",
        &ICON_HOME_CARD_SYSTEM
    },
    {
        "通信管理",
        &ICON_HOME_CARD_NETWORK
    }
};

static const int FT02_HOME_CARD_COUNT =
    sizeof(FT02_HOME_CARDS) / sizeof(FT02_HOME_CARDS[0]);

static void FT02_DrawHomeCardBitmapIcon(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    bool selected
)
{
    uint16_t inkColor = selected ? GxEPD_WHITE : GxEPD_BLACK;

    for(int row = 0; row < icon.height; row++)
    {
        for(int col = 0; col < icon.width; col++)
        {
            int byteIndex = row * icon.bytesPerRow + col / 8;
            uint8_t mask = 0x80 >> (col % 8);
            bool on = (icon.bitmap[byteIndex] & mask) != 0;

            if(on)
            {
                display.drawPixel(
                    x + col,
                    y + row,
                    inkColor
                );
            }
        }
    }
}

static void FT02_DrawHomePageIndicator(
    FT02Display& display,
    int currentPage,
    int totalPages
)
{
    if(totalPages <= 1)
    {
        return;
    }

    int centerX = display.width() / 2;
    int totalWidth = (totalPages - 1) * FT02_HOME_PAGE_DOT_GAP;
    int startX = centerX - totalWidth / 2;

    for(int i = 0; i < totalPages; i++)
    {
        int dotX = startX + i * FT02_HOME_PAGE_DOT_GAP;

        if(i == currentPage)
        {
            display.fillCircle(
                dotX,
                FT02_HOME_PAGE_INDICATOR_Y,
                FT02_HOME_PAGE_DOT_RADIUS,
                GxEPD_BLACK
            );
        }
        else
        {
            display.drawCircle(
                dotX,
                FT02_HOME_PAGE_INDICATOR_Y,
                FT02_HOME_PAGE_DOT_RADIUS,
                GxEPD_BLACK
            );
        }
    }
}

static void FT02_DrawHomeCard(
    FT02Display& display,
    int x,
    int y,
    const FT02HomeCard& card,
    bool selected
)
{
    if(selected)
    {
        display.fillRect(
            x,
            y,
            FT02_HOME_CARD_W,
            FT02_HOME_CARD_H,
            GxEPD_BLACK
        );
    }
    else
    {
        display.drawRect(
            x,
            y,
            FT02_HOME_CARD_W,
            FT02_HOME_CARD_H,
            GxEPD_BLACK
        );

        display.drawRect(
            x + 2,
            y + 2,
            FT02_HOME_CARD_W - 4,
            FT02_HOME_CARD_H - 4,
            GxEPD_BLACK
        );
    }

    int iconX = x + FT02_HOME_CARD_ICON_OFFSET_X;
    int iconY = y + FT02_HOME_CARD_ICON_OFFSET_Y;

    FT02_DrawHomeCardBitmapIcon(
        display,
        *card.icon,
        iconX,
        iconY,
        selected
    );

    int textX = x + FT02_HOME_CARD_TEXT_OFFSET_X;
    int textBaselineY = y + FT02_HOME_CARD_TEXT_BASELINE_OFFSET_Y;

    if(selected)
    {
        FT02_DrawTextPackInvert(
            display,
            ft02_menu_28m,
            card.label,
            textX,
            textBaselineY
        );
    }
    else
    {
        FT02_DrawTextPack(
            display,
            ft02_menu_28m,
            card.label,
            textX,
            textBaselineY
        );
    }
}

void FT02_DrawHomeCardGrid(
    FT02Display& display
)
{
    for(int i = 0; i < FT02_HOME_CARD_COUNT; i++)
    {
        int row = i / 3;
        int col = i % 3;

        int x = FT02_HOME_CARD_X
            + col * (FT02_HOME_CARD_W + FT02_HOME_CARD_GAP_X);

        int y = FT02_HOME_CARD_Y
            + row * (FT02_HOME_CARD_H + FT02_HOME_CARD_GAP_Y);

        FT02_DrawHomeCard(
            display,
            x,
            y,
            FT02_HOME_CARDS[i],
            i == FT02_HOME_SELECTED_CARD_INDEX
        );
    }

    FT02_DrawHomePageIndicator(
        display,
        FT02_HOME_CURRENT_PAGE,
        FT02_HOME_TOTAL_PAGES
    );
}
