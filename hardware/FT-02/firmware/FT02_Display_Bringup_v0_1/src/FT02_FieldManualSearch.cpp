#include "FT02_FieldManualSearch.h"

#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "FT02_FieldManual.h"

namespace
{
constexpr int MAX_TERMS = 8;

struct SearchResult
{
    int globalIndex;
    int score;
};

SearchResult g_results[FT02_FIELD_MANUAL_SEARCH_MAX_RESULTS] = {};
int g_resultCount = 0;
char g_lastQuery[FT02_FIELD_MANUAL_SEARCH_MAX_QUERY_BYTES + 1] = {};

// Search aliases are deliberately separate from the safety-reviewed card
// prose. They let ordinary field wording recall the correct safe action card
// without weakening the literal full-text search or rewriting card content.
// Keep aliases narrow: a colloquial term maps to a reviewed card, not to an
// instruction for an unsafe interpretation of that term.
struct SearchAlias
{
    const char* term;
    const char* cardId;
};

constexpr SearchAlias SEARCH_ALIASES[] = {
    {"陷阱", "CMP003"},      // safe camp perimeter awareness, never a harming trap
    {"营地陷阱", "CMP003"},
    {"野外陷阱", "CMP003"},
    {"捕兽陷阱", "CMP003"},
    {"警戒线", "CMP003"},
    {"报警线", "CMP003"},
    {"警报线", "CMP003"}
};

unsigned char foldAscii(unsigned char value)
{
    if(value >= 'A' && value <= 'Z') return static_cast<unsigned char>(value - 'A' + 'a');
    return value;
}

bool equalsFold(const char* a, const char* b)
{
    if(a == nullptr || b == nullptr) return false;
    while(*a && *b)
    {
        if(foldAscii(static_cast<unsigned char>(*a)) != foldAscii(static_cast<unsigned char>(*b))) return false;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

bool startsWithFold(const char* text, const char* term)
{
    if(text == nullptr || term == nullptr || term[0] == '\0') return false;
    while(*term)
    {
        if(*text == '\0') return false;
        if(foldAscii(static_cast<unsigned char>(*text)) != foldAscii(static_cast<unsigned char>(*term))) return false;
        ++text;
        ++term;
    }
    return true;
}

bool containsFold(const char* text, const char* term)
{
    if(text == nullptr || term == nullptr || term[0] == '\0') return false;
    for(const char* start = text; *start; ++start)
    {
        const char* a = start;
        const char* b = term;
        while(*a && *b && foldAscii(static_cast<unsigned char>(*a)) == foldAscii(static_cast<unsigned char>(*b)))
        {
            ++a;
            ++b;
        }
        if(*b == '\0') return true;
    }
    return false;
}

const FT02FieldManualCategory* categoryForCard(const FT02FieldManualCardIndex& card)
{
    const int count = FT02_FieldManualCategoryCount();
    for(int i = 0; i < count; ++i)
    {
        const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(i);
        if(category != nullptr && strcmp(category->id, card.category) == 0) return category;
    }
    return nullptr;
}

int splitTerms(char* query, const char** terms, int capacity)
{
    int count = 0;
    char* cursor = query;
    while(*cursor && count < capacity)
    {
        while(*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') ++cursor;
        if(!*cursor) break;
        terms[count++] = cursor;
        while(*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' && *cursor != '\n') ++cursor;
        if(*cursor) *cursor++ = '\0';
    }
    return count;
}

int aliasTermScore(const FT02FieldManualCardIndex& card, const char* term)
{
    if(term == nullptr || term[0] == '\0') return 0;
    for(const SearchAlias& alias : SEARCH_ALIASES)
    {
        if(equalsFold(term, alias.term) && equalsFold(card.id, alias.cardId)) return 85;
    }
    return 0;
}

int indexTermScore(const FT02FieldManualCardIndex& card, const FT02FieldManualCategory* category, const char* term)
{
    if(equalsFold(card.id, term)) return 180;
    if(equalsFold(card.title, term)) return 160;
    if(startsWithFold(card.id, term)) return 130;
    if(containsFold(card.id, term)) return 120;
    if(containsFold(card.title, term)) return 100;
    if(containsFold(card.summary, term)) return 70;
    if(category != nullptr)
    {
        if(containsFold(category->name, term)) return 55;
        if(containsFold(category->summary, term)) return 40;
        if(containsFold(category->id, term)) return 35;
    }
    return 0;
}

int bodyTermScore(const FT02FieldManualLoadedCard* loaded, const char* term)
{
    if(loaded == nullptr) return 0;
    for(int i = 0; i < loaded->sectionCount; ++i)
    {
        if(containsFold(loaded->sections[i].title, term)) return 50;
        if(containsFold(loaded->sections[i].text, term)) return 45;
    }
    return 0;
}

void insertResult(int globalIndex, int score)
{
    int insertAt = 0;
    while(insertAt < g_resultCount)
    {
        if(score > g_results[insertAt].score) break;
        if(score == g_results[insertAt].score && globalIndex < g_results[insertAt].globalIndex) break;
        ++insertAt;
    }
    if(insertAt >= FT02_FIELD_MANUAL_SEARCH_MAX_RESULTS) return;

    if(g_resultCount < FT02_FIELD_MANUAL_SEARCH_MAX_RESULTS) ++g_resultCount;
    for(int i = g_resultCount - 1; i > insertAt; --i) g_results[i] = g_results[i - 1];
    g_results[insertAt].globalIndex = globalIndex;
    g_results[insertAt].score = score;
}
}

bool FT02_FieldManualSearchExecute(const char* queryUtf8)
{
    g_resultCount = 0;
    g_lastQuery[0] = '\0';
    if(queryUtf8 == nullptr) return false;
    if(!FT02_FieldManualBegin(false)) return false;

    while(*queryUtf8 == ' ' || *queryUtf8 == '\t' || *queryUtf8 == '\r' || *queryUtf8 == '\n') ++queryUtf8;
    if(*queryUtf8 == '\0') return false;

    snprintf(g_lastQuery, sizeof(g_lastQuery), "%s", queryUtf8);
    size_t lastLen = strlen(g_lastQuery);
    while(lastLen > 0 && (g_lastQuery[lastLen - 1] == ' ' || g_lastQuery[lastLen - 1] == '\t' ||
                          g_lastQuery[lastLen - 1] == '\r' || g_lastQuery[lastLen - 1] == '\n'))
        g_lastQuery[--lastLen] = '\0';
    if(g_lastQuery[0] == '\0') return false;

    char termsBuffer[sizeof(g_lastQuery)] = {};
    snprintf(termsBuffer, sizeof(termsBuffer), "%s", g_lastQuery);
    const char* terms[MAX_TERMS] = {};
    const int termCount = splitTerms(termsBuffer, terms, MAX_TERMS);
    if(termCount < 1) return false;

    const int cardCount = FT02_FieldManualCardCount();
    for(int globalIndex = 0; globalIndex < cardCount; ++globalIndex)
    {
        const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(globalIndex);
        if(card == nullptr) continue;
        const FT02FieldManualCategory* category = categoryForCard(*card);

        int termScores[MAX_TERMS] = {};
        bool needsBody = false;
        for(int t = 0; t < termCount; ++t)
        {
            termScores[t] = indexTermScore(*card, category, terms[t]);
            if(termScores[t] == 0) termScores[t] = aliasTermScore(*card, terms[t]);
            if(termScores[t] == 0) needsBody = true;
        }

        if(needsBody)
        {
            if(!FT02_FieldManualLoadCard(globalIndex)) continue;
            const FT02FieldManualLoadedCard* loaded = FT02_FieldManualLoadedCardCurrent();
            for(int t = 0; t < termCount; ++t)
                if(termScores[t] == 0) termScores[t] = bodyTermScore(loaded, terms[t]);
        }

        bool allMatched = true;
        int totalScore = 0;
        for(int t = 0; t < termCount; ++t)
        {
            if(termScores[t] == 0)
            {
                allMatched = false;
                break;
            }
            totalScore += termScores[t];
        }
        if(allMatched) insertResult(globalIndex, totalScore);
        if((globalIndex & 0x0F) == 0) yield();
    }

    FT02_FieldManualUnloadCard();
    Serial.printf("[FIELD-SEARCH-A2] query='%s' results=%d cards=%d\n",
                  g_lastQuery, g_resultCount, cardCount);
    return true;
}

int FT02_FieldManualSearchResultCount()
{
    return g_resultCount;
}

int FT02_FieldManualSearchGlobalIndexAt(int resultIndex)
{
    if(resultIndex < 0 || resultIndex >= g_resultCount) return -1;
    return g_results[resultIndex].globalIndex;
}



void FT02_FieldManualSearchClear()
{
    g_resultCount = 0;
    g_lastQuery[0] = '\0';
}
