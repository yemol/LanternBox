#include "FT02_MapSearch.h"

#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "FT02_EpdLifecycle.h"
#include "FT02_GlobalCJK20FontData.h"
#include "FT02_GlobalCJKFontData.h"
#include "FT02_GlobalCJKBoldFontData.h"
#include "FT02_PinyinIme.h"
#include "FT02_PinyinLearning.h"
#include "FT02_StatusBar.h"
#include "FT02_TextInputMode.h"
#include "FT02_Storage.h"

namespace
{
enum SearchMode : uint8_t
{
    SEARCH_QUERY = 0,
    SEARCH_RESULTS
};

constexpr uint32_t SEARCH_REDRAW_IDLE_MS = 650u;
constexpr size_t PINYIN_VISIBLE = FT02_PINYIN_PAGE_SIZE;
constexpr uint8_t CARDKB_MODE_LEFT = static_cast<uint8_t>('<');
constexpr uint8_t CARDKB_MODE_RIGHT = static_cast<uint8_t>('>');

SearchMode g_mode = SEARCH_QUERY;
char g_query[FT02_MAP_SEARCH_QUERY_BYTES + 1] = {};
size_t g_queryLength = 0;
char g_pinyin[FT02_PINYIN_MAX_INPUT + 1] = {};
size_t g_pinyinLength = 0;
size_t g_pinyinPage = 0;
FT02MapSearchResult g_results[FT02_MAP_SEARCH_RESULT_COUNT] = {};
uint8_t g_resultScore[FT02_MAP_SEARCH_RESULT_COUNT] = {};
uint8_t g_resultPriority[FT02_MAP_SEARCH_RESULT_COUNT] = {};
size_t g_resultCount = 0;
size_t g_selected = 0;
char g_notice[96] = {};
bool g_dirty = false;
uint32_t g_lastEditMs = 0;
bool g_jumpReady = false;
FT02MapSearchResult g_jump = {};

void setNotice(const char* text);

#pragma pack(push, 1)
struct SearchIndexHeader
{
    char magic[8];
    uint16_t version;
    uint16_t bucketCount;
    uint16_t recordSize;
    uint16_t reserved0;
    uint32_t uniqueRecordCount;
    uint32_t dataRecordCount;
    uint32_t reserved1;
};

struct SearchBucketEntry
{
    uint32_t offsetBytes;
    uint32_t recordCount;
};

struct SearchIndexRecord
{
    char name[72];
    char detail[72];
    float latitude;
    float longitude;
    uint8_t priority;
    uint8_t reserved[3];
};
#pragma pack(pop)

static_assert(sizeof(SearchIndexRecord) == 156, "Map search binary record size mismatch");
static_assert(sizeof(SearchBucketEntry) == 8, "Map search bucket entry size mismatch");
constexpr char SEARCH_INDEX_MAGIC[8] = {'F','T','M','S','A','4','I','\0'};
constexpr uint16_t SEARCH_INDEX_VERSION = 4;
constexpr uint16_t SEARCH_BUCKET_COUNT = 256;
constexpr size_t SEARCH_STREAM_RECORDS = 8;

SearchBucketEntry g_buckets[SEARCH_BUCKET_COUNT] = {};
bool g_indexReady = false;
uint32_t g_uniqueRecordCount = 0;
uint32_t g_dataRecordCount = 0;

void resetIndexState()
{
    g_indexReady = false;
    g_uniqueRecordCount = 0;
    g_dataRecordCount = 0;
    memset(g_buckets, 0, sizeof(g_buckets));
}

uint32_t decodeFirstUtf8Codepoint(const char* text)
{
    if(text == nullptr || *text == '\0') return 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(text);
    if(p[0] < 0x80u)
    {
        uint8_t c = p[0];
        if(c >= 'A' && c <= 'Z') c = static_cast<uint8_t>(c - 'A' + 'a');
        return c;
    }
    if((p[0] & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u)
        return ((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu);
    if((p[0] & 0xF0u) == 0xE0u && (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u)
        return ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
    if((p[0] & 0xF8u) == 0xF0u && (p[1] & 0xC0u) == 0x80u &&
       (p[2] & 0xC0u) == 0x80u && (p[3] & 0xC0u) == 0x80u)
        return ((p[0] & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) |
               ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
    return p[0];
}

uint8_t bucketForQuery(const char* query)
{
    const uint32_t cp = decodeFirstUtf8Codepoint(query);
    // Same multiplicative hash is used by the offline index builder. Hash
    // collisions are harmless because every candidate is verified by the full
    // UTF-8 matcher before it can enter the result list.
    return static_cast<uint8_t>((cp * 2654435761u) >> 24);
}

bool ensureDiskIndexReady()
{
    if(g_indexReady) return true;

    FILE* f = FT02_StorageOpenReadFile(FT02_MAP_SEARCH_INDEX_PATH);
    if(f == nullptr)
    {
        setNotice("缺少搜索目录 .search.idx");
        Serial.printf("[MAP-SEARCH-A4] index missing path=%s\n", FT02_MAP_SEARCH_INDEX_PATH);
        return false;
    }

    const uint32_t started = millis();
    SearchIndexHeader header = {};
    const bool headerOk =
        fread(&header, 1, sizeof(header), f) == sizeof(header) &&
        memcmp(header.magic, SEARCH_INDEX_MAGIC, sizeof(header.magic)) == 0 &&
        header.version == SEARCH_INDEX_VERSION &&
        header.bucketCount == SEARCH_BUCKET_COUNT &&
        header.recordSize == sizeof(SearchIndexRecord);

    if(!headerOk ||
       fread(g_buckets, sizeof(SearchBucketEntry), SEARCH_BUCKET_COUNT, f) != SEARCH_BUCKET_COUNT)
    {
        fclose(f);
        resetIndexState();
        setNotice("搜索目录版本不匹配，请重新生成");
        Serial.println("[MAP-SEARCH-A4] invalid .search.idx header/directory");
        return false;
    }
    fclose(f);

    if(!FT02_StorageFileExists(FT02_MAP_SEARCH_DATA_PATH))
    {
        resetIndexState();
        setNotice("缺少搜索数据 .search.dat");
        Serial.printf("[MAP-SEARCH-A4] data missing path=%s\n", FT02_MAP_SEARCH_DATA_PATH);
        return false;
    }

    g_uniqueRecordCount = header.uniqueRecordCount;
    g_dataRecordCount = header.dataRecordCount;
    g_indexReady = true;
    Serial.printf("[MAP-SEARCH-A4] directory ready buckets=%u unique=%lu data_records=%lu bytes=%u elapsed=%lums heap=%u psram=%u\n",
                  static_cast<unsigned>(SEARCH_BUCKET_COUNT),
                  static_cast<unsigned long>(g_uniqueRecordCount),
                  static_cast<unsigned long>(g_dataRecordCount),
                  static_cast<unsigned>(sizeof(g_buckets)),
                  static_cast<unsigned long>(millis() - started),
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getFreePsram()));
    return true;
}

void setNotice(const char* text)
{
    snprintf(g_notice, sizeof(g_notice), "%s", text != nullptr ? text : "");
}

void markDirty()
{
    g_dirty = true;
    g_lastEditMs = millis();
}

void resetPinyin()
{
    g_pinyin[0] = '\0';
    g_pinyinLength = 0;
    g_pinyinPage = 0;
}

bool appendUtf8(const char* text)
{
    if(text == nullptr) return false;
    const size_t n = strlen(text);
    if(n == 0 || g_queryLength + n > FT02_MAP_SEARCH_QUERY_BYTES) return false;
    memcpy(g_query + g_queryLength, text, n + 1);
    g_queryLength += n;
    return true;
}

void utf8Backspace()
{
    if(g_queryLength == 0) return;
    size_t i = g_queryLength - 1;
    while(i > 0 && (static_cast<uint8_t>(g_query[i]) & 0xC0u) == 0x80u) --i;
    g_queryLength = i;
    g_query[i] = '\0';
}

void pinyinBackspace()
{
    if(g_pinyinLength == 0) return;
    g_pinyin[--g_pinyinLength] = '\0';
    g_pinyinPage = 0;
    markDirty();
}

void appendPinyin(char raw)
{
    if(g_pinyinLength >= FT02_PINYIN_MAX_INPUT) return;
    char c = static_cast<char>(tolower(static_cast<unsigned char>(raw)));
    if(c < 'a' || c > 'z') return;
    g_pinyin[g_pinyinLength++] = c;
    g_pinyin[g_pinyinLength] = '\0';
    g_pinyinPage = 0;
    markDirty();
}

size_t candidateCount()
{
    return FT02_PinyinImeCandidateCount(g_pinyin);
}

void clampCandidatePage()
{
    const size_t count = candidateCount();
    if(count == 0) { g_pinyinPage = 0; return; }
    const size_t pages = (count + PINYIN_VISIBLE - 1) / PINYIN_VISIBLE;
    if(g_pinyinPage >= pages) g_pinyinPage = pages - 1;
}

bool commitCandidate(size_t slot)
{
    const size_t count = candidateCount();
    const size_t index = g_pinyinPage * PINYIN_VISIBLE + slot;
    if(index >= count) return false;
    char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES] = {};
    size_t consumed = 0;
    if(!FT02_PinyinImeCandidate(g_pinyin, index, candidate, sizeof(candidate), &consumed)) return false;
    char learningKey[FT02_PINYIN_MAX_INPUT + 1] = {};
    if(consumed > g_pinyinLength) consumed = g_pinyinLength;
    memcpy(learningKey, g_pinyin, consumed);
    learningKey[consumed] = '\0';
    if(!appendUtf8(candidate)) { setNotice("搜索词已达到长度上限"); return false; }
    FT02_PinyinLearningRecord(learningKey, candidate);
    if(consumed >= g_pinyinLength) resetPinyin();
    else
    {
        memmove(g_pinyin, g_pinyin + consumed, g_pinyinLength - consumed + 1);
        g_pinyinLength -= consumed;
        g_pinyinPage = 0;
    }
    markDirty();
    return true;
}

bool commitRawPinyin()
{
    if(g_pinyinLength == 0) return false;
    if(!appendUtf8(g_pinyin)) { setNotice("搜索词已达到长度上限"); return false; }
    resetPinyin();
    markDirty();
    return true;
}

bool commitDefaultPinyin()
{
    clampCandidatePage();
    if(candidateCount() > 0) return commitCandidate(0);
    return commitRawPinyin();
}

void moveCandidatePage(int delta)
{
    const size_t count = candidateCount();
    if(count == 0) return;
    const size_t pages = (count + PINYIN_VISIBLE - 1) / PINYIN_VISIBLE;
    int next = static_cast<int>(g_pinyinPage) + delta;
    if(next < 0) next = 0;
    if(next >= static_cast<int>(pages)) next = static_cast<int>(pages) - 1;
    if(static_cast<size_t>(next) != g_pinyinPage)
    {
        g_pinyinPage = static_cast<size_t>(next);
        markDirty();
    }
}

bool asciiEqualsIgnoreCase(const char* a, const char* b)
{
    while(*a && *b)
    {
        const unsigned char ca = static_cast<unsigned char>(*a);
        const unsigned char cb = static_cast<unsigned char>(*b);
        if(ca < 0x80u && cb < 0x80u)
        {
            if(tolower(ca) != tolower(cb)) return false;
        }
        else if(ca != cb) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

bool startsWithIgnoreAsciiCase(const char* text, const char* query)
{
    while(*query)
    {
        if(!*text) return false;
        const unsigned char a = static_cast<unsigned char>(*text);
        const unsigned char b = static_cast<unsigned char>(*query);
        if(a < 0x80u && b < 0x80u)
        {
            if(tolower(a) != tolower(b)) return false;
        }
        else if(a != b) return false;
        ++text; ++query;
    }
    return true;
}

const char* findIgnoreAsciiCase(const char* text, const char* query)
{
    if(query == nullptr || query[0] == '\0') return text;
    for(const char* p = text; *p; ++p)
    {
        if(startsWithIgnoreAsciiCase(p, query)) return p;
        // Only advance at UTF-8 codepoint boundaries. Continuation bytes can
        // never begin a valid UTF-8 query and skipping them avoids false hits.
        const uint8_t c = static_cast<uint8_t>(*p);
        if((c & 0xC0u) == 0xC0u)
        {
            while((static_cast<uint8_t>(p[1]) & 0xC0u) == 0x80u) ++p;
        }
    }
    return nullptr;
}

uint8_t matchScore(const char* name)
{
    if(asciiEqualsIgnoreCase(name, g_query)) return 0;
    if(startsWithIgnoreAsciiCase(name, g_query)) return 1;
    return findIgnoreAsciiCase(name, g_query) != nullptr ? 2 : 255;
}

void insertResult(const FT02MapSearchResult& item, uint8_t score, uint8_t priority)
{
    // Exact duplicate protection is deliberately tiny and bounded.
    for(size_t i = 0; i < g_resultCount; ++i)
    {
        if(strcmp(g_results[i].name, item.name) == 0 &&
           fabs(g_results[i].latitude - item.latitude) < 0.000001 &&
           fabs(g_results[i].longitude - item.longitude) < 0.000001)
            return;
    }

    size_t pos = g_resultCount;
    for(size_t i = 0; i < g_resultCount; ++i)
    {
        if(score < g_resultScore[i] ||
           (score == g_resultScore[i] && priority < g_resultPriority[i]) ||
           (score == g_resultScore[i] && priority == g_resultPriority[i] && strlen(item.name) < strlen(g_results[i].name)))
        {
            pos = i;
            break;
        }
    }
    if(pos >= FT02_MAP_SEARCH_RESULT_COUNT && g_resultCount >= FT02_MAP_SEARCH_RESULT_COUNT) return;
    const size_t oldCount = g_resultCount;
    if(g_resultCount < FT02_MAP_SEARCH_RESULT_COUNT) ++g_resultCount;
    const size_t last = g_resultCount - 1;
    for(size_t i = last; i > pos; --i)
    {
        g_results[i] = g_results[i - 1];
        g_resultScore[i] = g_resultScore[i - 1];
        g_resultPriority[i] = g_resultPriority[i - 1];
    }
    if(pos < FT02_MAP_SEARCH_RESULT_COUNT)
    {
        g_results[pos] = item;
        g_resultScore[pos] = score;
        g_resultPriority[pos] = priority;
    }
    (void)oldCount;
}

void runSearch()
{
    if(g_pinyinLength > 0) (void)commitDefaultPinyin();
    if(g_queryLength == 0) { setNotice("请输入地点名称"); markDirty(); return; }

    g_resultCount = 0;
    g_selected = 0;
    memset(g_results, 0, sizeof(g_results));
    memset(g_resultScore, 0xFF, sizeof(g_resultScore));
    memset(g_resultPriority, 0xFF, sizeof(g_resultPriority));

    if(!ensureDiskIndexReady())
    {
        g_mode = SEARCH_RESULTS;
        return;
    }

    const uint8_t bucketId = bucketForQuery(g_query);
    const SearchBucketEntry bucket = g_buckets[bucketId];
    if(bucket.recordCount == 0)
    {
        g_mode = SEARCH_RESULTS;
        setNotice("没有找到匹配地点");
        Serial.printf("[MAP-SEARCH-A4] query=\"%s\" bucket=%u records=0 results=0 search=0ms\n",
                      g_query, static_cast<unsigned>(bucketId));
        return;
    }

    FILE* f = FT02_StorageOpenReadFile(FT02_MAP_SEARCH_DATA_PATH);
    if(f == nullptr)
    {
        g_mode = SEARCH_RESULTS;
        setNotice("搜索数据文件无法打开");
        Serial.printf("[MAP-SEARCH-A4] data open failed path=%s\n", FT02_MAP_SEARCH_DATA_PATH);
        return;
    }

    const uint32_t started = millis();
    if(fseek(f, static_cast<long>(bucket.offsetBytes), SEEK_SET) != 0)
    {
        fclose(f);
        g_mode = SEARCH_RESULTS;
        setNotice("搜索数据定位失败");
        Serial.printf("[MAP-SEARCH-A4] seek failed bucket=%u offset=%lu\n",
                      static_cast<unsigned>(bucketId), static_cast<unsigned long>(bucket.offsetBytes));
        return;
    }

    SearchIndexRecord stream[SEARCH_STREAM_RECORDS] = {};
    uint32_t remaining = bucket.recordCount;
    uint32_t scanned = 0;
    size_t bytesRead = 0;
    bool readError = false;
    while(remaining > 0)
    {
        const size_t wanted = remaining > SEARCH_STREAM_RECORDS ? SEARCH_STREAM_RECORDS : remaining;
        const size_t got = fread(stream, sizeof(SearchIndexRecord), wanted, f);
        if(got == 0)
        {
            readError = true;
            break;
        }
        bytesRead += got * sizeof(SearchIndexRecord);
        scanned += static_cast<uint32_t>(got);
        remaining -= static_cast<uint32_t>(got);

        for(size_t i = 0; i < got; ++i)
        {
            const SearchIndexRecord& source = stream[i];
            const uint8_t score = matchScore(source.name);
            if(score == 255) continue;
            FT02MapSearchResult item = {};
            snprintf(item.name, sizeof(item.name), "%s", source.name);
            snprintf(item.detail, sizeof(item.detail), "%s", source.detail);
            item.latitude = source.latitude;
            item.longitude = source.longitude;
            insertResult(item, score, source.priority);
        }

        if(got < wanted)
        {
            readError = true;
            break;
        }
    }
    fclose(f);

    g_mode = SEARCH_RESULTS;
    if(readError)
        setNotice("搜索数据读取不完整");
    else if(g_resultCount == 0)
        setNotice("没有找到匹配地点");
    else
        snprintf(g_notice, sizeof(g_notice), "找到 %u 个有效地点", static_cast<unsigned>(g_resultCount));

    Serial.printf("[MAP-SEARCH-A4] query=\"%s\" bucket=%u bucket_records=%lu scanned=%lu bytes=%u results=%u search=%lums heap=%u psram=%u\n",
                  g_query, static_cast<unsigned>(bucketId),
                  static_cast<unsigned long>(bucket.recordCount),
                  static_cast<unsigned long>(scanned),
                  static_cast<unsigned>(bytesRead),
                  static_cast<unsigned>(g_resultCount),
                  static_cast<unsigned long>(millis() - started),
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getFreePsram()));
}

void drawQuery(FT02Display& display)
{
    FT02_DrawTextPack(display, ft02_cjk_24b, "地图搜索", 28, 112);
    FT02_DrawTextPack(display, ft02_cjk_20r,
        FT02_TextInputModeIsChinese() ? "输入模式：中文   ` 切换英文" : "输入模式：英文   ` 切换中文",
        28, 146);

    display.drawRect(26, 164, 748, 58, GxEPD_BLACK);
    char shown[128] = {};
    snprintf(shown, sizeof(shown), "%s%s%s", g_query,
             g_pinyinLength > 0 ? " [" : "",
             g_pinyinLength > 0 ? g_pinyin : "");
    if(g_pinyinLength > 0 && strlen(shown) + 2 < sizeof(shown)) strcat(shown, "]");
    FT02_DrawTextPack(display, ft02_cjk_24r, shown[0] ? shown : "请输入地点名称", 40, 200);

    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
    {
        char segmented[48] = {};
        FT02_PinyinImeSegmentDisplay(g_pinyin, segmented, sizeof(segmented));
        char pyline[80] = {};
        snprintf(pyline, sizeof(pyline), "拼音：%s", segmented);
        FT02_DrawTextPack(display, ft02_cjk_20r, pyline, 30, 256);

        const size_t count = candidateCount();
        char candLine[260] = {};
        size_t used = 0;
        for(size_t slot = 0; slot < PINYIN_VISIBLE; ++slot)
        {
            const size_t idx = g_pinyinPage * PINYIN_VISIBLE + slot;
            char cand[FT02_PINYIN_MAX_CANDIDATE_BYTES] = {};
            if(idx >= count || !FT02_PinyinImeCandidate(g_pinyin, idx, cand, sizeof(cand))) break;
            int n = snprintf(candLine + used, sizeof(candLine) - used,
                             "%u.%s%s", static_cast<unsigned>(slot + 1), cand,
                             slot + 1 < PINYIN_VISIBLE ? "   " : "");
            if(n <= 0 || static_cast<size_t>(n) >= sizeof(candLine) - used) break;
            used += static_cast<size_t>(n);
        }
        FT02_DrawTextPack(display, ft02_cjk_20r, candLine, 30, 298);
        FT02_DrawTextPack(display, ft02_cjk_20r, "</> 或 6/7 翻页   0 原样上屏", 30, 334);
    }
    else
    {
        FT02_DrawTextPack(display, ft02_cjk_20r, "ENTER 搜索   DEL 删除   Sym+B/ESC 返回地图", 30, 290);
    }
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 30, 398);
}

void drawResults(FT02Display& display)
{
    char title[128] = {};
    snprintf(title, sizeof(title), "搜索：%s", g_query);
    FT02_DrawTextPack(display, ft02_cjk_24b, title, 28, 108);

    if(g_resultCount == 0)
    {
        FT02_DrawTextPack(display, ft02_cjk_24r, "没有匹配结果", 32, 205);
    }
    else
    {
        // Three roomy two-line rows keep the detail line fully visible on the
        // 4.26-inch panel. A4 used four 72 px rows; the 20 px AA font detail
        // line could be clipped by the next selection band. A5 gives each
        // result its own 88 px card-sized vertical slot.
        constexpr size_t VISIBLE_ROWS = 3;
        size_t first = 0;
        if(g_selected >= VISIBLE_ROWS) first = g_selected - VISIBLE_ROWS + 1;
        if(first + VISIBLE_ROWS > g_resultCount && g_resultCount > VISIBLE_ROWS)
            first = g_resultCount - VISIBLE_ROWS;

        int y = 150;
        for(size_t row = 0; row < VISIBLE_ROWS; ++row)
        {
            const size_t i = first + row;
            if(i >= g_resultCount) break;
            const bool selected = i == g_selected;
            if(selected) display.fillRect(22, y - 29, 756, 78, GxEPD_BLACK);

            char nameLine[100] = {};
            snprintf(nameLine, sizeof(nameLine), "%u. %s", static_cast<unsigned>(i + 1), g_results[i].name);
            char detail[128] = {};
            snprintf(detail, sizeof(detail), "%s", g_results[i].detail[0] ? g_results[i].detail : "地点");

            if(selected)
            {
                FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, nameLine, 34, y, 28, y - 28, 740, 32);
                FT02_DrawTextPackInvertClipped(display, ft02_cjk_20r, detail, 54, y + 36, 28, y + 4, 720, 34);
            }
            else
            {
                FT02_DrawTextPackClipped(display, ft02_cjk_20r, nameLine, 34, y, 28, y - 28, 740, 32);
                FT02_DrawTextPackClipped(display, ft02_cjk_20r, detail, 54, y + 36, 28, y + 4, 720, 34);
            }
            y += 88;
        }

        char position[48] = {};
        snprintf(position, sizeof(position), "%u/%u", static_cast<unsigned>(g_selected + 1), static_cast<unsigned>(g_resultCount));
        FT02_DrawTextPack(display, ft02_cjk_20r, position, 700, 425);
    }
    if(g_notice[0]) FT02_DrawTextPack(display, ft02_cjk_20r, g_notice, 30, 425);
}

}

void FT02_MapSearchOpen()
{
    resetIndexState();
    g_mode = SEARCH_QUERY;
    g_query[0] = '\0';
    g_queryLength = 0;
    resetPinyin();
    g_resultCount = 0;
    g_selected = 0;
    g_notice[0] = '\0';
    g_dirty = false;
    g_jumpReady = false;
    Serial.printf("[MAP-SEARCH-A4] open idx=%s exists=%d dat=%s exists=%d\n",
                  FT02_MAP_SEARCH_INDEX_PATH,
                  FT02_StorageFileExists(FT02_MAP_SEARCH_INDEX_PATH) ? 1 : 0,
                  FT02_MAP_SEARCH_DATA_PATH,
                  FT02_StorageFileExists(FT02_MAP_SEARCH_DATA_PATH) ? 1 : 0);
}

void FT02_DrawMapSearchScreen(FT02Display& display)
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        FT02_DrawStatusBar(display);
        display.drawLine(0, 76, 799, 76, GxEPD_BLACK);
        if(g_mode == SEARCH_QUERY) drawQuery(display);
        else drawResults(display);
        display.drawLine(0, 440, 799, 440, GxEPD_BLACK);
        FT02_DrawTextPack(display, ft02_cjk_20r,
            g_mode == SEARCH_QUERY ? "ENTER 搜索 | Sym+B/ESC 返回" : "↑↓ 选择 | ENTER 定位 | B 返回输入",
            24, 469);
    }
    while(display.nextPage());
    display.setFullWindow();
    FT02_EpdPowerOffAfterCommit(display, "map-search-full");
    g_dirty = false;
}

FT02MapSearchAction FT02_MapSearchHandleInput(const FT02InputEvent& event)
{
    const uint8_t raw = static_cast<uint8_t>(event.raw);

    if(g_mode == SEARCH_RESULTS)
    {
        if(event.key == FT02_KEY_UP || event.key == FT02_KEY_LEFT)
        {
            if(g_selected > 0) --g_selected;
            return FT02_MAP_SEARCH_ACTION_REDRAW;
        }
        if(event.key == FT02_KEY_DOWN || event.key == FT02_KEY_RIGHT)
        {
            if(g_selected + 1 < g_resultCount) ++g_selected;
            return FT02_MAP_SEARCH_ACTION_REDRAW;
        }
        if(event.key == FT02_KEY_SELECT && g_resultCount > 0)
        {
            g_jump = g_results[g_selected];
            g_jumpReady = true;
            return FT02_MAP_SEARCH_ACTION_JUMP;
        }
        if(event.key == FT02_KEY_BACK)
        {
            g_mode = SEARCH_QUERY;
            setNotice("");
            return FT02_MAP_SEARCH_ACTION_REDRAW;
        }
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    // CardKB2 DEL is 0x08 and InputManager labels it BACK globally. In a text
    // editor DEL must edit the query; ESC or the normalized B command exits.
    if(raw == 0x08u || raw == 0x7Fu)
    {
        if(g_pinyinLength > 0) pinyinBackspace();
        else { utf8Backspace(); markDirty(); }
        return FT02_MAP_SEARCH_ACTION_NONE;
    }
    // Normal ASCII 'b' is legitimate Pinyin/text and InputManager labels it
    // BACK globally. Do not use event.key here. CardKB2 Sym+B arrives as '>'
    // and acts as candidate-next while Pinyin is active; with no composition it
    // is the deterministic return-to-map shortcut. ESC is always return.
    if(raw == 0x1Bu || (raw == CARDKB_MODE_RIGHT && g_pinyinLength == 0))
    {
        return FT02_MAP_SEARCH_ACTION_EXIT_MAP;
    }

    if(raw == static_cast<uint8_t>('`'))
    {
        if(g_pinyinLength > 0) (void)commitDefaultPinyin();
        const FT02TextInputMode mode = FT02_TextInputModeToggle();
        setNotice(mode == FT02_TEXT_INPUT_CN ? "已切换中文输入" : "已切换英文输入");
        markDirty();
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 &&
       (raw == CARDKB_MODE_LEFT || raw == '6'))
    {
        moveCandidatePage(-1);
        return FT02_MAP_SEARCH_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 &&
       (raw == CARDKB_MODE_RIGHT || raw == '7'))
    {
        moveCandidatePage(+1);
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    if(raw == '\r' || raw == '\n')
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0)
        {
            (void)commitDefaultPinyin();
            return FT02_MAP_SEARCH_ACTION_NONE;
        }
        runSearch();
        return FT02_MAP_SEARCH_ACTION_REDRAW;
    }

    if(raw == ' ')
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0) (void)commitDefaultPinyin();
        else if(appendUtf8(" ")) markDirty();
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && raw >= '1' && raw <= '5')
    {
        if(!commitCandidate(static_cast<size_t>(raw - '1'))) setNotice("该候选不存在");
        return FT02_MAP_SEARCH_ACTION_NONE;
    }
    if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0 && raw == '0')
    {
        (void)commitRawPinyin();
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    if((raw >= 'a' && raw <= 'z') || (raw >= 'A' && raw <= 'Z'))
    {
        if(FT02_TextInputModeIsChinese()) appendPinyin(static_cast<char>(raw));
        else
        {
            char c[2] = {static_cast<char>(raw), '\0'};
            if(appendUtf8(c)) markDirty();
        }
        return FT02_MAP_SEARCH_ACTION_NONE;
    }

    if(raw >= 0x20u && raw < 0x7Fu)
    {
        if(FT02_TextInputModeIsChinese() && g_pinyinLength > 0) (void)commitDefaultPinyin();
        char c[2] = {static_cast<char>(raw), '\0'};
        if(appendUtf8(c)) markDirty();
    }
    return FT02_MAP_SEARCH_ACTION_NONE;
}

bool FT02_MapSearchTakeDeferredRedraw(uint32_t nowMs)
{
    if(g_mode != SEARCH_QUERY || !g_dirty) return false;
    if(nowMs - g_lastEditMs < SEARCH_REDRAW_IDLE_MS) return false;
    g_dirty = false;
    return true;
}

bool FT02_MapSearchTakeJump(double& longitude, double& latitude, char* name, size_t nameBytes)
{
    if(!g_jumpReady) return false;
    g_jumpReady = false;
    longitude = g_jump.longitude;
    latitude = g_jump.latitude;
    if(name != nullptr && nameBytes > 0) snprintf(name, nameBytes, "%s", g_jump.name);
    return true;
}
