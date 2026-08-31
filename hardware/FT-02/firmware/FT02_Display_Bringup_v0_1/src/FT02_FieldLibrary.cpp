#include "FT02_FieldLibrary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"

#include "FT02_BottomBar.h"
#include "FT02_EpdInteractiveRuntime.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_PinyinIme.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_Storage.h"
#include "FT02_TextInputMode.h"

namespace
{

static const char* FT02_LIBRARY_CATALOG_PATH = "/library/catalog.tsv";
static const char* FT02_LIBRARY_PROGRESS_DIR = "/lanternbox/library";
static const char* FT02_LIBRARY_RECENT_PATH = "/lanternbox/library/recent.txt";
static const char* FT02_LIBRARY_LEGACY_PROGRESS_DIR = "/library/user";
static const char* FT02_LIBRARY_LEGACY_RECENT_PATH = "/library/user/recent.txt";

static const int FT02_LIBRARY_MAX_BOOKS = 96;
static const int FT02_LIBRARY_MAX_CATEGORIES = 16;
static const int FT02_LIBRARY_MAX_CHAPTERS = 256;
static const int FT02_LIBRARY_MAX_SEARCH_RESULTS = 48;
static const int FT02_LIBRARY_MAX_MEDIA = 2048;
static const int FT02_LIBRARY_LIST_PAGE_SIZE = 4;
static const int FT02_LIBRARY_READER_LINES = 10;
static const int FT02_LIBRARY_READER_LINE_BYTES = 192;
static const int FT02_LIBRARY_READER_TEXT_W = 760;
static const int FT02_LIBRARY_READER_MAX_HISTORY = 96;
static const int FT02_LIBRARY_QUERY_MAX_BYTES = 72;

// Reading surfaces deliberately keep only the global top status bar.
// The old 48 px bottom command bar is removed in READER/MEDIA modes and the
// reclaimed pixels become real book content.
static const int FT02_LIBRARY_READING_PARTIAL_X = 0;
static const int FT02_LIBRARY_READING_PARTIAL_Y = 86;
static const int FT02_LIBRARY_READING_PARTIAL_W = 800;
static const int FT02_LIBRARY_READING_PARTIAL_H = 394;
static const uint16_t FT02_LIBRARY_READING_PARTIAL_LIMIT = 240;
static const int FT02_LIBRARY_READER_BODY_X = 20;
static const int FT02_LIBRARY_READER_BODY_Y = 145;
static const int FT02_LIBRARY_READER_BODY_W = 760;
static const int FT02_LIBRARY_READER_BODY_H = 331;
static const int FT02_LIBRARY_READER_LINE_ADVANCE = 30;
static const int FT02_LIBRARY_READER_PARAGRAPH_GAP = 4;
static const int FT02_LIBRARY_INLINE_IMAGE_MIN_H = 220;
static const int FT02_LIBRARY_INLINE_CAPTION_H = 24;
static const int FT02_LIBRARY_INLINE_IMAGE_Y = FT02_LIBRARY_READER_BODY_Y + FT02_LIBRARY_INLINE_CAPTION_H + 4;
static const int FT02_LIBRARY_INLINE_IMAGE_H = FT02_LIBRARY_READER_BODY_H - FT02_LIBRARY_INLINE_CAPTION_H - 4;
static const int FT02_LIBRARY_INLINE_IMAGE_SCROLL_OVERLAP = 26;
static const int FT02_LIBRARY_MEDIA_X = 8;
static const int FT02_LIBRARY_MEDIA_W = 784;
static const int FT02_LIBRARY_MEDIA_SCAN_Y = 124;
static const int FT02_LIBRARY_MEDIA_SCAN_H = 352;
static const int FT02_LIBRARY_MEDIA_EPUB_Y = 142;
static const int FT02_LIBRARY_MEDIA_EPUB_H = 334;

struct FT02LibraryLocation
{
    int chapter;
    uint64_t offset;
    bool mediaMode;
    bool fitPage;
    int inlineScrollY;
};

enum FT02LibraryScreen : uint8_t
{
    FT02_LIBRARY_SCREEN_HOME = 0,
    FT02_LIBRARY_SCREEN_CATEGORIES,
    FT02_LIBRARY_SCREEN_BOOKS,
    FT02_LIBRARY_SCREEN_TOC,
    FT02_LIBRARY_SCREEN_READER,
    FT02_LIBRARY_SCREEN_MEDIA_READER,
    FT02_LIBRARY_SCREEN_SEARCH,
    FT02_LIBRARY_SCREEN_SEARCH_RESULTS,
    FT02_LIBRARY_SCREEN_ERROR
};

struct FT02LibraryBook
{
    char id[20];
    char title[72];
    char author[48];
    char category[40];
    char priority[8];
    char format[12];
    char root[128];
    char tocPath[144];
    char legacyPath[144];
};

struct FT02LibraryCategory
{
    char name[40];
};

struct FT02LibraryChapter
{
    char title[96];
    char path[160];
    int mediaTarget;
};

struct FT02LibraryMedia
{
    int chapterIndex;
    uint16_t width;
    uint16_t height;
    char caption[96];
    char path[160];
};

struct FT02LibrarySearchResult
{
    int bookIndex;
    int chapterIndex;
    uint64_t offset;
    char chapterTitle[96];
    char snippet[112];
};

// Large library metadata must not live in internal DRAM .bss.
// FT-02 N16R8 has 8 MB OPI PSRAM; allocate cold library indexes there at runtime.
// This keeps the internal DRAM budget available for display, SD, audio, network and
// other latency-sensitive runtimes.
static FT02LibraryBook* g_books = nullptr;
static FT02LibraryCategory g_categories[FT02_LIBRARY_MAX_CATEGORIES] = {};
static FT02LibraryChapter* g_chapters = nullptr;
static FT02LibrarySearchResult* g_searchResults = nullptr;
static FT02LibraryMedia* g_media = nullptr;
static int g_bookCount = 0;
static int g_categoryCount = 0;
static int g_chapterCount = 0;
static int g_searchResultCount = 0;
static int g_mediaCount = 0;
static bool g_started = false;
static bool g_ready = false;
static FT02LibraryScreen g_screen = FT02_LIBRARY_SCREEN_HOME;
static FT02LibraryScreen g_tocReturnScreen = FT02_LIBRARY_SCREEN_READER;
static int g_homeSelected = 0;
static int g_categorySelected = 0;
static int g_bookSelected = 0;
static int g_tocSelected = 0;
static int g_searchSelected = 0;
static int g_activeCategory = -1;
static int g_activeBook = -1;
static int g_activeChapter = 0;
static int g_activeMedia = 0;
static int g_mediaScrollY = 0;
static bool g_mediaFitPage = false;
static bool g_mediaOpenedFromText = false;
static uint16_t g_readingPartialCount = 0;
static FILE* g_readerFile = nullptr;
static uint64_t g_readerFileSize = 0;
static uint64_t g_readerOffset = 0;
static uint64_t g_readerNextOffset = 0;
static FT02LibraryLocation g_readerHistory[FT02_LIBRARY_READER_MAX_HISTORY] = {};
static int g_readerHistoryCount = 0;
static char g_readerLines[FT02_LIBRARY_READER_LINES][FT02_LIBRARY_READER_LINE_BYTES] = {};
static bool g_readerParagraphAfter[FT02_LIBRARY_READER_LINES] = {};
static int g_readerLineCount = 0;
static int g_readerInlineMedia = -1;
static int g_readerInlineScrollY = 0;
static bool g_readingHeaderDirty = true;
static char g_errorTitle[48] = "书库不可用";
static char g_errorDetail[128] = "请检查 SD 卡 /library 数据。";

static char g_searchQuery[FT02_LIBRARY_QUERY_MAX_BYTES + 1] = {};
static size_t g_searchQueryLength = 0;
static char g_searchPinyin[FT02_PINYIN_MAX_INPUT + 1] = {};
static size_t g_searchPinyinLength = 0;
static size_t g_searchPinyinPage = 0;
static char g_searchNotice[80] = {};

static const FT02BottomBarItem FT02_LIBRARY_HOME_BOTTOM[3] = {
    {nullptr, "方向选择"}, {nullptr, "ENTER进入"}, {nullptr, "B返回"}
};
static const FT02BottomBarItem FT02_LIBRARY_LIST_BOTTOM[3] = {
    {nullptr, "左右 翻页"}, {nullptr, "上下 选择"}, {nullptr, "ENTER打开 B返回"}
};
static const FT02BottomBarItem FT02_LIBRARY_SEARCH_BOTTOM[3] = {
    {nullptr, "`中/EN"}, {nullptr, "ENTER搜索 DEL删除"}, {nullptr, "ESC返回"}
};
static const FT02BottomBarItem FT02_LIBRARY_ERROR_BOTTOM[3] = {
    {nullptr, "随身书库"}, {nullptr, "ENTER重载"}, {nullptr, "B返回"}
};

static bool FT02_LibraryDrawPbm(FT02Display& display, const FT02LibraryMedia& media,
                                int x, int y, int areaW, int areaH, bool fitPage, int scrollY);

static int FT02_LibraryClamp(int value, int low, int high)
{
    if(value < low) return low;
    if(value > high) return high;
    return value;
}

static char* FT02_LibraryTrim(char* text)
{
    if(text == nullptr) return text;
    while(*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') text++;
    size_t len = strlen(text);
    while(len > 0)
    {
        const char c = text[len - 1];
        if(c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        text[--len] = '\0';
    }
    return text;
}

static unsigned char FT02_LibraryFoldAscii(unsigned char value)
{
    return value >= 'A' && value <= 'Z' ? static_cast<unsigned char>(value - 'A' + 'a') : value;
}

static bool FT02_LibraryContainsFold(const char* text, const char* term)
{
    if(text == nullptr || term == nullptr || term[0] == '\0') return false;
    for(const char* start = text; *start; ++start)
    {
        const char* a = start;
        const char* b = term;
        while(*a && *b && FT02_LibraryFoldAscii(static_cast<unsigned char>(*a)) ==
                          FT02_LibraryFoldAscii(static_cast<unsigned char>(*b)))
        {
            ++a;
            ++b;
        }
        if(*b == '\0') return true;
    }
    return false;
}

static void FT02_LibraryCloseReaderFile()
{
    if(g_readerFile != nullptr)
    {
        fclose(g_readerFile);
        g_readerFile = nullptr;
    }
    g_readerFileSize = 0;
    g_readerOffset = 0;
    g_readerNextOffset = 0;
    g_readerLineCount = 0;
}

static void FT02_LibraryCloseReader()
{
    FT02_LibraryCloseReaderFile();
    g_readerHistoryCount = 0;
}

static void FT02_LibrarySetError(const char* title, const char* detail)
{
    snprintf(g_errorTitle, sizeof(g_errorTitle), "%s", title ? title : "书库不可用");
    snprintf(g_errorDetail, sizeof(g_errorDetail), "%s", detail ? detail : "请检查 SD 卡数据。");
    g_ready = false;
    g_screen = FT02_LIBRARY_SCREEN_ERROR;
}

static bool FT02_LibraryEnsurePsramBuffers()
{
    if(g_books != nullptr && g_chapters != nullptr && g_searchResults != nullptr && g_media != nullptr) return true;

    // Never silently fall back to internal DRAM here. The whole point of this
    // allocation policy is to keep these large, cold caches out of dram0.bss /
    // internal heap. If PSRAM is unavailable, fail the module cleanly instead.
    if(g_books == nullptr)
        g_books = static_cast<FT02LibraryBook*>(heap_caps_calloc(
            FT02_LIBRARY_MAX_BOOKS, sizeof(FT02LibraryBook), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if(g_chapters == nullptr)
        g_chapters = static_cast<FT02LibraryChapter*>(heap_caps_calloc(
            FT02_LIBRARY_MAX_CHAPTERS, sizeof(FT02LibraryChapter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if(g_searchResults == nullptr)
        g_searchResults = static_cast<FT02LibrarySearchResult*>(heap_caps_calloc(
            FT02_LIBRARY_MAX_SEARCH_RESULTS, sizeof(FT02LibrarySearchResult), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if(g_media == nullptr)
        g_media = static_cast<FT02LibraryMedia*>(heap_caps_calloc(
            FT02_LIBRARY_MAX_MEDIA, sizeof(FT02LibraryMedia), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if(g_books == nullptr || g_chapters == nullptr || g_searchResults == nullptr || g_media == nullptr)
    {
        if(g_books != nullptr) { heap_caps_free(g_books); g_books = nullptr; }
        if(g_chapters != nullptr) { heap_caps_free(g_chapters); g_chapters = nullptr; }
        if(g_searchResults != nullptr) { heap_caps_free(g_searchResults); g_searchResults = nullptr; }
        if(g_media != nullptr) { heap_caps_free(g_media); g_media = nullptr; }
        FT02_LibrarySetError("书库内存不足", "PSRAM 分配失败，请检查 N16R8/PSRAM 配置。");
        Serial.printf("[FIELD-LIBRARY] PSRAM alloc failed, free_psram=%u\n",
                      static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
        return false;
    }

    const size_t bytes = FT02_LIBRARY_MAX_BOOKS * sizeof(FT02LibraryBook)
                       + FT02_LIBRARY_MAX_CHAPTERS * sizeof(FT02LibraryChapter)
                       + FT02_LIBRARY_MAX_SEARCH_RESULTS * sizeof(FT02LibrarySearchResult)
                       + FT02_LIBRARY_MAX_MEDIA * sizeof(FT02LibraryMedia);
    Serial.printf("[FIELD-LIBRARY] PSRAM buffers=%u bytes, free_psram=%u\n",
                  static_cast<unsigned>(bytes),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    return true;
}

static bool FT02_LibraryCategoryExists(const char* name)
{
    for(int i = 0; i < g_categoryCount; i++)
        if(strcmp(g_categories[i].name, name) == 0) return true;
    return false;
}

static void FT02_LibraryAddCategory(const char* name)
{
    if(name == nullptr || name[0] == '\0' || g_categoryCount >= FT02_LIBRARY_MAX_CATEGORIES) return;
    if(FT02_LibraryCategoryExists(name)) return;
    snprintf(g_categories[g_categoryCount++].name, sizeof(g_categories[0].name), "%s", name);
}

static bool FT02_LibraryLoadCatalog()
{
    g_bookCount = 0;
    g_categoryCount = 0;
    if(!FT02_StorageIsReady())
    {
        FT02_LibrarySetError("SD卡不可用", "书库需要 SD 卡，请检查存储状态。");
        return false;
    }
    FILE* file = FT02_StorageOpenReadFile(FT02_LIBRARY_CATALOG_PATH);
    if(file == nullptr)
    {
        FT02_LibrarySetError("未找到书库", "请复制 /library/catalog.tsv 到 SD 卡根目录。");
        return false;
    }

    char line[640];
    while(fgets(line, sizeof(line), file) != nullptr && g_bookCount < FT02_LIBRARY_MAX_BOOKS)
    {
        char* row = FT02_LibraryTrim(line);
        if(row[0] == '\0' || row[0] == '#') continue;
        char* fields[8] = {};
        int fieldCount = 0;
        fields[fieldCount++] = row;
        for(char* cursor = row; *cursor && fieldCount < 8; ++cursor)
        {
            if(*cursor == '\t')
            {
                *cursor = '\0';
                fields[fieldCount++] = cursor + 1;
            }
        }
        for(int i = 0; i < fieldCount; i++) fields[i] = FT02_LibraryTrim(fields[i]);
        if(fieldCount < 5 || fields[0][0] == '\0' || fields[1][0] == '\0') continue;

        FT02LibraryBook& book = g_books[g_bookCount];
        snprintf(book.id, sizeof(book.id), "%s", fields[0]);
        snprintf(book.title, sizeof(book.title), "%s", fields[1]);
        snprintf(book.author, sizeof(book.author), "%s", fields[2]);
        snprintf(book.category, sizeof(book.category), "%s", fields[3]);
        if(fieldCount >= 8)
        {
            snprintf(book.priority, sizeof(book.priority), "%s", fields[4]);
            snprintf(book.format, sizeof(book.format), "%s", fields[5]);
            snprintf(book.root, sizeof(book.root), "%s", fields[6]);
            snprintf(book.tocPath, sizeof(book.tocPath), "%s", fields[7]);
            book.legacyPath[0] = '\0';
        }
        else
        {
            // v0.1 compatibility: id/title/author/category/path.
            snprintf(book.priority, sizeof(book.priority), "P0");
            snprintf(book.format, sizeof(book.format), "txt");
            book.root[0] = '\0';
            book.tocPath[0] = '\0';
            snprintf(book.legacyPath, sizeof(book.legacyPath), "%s", fields[4]);
        }

        // The first production batch is intentionally P0-only. Ignore lower
        // priority data if an older/experimental catalog contains it.
        if(book.priority[0] && strcmp(book.priority, "P0") != 0) continue;
        FT02_LibraryAddCategory(book.category);
        g_bookCount++;
    }
    fclose(file);

    if(g_bookCount <= 0)
    {
        FT02_LibrarySetError("P0书库为空", "catalog.tsv 中没有可读取的 P0 书籍。");
        return false;
    }
    g_ready = true;
    return true;
}

static int FT02_LibraryBookCountForCategory(int categoryIndex)
{
    if(categoryIndex < 0) return g_bookCount;
    if(categoryIndex >= g_categoryCount) return 0;
    int count = 0;
    for(int i = 0; i < g_bookCount; i++)
        if(strcmp(g_books[i].category, g_categories[categoryIndex].name) == 0) count++;
    return count;
}

static int FT02_LibraryGlobalBookIndex(int categoryIndex, int visibleIndex)
{
    if(visibleIndex < 0) return -1;
    if(categoryIndex < 0) return visibleIndex < g_bookCount ? visibleIndex : -1;
    int seen = 0;
    for(int i = 0; i < g_bookCount; i++)
    {
        if(strcmp(g_books[i].category, g_categories[categoryIndex].name) != 0) continue;
        if(seen++ == visibleIndex) return i;
    }
    return -1;
}

static bool FT02_LibraryLoadToc(int bookIndex)
{
    g_chapterCount = 0;
    if(bookIndex < 0 || bookIndex >= g_bookCount) return false;
    const FT02LibraryBook& book = g_books[bookIndex];
    if(book.tocPath[0] == '\0')
    {
        if(book.legacyPath[0] == '\0') return false;
        snprintf(g_chapters[0].title, sizeof(g_chapters[0].title), "正文");
        snprintf(g_chapters[0].path, sizeof(g_chapters[0].path), "%s", book.legacyPath);
        g_chapters[0].mediaTarget = -1;
        g_chapterCount = 1;
        return true;
    }
    FILE* file = FT02_StorageOpenReadFile(book.tocPath);
    if(file == nullptr) return false;
    char line[512];
    while(fgets(line, sizeof(line), file) != nullptr && g_chapterCount < FT02_LIBRARY_MAX_CHAPTERS)
    {
        char* row = FT02_LibraryTrim(line);
        if(row[0] == '\0' || row[0] == '#') continue;
        char* a = strchr(row, '\t');
        if(a == nullptr) continue;
        *a++ = '\0';
        char* b = strchr(a, '\t');
        if(b == nullptr) continue;
        *b++ = '\0';
        char* title = FT02_LibraryTrim(a);
        char* path = FT02_LibraryTrim(b);
        if(path[0] == '\0') continue;
        snprintf(g_chapters[g_chapterCount].title, sizeof(g_chapters[0].title), "%s",
                 title[0] ? title : "未命名章节");
        g_chapters[g_chapterCount].mediaTarget = -1;
        if(strncmp(path, "@media:", 7) == 0)
        {
            g_chapters[g_chapterCount].path[0] = '\0';
            g_chapters[g_chapterCount].mediaTarget = atoi(path + 7);
        }
        else
        {
            snprintf(g_chapters[g_chapterCount].path, sizeof(g_chapters[0].path), "%s", path);
        }
        g_chapterCount++;
    }
    fclose(file);
    return g_chapterCount > 0;
}

static bool FT02_LibraryBookIsScanPdf(int bookIndex)
{
    return bookIndex >= 0 && bookIndex < g_bookCount &&
           strcmp(g_books[bookIndex].format, "scanpdf") == 0;
}

static bool FT02_LibraryLoadMediaIndex(int bookIndex)
{
    g_mediaCount = 0;
    if(bookIndex < 0 || bookIndex >= g_bookCount) return false;
    const FT02LibraryBook& book = g_books[bookIndex];
    if(book.root[0] == '\0') return false;

    char path[176];
    snprintf(path, sizeof(path), "%s/media.tsv", book.root);
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;

    char line[640];
    while(fgets(line, sizeof(line), file) != nullptr && g_mediaCount < FT02_LIBRARY_MAX_MEDIA)
    {
        char* row = FT02_LibraryTrim(line);
        if(row[0] == '\0' || row[0] == '#') continue;
        char* fields[6] = {};
        int fieldCount = 0;
        fields[fieldCount++] = row;
        for(char* cursor = row; *cursor && fieldCount < 6; ++cursor)
        {
            if(*cursor == '\t')
            {
                *cursor = '\0';
                fields[fieldCount++] = cursor + 1;
            }
        }
        if(fieldCount < 6) continue;
        for(int i = 0; i < fieldCount; ++i) fields[i] = FT02_LibraryTrim(fields[i]);
        FT02LibraryMedia& item = g_media[g_mediaCount];
        item.chapterIndex = atoi(fields[1]);
        snprintf(item.caption, sizeof(item.caption), "%s", fields[2][0] ? fields[2] : "图片");
        snprintf(item.path, sizeof(item.path), "%s", fields[3]);
        item.width = static_cast<uint16_t>(atoi(fields[4]));
        item.height = static_cast<uint16_t>(atoi(fields[5]));
        if(item.path[0] == '\0' || item.width == 0 || item.height == 0) continue;
        g_mediaCount++;
    }
    fclose(file);
    return g_mediaCount > 0;
}

static int FT02_LibraryFirstMediaForChapter(int chapterIndex)
{
    for(int i = 0; i < g_mediaCount; ++i)
        if(g_media[i].chapterIndex == chapterIndex) return i;
    return -1;
}

static void FT02_LibraryMediaSetIndex(int index)
{
    if(g_mediaCount <= 0) return;
    g_activeMedia = FT02_LibraryClamp(index, 0, g_mediaCount - 1);
    g_mediaScrollY = 0;
}

static void FT02_LibraryMediaScroll(int delta)
{
    if(g_mediaFitPage || g_activeMedia < 0 || g_activeMedia >= g_mediaCount) return;
    const int areaW = FT02_LIBRARY_MEDIA_W;
    const int areaH = FT02_LibraryBookIsScanPdf(g_activeBook)
        ? FT02_LIBRARY_MEDIA_SCAN_H : FT02_LIBRARY_MEDIA_EPUB_H;
    // Fit-width now always uses the full reading width, including for EPUB
    // illustrations that are smaller than the panel.
    const FT02LibraryMedia& media = g_media[g_activeMedia];
    const int renderedH = media.width > 0
        ? static_cast<int>((static_cast<int64_t>(media.height) * areaW) / media.width) : 0;
    const int maxScroll = renderedH > areaH ? renderedH - areaH : 0;
    g_mediaScrollY = FT02_LibraryClamp(g_mediaScrollY + delta, 0, maxScroll);
}

static void FT02_LibraryProgressPath(const FT02LibraryBook& book, char* out, size_t outBytes)
{
    snprintf(out, outBytes, "%s/progress_%s.txt", FT02_LIBRARY_PROGRESS_DIR, book.id);
}

static void FT02_LibraryLegacyProgressPath(const FT02LibraryBook& book, char* out, size_t outBytes)
{
    snprintf(out, outBytes, "%s/progress_%s.txt", FT02_LIBRARY_LEGACY_PROGRESS_DIR, book.id);
}

static FT02LibraryLocation FT02_LibraryLoadProgress(const FT02LibraryBook& book)
{
    FT02LibraryLocation loc = {0, 0, false, false, 0};
    char path[96];
    FT02_LibraryProgressPath(book, path, sizeof(path));
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr)
    {
        FT02_LibraryLegacyProgressPath(book, path, sizeof(path));
        file = FT02_StorageOpenReadFile(path);
        if(file == nullptr) return loc;
    }
    char line[64] = {};
    if(fgets(line, sizeof(line), file) != nullptr)
    {
        if(line[0] == 'P' && line[1] == '\t')
        {
            char* first = line + 2;
            char* tab = strchr(first, '\t');
            if(tab)
            {
                *tab++ = '\0';
                loc.chapter = atoi(first); // media/page index
                char* tab2 = strchr(tab, '\t');
                if(tab2)
                {
                    *tab2++ = '\0';
                    loc.offset = strtoull(tab, nullptr, 10); // vertical scroll
                    loc.fitPage = atoi(tab2) != 0;
                }
                else loc.offset = strtoull(tab, nullptr, 10);
                loc.mediaMode = true;
            }
            fclose(file);
            return loc;
        }
        if(line[0] == 'T' && line[1] == '\t')
        {
            char* first = line + 2;
            char* tab = strchr(first, '\t');
            if(tab)
            {
                *tab++ = '\0';
                loc.chapter = atoi(first);
                char* tab2 = strchr(tab, '\t');
                if(tab2)
                {
                    *tab2++ = '\0';
                    loc.inlineScrollY = atoi(tab2);
                }
                loc.offset = strtoull(tab, nullptr, 10);
            }
            fclose(file);
            return loc;
        }
        char* tab = strchr(line, '\t');
        if(tab)
        {
            *tab++ = '\0';
            loc.chapter = atoi(line);
            loc.offset = strtoull(tab, nullptr, 10);
        }
        else
        {
            // v0.1 byte-only progress compatibility.
            loc.offset = strtoull(line, nullptr, 10);
        }
    }
    fclose(file);
    return loc;
}

static void FT02_LibrarySaveProgress()
{
    if(g_activeBook < 0 || g_activeBook >= g_bookCount) return;
    char path[96];
    FT02_LibraryProgressPath(g_books[g_activeBook], path, sizeof(path));
    FILE* file = FT02_StorageOpenWriteFile(path, true);
    if(file)
    {
        if(g_screen == FT02_LIBRARY_SCREEN_MEDIA_READER && FT02_LibraryBookIsScanPdf(g_activeBook))
        {
            fprintf(file, "P\t%d\t%d\t%d\n", g_activeMedia, g_mediaScrollY, g_mediaFitPage ? 1 : 0);
        }
        else
        {
            fprintf(file, "T\t%d\t%llu\t%d\n", g_activeChapter,
                    static_cast<unsigned long long>(g_readerOffset), g_readerInlineScrollY);
        }
        FT02_StorageSyncFile(file);
        fclose(file);
    }
    FILE* recent = FT02_StorageOpenWriteFile(FT02_LIBRARY_RECENT_PATH, true);
    if(recent)
    {
        if(g_screen == FT02_LIBRARY_SCREEN_MEDIA_READER && FT02_LibraryBookIsScanPdf(g_activeBook))
            fprintf(recent, "%s\tP\t%d\t%d\n", g_books[g_activeBook].id, g_activeMedia, g_mediaScrollY);
        else
            fprintf(recent, "%s\tT\t%d\t%llu\t%d\n", g_books[g_activeBook].id, g_activeChapter,
                    static_cast<unsigned long long>(g_readerOffset), g_readerInlineScrollY);
        FT02_StorageSyncFile(recent);
        fclose(recent);
    }
}

static void FT02_LibraryMigrateLegacyProgress()
{
    if(g_bookCount <= 0) return;

    for(int i = 0; i < g_bookCount; ++i)
    {
        char current[96];
        char legacy[96];
        FT02_LibraryProgressPath(g_books[i], current, sizeof(current));
        if(FT02_StorageFileExists(current)) continue;
        FT02_LibraryLegacyProgressPath(g_books[i], legacy, sizeof(legacy));
        FILE* in = FT02_StorageOpenReadFile(legacy);
        if(in == nullptr) continue;
        char line[96] = {};
        const bool got = fgets(line, sizeof(line), in) != nullptr;
        fclose(in);
        if(!got) continue;
        FILE* out = FT02_StorageOpenWriteFile(current, true);
        if(out)
        {
            fputs(line, out);
            FT02_StorageSyncFile(out);
            fclose(out);
        }
    }

    if(!FT02_StorageFileExists(FT02_LIBRARY_RECENT_PATH))
    {
        FILE* in = FT02_StorageOpenReadFile(FT02_LIBRARY_LEGACY_RECENT_PATH);
        if(in)
        {
            char line[128] = {};
            const bool got = fgets(line, sizeof(line), in) != nullptr;
            fclose(in);
            if(got)
            {
                FILE* out = FT02_StorageOpenWriteFile(FT02_LIBRARY_RECENT_PATH, true);
                if(out)
                {
                    fputs(line, out);
                    FT02_StorageSyncFile(out);
                    fclose(out);
                }
            }
        }
    }
}

static int FT02_LibraryRecentBookIndex()
{
    FILE* file = FT02_StorageOpenReadFile(FT02_LIBRARY_RECENT_PATH);
    if(file == nullptr) file = FT02_StorageOpenReadFile(FT02_LIBRARY_LEGACY_RECENT_PATH);
    if(file == nullptr) return -1;
    char line[128] = {};
    if(fgets(line, sizeof(line), file) == nullptr) { fclose(file); return -1; }
    fclose(file);
    char* tab = strchr(line, '\t');
    if(tab) *tab = '\0';
    char* id = FT02_LibraryTrim(line);
    for(int i = 0; i < g_bookCount; i++) if(strcmp(g_books[i].id, id) == 0) return i;
    return -1;
}

static int FT02_LibraryUtf8ExpectedBytes(uint8_t lead)
{
    if((lead & 0x80U) == 0) return 1;
    if((lead & 0xE0U) == 0xC0U) return 2;
    if((lead & 0xF0U) == 0xE0U) return 3;
    if((lead & 0xF8U) == 0xF0U) return 4;
    return 1;
}

static bool FT02_LibraryReadUtf8(FILE* file, char bytes[5], int& used, uint64_t& startOffset)
{
    used = 0;
    startOffset = static_cast<uint64_t>(ftell(file));
    const int first = fgetc(file);
    if(first == EOF) return false;
    bytes[used++] = static_cast<char>(first);
    const int expected = FT02_LibraryUtf8ExpectedBytes(static_cast<uint8_t>(first));
    for(int i = 1; i < expected; i++)
    {
        const int next = fgetc(file);
        if(next == EOF) break;
        bytes[used++] = static_cast<char>(next);
    }
    bytes[used] = '\0';
    return true;
}

static bool FT02_LibraryTryConsumeInlineMediaMarker(FILE* file, int& mediaIndex, uint64_t& markerStart)
{
    mediaIndex = -1;
    if(file == nullptr) return false;
    const long start = ftell(file);
    if(start < 0) return false;
    markerStart = static_cast<uint64_t>(start);

    char line[40] = {};
    size_t used = 0;
    int c = 0;
    while(used + 1 < sizeof(line) && (c = fgetc(file)) != EOF)
    {
        line[used++] = static_cast<char>(c);
        if(c == '\n') break;
    }
    line[used] = '\0';

    int parsed = -1;
    char extra = 0;
    // Marker is intentionally ASCII and occupies its own source line.
    const int matched = sscanf(line, "[[IMG:%d]]%c", &parsed, &extra);
    if(matched >= 1 && parsed >= 0)
    {
        mediaIndex = parsed;
        return true;
    }
    fseek(file, start, SEEK_SET);
    return false;
}

static bool FT02_LibraryLoadReaderPage(uint64_t offset)
{
    if(g_readerFile == nullptr) return false;
    if(offset > g_readerFileSize) offset = g_readerFileSize;
    if(fseek(g_readerFile, static_cast<long>(offset), SEEK_SET) != 0) return false;
    g_readerOffset = offset;
    g_readerNextOffset = offset;
    g_readerLineCount = 0;
    g_readerInlineMedia = -1;
    g_readerInlineScrollY = 0;
    for(int i = 0; i < FT02_LIBRARY_READER_LINES; i++)
    {
        g_readerLines[i][0] = '\0';
        g_readerParagraphAfter[i] = false;
    }

    int line = 0, lineBytes = 0, lineWidth = 0;
    uint64_t nextUnreadOffset = offset;
    while(line < FT02_LIBRARY_READER_LINES)
    {
        // Figures are full-width reading-flow pages, never squeezed under the
        // last few text rows. If text already occupies this page, leave the
        // marker unread so the next page becomes the dedicated figure page.
        if(lineBytes == 0)
        {
            int mediaIndex = -1;
            uint64_t markerStart = 0;
            if(FT02_LibraryTryConsumeInlineMediaMarker(g_readerFile, mediaIndex, markerStart))
            {
                const long afterMarker = ftell(g_readerFile);
                if(mediaIndex >= 0 && mediaIndex < g_mediaCount &&
                   g_media[mediaIndex].chapterIndex == g_activeChapter)
                {
                    if(line > 0)
                    {
                        fseek(g_readerFile, static_cast<long>(markerStart), SEEK_SET);
                        nextUnreadOffset = markerStart;
                        break;
                    }
                    g_readerInlineMedia = mediaIndex;
                    nextUnreadOffset = afterMarker >= 0 ? static_cast<uint64_t>(afterMarker) : markerStart;
                    break;
                }
                nextUnreadOffset = afterMarker >= 0 ? static_cast<uint64_t>(afterMarker) : markerStart;
                continue;
            }
        }

        char cpBytes[5] = {};
        int used = 0;
        uint64_t cpStart = 0;
        if(!FT02_LibraryReadUtf8(g_readerFile, cpBytes, used, cpStart)) break;
        const long afterCp = ftell(g_readerFile);
        if(used == 1 && cpBytes[0] == '\r')
        {
            if(afterCp >= 0) nextUnreadOffset = static_cast<uint64_t>(afterCp);
            continue;
        }
        if(used == 1 && cpBytes[0] == '\n')
        {
            if(afterCp >= 0) nextUnreadOffset = static_cast<uint64_t>(afterCp);
            if(lineBytes == 0)
            {
                if(line > 0) g_readerParagraphAfter[line - 1] = true;
                continue;
            }
            g_readerLines[line][lineBytes] = '\0';
            g_readerParagraphAfter[line] = true;
            line++; lineBytes = 0; lineWidth = 0;
            continue;
        }
        int cpUsed = 0;
        const uint32_t cp = FT02_ReadCodepoint(cpBytes, &cpUsed);
        int advance = FT02_CodepointAdvancePack(ft02_cjk_20r, cp);
        if(advance <= 0) advance = 20;
        if(lineBytes > 0 && lineWidth + advance > FT02_LIBRARY_READER_TEXT_W)
        {
            if(fseek(g_readerFile, static_cast<long>(cpStart), SEEK_SET) != 0) break;
            nextUnreadOffset = cpStart;
            g_readerLines[line][lineBytes] = '\0';
            line++; lineBytes = 0; lineWidth = 0;
            continue;
        }
        if(lineBytes + used >= FT02_LIBRARY_READER_LINE_BYTES - 1)
        {
            if(fseek(g_readerFile, static_cast<long>(cpStart), SEEK_SET) != 0) break;
            nextUnreadOffset = cpStart;
            g_readerLines[line][lineBytes] = '\0';
            line++; lineBytes = 0; lineWidth = 0;
            continue;
        }
        memcpy(g_readerLines[line] + lineBytes, cpBytes, static_cast<size_t>(used));
        lineBytes += used;
        g_readerLines[line][lineBytes] = '\0';
        lineWidth += advance;
        if(afterCp >= 0) nextUnreadOffset = static_cast<uint64_t>(afterCp);
    }
    if(line < FT02_LIBRARY_READER_LINES && lineBytes > 0)
    {
        g_readerLines[line++][lineBytes] = '\0';
    }
    g_readerLineCount = line;
    g_readerNextOffset = nextUnreadOffset > g_readerFileSize ? g_readerFileSize : nextUnreadOffset;
    return true;
}

static bool FT02_LibraryOpenChapterFile(int chapterIndex, uint64_t offset)
{
    if(chapterIndex < 0 || chapterIndex >= g_chapterCount) return false;
    FT02_LibraryCloseReaderFile();
    FILE* file = FT02_StorageOpenReadFile(g_chapters[chapterIndex].path);
    if(file == nullptr) return false;
    if(fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    const long size = ftell(file);
    if(size < 0) { fclose(file); return false; }
    g_readerFile = file;
    g_readerFileSize = static_cast<uint64_t>(size);
    g_activeChapter = chapterIndex;
    if(offset >= g_readerFileSize) offset = 0;
    return FT02_LibraryLoadReaderPage(offset);
}

static bool FT02_LibraryOpenMediaIndex(int mediaIndex, int scrollY = 0, bool fitPage = false,
                                       bool openedFromText = false)
{
    if(mediaIndex < 0 || mediaIndex >= g_mediaCount) return false;
    g_activeMedia = mediaIndex;
    g_mediaScrollY = scrollY < 0 ? 0 : scrollY;
    g_mediaFitPage = fitPage;
    g_mediaOpenedFromText = openedFromText;
    g_readingHeaderDirty = true;
    g_screen = FT02_LIBRARY_SCREEN_MEDIA_READER;
    return true;
}

static bool FT02_LibraryOpenBookAt(int globalBookIndex, int chapterIndex, uint64_t offset, bool clearHistory = true)
{
    if(globalBookIndex < 0 || globalBookIndex >= g_bookCount) return false;
    g_activeBook = globalBookIndex;
    FT02_LibraryLoadMediaIndex(globalBookIndex);
    if(!FT02_LibraryLoadToc(globalBookIndex))
    {
        FT02_LibrarySetError("目录读取失败", "请重新用 Library Builder 导入 EPUB/PDF/TXT。");
        return false;
    }

    if(FT02_LibraryBookIsScanPdf(globalBookIndex))
    {
        if(g_mediaCount <= 0)
        {
            FT02_LibrarySetError("扫描PDF页面缺失", "请重新运行 Builder 生成 media.tsv/pages。");
            return false;
        }
        if(chapterIndex < 0 || chapterIndex >= g_mediaCount) chapterIndex = 0;
        return FT02_LibraryOpenMediaIndex(chapterIndex, static_cast<int>(offset), false, false);
    }
    if(chapterIndex < 0 || chapterIndex >= g_chapterCount) chapterIndex = 0;
    if(clearHistory) g_readerHistoryCount = 0;
    if(!FT02_LibraryOpenChapterFile(chapterIndex, offset))
    {
        FT02_LibrarySetError("章节读取失败", g_chapters[chapterIndex].path);
        return false;
    }
    g_tocSelected = chapterIndex;
    g_readingHeaderDirty = true;
    g_screen = FT02_LIBRARY_SCREEN_READER;
    return true;
}

static bool FT02_LibraryOpenBook(int globalBookIndex)
{
    if(globalBookIndex < 0 || globalBookIndex >= g_bookCount) return false;
    const FT02LibraryLocation loc = FT02_LibraryLoadProgress(g_books[globalBookIndex]);
    if(FT02_LibraryBookIsScanPdf(globalBookIndex))
    {
        g_activeBook = globalBookIndex;
        FT02_LibraryLoadMediaIndex(globalBookIndex);
        if(!FT02_LibraryLoadToc(globalBookIndex) || g_mediaCount <= 0)
        {
            FT02_LibrarySetError("扫描PDF读取失败", "缺少 toc.tsv 或 media.tsv。请重新构建书库。");
            return false;
        }
        int page = loc.mediaMode ? loc.chapter : 0;
        if(page < 0 || page >= g_mediaCount) page = 0;
        return FT02_LibraryOpenMediaIndex(page, loc.mediaMode ? static_cast<int>(loc.offset) : 0,
                                          loc.mediaMode ? loc.fitPage : false, false);
    }
    const bool opened = FT02_LibraryOpenBookAt(globalBookIndex, loc.chapter, loc.offset, true);
    if(opened && g_readerInlineMedia >= 0) g_readerInlineScrollY = 0;
    return opened;
}

static void FT02_LibraryPushHistory(int chapter, uint64_t offset, int inlineScrollY = 0)
{
    const FT02LibraryLocation loc = {chapter, offset, false, false, inlineScrollY};
    if(g_readerHistoryCount < FT02_LIBRARY_READER_MAX_HISTORY)
        g_readerHistory[g_readerHistoryCount++] = loc;
    else
    {
        memmove(g_readerHistory, g_readerHistory + 1,
                sizeof(FT02LibraryLocation) * (FT02_LIBRARY_READER_MAX_HISTORY - 1));
        g_readerHistory[FT02_LIBRARY_READER_MAX_HISTORY - 1] = loc;
    }
}

static int FT02_LibraryInlineImageDisplayWidth(const FT02LibraryMedia& media)
{
    (void)media;
    return FT02_LIBRARY_READER_BODY_W;
}

static int FT02_LibraryInlineImageMaxScroll()
{
    // Inline figures default to "整图" so a diagram is understandable at a glance.
    // Press I to enter the dedicated media viewer for full-width zoom/scroll.
    return 0;
}

static void FT02_LibraryNextPage()
{
    if(g_readerFile == nullptr) return;
    g_readingHeaderDirty = false;

    if(g_readerInlineMedia >= 0)
    {
        const int maxScroll = FT02_LibraryInlineImageMaxScroll();
        if(g_readerInlineScrollY < maxScroll)
        {
            FT02_LibraryPushHistory(g_activeChapter, g_readerOffset, g_readerInlineScrollY);
            const int step = FT02_LIBRARY_INLINE_IMAGE_H - FT02_LIBRARY_INLINE_IMAGE_SCROLL_OVERLAP;
            g_readerInlineScrollY = FT02_LibraryClamp(g_readerInlineScrollY + step, 0, maxScroll);
            FT02_LibrarySaveProgress();
            return;
        }
        FT02_LibraryPushHistory(g_activeChapter, g_readerOffset, g_readerInlineScrollY);
        if(g_readerNextOffset < g_readerFileSize)
        {
            FT02_LibraryLoadReaderPage(g_readerNextOffset);
            FT02_LibrarySaveProgress();
            return;
        }
    }
    else if(g_readerNextOffset < g_readerFileSize)
    {
        FT02_LibraryPushHistory(g_activeChapter, g_readerOffset, 0);
        FT02_LibraryLoadReaderPage(g_readerNextOffset);
        FT02_LibrarySaveProgress();
        return;
    }

    if(g_activeChapter + 1 < g_chapterCount)
    {
        FT02_LibraryPushHistory(g_activeChapter, g_readerOffset, g_readerInlineScrollY);
        FT02_LibraryOpenChapterFile(g_activeChapter + 1, 0);
        g_tocSelected = g_activeChapter;
        g_readingHeaderDirty = true;
        FT02_LibrarySaveProgress();
    }
}

static void FT02_LibraryPrevPage()
{
    if(g_readerHistoryCount <= 0) return;
    const int oldChapter = g_activeChapter;
    const FT02LibraryLocation previous = g_readerHistory[--g_readerHistoryCount];
    FT02_LibraryOpenChapterFile(previous.chapter, previous.offset);
    if(g_readerInlineMedia >= 0) g_readerInlineScrollY = previous.inlineScrollY < 0 ? 0 : previous.inlineScrollY;
    g_tocSelected = g_activeChapter;
    g_readingHeaderDirty = g_activeChapter != oldChapter;
    FT02_LibrarySaveProgress();
}

static void FT02_LibraryStartBook()
{
    g_readerHistoryCount = 0;
    g_readingHeaderDirty = true;
    FT02_LibraryOpenChapterFile(0, 0);
    g_tocSelected = 0;
    FT02_LibrarySaveProgress();
}

static void FT02_LibraryCoalesceQueuedReaderTurns()
{
    FT02InputEvent queued;
    int folded = 0;
    while(folded < 8 && FT02_EpdInteractivePeekQueuedInput(queued))
    {
        if(queued.key == FT02_KEY_RIGHT || queued.key == FT02_KEY_DOWN)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryNextPage();
            ++folded;
            continue;
        }
        if(queued.key == FT02_KEY_LEFT || queued.key == FT02_KEY_UP)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryPrevPage();
            ++folded;
            continue;
        }
        break;
    }
    if(folded > 0)
        Serial.printf("[FIELD-LIBRARY-V0.6] coalesced %d queued reader turns into one refresh\n", folded);
}

static void FT02_LibraryCoalesceQueuedMediaNavigation()
{
    FT02InputEvent queued;
    int folded = 0;
    while(folded < 8 && FT02_EpdInteractivePeekQueuedInput(queued))
    {
        if(queued.key == FT02_KEY_RIGHT)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryMediaSetIndex(g_activeMedia + 1);
        }
        else if(queued.key == FT02_KEY_LEFT)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryMediaSetIndex(g_activeMedia - 1);
        }
        else if(queued.key == FT02_KEY_DOWN)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryMediaScroll(260);
        }
        else if(queued.key == FT02_KEY_UP)
        {
            (void)FT02_EpdInteractiveTakeQueuedInput(queued);
            FT02_LibraryMediaScroll(-260);
        }
        else break;
        ++folded;
    }
    if(folded > 0)
        Serial.printf("[FIELD-LIBRARY-V0.6] coalesced %d queued media moves into one refresh\n", folded);
}

static bool FT02_LibraryOpenMostRecent()
{
    const int recent = FT02_LibraryRecentBookIndex();
    return recent >= 0 && FT02_LibraryOpenBook(recent);
}

// ---------- Simple library query ----------

static void FT02_LibrarySearchSetNotice(const char* text)
{
    snprintf(g_searchNotice, sizeof(g_searchNotice), "%s", text ? text : "");
}

static void FT02_LibrarySearchResetPinyin()
{
    g_searchPinyinLength = 0;
    g_searchPinyin[0] = '\0';
    g_searchPinyinPage = 0;
}

static void FT02_LibrarySearchReset(bool clearQuery)
{
    if(clearQuery)
    {
        g_searchQueryLength = 0;
        g_searchQuery[0] = '\0';
    }
    FT02_LibrarySearchResetPinyin();
    g_searchResultCount = 0;
    g_searchSelected = 0;
    FT02_LibrarySearchSetNotice("");
}

static bool FT02_LibrarySearchAppend(const char* text, size_t bytes)
{
    if(!text || bytes == 0 || g_searchQueryLength + bytes > FT02_LIBRARY_QUERY_MAX_BYTES) return false;
    memcpy(g_searchQuery + g_searchQueryLength, text, bytes);
    g_searchQueryLength += bytes;
    g_searchQuery[g_searchQueryLength] = '\0';
    FT02_LibrarySearchSetNotice("");
    return true;
}

static void FT02_LibrarySearchBackspace()
{
    if(g_searchQueryLength == 0) return;
    size_t pos = g_searchQueryLength - 1;
    while(pos > 0 && (static_cast<uint8_t>(g_searchQuery[pos]) & 0xC0u) == 0x80u) --pos;
    g_searchQueryLength = pos;
    g_searchQuery[pos] = '\0';
}

static size_t FT02_LibrarySearchCandidateCount()
{
    return FT02_PinyinImeCandidateCount(g_searchPinyin);
}

static bool FT02_LibrarySearchCommitCandidate(size_t slot)
{
    const size_t count = FT02_LibrarySearchCandidateCount();
    const size_t index = g_searchPinyinPage * FT02_PINYIN_PAGE_SIZE + slot;
    if(index >= count) return false;
    char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
    size_t consumed = 0;
    if(!FT02_PinyinImeCandidate(g_searchPinyin, index, candidate, sizeof(candidate), &consumed)) return false;
    if(consumed == 0 || consumed > g_searchPinyinLength || !FT02_LibrarySearchAppend(candidate, strlen(candidate))) return false;
    char key[FT02_PINYIN_MAX_INPUT + 1] = {};
    memcpy(key, g_searchPinyin, consumed);
    key[consumed] = '\0';
    FT02_PinyinLearningRecord(key, candidate);
    if(consumed >= g_searchPinyinLength) FT02_LibrarySearchResetPinyin();
    else
    {
        const size_t remain = g_searchPinyinLength - consumed;
        memmove(g_searchPinyin, g_searchPinyin + consumed, remain);
        g_searchPinyinLength = remain;
        g_searchPinyin[remain] = '\0';
        g_searchPinyinPage = 0;
    }
    return true;
}

static bool FT02_LibrarySearchCommitRawPinyin()
{
    if(g_searchPinyinLength == 0) return false;
    if(!FT02_LibrarySearchAppend(g_searchPinyin, g_searchPinyinLength)) return false;
    FT02_LibrarySearchResetPinyin();
    return true;
}

static bool FT02_LibrarySearchCommitDefault()
{
    if(g_searchPinyinLength == 0) return false;
    return FT02_LibrarySearchCandidateCount() > 0
        ? FT02_LibrarySearchCommitCandidate(0)
        : FT02_LibrarySearchCommitRawPinyin();
}

static void FT02_LibrarySearchAppendPinyin(char raw)
{
    if(g_searchPinyinLength >= FT02_PINYIN_MAX_INPUT) return;
    if(raw >= 'A' && raw <= 'Z') raw = static_cast<char>(raw - 'A' + 'a');
    g_searchPinyin[g_searchPinyinLength++] = raw;
    g_searchPinyin[g_searchPinyinLength] = '\0';
    g_searchPinyinPage = 0;
}

static void FT02_LibrarySearchMovePinyinPage(int delta)
{
    const size_t count = FT02_LibrarySearchCandidateCount();
    if(count == 0) return;
    const int pages = static_cast<int>((count + FT02_PINYIN_PAGE_SIZE - 1) / FT02_PINYIN_PAGE_SIZE);
    int page = static_cast<int>(g_searchPinyinPage) + delta;
    if(page < 0) page = pages - 1;
    if(page >= pages) page = 0;
    g_searchPinyinPage = static_cast<size_t>(page);
}

static void FT02_LibrarySearchAddResult(int bookIndex, int chapterIndex, uint64_t offset,
                                        const char* chapterTitle, const char* snippet)
{
    if(g_searchResultCount >= FT02_LIBRARY_MAX_SEARCH_RESULTS) return;
    // One result per book/chapter is enough for the v0.2 simple query surface.
    for(int i = 0; i < g_searchResultCount; i++)
        if(g_searchResults[i].bookIndex == bookIndex && g_searchResults[i].chapterIndex == chapterIndex) return;
    FT02LibrarySearchResult& r = g_searchResults[g_searchResultCount++];
    r.bookIndex = bookIndex;
    r.chapterIndex = chapterIndex;
    r.offset = offset;
    snprintf(r.chapterTitle, sizeof(r.chapterTitle), "%s", chapterTitle ? chapterTitle : "正文");
    snprintf(r.snippet, sizeof(r.snippet), "%s", snippet ? snippet : "");
}

static bool FT02_LibrarySearchBook(int bookIndex, const char* query)
{
    if(bookIndex < 0 || bookIndex >= g_bookCount) return false;
    const FT02LibraryBook& book = g_books[bookIndex];
    const bool metadataHit = FT02_LibraryContainsFold(book.title, query) ||
                             FT02_LibraryContainsFold(book.author, query) ||
                             FT02_LibraryContainsFold(book.category, query);

    // Stream the TOC so querying does not disturb an open reader's loaded chapter table.
    if(book.tocPath[0] == '\0')
    {
        if(metadataHit) FT02_LibrarySearchAddResult(bookIndex, 0, 0, "正文", "书名/作者/分类匹配");
        FILE* body = FT02_StorageOpenReadFile(book.legacyPath);
        if(body)
        {
            char line[512];
            while(true)
            {
                const long start = ftell(body);
                if(fgets(line, sizeof(line), body) == nullptr) break;
                if(FT02_LibraryContainsFold(line, query))
                {
                    char* text = FT02_LibraryTrim(line);
                    FT02_LibrarySearchAddResult(bookIndex, 0, start >= 0 ? static_cast<uint64_t>(start) : 0,
                                                "正文", text);
                    break;
                }
            }
            fclose(body);
        }
        return true;
    }

    if(metadataHit) FT02_LibrarySearchAddResult(bookIndex, 0, 0, "书籍信息", "书名/作者/分类匹配");
    FILE* toc = FT02_StorageOpenReadFile(book.tocPath);
    if(toc == nullptr) return false;
    char row[512];
    int chapterIndex = 0;
    while(fgets(row, sizeof(row), toc) != nullptr && g_searchResultCount < FT02_LIBRARY_MAX_SEARCH_RESULTS)
    {
        char* text = FT02_LibraryTrim(row);
        if(text[0] == '\0' || text[0] == '#') continue;
        char* a = strchr(text, '\t');
        if(!a) continue;
        *a++ = '\0';
        char* b = strchr(a, '\t');
        if(!b) continue;
        *b++ = '\0';
        chapterIndex = atoi(text);
        char* chapterTitle = FT02_LibraryTrim(a);
        char* chapterPath = FT02_LibraryTrim(b);
        const bool mediaTarget = strncmp(chapterPath, "@media:", 7) == 0;
        const int resultTarget = mediaTarget ? atoi(chapterPath + 7) : chapterIndex;
        if(FT02_LibraryContainsFold(chapterTitle, query))
            FT02_LibrarySearchAddResult(bookIndex, resultTarget, 0, chapterTitle,
                                        mediaTarget ? "扫描PDF目录匹配" : "目录标题匹配");

        // Scanned PDFs have no trusted text layer by design. Search their
        // metadata/TOC only; OCR may be added later as a separate reviewed index.
        if(mediaTarget) continue;

        FILE* body = FT02_StorageOpenReadFile(chapterPath);
        if(body)
        {
            char bodyLine[512];
            while(g_searchResultCount < FT02_LIBRARY_MAX_SEARCH_RESULTS)
            {
                const long start = ftell(body);
                if(fgets(bodyLine, sizeof(bodyLine), body) == nullptr) break;
                if(FT02_LibraryContainsFold(bodyLine, query))
                {
                    char* snippet = FT02_LibraryTrim(bodyLine);
                    FT02_LibrarySearchAddResult(bookIndex, chapterIndex,
                        start >= 0 ? static_cast<uint64_t>(start) : 0, chapterTitle, snippet);
                    break;
                }
            }
            fclose(body);
        }
        yield();
    }
    fclose(toc);
    return true;
}

static void FT02_LibrarySearchExecute()
{
    g_searchResultCount = 0;
    g_searchSelected = 0;
    char query[FT02_LIBRARY_QUERY_MAX_BYTES + 1] = {};
    snprintf(query, sizeof(query), "%s", g_searchQuery);
    char* trimmed = FT02_LibraryTrim(query);
    if(trimmed[0] == '\0')
    {
        FT02_LibrarySearchSetNotice("请先输入查询词");
        return;
    }
    FT02_LibrarySearchSetNotice("正在查询 P0 图书...");
    for(int i = 0; i < g_bookCount && g_searchResultCount < FT02_LIBRARY_MAX_SEARCH_RESULTS; i++)
        FT02_LibrarySearchBook(i, trimmed);
    if(g_searchResultCount == 0)
    {
        FT02_LibrarySearchSetNotice("没有找到匹配内容，可缩短查询词后重试");
        return;
    }
    g_screen = FT02_LIBRARY_SCREEN_SEARCH_RESULTS;
}

// ---------- Drawing ----------

static void FT02_LibraryDrawHeader(FT02Display& display, const char* title, const char* right = nullptr)
{
    FT02_DrawTextPackBold(display, ft02_cjk_24r, title, 32, 118);
    if(right && right[0])
    {
        const int width = FT02_TextWidthPack(ft02_cjk_20r, right);
        FT02_DrawTextPack(display, ft02_cjk_20r, right, display.width() - 32 - width, 116);
    }
}

static void FT02_LibraryDrawSelectableRow(FT02Display& display, int y, const char* title,
                                           const char* subtitle, bool selected, int h = 62)
{
    const int x = 32, w = 736;
    if(selected)
    {
        display.fillRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackInvertClipped(display, ft02_cjk_24r, title, x + 18, y + 27,
            x + 8, y + 4, w - 16, 30);
        if(subtitle && subtitle[0])
            FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, subtitle, x + 22, y + 53,
                x + 8, y + 30, w - 16, 28);
    }
    else
    {
        display.drawRect(x, y, w, h, GxEPD_BLACK);
        FT02_DrawTextPackClipped(display, ft02_cjk_24r, title, x + 18, y + 27,
            x + 8, y + 4, w - 16, 30);
        if(subtitle && subtitle[0])
            FT02_DrawTextPackClipped(display, ft02_cjk_20r, subtitle, x + 22, y + 53,
                x + 8, y + 30, w - 16, 28);
    }
}

static void FT02_LibraryDrawHome(FT02Display& display)
{
    FT02_LibraryDrawHeader(display, "随身书库", "P0 · EPUB/TXT");
    const char* titles[4] = {"继续阅读", "分类浏览", "全部书籍", "查询资料"};
    char recent[96] = "暂无阅读记录";
    const int recentIndex = FT02_LibraryRecentBookIndex();
    if(recentIndex >= 0) snprintf(recent, sizeof(recent), "%s", g_books[recentIndex].title);
    const char* subtitles[4] = {recent, "按能力领域浏览 P0 资料", "浏览当前全部 P0 书籍", "查询书名、目录与正文"};
    for(int i = 0; i < 4; i++)
        FT02_LibraryDrawSelectableRow(display, 138 + i * 66, titles[i], subtitles[i], i == g_homeSelected, 58);
    char countLine[56];
    snprintf(countLine, sizeof(countLine), "P0 已索引 %d 本 · %d 个分类", g_bookCount, g_categoryCount);
    FT02_DrawTextPack(display, ft02_cjk_20r, countLine, 34, 413);
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_HOME_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawCategories(FT02Display& display)
{
    FT02_LibraryDrawHeader(display, "分类浏览", "P0");
    if(g_categoryCount <= 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "没有分类数据", 32, 206);
    }
    else
    {
        const int pageStart = (g_categorySelected / FT02_LIBRARY_LIST_PAGE_SIZE) * FT02_LIBRARY_LIST_PAGE_SIZE;
        for(int row = 0; row < FT02_LIBRARY_LIST_PAGE_SIZE; row++)
        {
            const int index = pageStart + row;
            if(index >= g_categoryCount) break;
            char subtitle[48];
            snprintf(subtitle, sizeof(subtitle), "%d 本 P0 资料", FT02_LibraryBookCountForCategory(index));
            FT02_LibraryDrawSelectableRow(display, 142 + row * 68, g_categories[index].name, subtitle,
                                           index == g_categorySelected);
        }
    }
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_LIST_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawBooks(FT02Display& display)
{
    const char* header = g_activeCategory >= 0 && g_activeCategory < g_categoryCount
        ? g_categories[g_activeCategory].name : "全部书籍";
    FT02_LibraryDrawHeader(display, header, "P0");
    const int total = FT02_LibraryBookCountForCategory(g_activeCategory);
    const int pageStart = total > 0 ? (g_bookSelected / FT02_LIBRARY_LIST_PAGE_SIZE) * FT02_LIBRARY_LIST_PAGE_SIZE : 0;
    for(int row = 0; row < FT02_LIBRARY_LIST_PAGE_SIZE; row++)
    {
        const int visibleIndex = pageStart + row;
        if(visibleIndex >= total) break;
        const int globalIndex = FT02_LibraryGlobalBookIndex(g_activeCategory, visibleIndex);
        if(globalIndex < 0) continue;
        const FT02LibraryBook& book = g_books[globalIndex];
        char subtitle[112];
        snprintf(subtitle, sizeof(subtitle), "%s · %s · %s", book.author[0] ? book.author : "佚名",
                 book.format[0] ? book.format : "txt", book.category);
        FT02_LibraryDrawSelectableRow(display, 142 + row * 68, book.title, subtitle,
                                       visibleIndex == g_bookSelected);
    }
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_LIST_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawToc(FT02Display& display)
{
    const char* title = g_activeBook >= 0 ? g_books[g_activeBook].title : "目录";
    char right[32];
    snprintf(right, sizeof(right), "目录 %d章", g_chapterCount);
    FT02_LibraryDrawHeader(display, title, right);
    const int pageStart = g_chapterCount > 0 ? (g_tocSelected / FT02_LIBRARY_LIST_PAGE_SIZE) * FT02_LIBRARY_LIST_PAGE_SIZE : 0;
    for(int row = 0; row < FT02_LIBRARY_LIST_PAGE_SIZE; row++)
    {
        const int index = pageStart + row;
        if(index >= g_chapterCount) break;
        char subtitle[48];
        snprintf(subtitle, sizeof(subtitle), "第 %d/%d 节", index + 1, g_chapterCount);
        FT02_LibraryDrawSelectableRow(display, 142 + row * 68, g_chapters[index].title, subtitle,
                                       index == g_tocSelected);
    }
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_LIST_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawReader(FT02Display& display)
{
    if(g_activeBook < 0 || g_activeBook >= g_bookCount || g_activeChapter < 0 || g_activeChapter >= g_chapterCount)
    {
        FT02_LibrarySetError("阅读器状态错误", "没有有效的当前书籍/章节。");
        return;
    }

    char progress[48];
    // Keep the slim reader header stable while turning pages. Per-byte percent
    // changed on every page and forced the header into the partial refresh
    // rectangle; chapter progress gives useful context without that penalty.
    snprintf(progress, sizeof(progress), "%d/%d", g_activeChapter + 1, g_chapterCount);

    // Compact reading header: one global top status bar + two slim book lines.
    FT02_DrawTextPackBold(display, ft02_cjk_20r, g_books[g_activeBook].title, 20, 111);
    const int pw = FT02_TextWidthPack(ft02_cjk_20r, progress);
    FT02_DrawTextPack(display, ft02_cjk_20r, progress, display.width() - 20 - pw, 111);
    FT02_DrawTextPackClipped(display, ft02_cjk_20r, g_chapters[g_activeChapter].title,
        20, 136, 18, 116, 764, 24);
    display.drawFastHLine(16, 143, 768, GxEPD_BLACK);

    if(g_readerInlineMedia >= 0 && g_readerInlineMedia < g_mediaCount)
    {
        const FT02LibraryMedia& media = g_media[g_readerInlineMedia];
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, media.caption,
            FT02_LIBRARY_READER_BODY_X, FT02_LIBRARY_READER_BODY_Y + 19,
            FT02_LIBRARY_READER_BODY_X, FT02_LIBRARY_READER_BODY_Y,
            FT02_LIBRARY_READER_BODY_W, FT02_LIBRARY_INLINE_CAPTION_H);
        // Default inline figure mode is "整图": keep the complete botanical /
        // technical diagram on one page. I still opens the zoom viewer for
        // full-width inspection when labels are tiny.
        (void)FT02_LibraryDrawPbm(display, media,
            FT02_LIBRARY_READER_BODY_X, FT02_LIBRARY_INLINE_IMAGE_Y,
            FT02_LIBRARY_READER_BODY_W, FT02_LIBRARY_INLINE_IMAGE_H,
            true, 0);
        return;
    }

    int y = 168;
    for(int i = 0; i < g_readerLineCount && i < FT02_LIBRARY_READER_LINES; i++)
    {
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, g_readerLines[i],
            FT02_LIBRARY_READER_BODY_X, y,
            FT02_LIBRARY_READER_BODY_X, FT02_LIBRARY_READER_BODY_Y,
            FT02_LIBRARY_READER_BODY_W, FT02_LIBRARY_READER_BODY_H);
        y += FT02_LIBRARY_READER_LINE_ADVANCE;
        if(g_readerParagraphAfter[i]) y += FT02_LIBRARY_READER_PARAGRAPH_GAP;
    }
    // Images now live inside the EPUB reading flow. The standalone image viewer
    // remains available with I for zoom/inspection, but is no longer required.
}

static bool FT02_LibraryReadPbmHeader(FILE* file, int& width, int& height, long& dataOffset)
{
    width = height = 0;
    dataOffset = 0;
    if(file == nullptr) return false;
    if(fseek(file, 0, SEEK_SET) != 0) return false;
    char magic[3] = {};
    if(fread(magic, 1, 2, file) != 2 || magic[0] != 'P' || magic[1] != '4') return false;

    auto readToken = [&](char* out, size_t outBytes) -> bool
    {
        size_t used = 0;
        int c = 0;
        while((c = fgetc(file)) != EOF)
        {
            if(c == '#')
            {
                while((c = fgetc(file)) != EOF && c != '\n') {}
                continue;
            }
            if(c > ' ') break;
        }
        if(c == EOF) return false;
        do
        {
            if(c <= ' ') break;
            if(used + 1 < outBytes) out[used++] = static_cast<char>(c);
            c = fgetc(file);
        } while(c != EOF);
        out[used] = '\0';
        return used > 0;
    };

    char token[24] = {};
    if(!readToken(token, sizeof(token))) return false;
    width = atoi(token);
    if(!readToken(token, sizeof(token))) return false;
    height = atoi(token);
    dataOffset = ftell(file);
    return width > 0 && height > 0 && dataOffset > 0;
}

static bool FT02_LibraryDrawPbm(FT02Display& display, const FT02LibraryMedia& media,
                                int x, int y, int areaW, int areaH, bool fitPage, int scrollY)
{
    FILE* file = FT02_StorageOpenReadFile(media.path);
    if(file == nullptr) return false;
    int sourceW = 0, sourceH = 0;
    long dataOffset = 0;
    if(!FT02_LibraryReadPbmHeader(file, sourceW, sourceH, dataOffset))
    {
        fclose(file);
        return false;
    }
    const int rowBytes = (sourceW + 7) / 8;
    uint8_t* row = static_cast<uint8_t*>(heap_caps_malloc(static_cast<size_t>(rowBytes), MALLOC_CAP_8BIT));
    if(row == nullptr)
    {
        fclose(file);
        return false;
    }

    int destW = areaW;
    int destH = static_cast<int>((static_cast<int64_t>(sourceH) * destW) / sourceW);
    int destX = x;
    int destY = y;
    int sourceStartY = 0;

    if(fitPage)
    {
        if(destH > areaH)
        {
            destH = areaH;
            destW = static_cast<int>((static_cast<int64_t>(sourceW) * destH) / sourceH);
        }
        if(destW < 1) destW = 1;
        if(destH < 1) destH = 1;
        destX = x + (areaW - destW) / 2;
        destY = y + (areaH - destH) / 2;
    }
    else
    {
        // "适宽" means exactly that: even a 302 px EPUB diagram is enlarged
        // to the full reader width. Scan-PDF pages are prebuilt at 784 px and
        // therefore remain a crisp 1:1 stream.
        sourceStartY = FT02_LibraryClamp(scrollY, 0, destH > areaH ? destH - areaH : 0);
    }

    const int drawRows = fitPage ? destH : (destH - sourceStartY < areaH ? destH - sourceStartY : areaH);
    for(int dy = 0; dy < drawRows; ++dy)
    {
        const int logicalY = fitPage ? dy : sourceStartY + dy;
        int sy = static_cast<int>((static_cast<int64_t>(logicalY) * sourceH) / destH);
        if(sy >= sourceH) sy = sourceH - 1;
        const long rowPos = dataOffset + static_cast<long>(sy) * rowBytes;
        if(fseek(file, rowPos, SEEK_SET) != 0 ||
           fread(row, 1, static_cast<size_t>(rowBytes), file) != static_cast<size_t>(rowBytes))
            break;

        // GxEPD2/Adafruit packed drawBitmap is fast for byte-aligned widths,
        // but several EPUB illustrations have widths such as 302/566 px. On
        // this panel stack those rows did not land reliably. Decode to black
        // horizontal runs instead: alignment-independent and still far cheaper
        // than one drawPixel call per black pixel.
        int runStart = -1;
        for(int dx = 0; dx < destW; ++dx)
        {
            int sx = static_cast<int>((static_cast<int64_t>(dx) * sourceW) / destW);
            if(sx >= sourceW) sx = sourceW - 1;
            const bool black = (row[sx >> 3] & (0x80U >> (sx & 7))) != 0;
            if(black && runStart < 0) runStart = dx;
            if((!black || dx == destW - 1) && runStart >= 0)
            {
                const int runEnd = black && dx == destW - 1 ? dx + 1 : dx;
                display.drawFastHLine(destX + runStart, destY + dy, runEnd - runStart, GxEPD_BLACK);
                runStart = -1;
            }
        }
        if((dy & 0x0F) == 0) yield();
    }
    heap_caps_free(row);
    fclose(file);
    return true;
}

static void FT02_LibraryDrawMediaReader(FT02Display& display)
{
    if(g_activeBook < 0 || g_activeBook >= g_bookCount || g_activeMedia < 0 || g_activeMedia >= g_mediaCount)
    {
        FT02_LibrarySetError("图片阅读器状态错误", "没有有效的图片/扫描页。");
        return;
    }
    const FT02LibraryMedia& media = g_media[g_activeMedia];
    const bool scanPdf = FT02_LibraryBookIsScanPdf(g_activeBook);
    char right[48];
    snprintf(right, sizeof(right), "%d/%d · %s", g_activeMedia + 1, g_mediaCount,
             g_mediaFitPage ? "整页" : "适宽");

    // Book title is stable while paging. The changing page counter is moved
    // into the media body so normal page turns no longer refresh the title row.
    FT02_DrawTextPackBold(display, ft02_cjk_20r, g_books[g_activeBook].title, 16, 109);

    int areaY = FT02_LIBRARY_MEDIA_SCAN_Y + 26;
    int areaH = FT02_LIBRARY_MEDIA_SCAN_H - 26;
    if(scanPdf)
    {
        display.drawFastHLine(8, 119, 784, GxEPD_BLACK);
        const int rw = FT02_TextWidthPack(ft02_cjk_20r, right);
        FT02_DrawTextPack(display, ft02_cjk_20r, right, display.width() - 16 - rw, 140);
    }
    else
    {
        const int rw = FT02_TextWidthPack(ft02_cjk_20r, right);
        FT02_DrawTextPack(display, ft02_cjk_20r, right, display.width() - 16 - rw, 134);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, media.caption,
            16, 134, 14, 113, 590, 24);
        display.drawFastHLine(8, 141, 784, GxEPD_BLACK);
        areaY = FT02_LIBRARY_MEDIA_EPUB_Y + 10;
        areaH = FT02_LIBRARY_MEDIA_EPUB_H - 10;
    }

    if(!FT02_LibraryDrawPbm(display, media, FT02_LIBRARY_MEDIA_X, areaY,
                            FT02_LIBRARY_MEDIA_W, areaH,
                            g_mediaFitPage, g_mediaScrollY))
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "图片读取失败，请重新生成 SD 图书数据。", 28, 285);
    }
    // No bottom command bar in image/PDF reading mode.
}

static void FT02_LibraryDrawSearch(FT02Display& display)
{
    FT02_LibraryDrawHeader(display, "查询资料", FT02_TextInputModeIsChinese() ? "中文输入" : "EN 输入");
    display.drawRect(32, 146, 736, 66, GxEPD_BLACK);
    FT02_DrawTextPack(display, ft02_cjk_20r, "查询词", 46, 173);
    if(g_searchQueryLength == 0)
        FT02_DrawTextPack(display, ft02_cjk_24r, "输入书名、章节或正文关键词...", 140, 188);
    else
        FT02_DrawTextPackClipped(display, ft02_cjk_24r, g_searchQuery, 140, 188, 132, 154, 620, 48);

    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
    {
        char segmented[64] = {};
        FT02_PinyinImeSegmentDisplay(g_searchPinyin, segmented, sizeof(segmented));
        char pyLine[96];
        snprintf(pyLine, sizeof(pyLine), "拼音：%s", segmented[0] ? segmented : g_searchPinyin);
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, pyLine, 44, 252, 40, 222, 720, 32);
        const size_t candidateCount = FT02_LibrarySearchCandidateCount();
        char candidates[300] = {};
        size_t used = 0;
        for(size_t slot = 0; slot < FT02_PINYIN_PAGE_SIZE; slot++)
        {
            const size_t index = g_searchPinyinPage * FT02_PINYIN_PAGE_SIZE + slot;
            if(index >= candidateCount) break;
            char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
            if(!FT02_PinyinImeCandidate(g_searchPinyin, index, candidate, sizeof(candidate))) continue;
            const int wrote = snprintf(candidates + used, sizeof(candidates) - used, "%s%u.%s",
                                       used ? "  " : "", static_cast<unsigned>(slot + 1), candidate);
            if(wrote <= 0 || static_cast<size_t>(wrote) >= sizeof(candidates) - used) break;
            used += static_cast<size_t>(wrote);
        }
        if(candidates[0] == '\0') snprintf(candidates, sizeof(candidates), "无中文候选，ENTER 可直接保留字母");
        FT02_DrawTextPackClipped(display, ft02_cjk_20r, candidates, 44, 292, 40, 262, 720, 34);
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "简单查询会匹配：书名、作者、分类、目录标题和正文。", 44, 266);
        FT02_DrawTextPack(display, ft02_cjk_20r, "首批只检索 P0 图书；每章返回第一个命中位置。", 44, 304);
    }
    if(g_searchNotice[0]) FT02_DrawTextPackClipped(display, ft02_cjk_20r, g_searchNotice, 44, 370, 40, 340, 720, 38);
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_SEARCH_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawSearchResults(FT02Display& display)
{
    char right[32];
    snprintf(right, sizeof(right), "%d 条", g_searchResultCount);
    FT02_LibraryDrawHeader(display, "查询结果", right);
    const int pageStart = g_searchResultCount > 0 ? (g_searchSelected / FT02_LIBRARY_LIST_PAGE_SIZE) * FT02_LIBRARY_LIST_PAGE_SIZE : 0;
    for(int row = 0; row < FT02_LIBRARY_LIST_PAGE_SIZE; row++)
    {
        const int index = pageStart + row;
        if(index >= g_searchResultCount) break;
        const FT02LibrarySearchResult& r = g_searchResults[index];
        char subtitle[128];
        snprintf(subtitle, sizeof(subtitle), "%s · %s", r.chapterTitle, r.snippet);
        FT02_LibraryDrawSelectableRow(display, 142 + row * 68, g_books[r.bookIndex].title, subtitle,
                                       index == g_searchSelected);
    }
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_LIST_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawError(FT02Display& display)
{
    FT02_LibraryDrawHeader(display, g_errorTitle);
    display.drawRect(32, 150, 736, 218, GxEPD_BLACK);
    FT02_DrawTextPackClipped(display, ft02_cjk_24r, g_errorDetail, 56, 218, 48, 166, 688, 86);
    FT02_DrawTextPack(display, ft02_cjk_20r, "目标路径：/library/catalog.tsv", 56, 305);
    FT02_DrawBottomBarWithFont(display, FT02_LIBRARY_ERROR_BOTTOM, ft02_cjk_20r);
}

static void FT02_LibraryDrawCurrentContent(FT02Display& display)
{
    switch(g_screen)
    {
        case FT02_LIBRARY_SCREEN_HOME: FT02_LibraryDrawHome(display); break;
        case FT02_LIBRARY_SCREEN_CATEGORIES: FT02_LibraryDrawCategories(display); break;
        case FT02_LIBRARY_SCREEN_BOOKS: FT02_LibraryDrawBooks(display); break;
        case FT02_LIBRARY_SCREEN_TOC: FT02_LibraryDrawToc(display); break;
        case FT02_LIBRARY_SCREEN_READER: FT02_LibraryDrawReader(display); break;
        case FT02_LIBRARY_SCREEN_MEDIA_READER: FT02_LibraryDrawMediaReader(display); break;
        case FT02_LIBRARY_SCREEN_SEARCH: FT02_LibraryDrawSearch(display); break;
        case FT02_LIBRARY_SCREEN_SEARCH_RESULTS: FT02_LibraryDrawSearchResults(display); break;
        case FT02_LIBRARY_SCREEN_ERROR: default: FT02_LibraryDrawError(display); break;
    }
}

static bool FT02_LibraryReadingScreen()
{
    return g_screen == FT02_LIBRARY_SCREEN_READER ||
           g_screen == FT02_LIBRARY_SCREEN_MEDIA_READER;
}

static void FT02_LibraryRedrawFull(FT02Display& display, const char* source)
{
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        FT02_DrawStatusBar(display);
        FT02_LibraryDrawCurrentContent(display);
    } while(display.nextPage());
    FT02_EpdInteractivePowerOffNow(display, source);
    FT02_EpdInteractiveEndBusyCapture(display);
    g_readingPartialCount = 0;
}

static void FT02_LibraryRedrawReadingPartial(FT02Display& display)
{
    if(g_readingPartialCount >= FT02_LIBRARY_READING_PARTIAL_LIMIT)
    {
        Serial.println("[FIELD-LIBRARY-V0.8.2] long-session cleanup full refresh");
        FT02_LibraryRedrawFull(display, "field-library-reading-clean-full");
        return;
    }

    // A16 real-device clean-slate reader refresh.  The previous fast reader
    // updated only x=16..783 and sometimes started at y=92/145.  That left
    // stale menu/selection pixels in the 16 px side margins and the strip
    // directly below the global status bar.  Every book/media page now clears
    // and redraws the complete reading surface below the status bar.  The
    // status bar itself (0..85) remains untouched.
    const int px = FT02_LIBRARY_READING_PARTIAL_X;
    const int py = FT02_LIBRARY_READING_PARTIAL_Y;
    const int pw = FT02_LIBRARY_READING_PARTIAL_W;
    const int ph = FT02_LIBRARY_READING_PARTIAL_H;

    const uint32_t startedMs = millis();
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(px, py, pw, ph);
    display.firstPage();
    do
    {
        display.fillRect(px, py, pw, ph, GxEPD_WHITE);
        FT02_LibraryDrawCurrentContent(display);
    } while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);

    // Keep the panel hot through the reading session; only the reader refresh
    // window is widened so stale pixels below the global status bar are cleared.
    display.setFullWindow();
    Serial.printf(
        "[FIELD-LIBRARY-A16] reader-clean-slate x=%d y=%d w=%d h=%d screen=%u elapsed=%lums\n",
        px, py, pw, ph, static_cast<unsigned>(g_screen),
        static_cast<unsigned long>(millis() - startedMs)
    );
    g_readingHeaderDirty = false;
    ++g_readingPartialCount;
}

// v0.8.2: Library menus used to route every key press through setFullWindow(),
// producing a slow black/white full-panel flash even when only the selection
// rectangle changed.  Keep the global 0..85 status area untouched and update
// only the actual library UI below it.  The shared interactive runtime keeps
// the controller hot for a short navigation burst and still captures CardKB
// input while BUSY.
static void FT02_LibraryRedrawUiPartialRect(
    FT02Display& display,
    int px, int py, int pw, int ph,
    const char* source
)
{
    px = FT02_LibraryClamp(px, 0, 799);
    py = FT02_LibraryClamp(py, 0, 479);
    if(pw <= 0 || ph <= 0) return;
    if(px + pw > 800) pw = 800 - px;
    if(py + ph > 480) ph = 480 - py;

    const uint32_t startedMs = millis();
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.setPartialWindow(px, py, pw, ph);
    display.firstPage();
    do
    {
        display.fillRect(px, py, pw, ph, GxEPD_WHITE);
        FT02_LibraryDrawCurrentContent(display);
    } while(display.nextPage());
    FT02_EpdInteractiveEndBusyCapture(display);
    FT02_EpdInteractiveFinishPartial(display, source, startedMs);

    Serial.printf("[FIELD-LIBRARY-V0.8.2] ui-partial source=%s x=%d y=%d w=%d h=%d elapsed=%lums\n",
        source ? source : "unknown", px, py, pw, ph,
        static_cast<unsigned long>(millis() - startedMs));
}

static void FT02_LibraryRedrawUiPartial(FT02Display& display, const char* source)
{
    // Header, menu/list body and bottom hints. The global status bar stays put.
    FT02_LibraryRedrawUiPartialRect(display, 0, 86, 800, 394, source);
}

static void FT02_LibraryRedrawSearchBody(FT02Display& display, const char* source)
{
    // Query box, pinyin/candidates and notice area. Header and bottom hints are
    // stable while typing, so do not refresh them on every keystroke.
    FT02_LibraryRedrawUiPartialRect(display, 28, 140, 744, 252, source);
}

static void FT02_LibraryRedrawSelectionRows(
    FT02Display& display,
    int oldSelected,
    int newSelected,
    int total,
    int baseY,
    int rowStep,
    int rowHeight,
    const char* source
)
{
    if(oldSelected == newSelected) return;
    if(total <= 0)
    {
        FT02_LibraryRedrawUiPartial(display, source);
        return;
    }

    const int oldPage = oldSelected / FT02_LIBRARY_LIST_PAGE_SIZE;
    const int newPage = newSelected / FT02_LIBRARY_LIST_PAGE_SIZE;
    if(oldPage != newPage)
    {
        // A page turn replaces all four list rows, but the header and status bar
        // are unchanged. Clear only the list body.
        FT02_LibraryRedrawUiPartialRect(display, 28, baseY - 6, 744,
            FT02_LIBRARY_LIST_PAGE_SIZE * rowStep + 2, source);
        return;
    }

    const int oldRow = oldSelected % FT02_LIBRARY_LIST_PAGE_SIZE;
    const int newRow = newSelected % FT02_LIBRARY_LIST_PAGE_SIZE;
    const int firstRow = oldRow < newRow ? oldRow : newRow;
    const int lastRow = oldRow > newRow ? oldRow : newRow;
    const int py = baseY + firstRow * rowStep - 3;
    const int ph = (lastRow - firstRow) * rowStep + rowHeight + 6;
    FT02_LibraryRedrawUiPartialRect(display, 28, py, 744, ph, source);
}

static void FT02_LibraryRedraw(FT02Display& display)
{
    if(FT02_LibraryReadingScreen())
    {
        FT02_LibraryRedrawReadingPartial(display);
        return;
    }
    FT02_LibraryRedrawUiPartial(display, "field-library-ui-partial");
}

static FT02FieldLibraryAction FT02_LibraryHandleSearchInput(FT02Display& display, const FT02InputEvent& event)
{
    const uint8_t raw = static_cast<uint8_t>(event.raw);
    if(raw == '`') { FT02_TextInputModeToggle(); FT02_LibraryRedraw(display); return FT02_LIBRARY_ACTION_NONE; }
    // Text input must inspect raw bytes before normalized navigation keys.
    // CardKB maps the literal letter B to BACK globally, but B still needs to
    // be typeable in English/Pinyin queries. ESC is the unambiguous exit key.
    if(raw == 0x1B)
    {
        if(g_searchPinyinLength > 0) FT02_LibrarySearchResetPinyin();
        else { g_screen = FT02_LIBRARY_SCREEN_HOME; FT02_LibraryRedraw(display); }
        return FT02_LIBRARY_ACTION_NONE;
    }
    if(raw == 0x08 || raw == 0x7F)
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
        {
            g_searchPinyin[--g_searchPinyinLength] = '\0';
            g_searchPinyinPage = 0;
        }
        else FT02_LibrarySearchBackspace();
        FT02_LibraryRedrawSearchBody(display, "field-library-search-edit");
        return FT02_LIBRARY_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && (raw == '6' || raw == '<'))
    { FT02_LibrarySearchMovePinyinPage(-1); FT02_LibraryRedrawSearchBody(display, "field-library-search-candidates"); return FT02_LIBRARY_ACTION_NONE; }
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && (raw == '7' || raw == '>'))
    { FT02_LibrarySearchMovePinyinPage(+1); FT02_LibraryRedrawSearchBody(display, "field-library-search-candidates"); return FT02_LIBRARY_ACTION_NONE; }
    if(raw == '\r' || raw == '\n')
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0)
        {
            FT02_LibrarySearchCommitDefault();
            FT02_LibraryRedrawSearchBody(display, "field-library-search-commit");
            return FT02_LIBRARY_ACTION_NONE;
        }
        FT02_LibrarySearchExecute();
        FT02_LibraryRedraw(display);
        return FT02_LIBRARY_ACTION_NONE;
    }
    if(raw == ' ')
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0) FT02_LibrarySearchCommitDefault();
        else FT02_LibrarySearchAppend(" ", 1);
        FT02_LibraryRedrawSearchBody(display, "field-library-search-edit");
        return FT02_LIBRARY_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0 && raw >= '1' && raw <= '5')
    {
        FT02_LibrarySearchCommitCandidate(static_cast<size_t>(raw - '1'));
        FT02_LibraryRedrawSearchBody(display, "field-library-search-candidate");
        return FT02_LIBRARY_ACTION_NONE;
    }
    if((raw >= 'a' && raw <= 'z') || (raw >= 'A' && raw <= 'Z'))
    {
        if(FT02_TextInputModeIsChinese()) FT02_LibrarySearchAppendPinyin(static_cast<char>(raw));
        else { const char c = static_cast<char>(raw); FT02_LibrarySearchAppend(&c, 1); }
        FT02_LibraryRedrawSearchBody(display, "field-library-search-type");
        return FT02_LIBRARY_ACTION_NONE;
    }
    if(raw >= 0x20 && raw < 0x7F)
    {
        if(FT02_TextInputModeIsChinese() && g_searchPinyinLength > 0) FT02_LibrarySearchCommitDefault();
        const char c = static_cast<char>(raw);
        FT02_LibrarySearchAppend(&c, 1);
        FT02_LibraryRedrawSearchBody(display, "field-library-search-type");
    }
    return FT02_LIBRARY_ACTION_NONE;
}

} // namespace

void FT02_FieldLibraryBegin(bool forceReload)
{
    if(g_started && !forceReload) return;
    FT02_LibraryCloseReader();
    g_started = true;
    g_ready = false;
    g_screen = FT02_LIBRARY_SCREEN_HOME;
    g_homeSelected = g_categorySelected = g_bookSelected = g_tocSelected = g_searchSelected = 0;
    g_activeCategory = -1;
    g_activeBook = -1;
    g_activeChapter = 0;
    g_activeMedia = 0;
    g_mediaCount = 0;
    g_mediaScrollY = 0;
    g_mediaFitPage = false;
    g_mediaOpenedFromText = false;
    g_readingPartialCount = 0;
    if(!FT02_LibraryEnsurePsramBuffers()) return;
    FT02_LibrarySearchReset(true);
    FT02_LibraryLoadCatalog();
    if(g_ready) FT02_LibraryMigrateLegacyProgress();
    if(g_ready) g_screen = FT02_LIBRARY_SCREEN_HOME;
    Serial.printf("[FIELD-LIBRARY-V0.8.2-BW] ready=%d P0-books=%d categories=%d\n",
                  g_ready ? 1 : 0, g_bookCount, g_categoryCount);
}

void FT02_FieldLibraryReset()
{
    FT02_LibraryCloseReader();
    g_screen = g_ready ? FT02_LIBRARY_SCREEN_HOME : FT02_LIBRARY_SCREEN_ERROR;
    g_homeSelected = g_categorySelected = g_bookSelected = g_tocSelected = g_searchSelected = 0;
    g_activeCategory = -1;
    g_activeBook = -1;
    g_activeChapter = 0;
    g_activeMedia = 0;
    g_mediaCount = 0;
    g_mediaScrollY = 0;
    g_mediaFitPage = false;
    g_mediaOpenedFromText = false;
    g_readingPartialCount = 0;
    FT02_LibrarySearchReset(true);
}

void FT02_DrawFieldLibraryScreen(FT02Display& display)
{
    // The top status bar is shared with the surrounding FT-02 UI. Clear and
    // replace only the module surface below it, so entering the library no
    // longer pays a full-screen waveform.
    FT02_LibraryRedrawUiPartial(display, "field-library-entry-partial");
}

FT02FieldLibraryAction FT02_HandleFieldLibraryInput(FT02Display& display, const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_NONE && event.raw == 0) return FT02_LIBRARY_ACTION_NONE;

    if(g_screen == FT02_LIBRARY_SCREEN_SEARCH)
        return FT02_LibraryHandleSearchInput(display, event);

    if(g_screen == FT02_LIBRARY_SCREEN_ERROR)
    {
        if(event.key == FT02_KEY_BACK) return FT02_LIBRARY_ACTION_EXIT_HOME;
        if(event.key == FT02_KEY_SELECT) { FT02_FieldLibraryBegin(true); FT02_LibraryRedraw(display); }
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_HOME)
    {
        if(event.key == FT02_KEY_BACK) return FT02_LIBRARY_ACTION_EXIT_HOME;
        const int oldSelected = g_homeSelected;
        if(event.key == FT02_KEY_UP) g_homeSelected = FT02_LibraryClamp(g_homeSelected - 1, 0, 3);
        else if(event.key == FT02_KEY_DOWN) g_homeSelected = FT02_LibraryClamp(g_homeSelected + 1, 0, 3);
        else if(event.key == FT02_KEY_SELECT)
        {
            if(g_homeSelected == 0)
            {
                if(!FT02_LibraryOpenMostRecent()) { g_activeCategory = -1; g_bookSelected = 0; g_screen = FT02_LIBRARY_SCREEN_BOOKS; }
            }
            else if(g_homeSelected == 1) { g_categorySelected = 0; g_screen = FT02_LIBRARY_SCREEN_CATEGORIES; }
            else if(g_homeSelected == 2) { g_activeCategory = -1; g_bookSelected = 0; g_screen = FT02_LIBRARY_SCREEN_BOOKS; }
            else { FT02_LibrarySearchReset(false); g_screen = FT02_LIBRARY_SCREEN_SEARCH; }
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else return FT02_LIBRARY_ACTION_NONE;
        FT02_LibraryRedrawSelectionRows(display, oldSelected, g_homeSelected, 4, 138, 66, 58,
                                        "field-library-home-select");
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_CATEGORIES)
    {
        const int oldSelected = g_categorySelected;
        if(event.key == FT02_KEY_BACK)
        {
            g_screen = FT02_LIBRARY_SCREEN_HOME;
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else if(g_categoryCount > 0 && event.key == FT02_KEY_UP) g_categorySelected = FT02_LibraryClamp(g_categorySelected - 1, 0, g_categoryCount - 1);
        else if(g_categoryCount > 0 && event.key == FT02_KEY_DOWN) g_categorySelected = FT02_LibraryClamp(g_categorySelected + 1, 0, g_categoryCount - 1);
        else if(g_categoryCount > 0 && event.key == FT02_KEY_LEFT) g_categorySelected = FT02_LibraryClamp(g_categorySelected - 4, 0, g_categoryCount - 1);
        else if(g_categoryCount > 0 && event.key == FT02_KEY_RIGHT) g_categorySelected = FT02_LibraryClamp(g_categorySelected + 4, 0, g_categoryCount - 1);
        else if(g_categoryCount > 0 && event.key == FT02_KEY_SELECT)
        {
            g_activeCategory = g_categorySelected; g_bookSelected = 0; g_screen = FT02_LIBRARY_SCREEN_BOOKS;
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else return FT02_LIBRARY_ACTION_NONE;
        FT02_LibraryRedrawSelectionRows(display, oldSelected, g_categorySelected, g_categoryCount, 142, 68, 62,
                                        "field-library-category-select");
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_BOOKS)
    {
        const int total = FT02_LibraryBookCountForCategory(g_activeCategory);
        const int oldSelected = g_bookSelected;
        if(event.key == FT02_KEY_BACK)
        {
            g_screen = g_activeCategory >= 0 ? FT02_LIBRARY_SCREEN_CATEGORIES : FT02_LIBRARY_SCREEN_HOME;
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else if(total > 0 && event.key == FT02_KEY_UP) g_bookSelected = FT02_LibraryClamp(g_bookSelected - 1, 0, total - 1);
        else if(total > 0 && event.key == FT02_KEY_DOWN) g_bookSelected = FT02_LibraryClamp(g_bookSelected + 1, 0, total - 1);
        else if(total > 0 && event.key == FT02_KEY_LEFT) g_bookSelected = FT02_LibraryClamp(g_bookSelected - 4, 0, total - 1);
        else if(total > 0 && event.key == FT02_KEY_RIGHT) g_bookSelected = FT02_LibraryClamp(g_bookSelected + 4, 0, total - 1);
        else if(total > 0 && event.key == FT02_KEY_SELECT)
        {
            FT02_LibraryOpenBook(FT02_LibraryGlobalBookIndex(g_activeCategory, g_bookSelected));
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else return FT02_LIBRARY_ACTION_NONE;
        FT02_LibraryRedrawSelectionRows(display, oldSelected, g_bookSelected, total, 142, 68, 62,
                                        "field-library-book-select");
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_TOC)
    {
        const int oldSelected = g_tocSelected;
        if(event.key == FT02_KEY_BACK)
        {
            g_screen = g_tocReturnScreen;
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else if(g_chapterCount > 0 && event.key == FT02_KEY_UP) g_tocSelected = FT02_LibraryClamp(g_tocSelected - 1, 0, g_chapterCount - 1);
        else if(g_chapterCount > 0 && event.key == FT02_KEY_DOWN) g_tocSelected = FT02_LibraryClamp(g_tocSelected + 1, 0, g_chapterCount - 1);
        else if(g_chapterCount > 0 && event.key == FT02_KEY_LEFT) g_tocSelected = FT02_LibraryClamp(g_tocSelected - 4, 0, g_chapterCount - 1);
        else if(g_chapterCount > 0 && event.key == FT02_KEY_RIGHT) g_tocSelected = FT02_LibraryClamp(g_tocSelected + 4, 0, g_chapterCount - 1);
        else if(g_chapterCount > 0 && event.key == FT02_KEY_SELECT)
        {
            if(g_chapters[g_tocSelected].mediaTarget >= 0)
            {
                FT02_LibraryMediaSetIndex(g_chapters[g_tocSelected].mediaTarget);
                g_mediaFitPage = false;
                g_mediaOpenedFromText = false;
                g_readingHeaderDirty = true;
                g_screen = FT02_LIBRARY_SCREEN_MEDIA_READER;
                FT02_LibrarySaveProgress();
            }
            else
            {
                g_readerHistoryCount = 0;
                FT02_LibraryOpenChapterFile(g_tocSelected, 0);
                FT02_LibrarySaveProgress();
                g_readingHeaderDirty = true;
                g_screen = FT02_LIBRARY_SCREEN_READER;
            }
        }
        else return FT02_LIBRARY_ACTION_NONE;
        if(g_screen == FT02_LIBRARY_SCREEN_TOC)
            FT02_LibraryRedrawSelectionRows(display, oldSelected, g_tocSelected, g_chapterCount, 142, 68, 62,
                                            "field-library-toc-select");
        else
            FT02_LibraryRedraw(display);
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_SEARCH_RESULTS)
    {
        const int oldSelected = g_searchSelected;
        if(event.key == FT02_KEY_BACK)
        {
            g_screen = FT02_LIBRARY_SCREEN_SEARCH;
            FT02_LibraryRedraw(display);
            return FT02_LIBRARY_ACTION_NONE;
        }
        else if(g_searchResultCount > 0 && event.key == FT02_KEY_UP) g_searchSelected = FT02_LibraryClamp(g_searchSelected - 1, 0, g_searchResultCount - 1);
        else if(g_searchResultCount > 0 && event.key == FT02_KEY_DOWN) g_searchSelected = FT02_LibraryClamp(g_searchSelected + 1, 0, g_searchResultCount - 1);
        else if(g_searchResultCount > 0 && event.key == FT02_KEY_LEFT) g_searchSelected = FT02_LibraryClamp(g_searchSelected - 4, 0, g_searchResultCount - 1);
        else if(g_searchResultCount > 0 && event.key == FT02_KEY_RIGHT) g_searchSelected = FT02_LibraryClamp(g_searchSelected + 4, 0, g_searchResultCount - 1);
        else if(g_searchResultCount > 0 && event.key == FT02_KEY_SELECT)
        {
            const FT02LibrarySearchResult r = g_searchResults[g_searchSelected];
            FT02_LibraryOpenBookAt(r.bookIndex, r.chapterIndex, r.offset, true);
            FT02_LibrarySaveProgress();
        }
        else return FT02_LIBRARY_ACTION_NONE;
        if(g_screen == FT02_LIBRARY_SCREEN_SEARCH_RESULTS)
            FT02_LibraryRedrawSelectionRows(display, oldSelected, g_searchSelected, g_searchResultCount, 142, 68, 62,
                                            "field-library-search-select");
        else
            FT02_LibraryRedraw(display);
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_READER)
    {
        const uint8_t raw = static_cast<uint8_t>(event.raw);
        if(event.key == FT02_KEY_BACK)
        {
            FT02_LibrarySaveProgress();
            FT02_LibraryCloseReader();
            g_activeBook = -1;
            g_screen = FT02_LIBRARY_SCREEN_BOOKS;
        }
        else if(event.key == FT02_KEY_SELECT)
        {
            g_tocSelected = g_activeChapter;
            g_tocReturnScreen = FT02_LIBRARY_SCREEN_READER;
            g_screen = FT02_LIBRARY_SCREEN_TOC;
        }
        else if(raw == 'i' || raw == 'I')
        {
            const int first = g_readerInlineMedia >= 0 ? g_readerInlineMedia : FT02_LibraryFirstMediaForChapter(g_activeChapter);
            if(first >= 0)
            {
                g_activeMedia = first;
                g_mediaScrollY = 0;
                g_mediaFitPage = false;
                g_mediaOpenedFromText = true;
                g_readingHeaderDirty = true;
                g_screen = FT02_LIBRARY_SCREEN_MEDIA_READER;
            }
            else return FT02_LIBRARY_ACTION_NONE;
        }
        else if(raw == 's' || raw == 'S')
        {
            FT02_LibrarySaveProgress();
            FT02_LibraryCloseReader();
            g_activeBook = -1;
            FT02_LibrarySearchReset(false);
            g_screen = FT02_LIBRARY_SCREEN_SEARCH;
        }
        else if(event.key == FT02_KEY_RIGHT || event.key == FT02_KEY_DOWN)
        {
            FT02_LibraryNextPage();
            FT02_LibraryCoalesceQueuedReaderTurns();
        }
        else if(event.key == FT02_KEY_LEFT || event.key == FT02_KEY_UP)
        {
            FT02_LibraryPrevPage();
            FT02_LibraryCoalesceQueuedReaderTurns();
        }
        else if(event.key == FT02_KEY_CHAR && event.command == '0') FT02_LibraryStartBook();
        else return FT02_LIBRARY_ACTION_NONE;
        FT02_LibraryRedraw(display);
        return FT02_LIBRARY_ACTION_NONE;
    }

    if(g_screen == FT02_LIBRARY_SCREEN_MEDIA_READER)
    {
        const uint8_t raw = static_cast<uint8_t>(event.raw);
        if(event.key == FT02_KEY_BACK)
        {
            FT02_LibrarySaveProgress();
            if(g_mediaOpenedFromText && !FT02_LibraryBookIsScanPdf(g_activeBook))
            {
                g_readingHeaderDirty = true;
                g_screen = FT02_LIBRARY_SCREEN_READER;
            }
            else
            {
                FT02_LibraryCloseReader();
                g_activeBook = -1;
                g_screen = FT02_LIBRARY_SCREEN_BOOKS;
            }
        }
        else if(event.key == FT02_KEY_SELECT)
        {
            g_tocReturnScreen = FT02_LIBRARY_SCREEN_MEDIA_READER;
            // For scan PDFs select the nearest TOC entry at/before this page.
            if(FT02_LibraryBookIsScanPdf(g_activeBook) && g_chapterCount > 0)
            {
                int nearest = 0;
                for(int i = 0; i < g_chapterCount; ++i)
                {
                    if(g_chapters[i].mediaTarget <= g_activeMedia && g_chapters[i].mediaTarget >= 0) nearest = i;
                    else if(g_chapters[i].mediaTarget > g_activeMedia) break;
                }
                g_tocSelected = nearest;
            }
            else g_tocSelected = FT02_LibraryClamp(g_activeChapter, 0, g_chapterCount - 1);
            g_screen = FT02_LIBRARY_SCREEN_TOC;
        }
        else if(raw == 'f' || raw == 'F')
        {
            g_mediaFitPage = !g_mediaFitPage;
            g_mediaScrollY = 0;
            FT02_LibrarySaveProgress();
        }
        else if(event.key == FT02_KEY_RIGHT)
        {
            FT02_LibraryMediaSetIndex(g_activeMedia + 1);
            FT02_LibraryCoalesceQueuedMediaNavigation();
            FT02_LibrarySaveProgress();
        }
        else if(event.key == FT02_KEY_LEFT)
        {
            FT02_LibraryMediaSetIndex(g_activeMedia - 1);
            FT02_LibraryCoalesceQueuedMediaNavigation();
            FT02_LibrarySaveProgress();
        }
        else if(event.key == FT02_KEY_DOWN)
        {
            FT02_LibraryMediaScroll(260);
            FT02_LibraryCoalesceQueuedMediaNavigation();
            FT02_LibrarySaveProgress();
        }
        else if(event.key == FT02_KEY_UP)
        {
            FT02_LibraryMediaScroll(-260);
            FT02_LibraryCoalesceQueuedMediaNavigation();
            FT02_LibrarySaveProgress();
        }
        else return FT02_LIBRARY_ACTION_NONE;
        FT02_LibraryRedraw(display);
        return FT02_LIBRARY_ACTION_NONE;
    }

    return FT02_LIBRARY_ACTION_NONE;
}

bool FT02_FieldLibraryReady() { return g_ready; }
int FT02_FieldLibraryBookCount() { return g_bookCount; }
