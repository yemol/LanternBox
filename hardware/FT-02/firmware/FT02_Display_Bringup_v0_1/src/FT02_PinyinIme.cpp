#include "FT02_PinyinIme.h"

#include <string.h>

#include "FT02_PinyinImeData.h"
#include "FT02_PinyinPhraseData.h"
#include "FT02_PinyinLearning.h"

namespace
{
constexpr size_t MAX_SYLLABLE_BYTES = 6;

const FT02PinyinImeEntry* findEntry(const char* pinyin)
{
    if(pinyin == nullptr || pinyin[0] == '\0') return nullptr;

    size_t lo = 0;
    size_t hi = FT02_PINYIN_IME_TABLE_COUNT;
    while(lo < hi)
    {
        const size_t mid = lo + (hi - lo) / 2;
        const int cmp = strcmp(pinyin, FT02_PINYIN_IME_TABLE[mid].pinyin);
        if(cmp == 0) return &FT02_PINYIN_IME_TABLE[mid];
        if(cmp < 0) hi = mid;
        else lo = mid + 1;
    }
    return nullptr;
}

const FT02PinyinPhraseEntry* findPhrase(const char* pinyin)
{
    if(pinyin == nullptr || pinyin[0] == '\0') return nullptr;

    size_t lo = 0;
    size_t hi = FT02_PINYIN_PHRASE_TABLE_COUNT;
    while(lo < hi)
    {
        const size_t mid = lo + (hi - lo) / 2;
        const int cmp = strcmp(pinyin, FT02_PINYIN_PHRASE_TABLE[mid].pinyin);
        if(cmp == 0) return &FT02_PINYIN_PHRASE_TABLE[mid];
        if(cmp < 0) hi = mid;
        else lo = mid + 1;
    }
    return nullptr;
}

bool exactSlice(const char* pinyin, size_t start, size_t length)
{
    if(pinyin == nullptr || length == 0 || length > MAX_SYLLABLE_BYTES) return false;
    char syllable[MAX_SYLLABLE_BYTES + 1] = {};
    memcpy(syllable, pinyin + start, length);
    syllable[length] = '\0';
    return findEntry(syllable) != nullptr;
}

// Longest-valid-syllable dynamic programming. This makes common continuous
// input stable while keeping exact single syllables intact.
bool segment(const char* pinyin, uint8_t* nextLen, size_t nextCapacity)
{
    if(pinyin == nullptr || pinyin[0] == '\0') return false;
    const size_t len = strlen(pinyin);
    if(len > FT02_PINYIN_MAX_INPUT || nextCapacity < len + 1u) return false;

    bool can[FT02_PINYIN_MAX_INPUT + 1] = {};
    can[len] = true;
    nextLen[len] = 0;

    for(size_t rev = len; rev > 0; --rev)
    {
        const size_t i = rev - 1u;
        const size_t maxLen = ((len - i) < MAX_SYLLABLE_BYTES) ? (len - i) : MAX_SYLLABLE_BYTES;
        for(size_t syllableLen = maxLen; syllableLen > 0; --syllableLen)
        {
            if(!can[i + syllableLen]) continue;
            if(!exactSlice(pinyin, i, syllableLen)) continue;
            can[i] = true;
            nextLen[i] = static_cast<uint8_t>(syllableLen);
            break;
        }
    }
    return can[0];
}

size_t firstSegmentLength(const char* pinyin)
{
    if(pinyin == nullptr || pinyin[0] == '\0') return 0;
    uint8_t nextLen[FT02_PINYIN_MAX_INPUT + 1] = {};
    if(!segment(pinyin, nextLen, sizeof(nextLen))) return 0;
    return nextLen[0];
}

size_t singleCandidateCount(const FT02PinyinImeEntry* entry)
{
    if(entry == nullptr || entry->candidates == nullptr) return 0;
    const char* bytes = reinterpret_cast<const char*>(entry->candidates);
    // A1 dictionary intentionally stores BMP CJK candidates only, each encoded
    // as exactly three UTF-8 bytes.
    return strlen(bytes) / 3u;
}

bool singleCandidateBase(const FT02PinyinImeEntry* entry, size_t index, char* outUtf8, size_t outSize)
{
    if(outUtf8 == nullptr || outSize < 4u || entry == nullptr || entry->candidates == nullptr) return false;
    outUtf8[0] = '\0';
    const char* bytes = reinterpret_cast<const char*>(entry->candidates);
    const size_t count = strlen(bytes) / 3u;
    if(index >= count) return false;
    const char* src = bytes + index * 3u;
    outUtf8[0] = src[0];
    outUtf8[1] = src[1];
    outUtf8[2] = src[2];
    outUtf8[3] = '\0';
    return true;
}

uint64_t learningRankScore(const char* key, const char* candidate, size_t baseIndex)
{
    // Preserve a meaningful default-order prior so one accidental choice does
    // not instantly reshuffle the whole dictionary. Repeated choices gradually
    // overcome the prior. Recent use breaks ties between equally learned items.
    const uint64_t basePrior = baseIndex < 12u ? (12u - baseIndex) * 8u : 0u;
    const uint64_t learned = static_cast<uint64_t>(FT02_PinyinLearningCount(key, candidate)) * 12u;
    const uint64_t recent = FT02_PinyinLearningLastUsed(key, candidate) != 0u ? 1u : 0u;
    return basePrior + learned + recent;
}

bool rankedSingleCandidate(const char* key, const FT02PinyinImeEntry* entry, size_t rankedIndex, char* outUtf8, size_t outSize)
{
    const size_t count = singleCandidateCount(entry);
    if(rankedIndex >= count) return false;
    if(!FT02_PinyinLearningHasData()) return singleCandidateBase(entry, rankedIndex, outUtf8, outSize);

    bool used[64] = {};
    if(count > sizeof(used) / sizeof(used[0])) return false;
    for(size_t rank = 0; rank <= rankedIndex; ++rank)
    {
        size_t best = count;
        uint64_t bestScore = 0;
        for(size_t base = 0; base < count; ++base)
        {
            if(used[base]) continue;
            char candidate[8] = {};
            if(!singleCandidateBase(entry, base, candidate, sizeof(candidate))) continue;
            const uint64_t score = learningRankScore(key, candidate, base);
            if(best == count || score > bestScore || (score == bestScore && base < best))
            {
                best = base;
                bestScore = score;
            }
        }
        if(best == count) return false;
        used[best] = true;
        if(rank == rankedIndex) return singleCandidateBase(entry, best, outUtf8, outSize);
    }
    return false;
}

size_t phraseCandidateCount(const FT02PinyinPhraseEntry* entry)
{
    if(entry == nullptr || entry->candidates == nullptr) return 0;
    const char* bytes = reinterpret_cast<const char*>(entry->candidates);
    if(bytes[0] == '\0') return 0;
    size_t count = 1;
    for(const char* p = bytes; *p; ++p) if(*p == '|') ++count;
    return count;
}

bool phraseCandidateBase(const FT02PinyinPhraseEntry* entry, size_t index, char* outUtf8, size_t outSize)
{
    if(outUtf8 == nullptr || outSize == 0 || entry == nullptr || entry->candidates == nullptr) return false;
    outUtf8[0] = '\0';
    const char* bytes = reinterpret_cast<const char*>(entry->candidates);
    const char* start = bytes;
    size_t current = 0;
    for(const char* p = bytes;; ++p)
    {
        if(*p == '|' || *p == '\0')
        {
            if(current == index)
            {
                const size_t n = static_cast<size_t>(p - start);
                if(n + 1u > outSize) return false;
                memcpy(outUtf8, start, n);
                outUtf8[n] = '\0';
                return n > 0;
            }
            if(*p == '\0') return false;
            ++current;
            start = p + 1;
        }
    }
}

bool rankedPhraseCandidate(const char* key, const FT02PinyinPhraseEntry* entry, size_t rankedIndex, char* outUtf8, size_t outSize)
{
    const size_t count = phraseCandidateCount(entry);
    if(rankedIndex >= count) return false;
    if(!FT02_PinyinLearningHasData()) return phraseCandidateBase(entry, rankedIndex, outUtf8, outSize);

    bool used[64] = {};
    if(count > sizeof(used) / sizeof(used[0])) return false;
    for(size_t rank = 0; rank <= rankedIndex; ++rank)
    {
        size_t best = count;
        uint64_t bestScore = 0;
        for(size_t base = 0; base < count; ++base)
        {
            if(used[base]) continue;
            char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1] = {};
            if(!phraseCandidateBase(entry, base, candidate, sizeof(candidate))) continue;
            const uint64_t score = learningRankScore(key, candidate, base);
            if(best == count || score > bestScore || (score == bestScore && base < best))
            {
                best = base;
                bestScore = score;
            }
        }
        if(best == count) return false;
        used[best] = true;
        if(rank == rankedIndex) return phraseCandidateBase(entry, best, outUtf8, outSize);
    }
    return false;
}

bool copySlice(const char* src, size_t start, size_t length, char* out, size_t outSize)
{
    if(src == nullptr || out == nullptr || outSize == 0 || length + 1u > outSize) return false;
    memcpy(out, src + start, length);
    out[length] = '\0';
    return true;
}
}

bool FT02_PinyinImeHasExact(const char* pinyin)
{
    return findEntry(pinyin) != nullptr;
}

bool FT02_PinyinImeHasPhrase(const char* pinyin)
{
    return findPhrase(pinyin) != nullptr;
}

size_t FT02_PinyinImeCandidateCount(const char* pinyin)
{
    if(pinyin == nullptr || pinyin[0] == '\0') return 0;

    if(const FT02PinyinPhraseEntry* phrase = findPhrase(pinyin))
        return phraseCandidateCount(phrase);

    if(const FT02PinyinImeEntry* exact = findEntry(pinyin))
        return singleCandidateCount(exact);

    const size_t firstLen = firstSegmentLength(pinyin);
    if(firstLen == 0 || firstLen >= strlen(pinyin)) return 0;

    char first[MAX_SYLLABLE_BYTES + 1] = {};
    if(!copySlice(pinyin, 0, firstLen, first, sizeof(first))) return 0;
    return singleCandidateCount(findEntry(first));
}

bool FT02_PinyinImeCandidate(
    const char* pinyin,
    size_t index,
    char* outUtf8,
    size_t outSize,
    size_t* consumedInputBytes
)
{
    if(outUtf8 == nullptr || outSize == 0) return false;
    outUtf8[0] = '\0';
    if(consumedInputBytes != nullptr) *consumedInputBytes = 0;
    if(pinyin == nullptr || pinyin[0] == '\0') return false;

    if(const FT02PinyinPhraseEntry* phrase = findPhrase(pinyin))
    {
        if(!rankedPhraseCandidate(pinyin, phrase, index, outUtf8, outSize)) return false;
        if(consumedInputBytes != nullptr) *consumedInputBytes = strlen(pinyin);
        return true;
    }

    if(const FT02PinyinImeEntry* exact = findEntry(pinyin))
    {
        if(!rankedSingleCandidate(pinyin, exact, index, outUtf8, outSize)) return false;
        if(consumedInputBytes != nullptr) *consumedInputBytes = strlen(pinyin);
        return true;
    }

    const size_t firstLen = firstSegmentLength(pinyin);
    const size_t totalLen = strlen(pinyin);
    if(firstLen == 0 || firstLen >= totalLen) return false;

    char first[MAX_SYLLABLE_BYTES + 1] = {};
    if(!copySlice(pinyin, 0, firstLen, first, sizeof(first))) return false;
    if(!rankedSingleCandidate(first, findEntry(first), index, outUtf8, outSize)) return false;
    if(consumedInputBytes != nullptr) *consumedInputBytes = firstLen;
    return true;
}

bool FT02_PinyinImeSegmentDisplay(const char* pinyin, char* out, size_t outSize)
{
    if(out == nullptr || outSize == 0) return false;
    out[0] = '\0';
    if(pinyin == nullptr || pinyin[0] == '\0') return false;

    const size_t len = strlen(pinyin);
    uint8_t nextLen[FT02_PINYIN_MAX_INPUT + 1] = {};
    if(!segment(pinyin, nextLen, sizeof(nextLen)))
    {
        if(len + 1u > outSize) return false;
        memcpy(out, pinyin, len + 1u);
        return false;
    }

    size_t srcPos = 0;
    size_t dstPos = 0;
    bool first = true;
    while(srcPos < len)
    {
        const size_t n = nextLen[srcPos];
        if(n == 0) break;
        if(!first)
        {
            if(dstPos + 2u > outSize) return false;
            out[dstPos++] = '\'';
        }
        if(dstPos + n + 1u > outSize) return false;
        memcpy(out + dstPos, pinyin + srcPos, n);
        dstPos += n;
        srcPos += n;
        first = false;
    }
    out[dstPos] = '\0';
    return srcPos == len;
}
