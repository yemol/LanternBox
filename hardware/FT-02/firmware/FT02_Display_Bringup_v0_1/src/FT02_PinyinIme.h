#pragma once

#include <stddef.h>
#include <stdint.h>

// FT-02 Offline Pinyin IME A4
// - tone-less Pinyin
// - continuous Pinyin segmentation
// - phrase candidates
// - single-character fallback when no phrase exists
// - bounded offline user-frequency candidate learning

constexpr size_t FT02_PINYIN_MAX_INPUT = 24;
constexpr size_t FT02_PINYIN_PAGE_SIZE = 5;
constexpr size_t FT02_PINYIN_MAX_CANDIDATE_BYTES = 48;

bool FT02_PinyinImeHasExact(const char* pinyin);
bool FT02_PinyinImeHasPhrase(const char* pinyin);

// Returns the candidate count for the current composition.
// Resolution order:
// 1) exact phrase candidate(s),
// 2) exact single-syllable Han candidates,
// 3) first syllable Han candidates after automatic segmentation.
size_t FT02_PinyinImeCandidateCount(const char* pinyin);

// Copies one UTF-8 candidate into outUtf8. consumedInputBytes tells the caller
// how much of the Pinyin composition should be removed when the candidate is
// committed. Phrase candidates consume the whole input; segmented fallback
// consumes only the first syllable and leaves the remaining Pinyin active.
bool FT02_PinyinImeCandidate(
    const char* pinyin,
    size_t index,
    char* outUtf8,
    size_t outSize,
    size_t* consumedInputBytes = nullptr
);

// Formats continuous Pinyin as readable syllables, e.g. "shoudao" ->
// "shou'dao". If segmentation is not yet possible, returns the raw input.
bool FT02_PinyinImeSegmentDisplay(const char* pinyin, char* out, size_t outSize);
