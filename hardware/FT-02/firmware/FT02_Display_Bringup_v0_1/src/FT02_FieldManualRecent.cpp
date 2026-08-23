#include "FT02_FieldManualRecent.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "FT02_FieldManual.h"
#include "FT02_Storage.h"

namespace
{
constexpr const char* TEMP_PATH = "/lanternbox/field_manual/recent_v1.tmp";
constexpr const char* BACKUP_PATH = "/lanternbox/field_manual/recent_v1.bak";
constexpr const char* FILE_HEADER = "# FT02_FIELD_MANUAL_RECENT_V1";
constexpr uint32_t FLUSH_IDLE_MS = 5000u;

struct RecentEntry
{
    char cardId[12];
    int globalIndex;
    uint16_t page;
    uint32_t lastUsedEpoch;
};

RecentEntry g_entries[FT02_FIELD_MANUAL_RECENT_MAX] = {};
int g_count = 0;
bool g_loaded = false;
bool g_dirty = false;
uint32_t g_lastDirtyMs = 0;
uint32_t g_lastLoadRetryMs = 0;

uint32_t currentEpoch()
{
    const time_t now = time(nullptr);
    if(now < static_cast<time_t>(1600000000)) return 0;
    if(static_cast<uint64_t>(now) > 0xFFFFFFFFULL) return 0xFFFFFFFFu;
    return static_cast<uint32_t>(now);
}

int resolveCardId(const char* cardId)
{
    if(cardId == nullptr || cardId[0] == '\0') return -1;
    const int count = FT02_FieldManualCardCount();
    for(int i = 0; i < count; ++i)
    {
        const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(i);
        if(card != nullptr && strcmp(card->id, cardId) == 0) return i;
    }
    return -1;
}

int findByGlobalIndex(int globalCardIndex)
{
    for(int i = 0; i < g_count; ++i)
        if(g_entries[i].globalIndex == globalCardIndex) return i;
    return -1;
}

void moveEntryToFront(int index)
{
    if(index <= 0 || index >= g_count) return;
    const RecentEntry moving = g_entries[index];
    memmove(&g_entries[1], &g_entries[0], sizeof(RecentEntry) * index);
    g_entries[0] = moving;
}

bool loadSnapshot()
{
    if(!FT02_StorageIsReady()) return false;
    if(!FT02_FieldManualBegin(false)) return false;

    g_count = 0;
    if(!FT02_StorageFileExists(FT02_FIELD_MANUAL_RECENT_PATH))
    {
        g_loaded = true;
        g_dirty = false;
        Serial.println("[FIELD-RECENT-A1] history absent; starting fresh");
        return true;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_FIELD_MANUAL_RECENT_PATH);
    if(file == nullptr) return false;

    char line[96] = {};
    bool headerSeen = false;
    while(fgets(line, sizeof(line), file) != nullptr && g_count < FT02_FIELD_MANUAL_RECENT_MAX)
    {
        size_t len = strlen(line);
        while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if(line[0] == '\0') continue;
        if(line[0] == '#')
        {
            if(!headerSeen && strcmp(line, FILE_HEADER) != 0)
                Serial.printf("[FIELD-RECENT-A1] header differs: %s\n", line);
            headerSeen = true;
            continue;
        }

        char* save = nullptr;
        char* id = strtok_r(line, "\t", &save);
        char* pageText = strtok_r(nullptr, "\t", &save);
        char* epochText = strtok_r(nullptr, "\t", &save);
        if(id == nullptr || pageText == nullptr || strlen(id) >= sizeof(g_entries[0].cardId)) continue;

        char* end = nullptr;
        unsigned long page = strtoul(pageText, &end, 10);
        if(end == pageText) continue;
        unsigned long epoch = 0;
        if(epochText != nullptr)
        {
            end = nullptr;
            epoch = strtoul(epochText, &end, 10);
            if(end == epochText) epoch = 0;
        }

        const int globalIndex = resolveCardId(id);
        if(globalIndex < 0) continue; // Card was removed/renamed in this pack.

        RecentEntry& entry = g_entries[g_count++];
        snprintf(entry.cardId, sizeof(entry.cardId), "%s", id);
        entry.globalIndex = globalIndex;
        entry.page = page > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(page);
        entry.lastUsedEpoch = static_cast<uint32_t>(epoch);
    }
    fclose(file);

    g_loaded = true;
    g_dirty = false;
    Serial.printf("[FIELD-RECENT-A1] loaded entries=%d\n", g_count);
    return true;
}

bool writeSnapshot()
{
    if(!FT02_StorageIsReady()) return false;

    (void)FT02_StorageDeleteFile(TEMP_PATH);
    FILE* file = FT02_StorageOpenWriteFile(TEMP_PATH, true);
    if(file == nullptr) return false;

    bool ok = fprintf(file, "%s\n", FILE_HEADER) > 0;
    for(int i = 0; ok && i < g_count; ++i)
    {
        const RecentEntry& entry = g_entries[i];
        if(fprintf(file, "%s\t%u\t%lu\n",
                   entry.cardId,
                   static_cast<unsigned>(entry.page),
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
    const bool hadState = FT02_StorageFileExists(FT02_FIELD_MANUAL_RECENT_PATH);
    if(hadState && !FT02_StorageRenameFile(FT02_FIELD_MANUAL_RECENT_PATH, BACKUP_PATH))
    {
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    if(!FT02_StorageRenameFile(TEMP_PATH, FT02_FIELD_MANUAL_RECENT_PATH))
    {
        if(hadState) (void)FT02_StorageRenameFile(BACKUP_PATH, FT02_FIELD_MANUAL_RECENT_PATH);
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    (void)FT02_StorageDeleteFile(BACKUP_PATH);
    return true;
}

void ensureLoaded()
{
    if(g_loaded) return;
    if(!loadSnapshot()) g_lastLoadRetryMs = millis();
}

void markDirty()
{
    g_dirty = true;
    g_lastDirtyMs = millis();
}
}

void FT02_FieldManualRecentBegin()
{
    ensureLoaded();
}

void FT02_FieldManualRecentPoll(uint32_t nowMs)
{
    if(!g_loaded)
    {
        if(FT02_StorageIsReady() && nowMs - g_lastLoadRetryMs >= 5000u)
        {
            g_lastLoadRetryMs = nowMs;
            (void)loadSnapshot();
        }
        return;
    }
    if(g_dirty && nowMs - g_lastDirtyMs >= FLUSH_IDLE_MS) (void)FT02_FieldManualRecentFlush();
}


int FT02_FieldManualRecentCount()
{
    ensureLoaded();
    return g_count;
}

int FT02_FieldManualRecentGlobalIndexAt(int recentIndex)
{
    ensureLoaded();
    if(recentIndex < 0 || recentIndex >= g_count) return -1;
    return g_entries[recentIndex].globalIndex;
}

int FT02_FieldManualRecentPageAt(int recentIndex)
{
    ensureLoaded();
    if(recentIndex < 0 || recentIndex >= g_count) return 0;
    return static_cast<int>(g_entries[recentIndex].page);
}

void FT02_FieldManualRecentRecord(int globalCardIndex, int page)
{
    ensureLoaded();
    const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(globalCardIndex);
    if(card == nullptr) return;
    if(page < 0) page = 0;
    if(page > 0xFFFF) page = 0xFFFF;

    int index = findByGlobalIndex(globalCardIndex);
    if(index < 0)
    {
        if(g_count < FT02_FIELD_MANUAL_RECENT_MAX) ++g_count;
        if(g_count > 1)
            memmove(&g_entries[1], &g_entries[0], sizeof(RecentEntry) * (g_count - 1));
        index = 0;
        memset(&g_entries[0], 0, sizeof(g_entries[0]));
    }
    else
    {
        moveEntryToFront(index);
        index = 0;
    }

    RecentEntry& entry = g_entries[index];
    snprintf(entry.cardId, sizeof(entry.cardId), "%s", card->id);
    entry.globalIndex = globalCardIndex;
    entry.page = static_cast<uint16_t>(page);
    entry.lastUsedEpoch = currentEpoch();
    markDirty();
}

void FT02_FieldManualRecentUpdatePage(int globalCardIndex, int page)
{
    ensureLoaded();
    int index = findByGlobalIndex(globalCardIndex);
    if(index < 0)
    {
        FT02_FieldManualRecentRecord(globalCardIndex, page);
        return;
    }
    moveEntryToFront(index);
    if(page < 0) page = 0;
    if(page > 0xFFFF) page = 0xFFFF;
    g_entries[0].page = static_cast<uint16_t>(page);
    g_entries[0].lastUsedEpoch = currentEpoch();
    markDirty();
}


bool FT02_FieldManualRecentFlush()
{
    if(!g_loaded || !g_dirty) return true;
    if(!writeSnapshot())
    {
        Serial.println("[FIELD-RECENT-A1] flush FAILED");
        return false;
    }
    g_dirty = false;
    Serial.printf("[FIELD-RECENT-A1] flush OK entries=%d\n", g_count);
    return true;
}
