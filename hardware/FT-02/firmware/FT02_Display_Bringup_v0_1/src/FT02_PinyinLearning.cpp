#include "FT02_PinyinLearning.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "FT02_Storage.h"
#include "FT02_PinyinIme.h"

namespace
{
constexpr const char* TEMP_PATH = "/lanternbox/input/pinyin_user_freq.tmp";
constexpr const char* BACKUP_PATH = "/lanternbox/input/pinyin_user_freq.bak";
constexpr const char* FILE_HEADER = "# FT02_PINYIN_FREQ_V1";
constexpr uint32_t FLUSH_IDLE_MS = 30000u;
constexpr uint16_t FLUSH_SELECTIONS = 8u;

struct LearningEntry
{
    char pinyin[FT02_PINYIN_MAX_INPUT + 1];
    char candidate[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1];
    uint32_t count;
    uint32_t lastUsedEpoch;
};

LearningEntry g_entries[FT02_PINYIN_LEARNING_MAX_ENTRIES] = {};
size_t g_entryCount = 0;
bool g_dirty = false;
bool g_loaded = false;
uint16_t g_dirtySelections = 0;
uint32_t g_lastDirtyMs = 0;
uint32_t g_lastLoadRetryMs = 0;

bool validKey(const char* key)
{
    if(key == nullptr || key[0] == '\0') return false;
    const size_t n = strlen(key);
    if(n == 0 || n > FT02_PINYIN_MAX_INPUT) return false;
    for(size_t i = 0; i < n; ++i)
        if(key[i] < 'a' || key[i] > 'z') return false;
    return true;
}

bool validCandidate(const char* candidate)
{
    if(candidate == nullptr || candidate[0] == '\0') return false;
    const size_t n = strlen(candidate);
    if(n == 0 || n > FT02_PINYIN_MAX_CANDIDATE_BYTES) return false;
    // Candidate data is UTF-8 or printable ASCII. Tabs/newlines would corrupt
    // the compact TSV snapshot and are never legitimate IME candidates.
    for(size_t i = 0; i < n; ++i)
    {
        if(candidate[i] == '\t' || candidate[i] == '\r' || candidate[i] == '\n') return false;
    }
    return true;
}

int findEntry(const char* key, const char* candidate)
{
    for(size_t i = 0; i < g_entryCount; ++i)
    {
        if(strcmp(g_entries[i].pinyin, key) == 0 && strcmp(g_entries[i].candidate, candidate) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

uint32_t currentEpoch()
{
    const time_t now = time(nullptr);
    if(now < static_cast<time_t>(1600000000)) return 0;
    if(static_cast<uint64_t>(now) > 0xFFFFFFFFULL) return 0xFFFFFFFFu;
    return static_cast<uint32_t>(now);
}

size_t evictionIndex()
{
    // Keep frequently-used entries. For equal counts, evict the oldest one;
    // entries without a valid wall-clock timestamp are considered oldest.
    size_t victim = 0;
    for(size_t i = 1; i < g_entryCount; ++i)
    {
        const LearningEntry& a = g_entries[i];
        const LearningEntry& b = g_entries[victim];
        if(a.count < b.count || (a.count == b.count && a.lastUsedEpoch < b.lastUsedEpoch))
            victim = i;
    }
    return victim;
}

void mergeLoaded(const char* key, const char* candidate, uint32_t count, uint32_t lastUsed)
{
    if(!validKey(key) || !validCandidate(candidate) || count == 0) return;
    const int existing = findEntry(key, candidate);
    if(existing >= 0)
    {
        LearningEntry& entry = g_entries[existing];
        const uint64_t merged = static_cast<uint64_t>(entry.count) + count;
        entry.count = merged > 0xFFFFFFFFULL ? 0xFFFFFFFFu : static_cast<uint32_t>(merged);
        if(lastUsed > entry.lastUsedEpoch) entry.lastUsedEpoch = lastUsed;
        return;
    }

    size_t slot = 0;
    if(g_entryCount < FT02_PINYIN_LEARNING_MAX_ENTRIES)
        slot = g_entryCount++;
    else
        slot = evictionIndex();

    LearningEntry& entry = g_entries[slot];
    snprintf(entry.pinyin, sizeof(entry.pinyin), "%s", key);
    snprintf(entry.candidate, sizeof(entry.candidate), "%s", candidate);
    entry.count = count;
    entry.lastUsedEpoch = lastUsed;
}

bool loadSnapshot()
{
    if(!FT02_StorageIsReady()) return false;

    const bool hadPendingLearning = g_dirty;
    if(!hadPendingLearning) g_entryCount = 0;
    if(!FT02_StorageFileExists(FT02_PINYIN_LEARNING_PATH))
    {
        g_loaded = true;
        if(!hadPendingLearning)
        {
            g_dirty = false;
            g_dirtySelections = 0;
        }
        Serial.println("[PINYIN-A3] learning file absent; starting fresh");
        return true;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_PINYIN_LEARNING_PATH);
    if(file == nullptr) return false;

    char line[160] = {};
    bool firstMeaningful = true;
    while(fgets(line, sizeof(line), file) != nullptr)
    {
        while(line[0] && (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r'))
            line[strlen(line) - 1] = '\0';
        if(line[0] == '\0') continue;
        if(line[0] == '#')
        {
            if(firstMeaningful && strcmp(line, FILE_HEADER) != 0)
                Serial.printf("[PINYIN-A3] learning header differs: %s\n", line);
            firstMeaningful = false;
            continue;
        }
        firstMeaningful = false;

        char* save = nullptr;
        char* key = strtok_r(line, "\t", &save);
        char* candidate = strtok_r(nullptr, "\t", &save);
        char* countText = strtok_r(nullptr, "\t", &save);
        char* lastText = strtok_r(nullptr, "\t", &save);
        if(key == nullptr || candidate == nullptr || countText == nullptr) continue;

        char* end = nullptr;
        const unsigned long countValue = strtoul(countText, &end, 10);
        if(end == countText || countValue == 0) continue;
        unsigned long lastValue = 0;
        if(lastText != nullptr)
        {
            end = nullptr;
            lastValue = strtoul(lastText, &end, 10);
            if(end == lastText) lastValue = 0;
        }
        mergeLoaded(key, candidate, static_cast<uint32_t>(countValue), static_cast<uint32_t>(lastValue));
    }
    fclose(file);
    g_loaded = true;
    if(!hadPendingLearning)
    {
        g_dirty = false;
        g_dirtySelections = 0;
    }
    Serial.printf("[PINYIN-A3] learning loaded entries=%u%s\n",
                  static_cast<unsigned>(g_entryCount),
                  hadPendingLearning ? " + pending RAM learning" : "");
    return true;
}

bool writeSnapshot()
{
    if(!FT02_StorageIsReady()) return false;

    (void)FT02_StorageDeleteFile(TEMP_PATH);
    FILE* file = FT02_StorageOpenWriteFile(TEMP_PATH, true);
    if(file == nullptr) return false;

    bool ok = fprintf(file, "%s\n", FILE_HEADER) > 0;
    for(size_t i = 0; ok && i < g_entryCount; ++i)
    {
        const LearningEntry& entry = g_entries[i];
        if(fprintf(file, "%s\t%s\t%lu\t%lu\n",
                   entry.pinyin,
                   entry.candidate,
                   static_cast<unsigned long>(entry.count),
                   static_cast<unsigned long>(entry.lastUsedEpoch)) <= 0)
            ok = false;
    }
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }

    (void)FT02_StorageDeleteFile(BACKUP_PATH);
    const bool hadState = FT02_StorageFileExists(FT02_PINYIN_LEARNING_PATH);
    if(hadState && !FT02_StorageRenameFile(FT02_PINYIN_LEARNING_PATH, BACKUP_PATH))
    {
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    if(!FT02_StorageRenameFile(TEMP_PATH, FT02_PINYIN_LEARNING_PATH))
    {
        if(hadState) (void)FT02_StorageRenameFile(BACKUP_PATH, FT02_PINYIN_LEARNING_PATH);
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    (void)FT02_StorageDeleteFile(BACKUP_PATH);
    return true;
}
}

void FT02_PinyinLearningBegin()
{
    if(g_loaded) return;
    if(!loadSnapshot())
    {
        g_lastLoadRetryMs = millis();
        Serial.println("[PINYIN-A3] learning load deferred: storage unavailable");
    }
}

void FT02_PinyinLearningPoll()
{
    const uint32_t now = millis();
    if(!g_loaded)
    {
        if(FT02_StorageIsReady() && now - g_lastLoadRetryMs >= 5000u)
        {
            g_lastLoadRetryMs = now;
            (void)loadSnapshot();
        }
        return;
    }

    if(!g_dirty) return;
    if(g_dirtySelections >= FLUSH_SELECTIONS || now - g_lastDirtyMs >= FLUSH_IDLE_MS)
        (void)FT02_PinyinLearningFlush();
}

void FT02_PinyinLearningRecord(const char* pinyinKey, const char* candidateUtf8)
{
    if(!validKey(pinyinKey) || !validCandidate(candidateUtf8)) return;

    int index = findEntry(pinyinKey, candidateUtf8);
    if(index < 0)
    {
        size_t slot = 0;
        if(g_entryCount < FT02_PINYIN_LEARNING_MAX_ENTRIES)
            slot = g_entryCount++;
        else
            slot = evictionIndex();
        LearningEntry& fresh = g_entries[slot];
        snprintf(fresh.pinyin, sizeof(fresh.pinyin), "%s", pinyinKey);
        snprintf(fresh.candidate, sizeof(fresh.candidate), "%s", candidateUtf8);
        fresh.count = 0;
        fresh.lastUsedEpoch = 0;
        index = static_cast<int>(slot);
    }

    LearningEntry& entry = g_entries[index];
    if(entry.count < 0xFFFFFFFFu) ++entry.count;
    entry.lastUsedEpoch = currentEpoch();
    g_dirty = true;
    if(g_dirtySelections < 0xFFFFu) ++g_dirtySelections;
    g_lastDirtyMs = millis();
}

uint32_t FT02_PinyinLearningCount(const char* pinyinKey, const char* candidateUtf8)
{
    if(!validKey(pinyinKey) || !validCandidate(candidateUtf8)) return 0;
    const int index = findEntry(pinyinKey, candidateUtf8);
    return index >= 0 ? g_entries[index].count : 0;
}

uint32_t FT02_PinyinLearningLastUsed(const char* pinyinKey, const char* candidateUtf8)
{
    if(!validKey(pinyinKey) || !validCandidate(candidateUtf8)) return 0;
    const int index = findEntry(pinyinKey, candidateUtf8);
    return index >= 0 ? g_entries[index].lastUsedEpoch : 0;
}

bool FT02_PinyinLearningHasData()
{
    return g_entryCount > 0;
}

size_t FT02_PinyinLearningEntryCount()
{
    return g_entryCount;
}

bool FT02_PinyinLearningDirty()
{
    return g_dirty;
}

bool FT02_PinyinLearningFlush()
{
    if(!g_dirty) return true;
    if(!writeSnapshot())
    {
        Serial.println("[PINYIN-A3] learning flush FAILED");
        return false;
    }
    g_dirty = false;
    g_dirtySelections = 0;
    Serial.printf("[PINYIN-A3] learning saved entries=%u\n", static_cast<unsigned>(g_entryCount));
    return true;
}
