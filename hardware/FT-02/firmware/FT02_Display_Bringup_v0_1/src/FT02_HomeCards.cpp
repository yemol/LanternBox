#include "FT02_HomeCards.h"
#include <string.h>
#include "FT02_EpdLifecycle.h"
#include "FT02_EpdInteractiveRuntime.h"

#include "FT02_FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_HomeCardIconData.h"

struct FT02HomeCard
{
    const char* label;
    const FT02Icon* icon;
    const FT02FontPack* font;
};

static const FT02FontIndexRecord FT02_HOME_LIBRARY_BOOK_28_IDX[] PROGMEM = {
    {0x4E66, 0}
};

static const uint8_t FT02_HOME_LIBRARY_BOOK_28_BIN[] PROGMEM = {
    0x1C, 0x1B, 0x04, 0x00, 0xE8, 0x1C, 0x00, 0x00, 0x00, 0x0C, 0x06, 0x00, 0x00, 0x0C, 0x06, 0x00,
    0x00, 0x0C, 0x0F, 0x00, 0x00, 0x0C, 0x07, 0x80, 0x00, 0x0C, 0x00, 0xE0, 0x0F, 0xFF, 0xFE, 0x40,
    0x0F, 0xFF, 0xFE, 0x00, 0x00, 0x0C, 0x0E, 0x00, 0x00, 0x0C, 0x0E, 0x00, 0x00, 0x0C, 0x0E, 0x00,
    0x00, 0x0C, 0x0E, 0x00, 0x00, 0x0C, 0x0E, 0x00, 0x00, 0x0C, 0x0E, 0x00, 0x3F, 0xFF, 0xFF, 0xE0,
    0x3F, 0xFF, 0xFF, 0xE0, 0x00, 0x0C, 0x00, 0x40, 0x00, 0x0C, 0x00, 0x40, 0x00, 0x0C, 0x00, 0xC0,
    0x00, 0x0C, 0x00, 0xC0, 0x00, 0x0C, 0x00, 0xC0, 0x00, 0x0C, 0x00, 0xC0, 0x00, 0x0C, 0x3F, 0xC0,
    0x00, 0x0C, 0x1F, 0x80, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

static const FT02FontPack FT02_HOME_LIBRARY_BOOK_28 = {
    FT02_HOME_LIBRARY_BOOK_28_IDX,
    1,
    FT02_HOME_LIBRARY_BOOK_28_BIN,
    sizeof(FT02_HOME_LIBRARY_BOOK_28_BIN)
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

static const int FT02_HOME_CARDS_PER_PAGE = 6;
static const int FT02_HOME_PAGE_INDICATOR_Y = 420;
static const int FT02_HOME_PAGE_DOT_RADIUS = 7;
static const int FT02_HOME_PAGE_DOT_GAP = 26;

static const FT02HomeCard FT02_HOME_CARDS[] = {
    {
        "知识库",
        &ICON_HOME_CARD_KNOWLEDGE,
        &ft02_menu_28m
    },
    {
        "地图导航",
        &ICON_HOME_CARD_MAP,
        &ft02_menu_28m
    },
    {
        "日志记录",
        &ICON_HOME_CARD_LOG,
        &ft02_menu_28m
    },
    {
        "定位记录",
        &ICON_HOME_CARD_LOCATION,
        &ft02_menu_28m
    },
    {
        "内部通讯",
        &ICON_HOME_CARD_NETWORK,
        &ft02_menu_28m
    },
    {
        "设备状态",
        &ICON_HOME_CARD_SYSTEM,
        &ft02_menu_28m
    },
    {
        "随身书库",
        &ICON_HOME_CARD_LIBRARY,
        &ft02_menu_28m
    },
    {
        "WiFi网络",
        &ICON_HOME_CARD_WIFI,
        &ft02_menu_28m
    },
    {
        "指南针",
        &ICON_HOME_CARD_COMPASS,
        &ft02_cjk_24b
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

    const int localIndex = cardIndex % FT02_HOME_CARDS_PER_PAGE;
    int row = localIndex / 3;
    int col = localIndex % 3;

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

static void FT02_DrawHomeWifiLabel(
    FT02Display& display,
    int x,
    int baselineY,
    bool selected
)
{
    const char* wifi = "WiFi";
    const char* network = "网络";

    if(selected) FT02_DrawTextPackInvert(display, ft02_menu_28m, wifi, x, baselineY);
    else FT02_DrawTextPack(display, ft02_menu_28m, wifi, x, baselineY);
    x += FT02_TextWidthPack(ft02_menu_28m, wifi) + 2;

    if(selected) FT02_DrawTextPackInvert(display, ft02_cjk_24b, network, x, baselineY - 2);
    else FT02_DrawTextPack(display, ft02_cjk_24b, network, x, baselineY - 2);
}

static void FT02_DrawHomeLibraryLabel(
    FT02Display& display,
    int x,
    int baselineY,
    bool selected
)
{
    const char* first = "随身";
    const char* middle = "书";
    const char* last = "库";

    if(selected) FT02_DrawTextPackInvert(display, ft02_menu_28m, first, x, baselineY);
    else FT02_DrawTextPack(display, ft02_menu_28m, first, x, baselineY);
    x += FT02_TextWidthPack(ft02_menu_28m, first);

    if(selected) FT02_DrawTextPackInvert(display, FT02_HOME_LIBRARY_BOOK_28, middle, x, baselineY);
    else FT02_DrawTextPack(display, FT02_HOME_LIBRARY_BOOK_28, middle, x, baselineY);
    x += FT02_TextWidthPack(FT02_HOME_LIBRARY_BOOK_28, middle);

    if(selected) FT02_DrawTextPackInvert(display, ft02_menu_28m, last, x, baselineY);
    else FT02_DrawTextPack(display, ft02_menu_28m, last, x, baselineY);
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

    if(strcmp(card.label, "随身书库") == 0)
    {
        FT02_DrawHomeLibraryLabel(display, textX, textBaselineY, selected);
    }
    else if(strcmp(card.label, "WiFi网络") == 0)
    {
        FT02_DrawHomeWifiLabel(display, textX, textBaselineY, selected);
    }
    else if(selected)
    {
        FT02_DrawTextPackInvert(
            display,
            *card.font,
            card.label,
            textX,
            textBaselineY
        );
    }
    else
    {
        FT02_DrawTextPack(
            display,
            *card.font,
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

    FT02_EpdInteractiveBeginBusyCapture(display);
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

        const int currentPage = FT02_HomeCardPageForIndex(newSelectedCardIndex);
        const int pageStart = currentPage * FT02_HOME_CARDS_PER_PAGE;
        const int pageEnd = FT02_MinInt(pageStart + FT02_HOME_CARDS_PER_PAGE, FT02_HOME_CARD_COUNT);
        for(int i = pageStart; i < pageEnd; i++)
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
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display);
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
    selectedCardIndex = FT02_ClampHomeCardIndex(selectedCardIndex);

    const int page = selectedCardIndex / FT02_HOME_CARDS_PER_PAGE;
    const int pageStart = page * FT02_HOME_CARDS_PER_PAGE;
    const int localIndex = selectedCardIndex - pageStart;
    const int row = localIndex / 3;
    const int col = localIndex % 3;

    if(deltaRow != 0)
    {
        int targetLocal = (row + deltaRow) * 3 + col;
        if(targetLocal >= 0 && targetLocal < FT02_HOME_CARDS_PER_PAGE)
        {
            const int candidate = pageStart + targetLocal;
            if(candidate >= 0 && candidate < FT02_HOME_CARD_COUNT) return candidate;
        }
        return selectedCardIndex;
    }

    if(deltaCol < 0)
    {
        if(col > 0) return selectedCardIndex - 1;
        if(page > 0)
        {
            const int previousPageStart = (page - 1) * FT02_HOME_CARDS_PER_PAGE;
            const int candidate = previousPageStart + row * 3 + 2;
            return FT02_ClampHomeCardIndex(candidate);
        }
        return selectedCardIndex;
    }

    if(deltaCol > 0)
    {
        if(col < 2)
        {
            const int candidate = selectedCardIndex + 1;
            if(candidate < FT02_HOME_CARD_COUNT &&
               candidate / FT02_HOME_CARDS_PER_PAGE == page) return candidate;
        }

        const int nextPageStart = (page + 1) * FT02_HOME_CARDS_PER_PAGE;
        if(nextPageStart < FT02_HOME_CARD_COUNT)
        {
            int candidate = nextPageStart + row * 3;
            if(candidate >= FT02_HOME_CARD_COUNT) candidate = nextPageStart;
            return candidate;
        }
        return selectedCardIndex;
    }

    return selectedCardIndex;
}

int FT02_HomeCardPageForIndex(
    int selectedCardIndex
)
{
    selectedCardIndex = FT02_ClampHomeCardIndex(selectedCardIndex);
    return selectedCardIndex / FT02_HOME_CARDS_PER_PAGE;
}

void FT02_DrawHomeCardGrid(
    FT02Display& display,
    int selectedCardIndex
)
{
    selectedCardIndex = FT02_ClampHomeCardIndex(
        selectedCardIndex
    );

    const int currentPage = FT02_HomeCardPageForIndex(selectedCardIndex);
    const int pageStart = currentPage * FT02_HOME_CARDS_PER_PAGE;
    const int pageEnd = FT02_MinInt(pageStart + FT02_HOME_CARDS_PER_PAGE, FT02_HOME_CARD_COUNT);

    for(int i = pageStart; i < pageEnd; i++)
    {
        FT02_DrawHomeCardByIndex(
            display,
            i,
            i == selectedCardIndex
        );
    }

    const int totalPages =
        (FT02_HOME_CARD_COUNT + FT02_HOME_CARDS_PER_PAGE - 1) / FT02_HOME_CARDS_PER_PAGE;
    FT02_DrawHomePageIndicator(
        display,
        currentPage,
        totalPages
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
