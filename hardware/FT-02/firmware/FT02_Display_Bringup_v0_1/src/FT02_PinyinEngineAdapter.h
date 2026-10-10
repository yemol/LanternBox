#pragma once

#include <stddef.h>
#include <stdint.h>

// FT-02 Pinyin IME A5 smart-dictionary adapter.
//
// The public UI-facing IME API remains in FT02_PinyinIme.h. This adapter only
// replaces the dictionary/search core when an A5 dictionary is present on SD.
// Missing/corrupt dictionary files are non-fatal: callers simply fall back to
// the built-in FT02_PinyinImeData / FT02_PinyinPhraseData tables.

constexpr const char* FT02_PINYIN_A5_INDEX_PATH = "/lanternbox/input/ime/ft02_pinyin_words.idx";
constexpr const char* FT02_PINYIN_A5_DATA_PATH  = "/lanternbox/input/ime/ft02_pinyin_words.dat";
constexpr uint16_t FT02_PINYIN_A5_FORMAT_VERSION = 1;
constexpr size_t FT02_PINYIN_A5_MAX_CANDIDATES = 64;

// Returns true only after the SD dictionary has been found and its headers have
// passed validation. It is intentionally lazy so boot order and SD mount timing
// do not need to change.
bool FT02_PinyinEngineAdapterAvailable();

// Exact-key lookup against the smart dictionary. Keys are continuous, lowercase
// tone-less Pinyin, e.g. "woxihuan". Results are kept in a small RAM cache so
// the existing UI can call Count() and Candidate() repeatedly without re-reading
// the SD card for every candidate cell.
size_t FT02_PinyinEngineAdapterCandidateCount(const char* pinyinKey);

bool FT02_PinyinEngineAdapterCandidate(
    const char* pinyinKey,
    size_t index,
    char* outUtf8,
    size_t outSize,
    uint32_t* baseFrequency = nullptr
);
