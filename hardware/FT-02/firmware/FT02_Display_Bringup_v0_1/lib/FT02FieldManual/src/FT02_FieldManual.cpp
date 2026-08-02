#include "FT02_FieldManual.h"

#include <stdio.h>
#include <string.h>

#include "FT02_Storage.h"

namespace
{

static const char* FT02_FM_MANIFEST_PATH = "/knowledge/field_manual/manifest.bin";
static const char* FT02_FM_CATEGORIES_PATH = "/knowledge/field_manual/categories.bin";
static const char* FT02_FM_INDEX_PATH = "/knowledge/field_manual/cards.idx";
static const char* FT02_FM_DATA_PATH = "/knowledge/field_manual/cards.dat";
static const char* FT02_FM_QUICK_PATH = "/knowledge/field_manual/quick.idx";

static const uint8_t FT02_FM_MAGIC[8] = {'F', 'T', '0', '2', 'F', 'M', '1', 0};
static const uint16_t FT02_FM_SCHEMA_VERSION = 1;
static const uint32_t FT02_FM_FILE_FORMAT_VERSION = 1;
static const size_t FT02_FM_CARD_BUFFER_SIZE = 4096;

#pragma pack(push, 1)
struct FT02FieldManualManifestBinary
{
    uint8_t magic[8];
    uint16_t schemaVersion;
    uint16_t categoryCount;
    uint16_t cardCount;
    uint16_t quickCount;
    char contentVersion[16];
    uint32_t categoriesCrc32;
    uint32_t cardsIndexCrc32;
    uint32_t cardsDataCrc32;
    uint32_t quickIndexCrc32;
    uint32_t fileFormatVersion;
};
#pragma pack(pop)

static_assert(sizeof(FT02FieldManualManifestBinary) == 52, "field manual manifest layout mismatch");
static_assert(sizeof(FT02FieldManualCategory) == 204, "field manual category layout mismatch");
static_assert(sizeof(FT02FieldManualCardIndex) == 316, "field manual card index layout mismatch");

static FT02FieldManualState g_state = FT02_FIELD_MANUAL_NOT_STARTED;
static FT02FieldManualManifestBinary g_manifest = {};
static FT02FieldManualCategory g_categories[FT02_FIELD_MANUAL_MAX_CATEGORIES] = {};
static FT02FieldManualCardIndex g_cards[FT02_FIELD_MANUAL_MAX_CARDS] = {};
static uint16_t g_quickIndices[FT02_FIELD_MANUAL_MAX_QUICK] = {};
static uint8_t g_cardBuffer[FT02_FM_CARD_BUFFER_SIZE] = {};
static FT02FieldManualLoadedCard g_loadedCard = {};
static uint64_t g_cardsDataFileSize = 0;

static uint16_t FT02_FmReadU16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8U);
}

static uint32_t FT02_FmCrc32Update(uint32_t crc, const uint8_t* data, size_t length)
{
    crc = ~crc;
    for(size_t index = 0; index < length; index++)
    {
        crc ^= data[index];
        for(int bit = 0; bit < 8; bit++)
        {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

static bool FT02_FmFileCrc32(const char* path, uint32_t& output)
{
    output = 0;
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr)
    {
        return false;
    }

    uint8_t buffer[512];
    uint32_t crc = 0;
    while(true)
    {
        const size_t count = fread(buffer, 1, sizeof(buffer), file);
        if(count > 0)
        {
            crc = FT02_FmCrc32Update(crc, buffer, count);
        }
        if(count < sizeof(buffer))
        {
            if(ferror(file))
            {
                fclose(file);
                return false;
            }
            break;
        }
        yield();
    }

    fclose(file);
    output = crc;
    return true;
}

static bool FT02_FmReadExact(const char* path, void* destination, size_t bytes)
{
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr)
    {
        return false;
    }
    const bool ok = fread(destination, 1, bytes, file) == bytes;
    const int trailing = fgetc(file);
    fclose(file);
    return ok && trailing == EOF;
}

static bool FT02_FmVerifyFile(const char* path, uint32_t expectedCrc)
{
    uint32_t actual = 0;
    if(!FT02_FmFileCrc32(path, actual))
    {
        Serial.print("[FIELD-MANUAL] unable to CRC file: ");
        Serial.println(path);
        return false;
    }
    if(actual != expectedCrc)
    {
        Serial.print("[FIELD-MANUAL] CRC mismatch path=");
        Serial.print(path);
        Serial.print(" expected=0x");
        Serial.print(expectedCrc, HEX);
        Serial.print(" actual=0x");
        Serial.println(actual, HEX);
        return false;
    }
    return true;
}

static bool FT02_FmValidateCatalog()
{
    for(int categoryIndex = 0; categoryIndex < g_manifest.categoryCount; categoryIndex++)
    {
        FT02FieldManualCategory& category = g_categories[categoryIndex];
        category.id[sizeof(category.id) - 1] = 0;
        category.name[sizeof(category.name) - 1] = 0;
        category.summary[sizeof(category.summary) - 1] = 0;

        const uint32_t end = static_cast<uint32_t>(category.firstCard) + category.cardCount;
        if(end > g_manifest.cardCount)
        {
            Serial.print("[FIELD-MANUAL] category card range invalid: ");
            Serial.println(category.id);
            return false;
        }
    }

    for(int cardIndex = 0; cardIndex < g_manifest.cardCount; cardIndex++)
    {
        FT02FieldManualCardIndex& card = g_cards[cardIndex];
        card.id[sizeof(card.id) - 1] = 0;
        card.category[sizeof(card.category) - 1] = 0;
        card.title[sizeof(card.title) - 1] = 0;
        card.summary[sizeof(card.summary) - 1] = 0;

        const uint64_t end = static_cast<uint64_t>(card.dataOffset) + card.dataLength;
        if(card.dataLength < 1 || card.dataLength > FT02_FM_CARD_BUFFER_SIZE || end > g_cardsDataFileSize)
        {
            Serial.print("[FIELD-MANUAL] card data range invalid: ");
            Serial.println(card.id);
            return false;
        }
    }

    for(int quickIndex = 0; quickIndex < g_manifest.quickCount; quickIndex++)
    {
        if(g_quickIndices[quickIndex] >= g_manifest.cardCount)
        {
            Serial.println("[FIELD-MANUAL] quick index out of range");
            return false;
        }
    }

    return true;
}

static void FT02_FmResetMemory()
{
    memset(&g_manifest, 0, sizeof(g_manifest));
    memset(g_categories, 0, sizeof(g_categories));
    memset(g_cards, 0, sizeof(g_cards));
    memset(g_quickIndices, 0, sizeof(g_quickIndices));
    memset(g_cardBuffer, 0, sizeof(g_cardBuffer));
    memset(&g_loadedCard, 0, sizeof(g_loadedCard));
    g_cardsDataFileSize = 0;
}

} // namespace

bool FT02_FieldManualBegin(bool forceReload)
{
    if(g_state == FT02_FIELD_MANUAL_READY && !forceReload)
    {
        return true;
    }

    FT02_FmResetMemory();
    g_state = FT02_FIELD_MANUAL_NOT_STARTED;

    if(!FT02_StorageIsReady())
    {
        g_state = FT02_FIELD_MANUAL_STORAGE_NOT_READY;
        Serial.println("[FIELD-MANUAL] SD storage is not ready");
        return false;
    }

    if(!FT02_StorageFileExists(FT02_FM_MANIFEST_PATH) ||
       !FT02_StorageFileExists(FT02_FM_CATEGORIES_PATH) ||
       !FT02_StorageFileExists(FT02_FM_INDEX_PATH) ||
       !FT02_StorageFileExists(FT02_FM_DATA_PATH) ||
       !FT02_StorageFileExists(FT02_FM_QUICK_PATH))
    {
        g_state = FT02_FIELD_MANUAL_PACK_NOT_FOUND;
        Serial.println("[FIELD-MANUAL] runtime pack not found at /knowledge/field_manual");
        return false;
    }

    if(!FT02_FmReadExact(FT02_FM_MANIFEST_PATH, &g_manifest, sizeof(g_manifest)))
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        Serial.println("[FIELD-MANUAL] manifest read failed");
        return false;
    }

    g_manifest.contentVersion[sizeof(g_manifest.contentVersion) - 1] = 0;
    if(memcmp(g_manifest.magic, FT02_FM_MAGIC, sizeof(FT02_FM_MAGIC)) != 0 ||
       g_manifest.schemaVersion != FT02_FM_SCHEMA_VERSION ||
       g_manifest.fileFormatVersion != FT02_FM_FILE_FORMAT_VERSION ||
       g_manifest.categoryCount < 1 ||
       g_manifest.categoryCount > FT02_FIELD_MANUAL_MAX_CATEGORIES ||
       g_manifest.cardCount < 1 ||
       g_manifest.cardCount > FT02_FIELD_MANUAL_MAX_CARDS ||
       g_manifest.quickCount > FT02_FIELD_MANUAL_MAX_QUICK)
    {
        g_state = FT02_FIELD_MANUAL_BAD_FORMAT;
        Serial.println("[FIELD-MANUAL] manifest format/count check failed");
        return false;
    }

    const size_t categoryBytes = static_cast<size_t>(g_manifest.categoryCount) * sizeof(FT02FieldManualCategory);
    const size_t cardIndexBytes = static_cast<size_t>(g_manifest.cardCount) * sizeof(FT02FieldManualCardIndex);
    const size_t quickBytes = static_cast<size_t>(g_manifest.quickCount) * sizeof(uint16_t);

    uint64_t fileSize = 0;
    if(!FT02_StorageFileSize(FT02_FM_CATEGORIES_PATH, fileSize) || fileSize != categoryBytes ||
       !FT02_StorageFileSize(FT02_FM_INDEX_PATH, fileSize) || fileSize != cardIndexBytes ||
       !FT02_StorageFileSize(FT02_FM_QUICK_PATH, fileSize) || fileSize != quickBytes ||
       !FT02_StorageFileSize(FT02_FM_DATA_PATH, g_cardsDataFileSize))
    {
        g_state = FT02_FIELD_MANUAL_BAD_FORMAT;
        Serial.println("[FIELD-MANUAL] runtime file size check failed");
        return false;
    }

    if(!FT02_FmVerifyFile(FT02_FM_CATEGORIES_PATH, g_manifest.categoriesCrc32) ||
       !FT02_FmVerifyFile(FT02_FM_INDEX_PATH, g_manifest.cardsIndexCrc32) ||
       !FT02_FmVerifyFile(FT02_FM_DATA_PATH, g_manifest.cardsDataCrc32) ||
       !FT02_FmVerifyFile(FT02_FM_QUICK_PATH, g_manifest.quickIndexCrc32))
    {
        g_state = FT02_FIELD_MANUAL_CHECKSUM_FAILED;
        return false;
    }

    if(!FT02_FmReadExact(FT02_FM_CATEGORIES_PATH, g_categories, categoryBytes) ||
       !FT02_FmReadExact(FT02_FM_INDEX_PATH, g_cards, cardIndexBytes) ||
       !FT02_FmReadExact(FT02_FM_QUICK_PATH, g_quickIndices, quickBytes))
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        Serial.println("[FIELD-MANUAL] catalog read failed");
        return false;
    }

    if(!FT02_FmValidateCatalog())
    {
        g_state = FT02_FIELD_MANUAL_BAD_FORMAT;
        return false;
    }

    g_state = FT02_FIELD_MANUAL_READY;
    Serial.print("[FIELD-MANUAL] ready version=");
    Serial.print(g_manifest.contentVersion);
    Serial.print(" categories=");
    Serial.print(g_manifest.categoryCount);
    Serial.print(" cards=");
    Serial.print(g_manifest.cardCount);
    Serial.print(" quick=");
    Serial.println(g_manifest.quickCount);
    return true;
}

void FT02_FieldManualUnloadCard()
{
    memset(g_cardBuffer, 0, sizeof(g_cardBuffer));
    memset(&g_loadedCard, 0, sizeof(g_loadedCard));
}

FT02FieldManualState FT02_FieldManualStateCurrent()
{
    return g_state;
}

const char* FT02_FieldManualStateText()
{
    switch(g_state)
    {
        case FT02_FIELD_MANUAL_NOT_STARTED: return "NOT_STARTED";
        case FT02_FIELD_MANUAL_READY: return "READY";
        case FT02_FIELD_MANUAL_STORAGE_NOT_READY: return "SD_NOT_READY";
        case FT02_FIELD_MANUAL_PACK_NOT_FOUND: return "PACK_NOT_FOUND";
        case FT02_FIELD_MANUAL_BAD_FORMAT: return "BAD_FORMAT";
        case FT02_FIELD_MANUAL_CHECKSUM_FAILED: return "CHECKSUM_FAILED";
        case FT02_FIELD_MANUAL_IO_ERROR: return "IO_ERROR";
        default: return "UNKNOWN";
    }
}

const char* FT02_FieldManualContentVersion()
{
    return g_manifest.contentVersion[0] != 0 ? g_manifest.contentVersion : "-";
}

int FT02_FieldManualCategoryCount()
{
    return g_state == FT02_FIELD_MANUAL_READY ? g_manifest.categoryCount : 0;
}

int FT02_FieldManualCardCount()
{
    return g_state == FT02_FIELD_MANUAL_READY ? g_manifest.cardCount : 0;
}

int FT02_FieldManualQuickCount()
{
    return g_state == FT02_FIELD_MANUAL_READY ? g_manifest.quickCount : 0;
}

const FT02FieldManualCategory* FT02_FieldManualCategoryAt(int index)
{
    if(g_state != FT02_FIELD_MANUAL_READY || index < 0 || index >= g_manifest.categoryCount)
    {
        return nullptr;
    }
    return &g_categories[index];
}

const FT02FieldManualCardIndex* FT02_FieldManualCardAt(int globalIndex)
{
    if(g_state != FT02_FIELD_MANUAL_READY || globalIndex < 0 || globalIndex >= g_manifest.cardCount)
    {
        return nullptr;
    }
    return &g_cards[globalIndex];
}

int FT02_FieldManualCategoryCardGlobalIndex(int categoryIndex, int localIndex)
{
    const FT02FieldManualCategory* category = FT02_FieldManualCategoryAt(categoryIndex);
    if(category == nullptr || localIndex < 0 || localIndex >= category->cardCount)
    {
        return -1;
    }
    return category->firstCard + localIndex;
}

int FT02_FieldManualQuickGlobalIndex(int localIndex)
{
    if(g_state != FT02_FIELD_MANUAL_READY || localIndex < 0 || localIndex >= g_manifest.quickCount)
    {
        return -1;
    }
    return g_quickIndices[localIndex];
}

bool FT02_FieldManualLoadCard(int globalIndex)
{
    FT02_FieldManualUnloadCard();
    const FT02FieldManualCardIndex* index = FT02_FieldManualCardAt(globalIndex);
    if(index == nullptr || index->dataLength > sizeof(g_cardBuffer))
    {
        return false;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_FM_DATA_PATH);
    if(file == nullptr)
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        return false;
    }

    bool ok = fseek(file, static_cast<long>(index->dataOffset), SEEK_SET) == 0;
    ok = ok && fread(g_cardBuffer, 1, index->dataLength, file) == index->dataLength;
    fclose(file);
    if(!ok)
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        return false;
    }

    const uint8_t sectionCount = g_cardBuffer[0];
    if(sectionCount < 1 || sectionCount > FT02_FIELD_MANUAL_MAX_SECTIONS)
    {
        return false;
    }

    size_t cursor = 1;
    g_loadedCard.index = index;
    g_loadedCard.sectionCount = sectionCount;

    for(int sectionIndex = 0; sectionIndex < sectionCount; sectionIndex++)
    {
        if(cursor + 4 > index->dataLength)
        {
            FT02_FieldManualUnloadCard();
            return false;
        }
        const uint16_t titleLength = FT02_FmReadU16(g_cardBuffer + cursor);
        const uint16_t textLength = FT02_FmReadU16(g_cardBuffer + cursor + 2);
        cursor += 4;
        if(titleLength < 1 || textLength < 1 || cursor + titleLength + textLength > index->dataLength)
        {
            FT02_FieldManualUnloadCard();
            return false;
        }

        const char* title = reinterpret_cast<const char*>(g_cardBuffer + cursor);
        cursor += titleLength;
        const char* text = reinterpret_cast<const char*>(g_cardBuffer + cursor);
        cursor += textLength;
        if(title[titleLength - 1] != 0 || text[textLength - 1] != 0)
        {
            FT02_FieldManualUnloadCard();
            return false;
        }

        g_loadedCard.sections[sectionIndex].title = title;
        g_loadedCard.sections[sectionIndex].text = text;
    }

    if(cursor != index->dataLength)
    {
        FT02_FieldManualUnloadCard();
        return false;
    }

    return true;
}

const FT02FieldManualLoadedCard* FT02_FieldManualLoadedCardCurrent()
{
    return g_loadedCard.index != nullptr ? &g_loadedCard : nullptr;
}
