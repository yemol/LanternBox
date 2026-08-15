#pragma once

#include <stddef.h>
#include <stdint.h>

// FT-02 Offline Pinyin IME A3 user-frequency learning.
//
// Privacy / persistence model:
// - stores only Pinyin key + selected candidate + count + last-used epoch
// - never stores the composed/sent message body
// - keeps a bounded RAM table and periodically persists an atomic snapshot
// - if SD is unavailable, the IME remains fully functional and learning stays
//   RAM-only until storage becomes available again

constexpr size_t FT02_PINYIN_LEARNING_MAX_ENTRIES = 160;
constexpr const char* FT02_PINYIN_LEARNING_PATH = "/lanternbox/input/pinyin_user_freq.dat";

void FT02_PinyinLearningBegin();
void FT02_PinyinLearningPoll();

// Record one successful candidate commit.
void FT02_PinyinLearningRecord(const char* pinyinKey, const char* candidateUtf8);

// Return how many times this exact candidate was selected for this Pinyin key.
uint32_t FT02_PinyinLearningCount(const char* pinyinKey, const char* candidateUtf8);

// Last persisted/known use timestamp. Zero means no valid wall-clock time was
// available when the entry was last selected.
uint32_t FT02_PinyinLearningLastUsed(const char* pinyinKey, const char* candidateUtf8);

bool FT02_PinyinLearningHasData();
size_t FT02_PinyinLearningEntryCount();
bool FT02_PinyinLearningDirty();

// Persist immediately if dirty. Safe to call after a successful message send.
bool FT02_PinyinLearningFlush();
