#include "FT02_FieldManualFavorites.h"
#include "FT02_Log.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "FT02_FieldManual.h"
#include "FT02_Storage.h"

namespace
{
constexpr const char* TEMP_PATH = "/lanternbox/field_manual/favorites_v1.tmp";
constexpr const char* BACKUP_PATH = "/lanternbox/field_manual/favorites_v1.bak";
constexpr const char* FILE_HEADER = "# FT02_FIELD_MANUAL_FAVORITES_V1";

// At runtime only global card indexes are required. 256 uint16_t entries use
// 512 bytes and avoid a second copy of every title/ID in internal RAM.
uint16_t g_order[FT02_FIELD_MANUAL_MAX_CARDS] = {};
int g_count = 0;
bool g_loaded = false;
uint32_t g_lastLoadRetryMs = 0;

int resolveCardId(const char* cardId)
{
    if(cardId == nullptr || cardId[0] == '\0') return -1;
    const int cardCount = FT02_FieldManualCardCount();
    for(int i = 0; i < cardCount; ++i)
    {
        const FT02FieldManualCardIndex* card = FT02_FieldManualCardAt(i);
        if(card != nullptr && strcmp(card->id, cardId) == 0) return i;
    }
    return -1;
}

int findGlobalIndex(int globalCardIndex)
{
    for(int i = 0; i < g_count; ++i)
        if(static_cast<int>(g_order[i]) == globalCardIndex) return i;
    return -1;
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
        const FT02FieldManualCardIndex* card =
            FT02_FieldManualCardAt(static_cast<int>(g_order[i]));
        if(card == nullptr || card->id[0] == '\0')
        {
            ok = false;
            break;
        }
        if(fprintf(file, "%s\n", card->id) <= 0) ok = false;
    }
    if(ok) ok = FT02_StorageSyncFile(file);
    if(fclose(file) != 0) ok = false;
    if(!ok)
    {
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }

    (void)FT02_StorageDeleteFile(BACKUP_PATH);
    const bool hadState = FT02_StorageFileExists(FT02_FIELD_MANUAL_FAVORITES_PATH);
    if(hadState &&
       !FT02_StorageRenameFile(FT02_FIELD_MANUAL_FAVORITES_PATH, BACKUP_PATH))
    {
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    if(!FT02_StorageRenameFile(TEMP_PATH, FT02_FIELD_MANUAL_FAVORITES_PATH))
    {
        if(hadState)
            (void)FT02_StorageRenameFile(BACKUP_PATH, FT02_FIELD_MANUAL_FAVORITES_PATH);
        (void)FT02_StorageDeleteFile(TEMP_PATH);
        return false;
    }
    (void)FT02_StorageDeleteFile(BACKUP_PATH);
    return true;
}

bool loadSnapshot()
{
    if(!FT02_StorageIsReady()) return false;
    if(!FT02_FieldManualBegin(false)) return false;

    g_count = 0;
    if(!FT02_StorageFileExists(FT02_FIELD_MANUAL_FAVORITES_PATH))
    {
        g_loaded = true;

        return true;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_FIELD_MANUAL_FAVORITES_PATH);
    if(file == nullptr) return false;

    char line[64] = {};
    bool headerSeen = false;
    int staleEntries = 0;
    int duplicateEntries = 0;
    while(fgets(line, sizeof(line), file) != nullptr &&
          g_count < FT02_FIELD_MANUAL_MAX_CARDS)
    {
        size_t len = strlen(line);
        while(len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if(line[0] == '\0') continue;
        if(line[0] == '#')
        {
            headerSeen = true;
            continue;
        }

        const int globalIndex = resolveCardId(line);
        if(globalIndex < 0)
        {
            staleEntries++;
            continue;
        }
        if(findGlobalIndex(globalIndex) >= 0)
        {
            duplicateEntries++;
            continue;
        }
        g_order[g_count++] = static_cast<uint16_t>(globalIndex);
    }
    fclose(file);

    g_loaded = true;

    // If a knowledge-pack upgrade removed/renamed cards, clean the snapshot so
    // stale IDs do not accumulate forever. Failure here is non-fatal because
    // the in-memory favorite list is already valid.
    if(staleEntries > 0 || duplicateEntries > 0)
    {
        if(!writeSnapshot())
            FT02_LOGE_PRINTLN("[ERROR][FIELD-FAVORITES] cleanup snapshot FAILED");
    }
    return true;
}

void ensureLoaded()
{
    if(g_loaded) return;
    if(!loadSnapshot()) g_lastLoadRetryMs = millis();
}
}

void FT02_FieldManualFavoritesBegin()
{
    ensureLoaded();
}

int FT02_FieldManualFavoritesCount()
{
    ensureLoaded();
    return g_count;
}

int FT02_FieldManualFavoritesGlobalIndexAt(int favoriteIndex)
{
    ensureLoaded();
    if(favoriteIndex < 0 || favoriteIndex >= g_count) return -1;
    return static_cast<int>(g_order[favoriteIndex]);
}

bool FT02_FieldManualFavoritesIsFavorite(int globalCardIndex)
{
    ensureLoaded();
    return findGlobalIndex(globalCardIndex) >= 0;
}

bool FT02_FieldManualFavoritesSet(int globalCardIndex, bool favorite)
{
    ensureLoaded();
    if(!g_loaded) return false;
    if(globalCardIndex < 0 || globalCardIndex >= FT02_FieldManualCardCount()) return false;
    if(FT02_FieldManualCardAt(globalCardIndex) == nullptr) return false;

    const int existing = findGlobalIndex(globalCardIndex);
    if(favorite)
    {
        if(existing >= 0) return true;
        if(g_count >= FT02_FIELD_MANUAL_MAX_CARDS) return false;

        if(g_count > 0)
            memmove(&g_order[1], &g_order[0], sizeof(g_order[0]) * g_count);
        g_order[0] = static_cast<uint16_t>(globalCardIndex);
        ++g_count;

        if(writeSnapshot())
        {

            return true;
        }

        // Roll back the in-memory mutation if SD persistence failed.
        --g_count;
        if(g_count > 0)
            memmove(&g_order[0], &g_order[1], sizeof(g_order[0]) * g_count);
        FT02_LOGE("[ERROR][FIELD-FAVORITES] add FAILED index=%d\n", globalCardIndex);
        return false;
    }

    if(existing < 0) return true;
    const int oldCount = g_count;
    const uint16_t removed = g_order[existing];
    if(existing < g_count - 1)
    {
        memmove(
            &g_order[existing],
            &g_order[existing + 1],
            sizeof(g_order[0]) * (g_count - existing - 1)
        );
    }
    --g_count;

    if(writeSnapshot())
    {

        return true;
    }

    // Restore the exact previous position if persistence failed.
    if(existing < g_count)
    {
        memmove(
            &g_order[existing + 1],
            &g_order[existing],
            sizeof(g_order[0]) * (g_count - existing)
        );
    }
    g_order[existing] = removed;
    g_count = oldCount;
    FT02_LOGE("[ERROR][FIELD-FAVORITES] remove FAILED index=%d\n", globalCardIndex);
    return false;
}

bool FT02_FieldManualFavoritesToggle(int globalCardIndex, bool* outFavorite)
{
    const bool next = !FT02_FieldManualFavoritesIsFavorite(globalCardIndex);
    if(!FT02_FieldManualFavoritesSet(globalCardIndex, next)) return false;
    if(outFavorite != nullptr) *outFavorite = next;
    return true;
}
