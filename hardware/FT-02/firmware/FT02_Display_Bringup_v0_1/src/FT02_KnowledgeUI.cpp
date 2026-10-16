#include "FT02_KnowledgeUI.h"
#include "FT02_Log.h"
#include "FT02_EpdInteractiveRuntime.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "FT02_BottomBar.h"
#include "FT02_BuildInfo.h"
#include "FT02_FieldManual.h"
#include "FT02_FieldManualFavorites.h"
#include "FT02_FieldManualRecent.h"
#include "FT02_FieldManualSearch.h"
#include "FT02_PinyinIme.h"
#include "FT02_PinyinLearning.h"
#include "FT02_TextInputMode.h"
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
    FT02_KNOWLEDGE_SCREEN_SEARCH,
    FT02_KNOWLEDGE_SCREEN_LIST,
    FT02_KNOWLEDGE_SCREEN_READER,
    FT02_KNOWLEDGE_SCREEN_PLACEHOLDER,
    FT02_KNOWLEDGE_SCREEN_ERROR
};

enum FT02KnowledgeListSource
{
    FT02_KNOWLEDGE_LIST_QUICK = 0,
    FT02_KNOWLEDGE_LIST_CATEGORY,
    FT02_KNOWLEDGE_LIST_FAVORITES,
    FT02_KNOWLEDGE_LIST_RECENT,
    FT02_KNOWLEDGE_LIST_SEARCH
};

enum FT02KnowledgeIcon
{
    FT02_KNOWLEDGE_ICON_EMERGENCY = 0,
    FT02_KNOWLEDGE_ICON_CATEGORY,
    FT02_KNOWLEDGE_ICON_SEARCH,
    FT02_KNOWLEDGE_ICON_FAVORITE,
    FT02_KNOWLEDGE_ICON_RECENT,
    FT02_KNOWLEDGE_ICON_MANUAL
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
static FT02KnowledgeListSource g_listSource = FT02_KNOWLEDGE_LIST_QUICK;

// Search input uses the same proven offline Pinyin engine and persistent
// CN/EN mode as communication compose, but keeps its own small query buffer.
static char g_searchQuery[FT02_FIELD_MANUAL_SEARCH_MAX_QUERY_BYTES + 1] = {};
static size_t g_searchQueryLength = 0;
static char g_searchPinyin[FT02_PINYIN_MAX_INPUT + 1] = {};
static size_t g_searchPinyinLength = 0;
static size_t g_searchPinyinPage = 0;
static bool g_searchDirty = false;
static uint32_t g_searchLastEditMs = 0;
static char g_searchNotice[72] = {};
static char g_readerFeedback[32] = {};
static const uint32_t FT02_KNOWLEDGE_SEARCH_REDRAW_IDLE_MS = 320u;

// FieldManual uses the global e-paper input FIFO so keys captured during BUSY
// and live keys share one ordering source. Navigation bursts are coalesced
// before newer live input is polled.
static char g_errorTitle[48] = "手册不可用";
static char g_errorDetail[120] = "请检查 SD 卡数据包。";

static const FT02KnowledgeHomeCard FT02_KNOWLEDGE_HOME_CARDS[] = {
    {"快速应急", "直接查看高优先级行动卡", FT02_KNOWLEDGE_ICON_EMERGENCY},
    {"分类浏览", "按现场情况查找行动卡", FT02_KNOWLEDGE_ICON_CATEGORY},
    {"搜索知识", "输入标题片段或常用说法", FT02_KNOWLEDGE_ICON_SEARCH},
    {"我的收藏", "保存常用现场行动卡", FT02_KNOWLEDGE_ICON_FAVORITE},
    {"最近阅读", "继续上次查看的位置", FT02_KNOWLEDGE_ICON_RECENT},
    {"设备手册", "FT-02 使用与维护", FT02_KNOWLEDGE_ICON_MANUAL}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_NAV_BOTTOM[3] = {
    {nullptr, "方向选择"},
    {nullptr, "ENTER进入"},
    {nullptr, "B返回"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_LIST_BOTTOM[3] = {
    {nullptr, "左右 翻页"},
    {nullptr, "上下 上/下条"},
    {nullptr, "ENTER阅读 B返回"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_READER_BOTTOM[3] = {
    {nullptr, "左右 翻页"},
    {nullptr, "0第一页"},
    {nullptr, "F收藏/取消 B返回"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_SEARCH_BOTTOM[3] = {
    {nullptr, "ENTER搜索"},
    {nullptr, "`中/英 DEL删除"},
    {nullptr, "ESC返回"}
};

static const FT02BottomBarItem FT02_KNOWLEDGE_PLACEHOLDER_BOTTOM[3] = {
    {nullptr, "功能预留"},
    {nullptr, "确认返回"},
    {nullptr, "B返回"}
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

static void FT02_KnowledgeSearchSetNotice(const char* text)
{
    snprintf(g_searchNotice, sizeof(g_searchNotice), "%s", text != nullptr ? text : "");
}

static void FT02_KnowledgeSearchMarkDirty()
{
    g_searchDirty = true;
    g_searchLastEditMs = millis();
}

static void FT02_KnowledgeSearchResetPinyin()
{
    g_searchPinyinLength = 0;
    g_searchPinyin[0] = '\0';
    g_searchPinyinPage = 0;
}

static void FT02_KnowledgeSearchResetInput(bool clearQuery)
{
    if(clearQuery)
    {
        g_searchQueryLength = 0;
        g_searchQuery[0] = '\0';
        FT02_FieldManualSearchClear();
    }
    FT02_KnowledgeSearchResetPinyin();
    g_searchDirty = false;
    g_searchLastEditMs = 0;
    FT02_KnowledgeSearchSetNotice("");
}

static bool FT02_KnowledgeSearchAppendBytes(const char* text, size_t bytes)
{
    if(text == nullptr || bytes == 0) return false;
    if(g_searchQueryLength + bytes > FT02_FIELD_MANUAL_SEARCH_MAX_QUERY_BYTES)
    {
        FT02_KnowledgeSearchSetNotice("搜索词已达到长度上限");
        return false;
    }
    memcpy(g_searchQuery + g_searchQueryLength, text, bytes);
    g_searchQueryLength += bytes;
    g_searchQuery[g_searchQueryLength] = '\0';
    FT02_KnowledgeSearchSetNotice("");
    FT02_KnowledgeSearchMarkDirty();
    return true;
}

static void FT02_KnowledgeSearchUtf8Backspace()
{
    if(g_searchQueryLength == 0) return;
    size_t pos = g_searchQueryLength - 1;
    while(pos > 0 && (static_cast<uint8_t>(g_searchQuery[pos]) & 0xC0u) == 0x80u) --pos;
    g_searchQueryLength = pos;
    g_searchQuery[pos] = '\0';
    FT02_KnowledgeSearchSetNotice("");
    FT02_KnowledgeSearchMarkDirty();
}

static size_t FT02_KnowledgeSearchCandidateCount()
{
    return FT02_PinyinImeCandidateCount(g_searchPinyin);
}

static void FT02_KnowledgeSearchClampPinyinPage()
{
    const size_t count = FT02_KnowledgeSearchCandidateCount();
    if(count == 0)
    {
        g_searchPinyinPage = 0;
        return;
    }
    const size_t pages = (count + FT02_PINYIN_PAGE_SIZE - 1u) / FT02_PINYIN_PAGE_SIZE;
    if(g_searchPinyinPage >= pages) g_searchPinyinPage = pages - 1u;
}

static bool FT02_KnowledgeSearchCommitCandidate(size_t slot)
{
    const size_t count = FT02_KnowledgeSearchCandidateCount();
    const size_t index = g_searchPinyinPage * FT02_PINYIN_PAGE_SIZE + slot;
    if(index >= count) return false;

    char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
    size_t consumed = 0;
    if(!FT02_PinyinImeCandidate(g_searchPinyin, index, candidate, sizeof(candidate), &consumed)) return false;
    if(consumed == 0 || consumed > g_searchPinyinLength) return false;
    if(!FT02_KnowledgeSearchAppendBytes(candidate, strlen(candidate))) return false;

    char learningKey[FT02_PINYIN_MAX_INPUT + 1] = {};
    memcpy(learningKey, g_searchPinyin, consumed);
    learningKey[consumed] = '\0';
    FT02_PinyinLearningRecord(learningKey, candidate);

    if(consumed >= g_searchPinyinLength)
    {
        FT02_KnowledgeSearchResetPinyin();
    }
    else
    {
        const size_t remain = g_searchPinyinLength - consumed;
        memmove(g_searchPinyin, g_searchPinyin + consumed, remain);
        g_searchPinyinLength = remain;
        g_searchPinyin[remain] = '\0';
        g_searchPinyinPage = 0;
    }
    FT02_KnowledgeSearchMarkDirty();
    return true;
}

static bool FT02_KnowledgeSearchCommitRawPinyin()
{
    if(g_searchPinyinLength == 0) return false;
    if(!FT02_KnowledgeSearchAppendBytes(g_searchPinyin, g_searchPinyinLength)) return false;
    FT02_KnowledgeSearchResetPinyin();
    FT02_KnowledgeSearchMarkDirty();
    return true;
}

static bool FT02_KnowledgeSearchCommitPinyinDefault()
{
    if(g_searchPinyinLength == 0) return false;
    FT02_KnowledgeSearchClampPinyinPage();
    if(FT02_KnowledgeSearchCandidateCount() > 0) return FT02_KnowledgeSearchCommitCandidate(0);
    return FT02_KnowledgeSearchCommitRawPinyin();
}

static void FT02_KnowledgeSearchAppendPinyinLetter(char raw)
{
    if(g_searchPinyinLength >= FT02_PINYIN_MAX_INPUT)
    {
        FT02_KnowledgeSearchSetNotice("拼音输入已达到长度上限");
        return;
    }
    if(raw >= 'A' && raw <= 'Z') raw = static_cast<char>(raw - 'A' + 'a');
    g_searchPinyin[g_searchPinyinLength++] = raw;
    g_searchPinyin[g_searchPinyinLength] = '\0';
    g_searchPinyinPage = 0;
    FT02_KnowledgeSearchSetNotice("");
    FT02_KnowledgeSearchMarkDirty();
}

static void FT02_KnowledgeSearchPinyinBackspace()
{
    if(g_searchPinyinLength == 0) return;
    --g_searchPinyinLength;
    g_searchPinyin[g_searchPinyinLength] = '\0';
    g_searchPinyinPage = 0;
    FT02_KnowledgeSearchMarkDirty();
}

static void FT02_KnowledgeSearchMovePinyinPage(int delta)
{
    const size_t count = FT02_KnowledgeSearchCandidateCount();
    if(count == 0) return;
    const size_t pages = (count + FT02_PINYIN_PAGE_SIZE - 1u) / FT02_PINYIN_PAGE_SIZE;
    int next = static_cast<int>(g_searchPinyinPage) + delta;
    if(next < 0) next = 0;
    if(next >= static_cast<int>(pages)) next = static_cast<int>(pages) - 1;
    if(next == static_cast<int>(g_searchPinyinPage)) return;
    g_searchPinyinPage = static_cast<size_t>(next);
    FT02_KnowledgeSearchMarkDirty();
}

static void FT02_KnowledgeSearchToggleInputMode()
{
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
        (void)FT02_KnowledgeSearchCommitPinyinDefault();
    const FT02TextInputMode mode = FT02_TextInputModeToggle();
    FT02_KnowledgeSearchSetNotice(mode == FT02_TEXT_INPUT_CN ? "已切换中文输入" : "已切换英文输入");
    FT02_KnowledgeSearchMarkDirty();
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

static void FT02_KnowledgeDrawReaderHeader(
    FT02Display& display,
    const char* title,
    const char* rightText,
    bool favorite
)
{
    // Keep the page/card indicator on the far right. Favorite
    // state is rendered as a compact black/white badge immediately after the
    // visible title, so it cannot cover or compete with pagination.
    const char* badgeText = nullptr;
    if(g_readerFeedback[0] != '\0') badgeText = g_readerFeedback;
    else if(favorite) badgeText = "已收藏";

    int rightX = display.width() - FT02_KNOWLEDGE_RIGHT_MARGIN;
    if(rightText != nullptr)
    {
        const int rightWidth = FT02_TextWidthPack(ft02_cjk_24r, rightText);
        rightX -= rightWidth;
        FT02_DrawTextPack(
            display,
            ft02_cjk_24r,
            rightText,
            rightX,
            FT02_KNOWLEDGE_HEADER_BASELINE - 2
        );
    }

    const int titleAreaRight = rightX - 18;
    int badgeW = 0;
    if(badgeText != nullptr)
        badgeW = FT02_TextWidthPack(ft02_cjk_20r, badgeText) + 20;

    int titleClipWidth = titleAreaRight - FT02_KNOWLEDGE_HEADER_X;
    if(badgeW > 0) titleClipWidth -= badgeW + 10;
    if(titleClipWidth < 24) titleClipWidth = 24;

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

    if(badgeText != nullptr)
    {
        const int fullTitleW = FT02_TextWidthPack(ft02_cjk_24b, title);
        const int visibleTitleW = fullTitleW < titleClipWidth ? fullTitleW : titleClipWidth;
        const int badgeX = FT02_KNOWLEDGE_HEADER_X + visibleTitleW + 10;
        const int badgeY = FT02_KNOWLEDGE_HEADER_BASELINE - 27;
        const int badgeH = 30;
        display.fillRect(badgeX, badgeY, badgeW, badgeH, GxEPD_BLACK);
        FT02_DrawTextPackInvert(
            display,
            ft02_cjk_20r,
            badgeText,
            badgeX + 10,
            badgeY + 23
        );
    }
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
    }
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

    // Category browser intentionally uses text-only cards. Icons consumed a large
    // part of the 356 px card width without adding useful field information.
    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_24b, category->name,
                                       x + 18, y + 36, x + 14, y + 5, w - 28, 36);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, category->summary,
                                       x + 18, y + 68, x + 14, y + 40, w - 28, 34);
    }
    else
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, w - 4, h - 4, GxEPD_BLACK);
        FT02_DrawTextPackClipped(display, ft02_cjk_24b, category->name,
                                 x + 18, y + 36, x + 14, y + 5, w - 28, 36);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, category->summary,
                                 x + 18, y + 68, x + 14, y + 40, w - 28, 34);
    }
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
    char rightLabel[32] = {};
    if(g_listSource == FT02_KNOWLEDGE_LIST_RECENT)
    {
        const int pageStart = (g_listSelected / FT02_KNOWLEDGE_LIST_PAGE_SIZE) * FT02_KNOWLEDGE_LIST_PAGE_SIZE;
        const int savedPage = FT02_FieldManualRecentPageAt(pageStart + visibleIndex);
        snprintf(rightLabel, sizeof(rightLabel), "%s  P%d", card.id, savedPage + 1);
    }
    else
    {
        snprintf(rightLabel, sizeof(rightLabel), "%s", card.id);
    }
    const int codeWidth = FT02_TextWidthPack(ft02_cjk_20r, rightLabel);
    const int titleClipW = w - 58 - codeWidth;

    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_24b, card.title, x + 20, y + 32, x + 16, y + 4, titleClipW, 34);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, card.summary, x + 20, y + 59, x + 16, y + 35, w - 40, 28);
        FT02_DrawTextPackInvert(display, ft02_cjk_20r, rightLabel, x + w - 18 - codeWidth, y + 30);
    }
    else
    {
        display.fillRect(x, y, w, h, GxEPD_WHITE);
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        display.drawRect(x + 2, y + 2, w - 4, h - 4, GxEPD_BLACK);
        FT02_DrawTextPackClipped(display, ft02_cjk_24b, card.title, x + 20, y + 32, x + 16, y + 4, titleClipW, 34);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, card.summary, x + 20, y + 59, x + 16, y + 35, w - 40, 28);
        FT02_DrawTextPack(display, ft02_cjk_20r, rightLabel, x + w - 18 - codeWidth, y + 30);
    }
}

static int FT02_KnowledgeListCount()
{
    if(!FT02_KnowledgeEnsureRuntime()) return 0;
    switch(g_listSource)
    {
        case FT02_KNOWLEDGE_LIST_QUICK:
            return FT02_FieldManualQuickCount();
        case FT02_KNOWLEDGE_LIST_CATEGORY:
        {
            const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(g_currentCategory);
            return category != nullptr ? category->cardCount : 0;
        }
        case FT02_KNOWLEDGE_LIST_FAVORITES:
            return FT02_FieldManualFavoritesCount();
        case FT02_KNOWLEDGE_LIST_RECENT:
            return FT02_FieldManualRecentCount();
        case FT02_KNOWLEDGE_LIST_SEARCH:
            return FT02_FieldManualSearchResultCount();
    }
    return 0;
}

static int FT02_KnowledgeListGlobalIndex(int localIndex)
{
    switch(g_listSource)
    {
        case FT02_KNOWLEDGE_LIST_QUICK:
            return FT02_FieldManualQuickGlobalIndex(localIndex);
        case FT02_KNOWLEDGE_LIST_CATEGORY:
            return FT02_FieldManualCategoryCardGlobalIndex(g_currentCategory, localIndex);
        case FT02_KNOWLEDGE_LIST_FAVORITES:
            return FT02_FieldManualFavoritesGlobalIndexAt(localIndex);
        case FT02_KNOWLEDGE_LIST_RECENT:
            return FT02_FieldManualRecentGlobalIndexAt(localIndex);
        case FT02_KNOWLEDGE_LIST_SEARCH:
            return FT02_FieldManualSearchGlobalIndexAt(localIndex);
    }
    return -1;
}

static const char* FT02_KnowledgeListTitle()
{
    switch(g_listSource)
    {
        case FT02_KNOWLEDGE_LIST_QUICK:
            return "快速应急";
        case FT02_KNOWLEDGE_LIST_CATEGORY:
        {
            const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(g_currentCategory);
            return category != nullptr ? category->name : "行动卡";
        }
        case FT02_KNOWLEDGE_LIST_FAVORITES:
            return "我的收藏";
        case FT02_KNOWLEDGE_LIST_RECENT:
            return "最近阅读";
        case FT02_KNOWLEDGE_LIST_SEARCH:
            return "搜索结果";
    }
    return "行动卡";
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

static void FT02_KnowledgeDrawSearchContent(FT02Display& display)
{
    if(!FT02_KnowledgeEnsureRuntime())
    {
        FT02_KnowledgeDrawErrorContent(display);
        return;
    }

    FT02_KnowledgeDrawHeader(
        display,
        "搜索知识",
        FT02_TextInputModeIsChinese() ? "中文输入" : "EN 输入"
    );

    const int boxX = 32;
    const int boxY = 146;
    const int boxW = 736;
    const int boxH = 66;
    display.drawRect(boxX, boxY, boxW, boxH, GxEPD_BLACK);
    display.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_20r, "搜索词", boxX + 14, boxY + 26);
    if(g_searchQueryLength == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "输入标题片段、编号或内容词...", boxX + 105, boxY + 43);
    }
    else
    {
        FT02_DrawTextPackClipped(
            display, ft02_cjk_24b, g_searchQuery,
            boxX + 105, boxY + 43,
            boxX + 98, boxY + 8, boxW - 116, boxH - 16
        );
    }

    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
    {
        char pyDisplay[FT02_PINYIN_MAX_INPUT * 2 + 1] = {};
        FT02_PinyinImeSegmentDisplay(g_searchPinyin, pyDisplay, sizeof(pyDisplay));
        char pyLine[96] = {};
        snprintf(pyLine, sizeof(pyLine), "拼音：%s", pyDisplay[0] ? pyDisplay : g_searchPinyin);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, pyLine, 44, 252, 40, 222, 720, 34);

        const size_t candidateCount = FT02_KnowledgeSearchCandidateCount();
        char candidateLine[300] = {};
        size_t used = 0;
        for(size_t slot = 0; slot < FT02_PINYIN_PAGE_SIZE; ++slot)
        {
            const size_t index = g_searchPinyinPage * FT02_PINYIN_PAGE_SIZE + slot;
            if(index >= candidateCount) break;
            char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
            if(!FT02_PinyinImeCandidate(g_searchPinyin, index, candidate, sizeof(candidate))) continue;
            const int wrote = snprintf(
                candidateLine + used,
                sizeof(candidateLine) - used,
                "%s%u.%s",
                used > 0 ? "  " : "",
                static_cast<unsigned>(slot + 1u),
                candidate
            );
            if(wrote <= 0 || static_cast<size_t>(wrote) >= sizeof(candidateLine) - used) break;
            used += static_cast<size_t>(wrote);
        }
        if(candidateLine[0] == '\0') snprintf(candidateLine, sizeof(candidateLine), "暂无中文候选；按 ENTER 直接输入这些字母");
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, candidateLine, 44, 290, 40, 260, 720, 34);

        if(candidateCount > 0)
        {
            const size_t pages = (candidateCount + FT02_PINYIN_PAGE_SIZE - 1u) / FT02_PINYIN_PAGE_SIZE;
            char pageLine[64] = {};
            snprintf(pageLine, sizeof(pageLine), "候选 %u/%u页   1-5选择   6/7翻候选",
                     static_cast<unsigned>(g_searchPinyinPage + 1u),
                     static_cast<unsigned>(pages));
            FT02_DrawTextPack(display, ft02_cjk_20r, pageLine, 44, 328);
        }
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "标题不用完全一致：输入其中一部分也能搜索。", 44, 260);
        FT02_DrawTextPack(display, ft02_cjk_20r, "同时搜索摘要、分类、正文和常用说法；长按SYM切换中英文。", 44, 302);
    }

    if(g_searchNotice[0] != '\0')
    {
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, g_searchNotice, 44, 374, 40, 342, 720, 36);
    }
    else if(g_searchQueryLength > 0 && g_searchPinyinLength == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "按 ENTER 开始搜索。", 44, 374);
    }

    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_SEARCH_BOTTOM, ft02_cjk_20r);
}

static void FT02_KnowledgeDrawEmptyFavoritesContent(FT02Display& display)
{
    FT02_KnowledgeDrawHeader(display, "我的收藏", "0 条");
    const int x = 92;
    const int y = 170;
    const int w = 616;
    const int h = 176;
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.drawRect(x + 3, y + 3, w - 6, h - 6, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "还没有收藏行动卡", x + 44, y + 62);
    FT02_DrawTextPack(display, ft02_cjk_20r, "打开任意行动卡，在阅读页按 F 即可收藏。", x + 44, y + 108);
    FT02_DrawTextPack(display, ft02_cjk_20r, "收藏会保存到 SD，重启后仍然保留。", x + 44, y + 144);
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_LIST_BOTTOM, ft02_cjk_20r);
}

static void FT02_KnowledgeDrawEmptyRecentContent(FT02Display& display)
{
    FT02_KnowledgeDrawHeader(display, "最近阅读", "0 条");
    const int x = 92;
    const int y = 170;
    const int w = 616;
    const int h = 176;
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.drawRect(x + 3, y + 3, w - 6, h - 6, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_24b, "还没有阅读记录", x + 44, y + 62);
    FT02_DrawTextPack(display, ft02_cjk_20r, "打开任意行动卡后，会自动记录卡片和最后阅读页。", x + 44, y + 108);
    FT02_DrawTextPack(display, ft02_cjk_20r, "记录会保存到 SD，重启后仍可继续阅读。", x + 44, y + 144);
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_LIST_BOTTOM, ft02_cjk_20r);
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
        if(g_listSource == FT02_KNOWLEDGE_LIST_FAVORITES)
        {
            FT02_KnowledgeDrawEmptyFavoritesContent(display);
            return;
        }
        if(g_listSource == FT02_KNOWLEDGE_LIST_RECENT)
        {
            FT02_KnowledgeDrawEmptyRecentContent(display);
            return;
        }
        FT02_KnowledgeSetError("暂无可用行动卡", "请检查当前列表或重新复制现场行动手册数据包。");
        return;
    }

    g_listSelected = FT02_KnowledgeClamp(g_listSelected, 0, count - 1);
    const int page = g_listSelected / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int pageCount = (count + FT02_KNOWLEDGE_LIST_PAGE_SIZE - 1) / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    char indicator[32];
    // The black selection row already makes the current item unambiguous. Keep
    // the header page-scoped so same-page LEFT/RIGHT moves can update only the
    // changed rows instead of repainting the whole 752x346 content region.
    snprintf(indicator, sizeof(indicator), "%d/%d页  共%d条",
             page + 1, pageCount, count);
    FT02_KnowledgeDrawHeader(display, FT02_KnowledgeListTitle(), indicator);

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
    FT02_DrawBottomBarWithFont(display, FT02_KNOWLEDGE_LIST_BOTTOM, ft02_cjk_20r);
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

static int FT02_KnowledgeReaderTextPageCount(const FT02FieldManualLoadedCard& card)
{
    int pages = 0;
    for(int sectionIndex = 0; sectionIndex < card.sectionCount; sectionIndex++)
    {
        pages += FT02_KnowledgeSectionPageCount(card.sections[sectionIndex].text);
    }
    return pages > 0 ? pages : 1;
}

static int FT02_KnowledgeReaderPageCount(const FT02FieldManualLoadedCard& card)
{
    const int globalCardIndex = FT02_FieldManualLoadedCardGlobalIndex();
    const int mediaPages = FT02_FieldManualMediaCountForCard(globalCardIndex);
    return mediaPages + FT02_KnowledgeReaderTextPageCount(card);
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

static void FT02_KnowledgeDrawBitmapFit1bpp(
    FT02Display& display,
    const uint8_t* data,
    int sourceWidth,
    int sourceHeight,
    int areaX,
    int areaY,
    int areaWidth,
    int areaHeight,
    bool allowUpscale = false
)
{
    if(data == nullptr || sourceWidth < 1 || sourceHeight < 1 ||
       areaWidth < 1 || areaHeight < 1)
    {
        return;
    }

    // Media stays 1:1 when it already fits. Split visual pages
    // may opt in to upscaling so tightly-cropped action art can use the full
    // pane instead of floating in a large empty rectangle.
    if(!allowUpscale && sourceWidth <= areaWidth && sourceHeight <= areaHeight)
    {
        const int x = areaX + (areaWidth - sourceWidth) / 2;
        const int y = areaY + (areaHeight - sourceHeight) / 2;
        display.drawBitmap(x, y, data, sourceWidth, sourceHeight, GxEPD_BLACK);
        return;
    }

    // Compatibility guard for older media packs. Scale down with nearest
    // neighbour so media can never escape the reader rectangle.
    int destWidth = areaWidth;
    int destHeight = static_cast<int>(
        (static_cast<int64_t>(sourceHeight) * areaWidth) / sourceWidth
    );
    if(destHeight > areaHeight)
    {
        destHeight = areaHeight;
        destWidth = static_cast<int>(
            (static_cast<int64_t>(sourceWidth) * areaHeight) / sourceHeight
        );
    }
    if(destWidth < 1) destWidth = 1;
    if(destHeight < 1) destHeight = 1;

    const int destX = areaX + (areaWidth - destWidth) / 2;
    const int destY = areaY + (areaHeight - destHeight) / 2;
    const int sourceRowBytes = (sourceWidth + 7) / 8;

    for(int dy = 0; dy < destHeight; dy++)
    {
        const int sy = static_cast<int>(
            (static_cast<int64_t>(dy) * sourceHeight) / destHeight
        );
        for(int dx = 0; dx < destWidth; dx++)
        {
            const int sx = static_cast<int>(
                (static_cast<int64_t>(dx) * sourceWidth) / destWidth
            );
            const uint8_t value = data[sy * sourceRowBytes + (sx >> 3)];
            if((value & (0x80U >> (sx & 7))) != 0)
            {
                display.drawPixel(destX + dx, destY + dy, GxEPD_BLACK);
            }
        }
        if((dy & 0x0F) == 0) yield();
    }
}

static bool FT02_KnowledgeDrawReaderMediaPage(
    FT02Display& display,
    int globalCardIndex,
    int mediaOrdinal
)
{
    if(!FT02_FieldManualLoadMediaForCard(globalCardIndex, mediaOrdinal))
    {
        return false;
    }
    const FT02FieldManualLoadedMedia* media = FT02_FieldManualLoadedMediaCurrent();
    if(media == nullptr || media->index == nullptr || media->data == nullptr)
    {
        return false;
    }

    const FT02FieldManualMediaIndex& info = *media->index;
    const char* caption = info.caption[0] != 0 ? info.caption : "图解";
    const bool captionBelow =
        (info.flags & FT02_FIELD_MANUAL_MEDIA_FLAG_CAPTION_BELOW) != 0;
    const bool splitLeft =
        (info.flags & FT02_FIELD_MANUAL_MEDIA_FLAG_SPLIT_LEFT) != 0;

    if(splitLeft)
    {
        // Use a large fitted illustration on the left and the
        // step caption on the right. This layout is intended for concrete
        // posture / structure / placement diagrams where the old 130px-high
        // banner made the useful subject too small. The bitmap is always fit
        // inside its pane, never cropped.
        const int mediaCount = FT02_FieldManualMediaCountForCard(globalCardIndex);
        const int contentX = FT02_KNOWLEDGE_READER_X + 16;
        const int contentY = FT02_KNOWLEDGE_READER_Y + 14;
        const int contentH = FT02_KNOWLEDGE_READER_H - 28;
        const int imageAreaH = contentH - 8;
        const int gap = 12;
        const int rightEdge = FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_W - 16;
        const int minTextW = 188;

        // The split ratio follows the actual media aspect ratio.
        // Narrow action art no longer floats inside a fixed 432 px pane,
        // while wide structure diagrams may grow up to 500 px. Text keeps
        // a readable minimum width.
        int imagePaneW = static_cast<int>(
            (static_cast<int64_t>(info.width) * imageAreaH) / info.height
        ) + 12;
        if(imagePaneW < 280) imagePaneW = 280;
        const int maxImagePaneW = rightEdge - contentX - gap - minTextW;
        if(imagePaneW > 500) imagePaneW = 500;
        if(imagePaneW > maxImagePaneW) imagePaneW = maxImagePaneW;

        const int dividerX = contentX + imagePaneW;
        const int textX = dividerX + gap;
        const int textW = rightEdge - textX;

        FT02_KnowledgeDrawBitmapFit1bpp(
            display,
            media->data,
            info.width,
            info.height,
            contentX + 4,
            contentY + 4,
            imagePaneW - 12,
            imageAreaH,
            true
        );

        display.drawLine(
            dividerX,
            contentY,
            dividerX,
            contentY + contentH,
            GxEPD_BLACK
        );

        char heading[32];
        snprintf(heading, sizeof(heading), "图解 %d/%d", mediaOrdinal + 1, mediaCount);
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_24b,
            heading,
            textX,
            contentY + 30,
            textX,
            contentY + 4,
            textW,
            36
        );

        display.fillRect(textX, contentY + 42, textW, 1, GxEPD_BLACK);
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_24b,
            "本步",
            textX,
            contentY + 74,
            textX,
            contentY + 48,
            textW,
            34
        );

        char line[128];
        const char* cursor = caption;
        int baseline = contentY + 111;
        for(int lineIndex = 0; lineIndex < 4 && *cursor; lineIndex++)
        {
            const char* next = cursor;
            FT02_KnowledgeNextWrappedLine(cursor, textW, line, sizeof(line), &next);
            // Split-page step text must use the exact same body
            // font and line spacing as the following pure-text reader pages.
            // Wrapping already measures glyphs with ft02_cjk_24r in
            // FT02_KnowledgeNextWrappedLine(), so drawing with 20r here made
            // the split caption visibly smaller and inconsistent.
            FT02_DrawTextPackClipped(
                display,
                ft02_cjk_24r,
                line,
                textX,
                baseline + lineIndex * FT02_KNOWLEDGE_READER_TEXT_LINE_HEIGHT,
                textX,
                contentY + 82,
                textW,
                contentH - 88
            );
            if(next <= cursor) break;
            cursor = next;
        }

        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_20r,
            "←/→继续",
            textX,
            contentY + contentH - 10,
            textX,
            contentY + contentH - 34,
            textW,
            28
        );
    }
    else if(captionBelow)
    {
        const int mediaCount = FT02_FieldManualMediaCountForCard(globalCardIndex);
        char heading[32];
        snprintf(heading, sizeof(heading), "图解 %d / %d", mediaOrdinal + 1, mediaCount);
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_24b,
            heading,
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

        // Step pages intentionally use a shorter image so the
        // instruction can sit directly below it. This is much easier to
        // follow on the 800x480 EPD than a dense all-steps overview.
        const int imageAreaX = FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X;
        const int imageAreaY = FT02_KNOWLEDGE_READER_Y + 72;
        const int imageAreaW = FT02_KNOWLEDGE_READER_TEXT_W;
        const int imageAreaH = 130;
        FT02_KnowledgeDrawBitmapFit1bpp(
            display,
            media->data,
            info.width,
            info.height,
            imageAreaX,
            imageAreaY,
            imageAreaW,
            imageAreaH
        );

        display.fillRect(
            FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X,
            FT02_KNOWLEDGE_READER_Y + 207,
            FT02_KNOWLEDGE_READER_TEXT_W,
            1,
            GxEPD_BLACK
        );
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_20r,
            caption,
            FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X,
            FT02_KNOWLEDGE_READER_Y + 239,
            FT02_KNOWLEDGE_READER_X + 10,
            FT02_KNOWLEDGE_READER_Y + 210,
            FT02_KNOWLEDGE_READER_W - 20,
            52
        );
    }
    else
    {
        FT02_DrawTextPackClipped(
            display,
            ft02_cjk_24b,
            caption,
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

        const int imageAreaX = FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_INSET_X;
        const int imageAreaY = FT02_KNOWLEDGE_READER_Y + 76;
        const int imageAreaW = FT02_KNOWLEDGE_READER_TEXT_W;
        const int imageAreaH = FT02_FIELD_MANUAL_MEDIA_MAX_HEIGHT;
        FT02_KnowledgeDrawBitmapFit1bpp(
            display,
            media->data,
            info.width,
            info.height,
            imageAreaX,
            imageAreaY,
            imageAreaW,
            imageAreaH
        );
    }
    return true;
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
    char indicator[40];
    const int globalCardIndex = FT02_FieldManualLoadedCardGlobalIndex();
    const bool favorite = FT02_FieldManualFavoritesIsFavorite(globalCardIndex);
    snprintf(indicator, sizeof(indicator), "%s  %d/%d", card->index->id, g_readerPage + 1, pageCount);
    FT02_KnowledgeDrawReaderHeader(display, card->index->title, indicator, favorite);

    display.drawRect(FT02_KNOWLEDGE_READER_X, FT02_KNOWLEDGE_READER_Y, FT02_KNOWLEDGE_READER_W, FT02_KNOWLEDGE_READER_H, GxEPD_BLACK);
    display.drawRect(FT02_KNOWLEDGE_READER_X + 2, FT02_KNOWLEDGE_READER_Y + 2, FT02_KNOWLEDGE_READER_W - 4, FT02_KNOWLEDGE_READER_H - 4, GxEPD_BLACK);

    const int mediaPages = FT02_FieldManualMediaCountForCard(globalCardIndex);
    bool mediaDrawn = false;
    if(g_readerPage < mediaPages)
    {
        mediaDrawn = FT02_KnowledgeDrawReaderMediaPage(display, globalCardIndex, g_readerPage);
        if(!mediaDrawn)
        {
            FT02_DrawTextPack(display, ft02_cjk_24b, "图解读取失败", FT02_KNOWLEDGE_READER_X + 24, FT02_KNOWLEDGE_READER_Y + 52);
            FT02_DrawTextPack(display, ft02_cjk_24r, "正文仍可继续阅读。", FT02_KNOWLEDGE_READER_X + 24, FT02_KNOWLEDGE_READER_Y + 108);
        }
    }
    else
    {
        const int textPage = g_readerPage - mediaPages;
        const char* section = card->sections[0].title;
        const char* pageText = card->sections[0].text;
        FT02_KnowledgeResolveReaderPage(*card, textPage, &section, &pageText);

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
    }

    // Keep the full 184 px image area clear. On image pages the bottom bar
    // already shows Z/C navigation, so the in-card text hint is omitted.
    if(g_readerPage >= mediaPages)
    {
        const char* hint = g_readerPage < pageCount - 1 ? "下一页继续" : "本卡结束";
        const int hintWidth = FT02_TextWidthPack(ft02_cjk_20r, hint);
        FT02_DrawTextPack(
            display,
            ft02_cjk_20r,
            hint,
            FT02_KNOWLEDGE_READER_X + FT02_KNOWLEDGE_READER_W - 22 - hintWidth,
            FT02_KNOWLEDGE_READER_Y + FT02_KNOWLEDGE_READER_H - 16
        );
    }

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
    FT02_DrawTextPack(display, ft02_cjk_24r, "收藏、搜索和最近阅读已经接入 FieldManual。", x + 124, y + 113);
    FT02_DrawTextPack(display, ft02_cjk_24r, "当前入口仍为后续扩展功能。", x + 124, y + 151);
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
        case FT02_KNOWLEDGE_SCREEN_SEARCH:
            FT02_KnowledgeDrawSearchContent(display);
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

// FieldManual navigation refresh policy:
//
// A CardKB press is available to InputManager immediately, but GxEPD2's
// full-window e-paper transaction is blocking. Redrawing the complete
// 800x480 frame after every selection move, so the main loop could not poll
// the next key until that full refresh finished.  Keep full refreshes for
// screen transitions, but use the panel's already proven partial-refresh path
// for in-screen navigation.
static const int FT02_KNOWLEDGE_PARTIAL_X = 24;
static const int FT02_KNOWLEDGE_PARTIAL_Y = 86;
static const int FT02_KNOWLEDGE_PARTIAL_W = 752;
static const int FT02_KNOWLEDGE_PARTIAL_H = 346;

static int FT02_KnowledgeMin(int a, int b)
{
    return a < b ? a : b;
}

static int FT02_KnowledgeMax(int a, int b)
{
    return a > b ? a : b;
}

static bool FT02_KnowledgeRectsIntersect(
    int ax, int ay, int aw, int ah,
    int bx, int by, int bw, int bh
)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static bool FT02_KnowledgeDirectional(const FT02InputEvent& event)
{
    return event.key == FT02_KEY_LEFT || event.key == FT02_KEY_RIGHT ||
           event.key == FT02_KEY_UP || event.key == FT02_KEY_DOWN;
}

static void FT02_KnowledgeRedrawContentPartial(FT02Display& display)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(
        FT02_KNOWLEDGE_PARTIAL_X,
        FT02_KNOWLEDGE_PARTIAL_Y,
        FT02_KNOWLEDGE_PARTIAL_W,
        FT02_KNOWLEDGE_PARTIAL_H
    );
    display.firstPage();
    do
    {
        display.fillRect(
            FT02_KNOWLEDGE_PARTIAL_X,
            FT02_KNOWLEDGE_PARTIAL_Y,
            FT02_KNOWLEDGE_PARTIAL_W,
            FT02_KNOWLEDGE_PARTIAL_H,
            GxEPD_WHITE
        );
        // The footer begins at y=440 and is therefore outside this window.
        // DrawCurrentContent can stay the single rendering authority while the
        // partial window clips writes to the header/body region only.
        FT02_KnowledgeDrawCurrentContent(display);
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display);
}

static void FT02_KnowledgeRedrawGridSelectionPartial(
    FT02Display& display,
    int oldSelected,
    int newSelected,
    bool categoryMode
)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    int oldX, oldY, oldW, oldH;
    int newX, newY, newW, newH;
    FT02_KnowledgeGridRect(oldSelected % FT02_KNOWLEDGE_GRID_PAGE_SIZE,
                           &oldX, &oldY, &oldW, &oldH);
    FT02_KnowledgeGridRect(newSelected % FT02_KNOWLEDGE_GRID_PAGE_SIZE,
                           &newX, &newY, &newW, &newH);

    int partialX = FT02_KnowledgeMin(oldX, newX) & ~7;
    const int partialY = FT02_KnowledgeMin(oldY, newY);
    const int partialRight = (FT02_KnowledgeMax(oldX + oldW, newX + newW) + 7) & ~7;
    const int partialBottom = FT02_KnowledgeMax(oldY + oldH, newY + newH);
    const int partialW = partialRight - partialX;
    const int partialH = partialBottom - partialY;

    display.setPartialWindow(partialX, partialY, partialW, partialH);
    display.firstPage();
    do
    {
        display.fillRect(partialX, partialY, partialW, partialH, GxEPD_WHITE);

        if(categoryMode)
        {
            const int pageStart = (newSelected / FT02_KNOWLEDGE_GRID_PAGE_SIZE) * FT02_KNOWLEDGE_GRID_PAGE_SIZE;
            const int count = FT02_FieldManualCategoryCount();
            const int pageEnd = FT02_KnowledgeClamp(
                pageStart + FT02_KNOWLEDGE_GRID_PAGE_SIZE,
                0,
                count
            );
            for(int index = pageStart; index < pageEnd; index++)
            {
                int x, y, w, h;
                FT02_KnowledgeGridRect(index % FT02_KNOWLEDGE_GRID_PAGE_SIZE, &x, &y, &w, &h);
                if(FT02_KnowledgeRectsIntersect(
                    x, y, w, h,
                    partialX, partialY, partialW, partialH
                ))
                {
                    FT02_KnowledgeDrawCategoryCard(display, index, index == newSelected);
                }
            }
        }
        else
        {
            for(int index = 0; index < FT02_KnowledgeHomeCount(); index++)
            {
                int x, y, w, h;
                FT02_KnowledgeGridRect(index, &x, &y, &w, &h);
                if(FT02_KnowledgeRectsIntersect(
                    x, y, w, h,
                    partialX, partialY, partialW, partialH
                ))
                {
                    FT02_KnowledgeDrawHomeCard(display, index, index == newSelected);
                }
            }
        }
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display);
}

static void FT02_KnowledgeRedrawListSelectionPartial(
    FT02Display& display,
    int oldSelected,
    int newSelected
)
{
    if(oldSelected == newSelected) return;
    const int oldPage = oldSelected / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int newPage = newSelected / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    if(oldPage != newPage)
    {
        FT02_KnowledgeRedrawContentPartial(display);
        return;
    }

    int oldX, oldY, oldW, oldH;
    int newX, newY, newW, newH;
    FT02_KnowledgeListRect(oldSelected % FT02_KNOWLEDGE_LIST_PAGE_SIZE,
                           &oldX, &oldY, &oldW, &oldH);
    FT02_KnowledgeListRect(newSelected % FT02_KNOWLEDGE_LIST_PAGE_SIZE,
                           &newX, &newY, &newW, &newH);

    const int partialX = FT02_KnowledgeMin(oldX, newX) & ~7;
    const int partialY = FT02_KnowledgeMin(oldY, newY);
    const int partialRight = (FT02_KnowledgeMax(oldX + oldW, newX + newW) + 7) & ~7;
    const int partialBottom = FT02_KnowledgeMax(oldY + oldH, newY + newH);
    const int partialW = partialRight - partialX;
    const int partialH = partialBottom - partialY;
    const int pageStart = newPage * FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int count = FT02_KnowledgeListCount();
    const int pageEnd = FT02_KnowledgeClamp(
        pageStart + FT02_KNOWLEDGE_LIST_PAGE_SIZE,
        0,
        count
    );

    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(partialX, partialY, partialW, partialH);
    display.firstPage();
    do
    {
        // Redraw every row touched by the union. This is important when queued
        // input jumps from row 1 to row 4 inside one coalesced BUSY burst.
        for(int localIndex = pageStart; localIndex < pageEnd; ++localIndex)
        {
            int x, y, w, h;
            FT02_KnowledgeListRect(localIndex - pageStart, &x, &y, &w, &h);
            if(!FT02_KnowledgeRectsIntersect(
                x, y, w, h,
                partialX, partialY, partialW, partialH
            ))
            {
                continue;
            }
            const int globalIndex = FT02_KnowledgeListGlobalIndex(localIndex);
            const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(globalIndex);
            if(card != nullptr)
            {
                FT02_KnowledgeDrawListCard(
                    display,
                    localIndex - pageStart,
                    *card,
                    localIndex == newSelected
                );
            }
        }
    }
    while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display);
}

static void FT02_KnowledgeApplyListSelection(
    FT02Display& display,
    int next
)
{
    const int count = FT02_KnowledgeListCount();
    if(count < 1) return;
    next = FT02_KnowledgeClamp(next, 0, count - 1);
    if(next == g_listSelected) return;

    const int old = g_listSelected;
    const int oldPage = old / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    const int newPage = next / FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    g_listSelected = next;
    if(oldPage == newPage)
    {
        FT02_KnowledgeRedrawListSelectionPartial(display, old, next);
    }
    else
    {
        FT02_KnowledgeRedrawContentPartial(display);
    }
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
                g_listSource = FT02_KNOWLEDGE_LIST_QUICK;
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
        else if(g_homeSelected == 2)
        {
            if(FT02_KnowledgeEnsureRuntime())
            {
                FT02_KnowledgeSearchResetInput(false);
                g_screen = FT02_KNOWLEDGE_SCREEN_SEARCH;
            }
        }
        else if(g_homeSelected == 3)
        {
            if(FT02_KnowledgeEnsureRuntime())
            {
                FT02_FieldManualFavoritesBegin();
                g_listSource = FT02_KNOWLEDGE_LIST_FAVORITES;
                g_listSelected = 0;
                g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
            }
        }
        else if(g_homeSelected == 4)
        {
            if(FT02_KnowledgeEnsureRuntime())
            {
                FT02_FieldManualRecentBegin();
                g_listSource = FT02_KNOWLEDGE_LIST_RECENT;
                g_listSelected = 0;
                g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
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
        const int old = g_homeSelected;
        g_homeSelected = next;
        FT02_KnowledgeRedrawGridSelectionPartial(display, old, next, false);
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
        g_listSource = FT02_KNOWLEDGE_LIST_CATEGORY;
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
        const int old = g_categorySelected;
        const int oldPage = old / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
        const int newPage = next / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
        g_categorySelected = next;
        if(oldPage == newPage)
        {
            FT02_KnowledgeRedrawGridSelectionPartial(display, old, next, true);
        }
        else
        {
            FT02_KnowledgeRedrawContentPartial(display);
        }
    }
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleSearch(FT02Display& display, const FT02InputEvent& event)
{
    const uint8_t raw = static_cast<uint8_t>(event.raw);

    // Text-entry surfaces must inspect event.raw before event.key because the
    // global InputManager intentionally maps D/Z/X/C/B to navigation hotkeys.
    // Search still needs those letters for Pinyin/English queries.
    if(event.key == FT02_KEY_IME_TOGGLE)
    {
        FT02_KnowledgeSearchToggleInputMode();
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(raw == 0x1Bu)
    {
        if(g_searchPinyinLength > 0)
        {
            FT02_KnowledgeSearchResetPinyin();
            FT02_KnowledgeSearchSetNotice("已取消当前拼音");
            FT02_KnowledgeSearchMarkDirty();
        }
        else
        {
            g_searchDirty = false;
            g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
            FT02_KnowledgeRedraw(display);
        }
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(raw == 0x08u || raw == 0x7Fu)
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
            FT02_KnowledgeSearchPinyinBackspace();
        else
            FT02_KnowledgeSearchUtf8Backspace();
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && (raw == '<' || raw == '6'))
    {
        FT02_KnowledgeSearchMovePinyinPage(-1);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && (raw == '>' || raw == '7'))
    {
        FT02_KnowledgeSearchMovePinyinPage(+1);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(raw == '\r' || raw == '\n')
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
        {
            // ENTER should remain intuitive when a Pinyin syllable has no
            // Chinese candidate (abbreviations, model names, English words).
            // Prefer the first Chinese candidate when one exists; otherwise
            // keep the typed letters directly. The old 0 shortcut remains
            // accepted for compatibility but no longer needs explanation.
            const bool committed = FT02_KnowledgeSearchCandidateCount() > 0
                ? FT02_KnowledgeSearchCommitPinyinDefault()
                : FT02_KnowledgeSearchCommitRawPinyin();
            if(!committed) FT02_KnowledgeSearchSetNotice("当前输入无法加入搜索词");
            FT02_KnowledgeSearchMarkDirty();
            return FT02_KNOWLEDGE_ACTION_NONE;
        }
        if(g_searchQueryLength == 0)
        {
            FT02_KnowledgeSearchSetNotice("请先输入搜索词");
            FT02_KnowledgeRedrawContentPartial(display);
            g_searchDirty = false;
            return FT02_KNOWLEDGE_ACTION_NONE;
        }

        FT02_KnowledgeSearchSetNotice("正在搜索...");
        if(!FT02_FieldManualSearchExecute(g_searchQuery))
        {
            FT02_KnowledgeSearchSetNotice("搜索失败，请检查行动手册数据");
            FT02_KnowledgeRedrawContentPartial(display);
            g_searchDirty = false;
            return FT02_KNOWLEDGE_ACTION_NONE;
        }
        const int count = FT02_FieldManualSearchResultCount();
        if(count < 1)
        {
            FT02_KnowledgeSearchSetNotice("没有找到匹配的行动卡，可缩短搜索词后重试");
            FT02_KnowledgeRedrawContentPartial(display);
            g_searchDirty = false;
            return FT02_KNOWLEDGE_ACTION_NONE;
        }

        char notice[72] = {};
        snprintf(notice, sizeof(notice), "找到 %d 张行动卡", count);
        FT02_KnowledgeSearchSetNotice(notice);
        g_listSource = FT02_KNOWLEDGE_LIST_SEARCH;
        g_listSelected = 0;
        g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
        g_searchDirty = false;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(raw == ' ')
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
        {
            (void)FT02_KnowledgeSearchCommitPinyinDefault();
        }
        else
        {
            (void)FT02_KnowledgeSearchAppendBytes(" ", 1);
        }
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && raw >= '1' && raw <= '5')
    {
        if(!FT02_KnowledgeSearchCommitCandidate(static_cast<size_t>(raw - '1')))
            FT02_KnowledgeSearchSetNotice("该候选不存在");
        FT02_KnowledgeSearchMarkDirty();
        return FT02_KNOWLEDGE_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && raw == '0')
    {
        (void)FT02_KnowledgeSearchCommitRawPinyin();
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if((raw >= 'a' && raw <= 'z') || (raw >= 'A' && raw <= 'Z'))
    {
        if(FT02_TextInputModeIsChinese())
            FT02_KnowledgeSearchAppendPinyinLetter(static_cast<char>(raw));
        else
        {
            const char one = static_cast<char>(raw);
            (void)FT02_KnowledgeSearchAppendBytes(&one, 1);
        }
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    if(raw >= 0x20u && raw < 0x7Fu)
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
            (void)FT02_KnowledgeSearchCommitPinyinDefault();
        const char one = static_cast<char>(raw);
        (void)FT02_KnowledgeSearchAppendBytes(&one, 1);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleList(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        if(g_listSource == FT02_KNOWLEDGE_LIST_CATEGORY)
            g_screen = FT02_KNOWLEDGE_SCREEN_CATEGORIES;
        else if(g_listSource == FT02_KNOWLEDGE_LIST_SEARCH)
            g_screen = FT02_KNOWLEDGE_SCREEN_SEARCH;
        else
            g_screen = FT02_KNOWLEDGE_SCREEN_HOME;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    const int count = FT02_KnowledgeListCount();
    if(count < 1) return FT02_KNOWLEDGE_ACTION_NONE;

    if(event.key == FT02_KEY_SELECT)
    {
        const int globalIndex = FT02_KnowledgeListGlobalIndex(g_listSelected);
        const int requestedPage = g_listSource == FT02_KNOWLEDGE_LIST_RECENT
            ? FT02_FieldManualRecentPageAt(g_listSelected)
            : 0;
        if(FT02_FieldManualLoadCard(globalIndex))
        {
            const FT02FieldManualLoadedCard* loaded = FT02_FieldManualLoadedCardCurrent();
            const int pageCount = loaded != nullptr ? FT02_KnowledgeReaderPageCount(*loaded) : 1;
            g_readerFeedback[0] = '\0';
            g_readerPage = FT02_KnowledgeClamp(requestedPage, 0, pageCount - 1);
            FT02_FieldManualRecentRecord(globalIndex, g_readerPage);
            if(g_listSource == FT02_KNOWLEDGE_LIST_RECENT) g_listSelected = 0;
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
    // List navigation follows spatial reading logic:
    // LEFT/RIGHT change page; UP/DOWN move one card inside/across pages.
    if(event.key == FT02_KEY_LEFT) next -= FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    else if(event.key == FT02_KEY_RIGHT) next += FT02_KNOWLEDGE_LIST_PAGE_SIZE;
    else if(event.key == FT02_KEY_UP) next--;
    else if(event.key == FT02_KEY_DOWN) next++;
    FT02_KnowledgeApplyListSelection(
        display,
        next);
    return FT02_KNOWLEDGE_ACTION_NONE;
}

static FT02KnowledgeAction FT02_KnowledgeHandleReader(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_BACK)
    {
        (void)FT02_FieldManualRecentFlush();
        FT02_FieldManualUnloadCard();
        g_screen = FT02_KNOWLEDGE_SCREEN_LIST;
        FT02_KnowledgeRedraw(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    const FT02FieldManualLoadedCard* card = FT02_FieldManualLoadedCardCurrent();
    if(card == nullptr) return FT02_KNOWLEDGE_ACTION_NONE;

    // F toggles the persisted favorite state for the current card. The
    // operation is Reader-local, so the same F key remains available for map
    // cache rebuild and other page-specific commands elsewhere in FT-02.
    if(event.command == 'f')
    {
        const int globalIndex = FT02_FieldManualLoadedCardGlobalIndex();
        bool favorite = false;
        if(FT02_FieldManualFavoritesToggle(globalIndex, &favorite))
        {
            // Success feedback is the state itself: the title-adjacent
            // `已收藏` badge appears for favorite cards and disappears after
            // removing the favorite. No transient banner is needed.
            g_readerFeedback[0] = '\0';
        }
        else
        {
            // Exceptional persistence failure uses the same title-adjacent
            // badge slot so pagination on the right is never obscured.
            snprintf(g_readerFeedback, sizeof(g_readerFeedback), "%s", "保存失败");
            FT02_LOGE("[ERROR][KNOWLEDGE-FAVORITES] toggle FAILED card=%s index=%d\n",
                          card->index->id,
                          globalIndex);
        }
        FT02_KnowledgeRedrawContentPartial(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    // `0` means "this card's first page", not FT-02 system Home.
    // Keep it Reader-local so number keys retain their normal meaning
    // everywhere else in FieldManual and the rest of the firmware.
    if(event.key == FT02_KEY_CHAR && event.command == '0')
    {
        if(g_readerPage != 0)
        {
            g_readerFeedback[0] = '\0';
            g_readerPage = 0;
            FT02_FieldManualRecentUpdatePage(FT02_FieldManualLoadedCardGlobalIndex(), g_readerPage);
            FT02_KnowledgeRedrawContentPartial(display);
        }
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    int next = g_readerPage;
    if(event.key == FT02_KEY_LEFT) next--;
    else if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_SELECT) next++;
    next = FT02_KnowledgeClamp(next, 0, FT02_KnowledgeReaderPageCount(*card) - 1);
    if(next != g_readerPage)
    {
        g_readerFeedback[0] = '\0';
        g_readerPage = next;
        FT02_FieldManualRecentUpdatePage(FT02_FieldManualLoadedCardGlobalIndex(), g_readerPage);
        FT02_KnowledgeRedrawContentPartial(display);
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

static FT02KnowledgeAction FT02_KnowledgeDispatchInputInternal(
    FT02Display& display,
    const FT02InputEvent& event
)
{
    switch(g_screen)
    {
        case FT02_KNOWLEDGE_SCREEN_HOME:
            return FT02_KnowledgeHandleHome(display, event);
        case FT02_KNOWLEDGE_SCREEN_CATEGORIES:
            return FT02_KnowledgeHandleCategories(display, event);
        case FT02_KNOWLEDGE_SCREEN_SEARCH:
            return FT02_KnowledgeHandleSearch(display, event);
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

static FT02KnowledgeAction FT02_KnowledgeProcessQueuedInputInternal(
    FT02Display& display,
    bool* outProcessed
)
{
    if(outProcessed != nullptr) *outProcessed = false;
    if(FT02_EpdInteractiveQueuedInputCount() == 0) return FT02_KNOWLEDGE_ACTION_NONE;

    FT02InputEvent first = {};
    if(!FT02_EpdInteractivePeekQueuedInput(first)) return FT02_KNOWLEDGE_ACTION_NONE;

    // Search deliberately treats D/Z/X/C as literal text even though the
    // global InputManager classifies them as directions. Never coalesce them
    // as navigation while the search editor owns the keyboard.
    const bool canBatchNavigation =
        g_screen == FT02_KNOWLEDGE_SCREEN_HOME ||
        g_screen == FT02_KNOWLEDGE_SCREEN_CATEGORIES ||
        g_screen == FT02_KNOWLEDGE_SCREEN_LIST ||
        g_screen == FT02_KNOWLEDGE_SCREEN_READER;

    if(canBatchNavigation && FT02_KnowledgeDirectional(first))
    {
        if(outProcessed != nullptr) *outProcessed = true;

        if(g_screen == FT02_KNOWLEDGE_SCREEN_HOME)
        {
            const int old = g_homeSelected;
            int next = old;
            FT02InputEvent event = {};
            while(FT02_EpdInteractivePeekQueuedInput(event) && FT02_KnowledgeDirectional(event))
            {
                (void)FT02_EpdInteractiveTakeQueuedInput(event);
                if(event.key == FT02_KEY_LEFT) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 0, -1);
                else if(event.key == FT02_KEY_RIGHT) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 0, 1);
                else if(event.key == FT02_KEY_UP) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), -1, 0);
                else if(event.key == FT02_KEY_DOWN) next = FT02_KnowledgeMoveGrid(next, FT02_KnowledgeHomeCount(), 1, 0);
            }
            if(next != old)
            {
                g_homeSelected = next;
                FT02_KnowledgeRedrawGridSelectionPartial(display, old, next, false);
            }

            return FT02_KNOWLEDGE_ACTION_NONE;
        }

        if(g_screen == FT02_KNOWLEDGE_SCREEN_CATEGORIES)
        {
            const int count = FT02_FieldManualCategoryCount();
            const int old = g_categorySelected;
            int next = old;
            FT02InputEvent event = {};
            while(FT02_EpdInteractivePeekQueuedInput(event) && FT02_KnowledgeDirectional(event))
            {
                (void)FT02_EpdInteractiveTakeQueuedInput(event);
                if(event.key == FT02_KEY_LEFT) next = FT02_KnowledgeMoveGrid(next, count, 0, -1);
                else if(event.key == FT02_KEY_RIGHT) next = FT02_KnowledgeMoveGrid(next, count, 0, 1);
                else if(event.key == FT02_KEY_UP) next = FT02_KnowledgeMoveGrid(next, count, -1, 0);
                else if(event.key == FT02_KEY_DOWN) next = FT02_KnowledgeMoveGrid(next, count, 1, 0);
            }
            if(next != old)
            {
                const int oldPage = old / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
                const int newPage = next / FT02_KNOWLEDGE_GRID_PAGE_SIZE;
                g_categorySelected = next;
                if(oldPage == newPage)
                    FT02_KnowledgeRedrawGridSelectionPartial(display, old, next, true);
                else
                    FT02_KnowledgeRedrawContentPartial(display);
            }

            return FT02_KNOWLEDGE_ACTION_NONE;
        }

        if(g_screen == FT02_KNOWLEDGE_SCREEN_LIST)
        {
            const int count = FT02_KnowledgeListCount();
            const int old = g_listSelected;
            int next = old;
            FT02InputEvent event = {};
            while(FT02_EpdInteractivePeekQueuedInput(event) && FT02_KnowledgeDirectional(event))
            {
                (void)FT02_EpdInteractiveTakeQueuedInput(event);
                if(event.key == FT02_KEY_LEFT) next -= FT02_KNOWLEDGE_LIST_PAGE_SIZE;
                else if(event.key == FT02_KEY_RIGHT) next += FT02_KNOWLEDGE_LIST_PAGE_SIZE;
                else if(event.key == FT02_KEY_UP) next--;
                else if(event.key == FT02_KEY_DOWN) next++;
                next = FT02_KnowledgeClamp(next, 0, count - 1);
            }
            FT02_KnowledgeApplyListSelection(display, next);

            return FT02_KNOWLEDGE_ACTION_NONE;
        }

        if(g_screen == FT02_KNOWLEDGE_SCREEN_READER)
        {
            const FT02FieldManualLoadedCard* card = FT02_FieldManualLoadedCardCurrent();
            if(card == nullptr)
            {
                FT02InputEvent discard = {};
                (void)FT02_EpdInteractiveTakeQueuedInput(discard);
                return FT02_KNOWLEDGE_ACTION_NONE;
            }
            const int pageCount = FT02_KnowledgeReaderPageCount(*card);
            const int old = g_readerPage;
            int next = old;
            FT02InputEvent event = {};
            while(FT02_EpdInteractivePeekQueuedInput(event) && FT02_KnowledgeDirectional(event))
            {
                (void)FT02_EpdInteractiveTakeQueuedInput(event);
                if(event.key == FT02_KEY_LEFT) next--;
                else if(event.key == FT02_KEY_RIGHT) next++;
                next = FT02_KnowledgeClamp(next, 0, pageCount - 1);
            }
            if(next != old)
            {
                g_readerFeedback[0] = '\0';
                g_readerPage = next;
                FT02_FieldManualRecentUpdatePage(FT02_FieldManualLoadedCardGlobalIndex(), g_readerPage);
                FT02_KnowledgeRedrawContentPartial(display);
            }

            return FT02_KNOWLEDGE_ACTION_NONE;
        }
    }

    // Do not consume non-navigation events here. The main loop will replay the
    // same oldest FIFO item through the normal page router before it ever polls
    // a newer live key. This preserves global HELP/BACK semantics outside the
    // search editor, while search still receives raw H/B/D/Z/X/C as text.
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
    g_listSource = FT02_KNOWLEDGE_LIST_QUICK;
    FT02_KnowledgeSearchResetInput(true);
    FT02_FieldManualFavoritesBegin();
    FT02_FieldManualRecentBegin();
}

void FT02_DrawKnowledgeScreen(FT02Display& display)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_KnowledgeDrawCurrentContent(display);
    }
    while(display.nextPage());
    FT02_EpdInteractivePowerOffNow(display);
    FT02_EpdInteractiveEndBusyCapture(display);
}

FT02KnowledgeAction FT02_KnowledgeProcessQueuedInput(
    FT02Display& display,
    bool* outProcessed
)
{
    return FT02_KnowledgeProcessQueuedInputInternal(display, outProcessed);
}

FT02KnowledgeAction FT02_KnowledgeService(FT02Display& display, uint32_t nowMs)
{
    FT02_FieldManualRecentPoll(nowMs);

    // E-paper text entry is intentionally deferred until the user pauses.
    // Keystrokes are accepted immediately in RAM; a 320 ms idle gap coalesces
    // a burst of typing into one partial waveform instead of one ~0.5 s panel
    // transaction per character. Keep the 320 ms settle gap while all
    // BUSY-captured keys now share the global strict-FIFO ordering path.
    if(g_screen == FT02_KNOWLEDGE_SCREEN_SEARCH && g_searchDirty &&
       static_cast<uint32_t>(nowMs - g_searchLastEditMs) >= FT02_KNOWLEDGE_SEARCH_REDRAW_IDLE_MS)
    {
        g_searchDirty = false;
        FT02_KnowledgeRedrawContentPartial(display);
        return FT02_KNOWLEDGE_ACTION_NONE;
    }

    return FT02_KNOWLEDGE_ACTION_NONE;
}

bool FT02_KnowledgeTextInputActive()
{
    return g_screen == FT02_KNOWLEDGE_SCREEN_SEARCH;
}

FT02KnowledgeAction FT02_HandleKnowledgeInput(FT02Display& display, const FT02InputEvent& event)
{
    return FT02_KnowledgeDispatchInputInternal(display, event);
}
