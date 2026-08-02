#include "FT02_HomeCards.h"
#include "FT02_EpdLifecycle.h"

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
        "定位记录",
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

static void FT02_GetHomeCardRect(
    int cardIndex,
    int* x,
    int* y,
    int* w,
    int* h
)
{
    cardIndex = FT02_ClampHomeCardIndex(
        cardIndex
    );

    int row = cardIndex / 3;
    int col = cardIndex % 3;

    *x = FT02_HOME_CARD_X
        + col * (FT02_HOME_CARD_W + FT02_HOME_CARD_GAP_X);

    *y = FT02_HOME_CARD_Y
        + row * (FT02_HOME_CARD_H + FT02_HOME_CARD_GAP_Y);

    *w = FT02_HOME_CARD_W;
    *h = FT02_HOME_CARD_H;
}

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
        display.fillRect(
            x,
            y,
            FT02_HOME_CARD_W,
            FT02_HOME_CARD_H,
            GxEPD_WHITE
        );

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

static void FT02_DrawHomeCardByIndex(
    FT02Display& display,
    int cardIndex,
    bool selected
)
{
    cardIndex = FT02_ClampHomeCardIndex(
        cardIndex
    );

    int x;
    int y;
    int w;
    int h;

    FT02_GetHomeCardRect(
        cardIndex,
        &x,
        &y,
        &w,
        &h
    );

    FT02_DrawHomeCard(
        display,
        x,
        y,
        FT02_HOME_CARDS[cardIndex],
        selected
    );
}

static bool FT02_RectsIntersect(
    int ax,
    int ay,
    int aw,
    int ah,
    int bx,
    int by,
    int bw,
    int bh
)
{
    if(ax + aw <= bx) return false;
    if(bx + bw <= ax) return false;
    if(ay + ah <= by) return false;
    if(by + bh <= ay) return false;

    return true;
}

static int FT02_MinInt(
    int a,
    int b
)
{
    return a < b ? a : b;
}

static int FT02_MaxInt(
    int a,
    int b
)
{
    return a > b ? a : b;
}

static void FT02_RedrawHomeCardSelectionPartial(
    FT02Display& display,
    int oldSelectedCardIndex,
    int newSelectedCardIndex
)
{
    int oldX;
    int oldY;
    int oldW;
    int oldH;

    int newX;
    int newY;
    int newW;
    int newH;

    FT02_GetHomeCardRect(
        oldSelectedCardIndex,
        &oldX,
        &oldY,
        &oldW,
        &oldH
    );

    FT02_GetHomeCardRect(
        newSelectedCardIndex,
        &newX,
        &newY,
        &newW,
        &newH
    );

    int unionX = FT02_MinInt(
        oldX,
        newX
    );

    int unionY = FT02_MinInt(
        oldY,
        newY
    );

    int unionRight = FT02_MaxInt(
        oldX + oldW,
        newX + newW
    );

    int unionBottom = FT02_MaxInt(
        oldY + oldH,
        newY + newH
    );

    int partialX = unionX & ~7;
    int partialY = unionY;
    int partialRight = (unionRight + 7) & ~7;
    int partialW = partialRight - partialX;
    int partialH = unionBottom - partialY;

    display.setPartialWindow(
        partialX,
        partialY,
        partialW,
        partialH
    );

    display.firstPage();

    do
    {
        display.fillRect(
            partialX,
            partialY,
            partialW,
            partialH,
            GxEPD_WHITE
        );

        for(int i = 0; i < FT02_HOME_CARD_COUNT; i++)
        {
            int cardX;
            int cardY;
            int cardW;
            int cardH;

            FT02_GetHomeCardRect(
                i,
                &cardX,
                &cardY,
                &cardW,
                &cardH
            );

            if(FT02_RectsIntersect(
                cardX,
                cardY,
                cardW,
                cardH,
                partialX,
                partialY,
                partialW,
                partialH
            ))
            {
                FT02_DrawHomeCardByIndex(
                    display,
                    i,
                    i == newSelectedCardIndex
                );
            }
        }
    }
    while(display.nextPage());

    FT02_EpdPowerOffAfterCommit(display, "home-selection-partial");
}

int FT02_HomeCardCount()
{
    return FT02_HOME_CARD_COUNT;
}

int FT02_ClampHomeCardIndex(
    int selectedCardIndex
)
{
    if(selectedCardIndex < 0)
    {
        return 0;
    }

    if(selectedCardIndex >= FT02_HOME_CARD_COUNT)
    {
        return FT02_HOME_CARD_COUNT - 1;
    }

    return selectedCardIndex;
}

int FT02_MoveHomeCardSelection(
    int selectedCardIndex,
    int deltaRow,
    int deltaCol
)
{
    selectedCardIndex = FT02_ClampHomeCardIndex(
        selectedCardIndex
    );

    int row = selectedCardIndex / 3;
    int col = selectedCardIndex % 3;

    int nextRow = row + deltaRow;
    int nextCol = col + deltaCol;

    if(nextRow < 0) nextRow = 0;
    if(nextRow > 1) nextRow = 1;

    if(nextCol < 0) nextCol = 0;
    if(nextCol > 2) nextCol = 2;

    int nextIndex = nextRow * 3 + nextCol;

    return FT02_ClampHomeCardIndex(
        nextIndex
    );
}

void FT02_DrawHomeCardGrid(
    FT02Display& display,
    int selectedCardIndex
)
{
    selectedCardIndex = FT02_ClampHomeCardIndex(
        selectedCardIndex
    );

    for(int i = 0; i < FT02_HOME_CARD_COUNT; i++)
    {
        FT02_DrawHomeCardByIndex(
            display,
            i,
            i == selectedCardIndex
        );
    }

    FT02_DrawHomePageIndicator(
        display,
        FT02_HOME_CURRENT_PAGE,
        FT02_HOME_TOTAL_PAGES
    );
}

void FT02_RedrawHomeCardSelection(
    FT02Display& display,
    int oldSelectedCardIndex,
    int newSelectedCardIndex
)
{
    oldSelectedCardIndex = FT02_ClampHomeCardIndex(
        oldSelectedCardIndex
    );

    newSelectedCardIndex = FT02_ClampHomeCardIndex(
        newSelectedCardIndex
    );

    if(oldSelectedCardIndex == newSelectedCardIndex)
    {
        return;
    }

    FT02_RedrawHomeCardSelectionPartial(
        display,
        oldSelectedCardIndex,
        newSelectedCardIndex
    );

    display.setFullWindow();
}
