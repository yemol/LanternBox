#include "FT02_KnowledgeUI.h"
#include "FT02_EpdLifecycle.h"

#include <Arduino.h>
#include <string.h>

#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_FieldManual.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_KnowledgeHomeIconData.h"
#include "FT02_StatusBar.h"

namespace
{

enum FT02KnowledgeScreen
{
    FT02_KNOWLEDGE_SCREEN_HOME = 0,
    FT02_KNOWLEDGE_SCREEN_CATEGORIES,
    FT02_KNOWLEDGE_SCREEN_LIST,
    FT02_KNOWLEDGE_SCREEN_READER,
    FT02_KNOWLEDGE_SCREEN_PLACEHOLDER,
    FT02_KNOWLEDGE_SCREEN_ERROR
};

enum FT02KnowledgeIcon
{
    FT02_KNOWLEDGE_ICON_EMERGENCY = 0,
    FT02_KNOWLEDGE_ICON_CATEGORY,
    FT02_KNOWLEDGE_ICON_SEARCH,
    FT02_KNOWLEDGE_ICON_FAVORITE,
    FT02_KNOWLEDGE_ICON_RECENT,
    FT02_KNOWLEDGE_ICON_MANUAL,
    FT02_KNOWLEDGE_ICON_MEDICAL,
    FT02_KNOWLEDGE_ICON_WATER,
    FT02_KNOWLEDGE_ICON_FOOD,
    FT02_KNOWLEDGE_ICON_FIRE,
    FT02_KNOWLEDGE_ICON_TOOLS,
    FT02_KNOWLEDGE_ICON_ENERGY,
    FT02_KNOWLEDGE_ICON_NAVIGATION,
    FT02_KNOWLEDGE_ICON_SHELTER,
    FT02_KNOWLEDGE_ICON_ORGANIZATION
};

struct FT02KnowledgeHomeCard
{
    const char* title;
    const char* subtitle;
    FT02KnowledgeIcon icon;
};

static const int FT02_KNOWLEDGE_HEADER_BASELINE = 121;
static const int FT02_KNOWLEDGE_HEADER_X = 32;
static const int FT02_KNOWLEDGE_RIGHT_MARGIN = 32;

static const int FT02_KNOWLEDGE_GRID_X = 32;
static const int FT02_KNOWLEDGE_GRID_Y = 142;
static const int FT02_KNOWLEDGE_GRID_W = 356;
static const int FT02_KNOWLEDGE_GRID_H = 82;
static const int FT02_KNOWLEDGE_GRID_GAP_X = 24;
static const int FT02_KNOWLEDGE_GRID_GAP_Y = 12;
static const int FT02_KNOWLEDGE_GRID_COLS = 2;
static const int FT02_KNOWLEDGE_GRID_PAGE_SIZE = 6;

static const int FT02_KNOWLEDGE_LIST_X = 32;
static const int FT02_KNOWLEDGE_LIST_Y = 136;
static const int FT02_KNOWLEDGE_LIST_W = 736;
static const int FT02_KNOWLEDGE_LIST_H = 68;
static const int FT02_KNOWLEDGE_LIST_GAP_Y = 6;
static const int FT02_KNOWLEDGE_LIST_PAGE_SIZE = 4;

static const int FT02_KNOWLEDGE_READER_X = 32;
static const int FT02_KNOWLEDGE_READER_Y = 142;
static const int FT02_KNOWLEDGE_READER_W = 736;
static const int FT02_KNOWLEDGE_READER_H = 276;
static const int FT02_KNOWLEDGE_READER_INSET_X = 24;
static const int FT02_KNOWLEDGE_READER_TEXT_W =
    FT02_KNOWLEDGE_READER_W - FT02_KNOWLEDGE_READER_INSET_X * 2;
static const int FT02_KNOWLEDGE_READER_TEXT_FIRST_BASELINE =
    FT02_KNOWLEDGE_READER_Y + 101;
static const int FT02_KNOWLEDGE_READER_TEXT_LINE_HEIGHT = 36;
static const int FT02_KNOWLEDGE_READER_LINES_PER_PAGE = 4;
static const int FT02_KNOWLEDGE_READER_TEXT_CLIP_Y =
    FT02_KNOWLEDGE_READER_Y + 76;
static const int FT02_KNOWLEDGE_READER_TEXT_CLIP_H =
    FT02_KNOWLEDGE_READER_Y + FT02_KNOWLEDGE_READER_H - 46 -
    FT02_KNOWLEDGE_READER_TEXT_CLIP_Y;

static FT02KnowledgeScreen g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
static int g_homeSelected = 0;
static int g_categorySelected = 0;
static int g_currentCategory = 0;
static int g_listSelected = 0;
static int g_readerPage = 0;
static int g_placeholderSource = 0;
static bool g_quickMode = false;
static char g_errorTitle[48] = "手册不可用";
static char g_errorDetail[120] = "请检查 SD 卡数据包。";

static const FT02KnowledgeHomeCard FT02_KNOWLEDGE_HOME_CARDS[] = {
    {"快速应急", "直接查看高优先级行动卡", FT02_KNOWLEDGE_ICON_EMERGENCY},
    {"分类浏览", "按现场情况查找行动卡", FT02_KNOWLEDGE_ICON_CATEGORY},
    {"搜索知识", "按编号、标题和关键词搜索", FT02_KNOWLEDGE_ICON_SEARCH},
    {"我的收藏", "保存常用现场行动卡", FT02_KNOWLEDGE_ICON_FAVORITE},
    {"最近阅读", "继续上次查看的位置", FT02_KNOWLEDGE_ICON_RECENT},
    {"设备手册", "FT-02 使用与维护", FT02_KNOWLEDGE_ICON_MANUAL}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_NAV_BOTTOM[3] = {
    {nullptr, "方向选择"},
    {nullptr, "确认进入"},
    {nullptr, "返回(B)"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_LIST_BOTTOM[3] = {
    {nullptr, "方向选择"},
    {nullptr, "确认阅读"},
    {nullptr, "返回(B)"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_READER_BOTTOM[3] = {
    {nullptr, "Z 上一页"},
    {nullptr, "C 下一页"},
    {nullptr, "返回(B)"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_PLACEHOLDER_BOTTOM[3] = {
    {nullptr, "功能预留"},
    {nullptr, "确认返回"},
    {nullptr, "返回(B)"}
};

static int FT02_KnowledgeClamp(int value, int low, int high)
{
    if(value < low) return low;
    if(value > high) return high;
    return value;
}

static int FT02_KnowledgeHomeCount()
{
    return sizeof(FT02_KNOWLEDGE_HOME_CARDS) / sizeof(FT02_KNOWLEDGE_HOME_CARDS[0]);
}

static void FT02_KnowledgeSetError(const char* title, const char* detail)
{
    snprintf(g_errorTitle, sizeof(g_errorTitle), "%s", title != nullptr ? title : "手册不可用");
    snprintf(g_errorDetail, sizeof(g_errorDetail), "%s", detail != nullptr ? detail : "请检查 SD 卡数据包。");
    g_screen = FT02_KNOWLEDGE_SCREEN_ERROR;
}

static bool FT02_KnowledgeEnsureRuntime()
{
    if(FT02_FieldManualBegin(false))
    {
        return true;
    }

    const FT02FieldManualState state = FT02_FieldManualStateCurrent();
    if(state == FT02_FIELD_MANUAL_STORAGE_NOT_READY)
    {
        FT02_KnowledgeSetError("SD 卡不可用", "确认 SD 卡已插入并重新启动设备。");
    }
    else if(state == FT02_FIELD_MANUAL_PACK_NOT_FOUND)
    {
        FT02_KnowledgeSetError("未找到行动手册", "复制 knowledge/field_manual 到 SD 卡根目录。");
    }
    else if(state == FT02_FIELD_MANUAL_CHECKSUM_FAILED)
    {
        FT02_KnowledgeSetError("行动手册已损坏", "重新复制完整的 field_manual 数据目录。");
    }
    else
    {
        FT02_KnowledgeSetError("行动手册格式错误", FT02_FieldManualStateText());
    }
    return false;
}

static void FT02_KnowledgeDrawHeader(
    FT02Display& display,
    const char* title,
    const char* rightText
)
{
    int titleClipWidth = display.width() - FT02_KNOWLEDGE_HEADER_X - FT02_KNOWLEDGE_RIGHT_MARGIN;
    if(rightText != nullptr)
    {
        const int width = FT02_TextWidthPack(ft02_cjk_24r, rightText);
        const int rightX = display.width() - FT02_KNOWLEDGE_RIGHT_MARGIN - width;
        titleClipWidth = rightX - FT02_KNOWLEDGE_HEADER_X - 18;
        FT02_DrawTextPack(
            display,
            ft02_cjk_24r,
            rightText,
            rightX,
            FT02_KNOWLEDGE_HEADER_BASELINE - 2
        );
    }

    FT02_DrawTextPackClipped(
        display,
        ft02_cjk_24b,
        title,
        FT02_KNOWLEDGE_HEADER_X,
        FT02_KNOWLEDGE_HEADER_BASELINE,
        FT02_KNOWLEDGE_HEADER_X,
        FT02_KNOWLEDGE_HEADER_BASELINE - 30,
        titleClipWidth,
        36
    );
}

static void FT02_KnowledgeDrawBitmapIcon(
    FT02Display& display,
    const FT02Icon& icon,
    int x,
    int y,
    uint16_t ink
)
{
    for(int row = 0; row < icon.height; row++)
    {
        for(int col = 0; col < icon.width; col++)
        {
            const int byteIndex = row * icon.bytesPerRow + col / 8;
            const uint8_t mask = 0x80 >> (col % 8);
            if((icon.bitmap[byteIndex] & mask) != 0)
            {
                display.drawPixel(x + col, y + row, ink);
            }
        }
    }
}

static void FT02_KnowledgeDrawIcon(
    FT02Display& display,
    FT02KnowledgeIcon icon,
    int x,
    int y,
    bool selected
)
{
    const uint16_t ink = selected ? GxEPD_WHITE : GxEPD_BLACK;
    const int cx = x + 22;
    const int cy = y + 22;

    switch(icon)
    {
        case FT02_KNOWLEDGE_ICON_EMERGENCY:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_EMERGENCY, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_CATEGORY:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_CATEGORY, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_SEARCH:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_SEARCH, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_FAVORITE:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_FAVORITE, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_RECENT:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_RECENT, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_MANUAL:
            FT02_KnowledgeDrawBitmapIcon(display, ICON_KNOW_HOME_MANUAL, x - 2, y - 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_MEDICAL:
            display.fillRect(x + 17, y + 4, 10, 36, ink);
            display.fillRect(x + 4, y + 17, 36, 10, ink);
            break;
        case FT02_KNOWLEDGE_ICON_WATER:
            display.drawTriangle(cx, y + 2, x + 7, y + 29, x + 37, y + 29, ink);
            display.drawCircle(cx, y + 29, 15, ink);
            break;
        case FT02_KNOWLEDGE_ICON_FOOD:
            display.drawLine(x + 4, y + 20, x + 40, y + 20, ink);
            display.drawLine(x + 6, y + 21, x + 10, y + 33, ink);
            display.drawLine(x + 10, y + 33, x + 34, y + 33, ink);
            display.drawLine(x + 34, y + 33, x + 39, y + 21, ink);
            display.drawLine(x + 13, y + 39, x + 31, y + 39, ink);
            break;
        case FT02_KNOWLEDGE_ICON_FIRE:
            display.drawTriangle(cx, y + 2, x + 8, y + 40, x + 36, y + 40, ink);
            display.drawTriangle(cx, y + 15, x + 16, y + 38, x + 29, y + 38, ink);
            break;
        case FT02_KNOWLEDGE_ICON_TOOLS:
            display.drawLine(x + 6, y + 6, x + 38, y + 38, ink);
            display.drawLine(x + 7, y + 7, x + 37, y + 39, ink);
            display.drawCircle(x + 8, y + 8, 5, ink);
            display.drawCircle(x + 36, y + 36, 5, ink);
            display.drawLine(x + 37, y + 6, x + 7, y + 38, ink);
            break;
        case FT02_KNOWLEDGE_ICON_ENERGY:
            display.drawLine(x + 27, y + 2, x + 10, y + 24, ink);
            display.drawLine(x + 10, y + 24, x + 22, y + 24, ink);
            display.drawLine(x + 22, y + 24, x + 17, y + 42, ink);
            display.drawLine(x + 17, y + 42, x + 36, y + 18, ink);
            display.drawLine(x + 36, y + 18, x + 24, y + 18, ink);
            display.drawLine(x + 24, y + 18, x + 27, y + 2, ink);
            break;
        case FT02_KNOWLEDGE_ICON_NAVIGATION:
            display.drawCircle(cx, cy, 19, ink);
            display.drawTriangle(cx, y + 5, x + 16, y + 30, x + 31, y + 24, ink);
            display.drawTriangle(cx, y + 39, x + 16, y + 14, x + 31, y + 20, ink);
            break;
        case FT02_KNOWLEDGE_ICON_SHELTER:
            display.drawTriangle(cx, y + 3, x + 2, y + 23, x + 42, y + 23, ink);
            display.drawRect(x + 8, y + 23, 28, 20, ink);
            display.drawRect(x + 20, y + 31, 7, 12, ink);
            break;
        case FT02_KNOWLEDGE_ICON_ORGANIZATION:
            display.drawCircle(x + 14, y + 14, 7, ink);
            display.drawCircle(x + 30, y + 14, 7, ink);
            display.drawCircle(cx, y + 30, 7, ink);
            display.drawLine(x + 18, y + 20, x + 20, y + 24, ink);
            display.drawLine(x + 27, y + 20, x + 24, y + 24, ink);
            break;
    }
}

static FT02KnowledgeIcon FT02_KnowledgeCategoryIcon(const FT02FieldManualCategory& category)
{
    if(strcmp(category.id, "MED") == 0 || strcmp(category.id, "HYG") == 0) return FT02_KNOWLEDGE_ICON_MEDICAL;
    if(strcmp(category.id, "WAT") == 0) return FT02_KNOWLEDGE_ICON_WATER;
    if(strcmp(category.id, "FOD") == 0) return FT02_KNOWLEDGE_ICON_FOOD;
    if(strcmp(category.id, "FIR") == 0) return FT02_KNOWLEDGE_ICON_FIRE;
    if(strcmp(category.id, "REP") == 0) return FT02_KNOWLEDGE_ICON_TOOLS;
    if(strcmp(category.id, "ENR") == 0) return FT02_KNOWLEDGE_ICON_ENERGY;
    if(strcmp(category.id, "NAV") == 0) return FT02_KNOWLEDGE_ICON_NAVIGATION;
    if(strcmp(category.id, "ORG") == 0) return FT02_KNOWLEDGE_ICON_ORGANIZATION;
    return FT02_KNOWLEDGE_ICON_SHELTER;
}

static void FT02_KnowledgeGridRect(int visibleIndex, int* x, int* y, int* w, int* h)
{
    const int row = visibleIndex / FT02_KNOWLEDGE_GRID_COLS;
    const int col = visibleIndex % FT02_KNOWLEDGE_GRID_COLS;
    *x = FT02_KNOWLEDGE_GRID_X + col * (FT02_KNOWLEDGE_GRID_W + FT02_KNOWLEDGE_GRID_GAP_X);
    *y = FT02_KNOWLEDGE_GRID_Y + row * (FT02_KNOWLEDGE_GRID_H + FT02_KNOWLEDGE_GRID_GAP_Y);
    *w = FT02_KNOWLEDGE_GRID_W;
    *h = FT02_KNOWLEDGE_GRID_H;
}

static void FT02_KnowledgeListRect(int visibleIndex, int* x, int* y, int* w, int* h)
{
    *x = FT02_KNOWLEDGE_LIST_X;
    *y = FT02_KNOWLEDGE_LIST_Y + visibleIndex * (FT02_KNOWLEDGE_LIST_H + FT02_KNOWLEDGE_LIST_GAP_Y);
    *w = FT02_KNOWLEDGE_LIST_W;
    *h = FT02_KNOWLEDGE_LIST_H;
}

static void FT02_KnowledgeDrawGridCard(
    FT02Display& display,
    int x,
    int y,
    const char* title,
    const char* subtitle,
    FT02KnowledgeIcon icon,
    bool selected
)
{
    if(selected)
    {
        display.fillRect(x, y, FT02_KNOWLEDGE_GRID_W, FT02_KNOWLEDGE_GRID_H, GxEPD_BLACK);
    }
    else
    {
        display.fillRect(x, y, FT02_KNOWLEDGE_GRID_W, FT02_KNOWLEDGE_GRID_H, GxEPD_WHITE);
        display.drawRect(x, y, FT02_KNOWLEDGE_GRID_W, FT02_KNOWLEDGE_GRID_H, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, FT02_KNOWLEDGE_GRID_W - 4, FT02_KNOWLEDGE_GRID_H - 4, GxEPD_BLACK);
    }

    FT02_KnowledgeDrawIcon(display, icon, x + 18, y + 19, selected);
    const int textX = x + 82;
    const int clipW = FT02_KNOWLEDGE_GRID_W - 96;
    if(selected)
    {
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_24b, title, textX, y + 36, textX, y + 5, clipW, 36);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, subtitle, textX, y + 68, textX, y + 40, clipW, 34);
    }
    else
    {
        FT02_DrawTextPackClipped(display, ft02_cjk_24b, title, textX, y + 36, textX, y + 5, clipW, 36);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, subtitle, textX, y + 68, textX, y + 40, clipW, 34);
    }
}

static void FT02_KnowledgeDrawHomeCard(FT02Display& display, int index, bool selected)
{
    int x, y, w, h;
    FT02_KnowledgeGridRect(index, &x, &y, &w, &h);
    const FT02KnowledgeHomeCard& card = FT02_KNOWLEDGE_HOME_CARDS[index];
    FT02_KnowledgeDrawGridCard(display, x, y, card.title, card.subtitle, card.icon, selected);
}

static void FT02_KnowledgeDrawCategoryCard(FT02Display& display, int globalIndex, bool selected)
{
    const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(globalIndex);
    if(category == nullptr) return;
    const int visible = globalIndex % FT02_KNOWLEDGE_GRID_PAGE_SIZE;
    int x, y, w, h;
    FT02_KnowledgeGridRect(visible, &x, &y, &w, &h);
    FT02_KnowledgeDrawGridCard(
        display,
        x,
        y,
        category->name,
        category->summary,
        FT02_KnowledgeCategoryIcon(*category),
        selected
    );
}

static void FT02_KnowledgeDrawListCard(
    FT02Display& display,
    int visibleIndex,
    const FT02FieldManualCardIndex& card,
    bool selected
)
{
    int x, y, w, h;
    FT02_KnowledgeListRect(visibleIndex, &x, &y, &w, &h);
    const int codeWidth = FT02_TextWidthPack(ft02_cjk_20r, card.id);
    const int titleClipW = w - 58 - codeWidth;

    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_24b, card.title, x + 20, y + 32, x + 16, y + 4, titleClipW, 34);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, card.summary, x + 20, y + 59, x + 16, y + 35, w - 40, 28);
        FT02_DrawTextPackInvert(display, ft02_cjk_20r, card.id, x + w - 18 - codeWidth, y + 30);
    }
    else
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, w - 4, h - 4, GxEPD_BLACK);
        FT02_DrawTextPackClipped(display, ft02_cjk_24b, card.title, x + 20, y + 32, x + 16, y + 4, titleClipW, 34);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, card.summary, x + 20, y + 59, x + 16, y + 35, w - 40, 28);
        FT02_DrawTextPack(display, ft02_cjk_20r, card.id, x + w - 18 - codeWidth, y + 30);
    }
}

static int FT02_KnowledgeListCount()
{
    if(!FT02_KnowledgeEnsureRuntime()) return 0;
    if(g_quickMode) return FT02_FieldManualQuickCount();
    const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(g_currentCategory);
    return category != nullptr ? category->cardCount : 0;
}

static int FT02_KnowledgeListGlobalIndex(int localIndex)
{
    if(g_quickMode) return FT02_FieldManualQuickGlobalIndex(localIndex);
    return FT02_FieldManualCategoryCardGlobalIndex(g_currentCategory, localIndex);
}

static void FT02_KnowledgeDrawErrorContent(FT02Display& display);

static void FT02_KnowledgeDrawHomeContent(FT02Display& display)
{
    char rightText[40];
    if(FT02_FieldManualStateCurrent() == FT02_FIELD_MANUAL_READY)
    {
        snprintf(rightText, sizeof(rightText), "%d 张行动卡", FT02_FieldManualCardCount());
    }
    else
    {
        snprintf(rightText, sizeof(rightText), "SD 行动手册");
    }
    FT02_KnowledgeDrawHeader(display, "现场行动手册", rightText);
    for(int index = 0; index < FT02_KnowledgeHomeCount(); index++)
    {
        FT02_KnowledgeDrawHomeCard(display, index, index == g_homeSelected);
    }
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_NAV_BOTTOM, ft02_cjk_24r);
}

static void FT02_KnowledgeDrawCategoryContent(FT02Display& display)
{
    if(!FT02_KnowledgeEnsureRuntime())
    {
        FT02_KnowledgeDrawErrorContent(display);
        return;
    }
    const int count = FT02_FieldManualCategoryCount();
    const int page = g_categorySelected / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
    const int pageCount = (count + FT02_KNOWLEDGE_GRID_PAGE_SIZE - 1) / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
    char indicator[20];
    snprintf(indicator, sizeof(indicator), "%d / %d", page + 1, pageCount);
    FT02_KnowledgeDrawHeader(display, "分类浏览", indicator);

    const int start = page * FT02_KNOWLEDGE_GRID_PAGE_SIZE;
    const int end = FT02_KnowledgeClamp(start + FT02_KNOWLEDGE_GRID_PAGE_SIZE, 0, count);
    for(int index = start; index < end; index++)
    {
        FT02_KnowledgeDrawCategoryCard(display, index, index == g_categorySelected);
    }
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_NAV_BOTTOM, ft02_cjk_24r);
}

static void FT02_KnowledgeDrawListContent(FT02Display& display)
{
    if(!FT02_KnowledgeEnsureRuntime())
    {
        FT02_KnowledgeDrawErrorContent(display);
        return;
    }
    const int count = FT02_KnowledgeListCount();
    if(count < 1)
    {
        FT02_KnowledgeSetError("分类暂无行动卡", "请重新构建并复制现场行动手册数据包。");
        return;
    }

    g_listSelected = FT02_KnowledgeClamp(g_listSelected, 0, count - 1);
    char indicator[24];
    snprintf(indicator, sizeof(indicator), "%d / %d", g_listSelected + 1, count);
    const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(g_currentCategory);
    const char* title = g_quickMode ? "快速应急" : (category != nullptr ? category->name : "行动卡");
    FT02_KnowledgeDrawHeader(display, title, indicator);

    const int page = g_listSelected / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int start = page * FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int end = FT02_KnowledgeClamp(start + FT02_KNOWLEDGE_LIST_PAGE_SIZE, 0, count);
    for(int localIndex = start; localIndex < end; localIndex++)
    {
        const int globalIndex = FT02_KnowledgeListGlobalIndex(localIndex);
        const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(globalIndex);
        if(card != nullptr)
        {
            FT02_KnowledgeDrawListCard(display, localIndex - start, *card, localIndex == g_listSelected);
        }
    }
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_LIST_BOTTOM, ft02_cjk_24r);
}

static int FT02_KnowledgeNextWrappedLine(
    const char* text,
    int maxPixelWidth,
    char* outLine,
    int outCapacity,
    const char** nextText
)
{
    int lineBytes = 0;
    int lineWidth = 0;
    const char* cursor = text;

    while(*cursor)
    {
        int used = 0;
        const uint32_t cp = FT02_ReadCodepoint(cursor, &used);
        if(cp == '\r')
        {
            cursor += used;
            continue;
        }
        if(cp == '\n')
        {
            cursor += used;
            break;
        }

        const int advance = FT02_CodepointAdvancePack(ft02_cjk_24r, cp);
        if(lineBytes > 0 && lineWidth + advance > maxPixelWidth) break;
        if(lineBytes + used >= outCapacity - 1) break;

        for(int byteIndex = 0; byteIndex < used; byteIndex++)
        {
            outLine[lineBytes++] = cursor[byteIndex];
        }
        lineWidth += advance;
        cursor += used;
        if(lineWidth >= maxPixelWidth) break;
    }

    if(lineBytes == 0 && *cursor)
    {
        int used = 0;
        FT02_ReadCodepoint(cursor, &used);
        if(used < outCapacity - 1)
        {
            for(int byteIndex = 0; byteIndex < used; byteIndex++) outLine[lineBytes++] = cursor[byteIndex];
            cursor += used;
        }
    }

    outLine[lineBytes] = 0;
    *nextText = cursor;
    return lineBytes;
}

static int FT02_KnowledgeCountWrappedLines(const char* text)
{
    if(text == nullptr || *text == 0) return 1;
    char line[192];
    int count = 0;
    const char* cursor = text;
    while(*cursor)
    {
        const char* next = cursor;
        FT02_KnowledgeNextWrappedLine(cursor, FT02_KNOWLEDGE_READER_TEXT_W, line, sizeof(line), &next);
        count++;
        if(next <= cursor) break;
        cursor = next;
    }
    return count > 0 ? count : 1;
}

static int FT02_KnowledgeSectionPageCount(const char* text)
{
    const int lines = FT02_KnowledgeCountWrappedLines(text);
    return (lines + FT02_KNOWLEDGE_READER_LINES_PER_PAGE - 1) / FT02_KNOWLEDGE_READER_LINES_PER_PAGE;
}

static int FT02_KnowledgeReaderPageCount(const FT02FieldManualLoadedCard& card)
{
    int pages = 0;
    for(int sectionIndex = 0; sectionIndex < card.sectionCount; sectionIndex++)
    {
        pages += FT02_KnowledgeSectionPageCount(card.sections[sectionIndex].text);
    }
    return pages > 0 ? pages : 1;
}

static void FT02_KnowledgeResolveReaderPage(
    const FT02FieldManualLoadedCard& card,
    int requestedPage,
    const char** section,
    const char** pageText
)
{
    int remainingPage = requestedPage;
    for(int sectionIndex = 0; sectionIndex < card.sectionCount; sectionIndex++)
    {
        const char* text = card.sections[sectionIndex].text;
        const int sectionPages = FT02_KnowledgeSectionPageCount(text);
        if(remainingPage < sectionPages)
        {
            *section = card.sections[sectionIndex].title;
            const char* cursor = text;
            char skipped[192];
            const int linesToSkip = remainingPage * FT02_KNOWLEDGE_READER_LINES_PER_PAGE;
            for(int line = 0; line < linesToSkip && *cursor; line++)
            {
                const char* next = cursor;
                FT02_KnowledgeNextWrappedLine(cursor, FT02_KNOWLEDGE_READER_TEXT_W, skipped, sizeof(skipped), &next);
                if(next <= cursor) break;
                cursor = next;
            }
            *pageText = cursor;
            return;
        }
        remainingPage -= sectionPages;
    }

    *section = card.sections[card.sectionCount - 1].title;
    *pageText = card.sections[card.sectionCount - 1].text;
}

static void FT02_KnowledgeDrawWrappedPage(FT02Display& display, const char* text)
{
    const int textX = FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X;
    char line[192];
    const char* cursor = text;
    for(int lineIndex = 0; lineIndex < FT02_KNOWLEDGE_READER_LINES_PER_PAGE && *cursor; lineIndex++)
    {
        const char* next = cursor;
        FT02_KnowledgeNextWrappedLine(cursor, FT02_KNOWLEDGE_READER_TEXT_W, line, sizeof(line), &next);
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_24r,
            line,
            textX,
            FT02_KNOWLEDGE_READER_TEXT_FIRST_BASELINE + lineIndex * FT02_KNOWLEDGE_READER_TEXT_LINE_HEIGHT,
            textX,
            FT02_KNOWLEDGE_READER_TEXT_CLIP_Y,
            FT02_KNOWLEDGE_READER_TEXT_W,
            FT02_KNOWLEDGE_READER_TEXT_CLIP_H
        );
        if(next <= cursor) break;
        cursor = next;
    }
}

static void FT02_KnowledgeDrawReaderContent(FT02Display& display)
{
    const FT02FieldManualLoadedCard* card = FT02_FieldManualLoadedCardCurrent();
    if(card == nullptr || card->index == nullptr)
    {
        FT02_KnowledgeSetError("行动卡读取失败", "返回列表后重新打开，或重新复制数据包。");
        FT02_KnowledgeDrawErrorContent(display);
        return;
    }

    const int pageCount = FT02_KnowledgeReaderPageCount(*card);
    g_readerPage = FT02_KnowledgeClamp(g_readerPage, 0, pageCount - 1);
    char indicator[28];
    snprintf(indicator, sizeof(indicator), "%s  %d / %d", card->index->id, g_readerPage + 1, pageCount);
    FT02_KnowledgeDrawHeader(display, card->index->title, indicator);

    const char* section = card->sections[0].title;
    const char* pageText = card->sections[0].text;
    FT02_KnowledgeResolveReaderPage(*card, g_readerPage, &section, &pageText);

    display.drawRect(FT02_KNOWLEDGE_READER_X, FT02_KNOWLEDGE_READER_Y, FT02_KNOWLEDGE_READER_W, FT02_KNOWLEDGE_READER_H, GxEPD_BLACK);
    display.drawRect(FT02_KNOWLEDGE_READER_X + 2, FT02_KNOWLEDGE_READER_Y + 2, FT02_KNOWLEDGE_READER_W - 4, FT02_KNOWLEDGE_READER_H - 4, GxEPD_BLACK);
    FT02_DrawTextPackClipped(
        display,
        ft02_cjk_24b,
        section,
        FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X,
        FT02_KNOWLEDGE_READER_Y + 46,
        FT02_KNOWLEDGE_READER_X + 10,
        FT02_KNOWLEDGE_READER_Y + 8,
        FT02_KNOWLEDGE_READER_W - 20,
        48
    );
    display.fillRect(
        FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X,
        FT02_KNOWLEDGE_READER_Y + 63,
        FT02_KNOWLEDGE_READER_TEXT_W,
        2,
        GxEPD_BLACK
    );
    FT02_KnowledgeDrawWrappedPage(display, pageText);

    const char* hint = g_readerPage < pageCount - 1 ? "下一页继续" : "本卡结束";
    const int hintWidth = FT02_TextWidthPack(ft02_cjk_20r, hint);
    FT02_DrawTextPack(
        display,
        ft02_cjk_20r,
        hint,
        FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_W - 22 - hintWidth,
        FT02_KNOWLEDGE_READER_Y + FT02_KNOWLEDGE_READER_H - 16
    );
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_READER_BOTTOM, ft02_cjk_24r);
}

static void FT02_KnowledgeDrawPlaceholderContent(FT02Display& display)
{
    const FT02KnowledgeHomeCard& source = FT02_KNOWLEDGE_HOME_CARDS[g_placeholderSource];
    FT02_KnowledgeDrawHeader(display, source.title, "后续功能");
    const int x = 104;
    const int y = 156;
    const int w = 592;
    const int h = 226;
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.drawRect(x + 3, y + 3, w - 6, h - 6, GxEPD_BLACK);
    FT02_KnowledgeDrawIcon(display, source.icon, x + 44, y + 48, false);
    FT02_DrawTextPack(display, ft02_cjk_24b, "运行入口已经保留", x + 124, y + 74);
    FT02_DrawTextPack(display, ft02_cjk_24r, "当前阶段先完成真实行动卡读取。", x + 124, y + 113);
    FT02_DrawTextPack(display, ft02_cjk_24r, "搜索、收藏和历史将在后续接入。", x + 124, y + 151);
    FT02_DrawTextPack(display, ft02_cjk_24r, "按确认键或 B 返回。", x + 124, y + 189);
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_PLACEHOLDER_BOTTOM, ft02_cjk_24r);
}

static void FT02_KnowledgeDrawErrorContent(FT02Display& display)
{
    FT02_KnowledgeDrawHeader(display, g_errorTitle, FT02_FieldManualStateText());
    const int x = 72;
    const int y = 160;
    const int w = 656;
    const int h = 208;
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.drawRect(x + 3, y + 3, w - 6, h - 6, GxEPD_BLACK);
    FT02_KnowledgeDrawIcon(display, FT02_KNOWLEDGE_ICON_EMERGENCY, x + 34, y + 38, false);
    FT02_DrawTextPackClipped(display, ft02_cjk_24b, g_errorTitle, x + 112, y + 72, x + 108, y + 20, w - 140, 60);
    FT02_DrawTextPackClipped(display, ft02_cjk_24r, g_errorDetail, x + 34, y + 132, x + 28, y + 92, w - 56, 58);
    FT02_DrawTextPack(display, ft02_cjk_20r, "目标路径：/knowledge/field_manual", x + 34, y + 178);
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_PLACEHOLDER_BOTTOM, ft02_cjk_24r);
}

static void FT02_KnowledgeDrawCurrentContent(FT02Display& display)
{
    switch(g_screen)
    {
        case FT02_KNOWLEDGE_SCREEN_HOME:
            FT02_KnowledgeDrawHomeContent(display);
            break;
        case FT02_KNOWLEDGE_SCREEN_CATEGORIES:
            FT02_KnowledgeDrawCategoryContent(display);
            break;
        case FT02_KNOWLEDGE_SCREEN_LIST:
            FT02_KnowledgeDrawListContent(display);
            break;
        case FT02_KNOWLEDGE_SCREEN_READER:
            FT02_KnowledgeDrawReaderContent(display);
            break;
        case FT02_KNOWLEDGE_SCREEN_PLACEHOLDER:
            FT02_KnowledgeDrawPlaceholderContent(display);
            break;
        case FT02_KNOWLEDGE_SCREEN_ERROR:
            FT02_KnowledgeDrawErrorContent(display);
            break;
    }
}

static int FT02_KnowledgeMoveGrid(int current, int count, int deltaRow, int deltaCol)
{
    const int candidate = current + deltaRow * FT02_KNOWLEDGE_GRID_COLS + deltaCol;
    return FT02_KnowledgeClamp(candidate, 0, count - 1);
}

static void FT02_KnowledgeRedraw(FT02Display& display)
{
    FT02_DrawKnowledgeScreen(display);
}

static FT02KnowledgeAction FT02_KnowledgeHandleHome(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK) return FT02_KNOWLEDGE_ACTION_EXIT_HOME;

    if(event.key == FT02_KEY_SELECT)
    {
        if(g_homeSelected == 0)
        {
            if(FT02_KnowledgeEnsureRuntime())
            {
                g_quickMode = true;
                g_listSelected = 0;
                g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
            }
        }
        else if(g_homeSelected == 1)
        {
            if(FT02_KnowledgeEnsureRuntime())
            {
                g_screen = FT02_KNOWLEDGE_SCREEN_CATEGORIES;
            }
        }
        else
        {
            g_placeholderSource = g_homeSelected;
            g_screen = FT02_KNOWLEDGE_SCREEN_PLACEHOLDER;
        }
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    int next = g_homeSelected;
    if(event.key == FT02_KEY_LEFT) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 0, -1);
    else if(event.key == FT02_KEY_RIGHT) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 0, 1);
    else if(event.key == FT02_KEY_UP) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), -1, 0);
    else if(event.key == FT02_KEY_DOWN) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 1, 0);

    if(next != g_homeSelected)
    {
        g_homeSelected = next;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleCategories(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    const int count = FT02_FieldManualCategoryCount();
    if(count < 1)
    {
        FT02_KnowledgeSetError("没有可用分类", "重新构建并复制现场行动手册数据包。");
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(event.key == FT02_KEY_SELECT)
    {
        g_currentCategory = g_categorySelected;
        g_quickMode = false;
        g_listSelected = 0;
        g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    int next = g_categorySelected;
    if(event.key == FT02_KEY_LEFT) next = FT02_KnowledgeMoveGrid(next, count, 0, -1);
    else if(event.key == FT02_KEY_RIGHT) next = FT02_KnowledgeMoveGrid(next, count, 0, 1);
    else if(event.key == FT02_KEY_UP) next = FT02_KnowledgeMoveGrid(next, count, -1, 0);
    else if(event.key == FT02_KEY_DOWN) next = FT02_KnowledgeMoveGrid(next, count, 1, 0);

    if(next != g_categorySelected)
    {
        g_categorySelected = next;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleList(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        g_screen = g_quickMode ? FT02_KNOWLEDGE_SCREEN_HOME : FT02_KNOWLEDGE_SCREEN_CATEGORIES;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    const int count = FT02_KnowledgeListCount();
    if(count < 1) return FT02_KNOWLEDGE_ACTION_NONE;

    if(event.key == FT02_KEY_SELECT)
    {
        const int globalIndex = FT02_KnowledgeListGlobalIndex(g_listSelected);
        if(FT02_FieldManualLoadCard(globalIndex))
        {
            g_readerPage = 0;
            g_screen = FT02_KNOWLEDGE_SCREEN_READER;
        }
        else
        {
            FT02_KnowledgeSetError("行动卡读取失败", "检查 cards.dat 后重新复制完整数据包。");
        }
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    int next = g_listSelected;
    if(event.key == FT02_KEY_UP || event.key == FT02_KEY_LEFT) next--;
    else if(event.key == FT02_KEY_DOWN || event.key == FT02_KEY_RIGHT) next++;
    next = FT02_KnowledgeClamp(next, 0, count - 1);
    if(next != g_listSelected)
    {
        g_listSelected = next;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleReader(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        FT02_FieldManualUnloadCard();
        g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    const FT02FieldManualLoadedCard* card = FT02_FieldManualLoadedCardCurrent();
    if(card == nullptr) return FT02_KNOWLEDGE_ACTION_NONE;
    int next = g_readerPage;
    if(event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP) next--;
    else if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN || event.key == FT02_KEY_SELECT) next++;
    next = FT02_KnowledgeClamp(next, 0, FT02_KnowledgeReaderPageCount(*card) - 1);
    if(next != g_readerPage)
    {
        g_readerPage = next;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandlePlaceholder(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK || event.key == FT02_KEY_SELECT)
    {
        g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleError(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK || event.key == FT02_KEY_SELECT)
    {
        g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
        FT02_KnowledgeRedraw(display);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

} // namespace

void FT02_KnowledgeReset()
{
    FT02_FieldManualUnloadCard();
    FT02_FieldManualBegin(false);
    g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
    g_homeSelected = 0;
    g_categorySelected = 0;
    g_currentCategory = 0;
    g_listSelected = 0;
    g_readerPage = 0;
    g_placeholderSource = 0;
    g_quickMode = false;
}

void FT02_DrawKnowledgeScreen(FT02Display& display)
{
    Serial.print("[KNOWLEDGE] ");
    Serial.print(FT02_FIRMWARE_VERSION);
    Serial.print(" SD runtime state=");
    Serial.println(FT02_FieldManualStateText());
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_KnowledgeDrawCurrentContent(display);
    }
    while(display.nextPage());
    FT02_EpdPowerOffAfterCommit(display, "knowledge-full");
}

FT02KnowledgeAction FT02_HandleKnowledgeInput(FT02Display& display, const FT02InputEvent& event)
{
    switch(g_screen)
    {
        case FT02_KNOWLEDGE_SCREEN_HOME:
            return FT02_KnowledgeHandleHome(display, event);
        case FT02_KNOWLEDGE_SCREEN_CATEGORIES:
            return FT02_KnowledgeHandleCategories(display, event);
        case FT02_KNOWLEDGE_SCREEN_LIST:
            return FT02_KnowledgeHandleList(display, event);
        case FT02_KNOWLEDGE_SCREEN_READER:
            return FT02_KnowledgeHandleReader(display, event);
        case FT02_KNOWLEDGE_SCREEN_PLACEHOLDER:
            return FT02_KnowledgeHandlePlaceholder(display, event);
        case FT02_KNOWLEDGE_SCREEN_ERROR:
            return FT02_KnowledgeHandleError(display, event);
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}
