#include "FT02_FieldManual.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "FT02_Storage.h"

namespace
{

static const char* FT02_FM_MANIFEST_PATH = "/knowledge/field_manual/manifest.bin";
static const char* FT02_FM_CATEGORIES_PATH = "/knowledge/field_manual/categories.bin";
static const char* FT02_FM_INDEX_PATH = "/knowledge/field_manual/cards.idx";
static const char* FT02_FM_DATA_PATH = "/knowledge/field_manual/cards.dat";
static const char* FT02_FM_QUICK_PATH = "/knowledge/field_manual/quick.idx";
static const char* FT02_FM_MEDIA_INDEX_PATH = "/knowledge/field_manual/media.idx";
static const char* FT02_FM_MEDIA_DATA_PATH = "/knowledge/field_manual/media.dat";

static const uint8_t FT02_FM_MAGIC[8] = {'F', 'T', '0', '2', 'F', 'M', '1', 0};
static const uint16_t FT02_FM_SCHEMA_VERSION = 1;
static const uint32_t FT02_FM_FILE_FORMAT_VERSION = 1;
static const size_t FT02_FM_CARD_BUFFER_SIZE = 4096;
static const uint8_t FT02_FM_MEDIA_MAGIC[8] = {'F', 'T', '0', '2', 'M', 'D', '1', 0};
static const uint16_t FT02_FM_MEDIA_FORMAT_VERSION = 1;

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

struct FT02FieldManualMediaHeaderBinary
{
    uint8_t magic[8];
    uint16_t formatVersion;
    uint16_t mediaCount;
    uint32_t dataFileSize;
};
#pragma pack(pop)

static_assert(sizeof(FT02FieldManualManifestBinary) == 52, "field manual manifest layout mismatch");
static_assert(sizeof(FT02FieldManualCategory) == 204, "field manual category layout mismatch");
static_assert(sizeof(FT02FieldManualCardIndex) == 316, "field manual card index layout mismatch");
static_assert(sizeof(FT02FieldManualMediaHeaderBinary) == 16, "field manual media header layout mismatch");
static_assert(sizeof(FT02FieldManualMediaIndex) == 84, "field manual media index layout mismatch");

static FT02FieldManualState g_state = FT02_FIELD_MANUAL_NOT_STARTED;
static FT02FieldManualManifestBinary g_manifest = {};
static FT02FieldManualCategory g_categories[FT02_FIELD_MANUAL_MAX_CATEGORIES] = {};
static FT02FieldManualCardIndex* g_cards = nullptr;
static size_t g_cardsAllocatedBytes = 0;
static uint16_t g_quickIndices[FT02_FIELD_MANUAL_MAX_QUICK] = {};
static uint8_t g_cardBuffer[FT02_FM_CARD_BUFFER_SIZE] = {};
static FT02FieldManualLoadedCard g_loadedCard = {};
static uint64_t g_cardsDataFileSize = 0;

static bool g_mediaReady = false;
static uint16_t g_mediaCount = 0;
static uint64_t g_mediaDataFileSize = 0;
static FT02FieldManualMediaIndex* g_media = nullptr;
static size_t g_mediaAllocatedBytes = 0;
static uint8_t* g_mediaBuffer = nullptr;
static size_t g_mediaBufferAllocatedBytes = 0;
static FT02FieldManualLoadedMedia g_loadedMedia = {};

static void* FT02_FmPsramAlloc(size_t bytes)
{
    if(bytes == 0) return nullptr;
#if defined(ARDUINO_ARCH_ESP32)
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return malloc(bytes);
#endif
}

static void FT02_FmPsramFree(void* memory)
{
    if(memory == nullptr) return;
#if defined(ARDUINO_ARCH_ESP32)
    heap_caps_free(memory);
#else
    free(memory);
#endif
}

static bool FT02_FmReadExactStaged(const char* path, void* destination, size_t bytes)
{
    if(destination == nullptr && bytes != 0) return false;
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;

    uint8_t stage[512];
    uint8_t* output = static_cast<uint8_t*>(destination);
    size_t done = 0;
    bool ok = true;
    while(done < bytes)
    {
        const size_t chunk = (bytes - done) < sizeof(stage) ? (bytes - done) : sizeof(stage);
        if(fread(stage, 1, chunk, file) != chunk)
        {
            ok = false;
            break;
        }
        memcpy(output + done, stage, chunk);
        done += chunk;
        yield();
    }
    const int trailing = ok ? fgetc(file) : EOF;
    fclose(file);
    return ok && done == bytes && trailing == EOF;
}

static bool FT02_FmReadRangeStaged(const char* path, uint32_t offset, void* destination, size_t bytes)
{
    if(destination == nullptr && bytes != 0) return false;
    FILE* file = FT02_StorageOpenReadFile(path);
    if(file == nullptr) return false;
    bool ok = fseek(file, static_cast<long>(offset), SEEK_SET) == 0;

    uint8_t stage[512];
    uint8_t* output = static_cast<uint8_t*>(destination);
    size_t done = 0;
    while(ok && done < bytes)
    {
        const size_t chunk = (bytes - done) < sizeof(stage) ? (bytes - done) : sizeof(stage);
        if(fread(stage, 1, chunk, file) != chunk)
        {
            ok = false;
            break;
        }
        memcpy(output + done, stage, chunk);
        done += chunk;
        yield();
    }
    fclose(file);
    return ok && done == bytes;
}

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

static bool FT02_FmLoadOptionalMediaCatalog()
{
    g_mediaReady = false;
    g_mediaCount = 0;
    g_mediaDataFileSize = 0;
    if(g_media != nullptr)
    {
        FT02_FmPsramFree(g_media);
        g_media = nullptr;
        g_mediaAllocatedBytes = 0;
    }
    if(g_mediaBuffer != nullptr)
    {
        FT02_FmPsramFree(g_mediaBuffer);
        g_mediaBuffer = nullptr;
        g_mediaBufferAllocatedBytes = 0;
    }
    memset(&g_loadedMedia, 0, sizeof(g_loadedMedia));

    const bool hasIndex = FT02_StorageFileExists(FT02_FM_MEDIA_INDEX_PATH);
    const bool hasData = FT02_StorageFileExists(FT02_FM_MEDIA_DATA_PATH);
    if(!hasIndex && !hasData)
    {
        Serial.println("[FIELD-MANUAL] optional media pack not present; text-only mode");
        return true;
    }
    if(!hasIndex || !hasData)
    {
        Serial.println("[FIELD-MANUAL] media pack incomplete; media disabled");
        return true;
    }

    uint64_t indexFileSize = 0;
    uint64_t dataFileSize = 0;
    if(!FT02_StorageFileSize(FT02_FM_MEDIA_INDEX_PATH, indexFileSize) ||
       !FT02_StorageFileSize(FT02_FM_MEDIA_DATA_PATH, dataFileSize))
    {
        Serial.println("[FIELD-MANUAL] media file size read failed; media disabled");
        return true;
    }

    FILE* file = FT02_StorageOpenReadFile(FT02_FM_MEDIA_INDEX_PATH);
    if(file == nullptr)
    {
        Serial.println("[FIELD-MANUAL] media index open failed; media disabled");
        return true;
    }

    FT02FieldManualMediaHeaderBinary header = {};
    bool ok = fread(&header, 1, sizeof(header), file) == sizeof(header);
    if(!ok || memcmp(header.magic, FT02_FM_MEDIA_MAGIC, sizeof(header.magic)) != 0 ||
       header.formatVersion != FT02_FM_MEDIA_FORMAT_VERSION ||
       header.mediaCount > FT02_FIELD_MANUAL_MAX_MEDIA ||
       header.dataFileSize != dataFileSize)
    {
        fclose(file);
        Serial.println("[FIELD-MANUAL] media header invalid; media disabled");
        return true;
    }

    const uint64_t expectedIndexSize = sizeof(header) +
        static_cast<uint64_t>(header.mediaCount) * sizeof(FT02FieldManualMediaIndex);
    if(indexFileSize != expectedIndexSize)
    {
        fclose(file);
        Serial.println("[FIELD-MANUAL] media index size mismatch; media disabled");
        return true;
    }

    fclose(file);

    if(header.mediaCount > 0)
    {
        const size_t mediaBytes = static_cast<size_t>(header.mediaCount) * sizeof(FT02FieldManualMediaIndex);
        g_media = static_cast<FT02FieldManualMediaIndex*>(FT02_FmPsramAlloc(mediaBytes));
        if(g_media == nullptr)
        {
            Serial.println("[FIELD-MANUAL] media PSRAM catalog allocation failed; media disabled");
            return true;
        }
        g_mediaAllocatedBytes = mediaBytes;
        memset(g_media, 0, mediaBytes);
        if(!FT02_FmReadRangeStaged(
            FT02_FM_MEDIA_INDEX_PATH,
            static_cast<uint32_t>(sizeof(header)),
            g_media,
            mediaBytes
        ))
        {
            Serial.println("[FIELD-MANUAL] media index staged read failed; media disabled");
            FT02_FmPsramFree(g_media);
            g_media = nullptr;
            g_mediaAllocatedBytes = 0;
            return true;
        }
    }

    for(int mediaIndex = 0; mediaIndex < header.mediaCount; mediaIndex++)
    {
        FT02FieldManualMediaIndex& item = g_media[mediaIndex];
        item.caption[sizeof(item.caption) - 1] = 0;
        const uint32_t rowBytes = (static_cast<uint32_t>(item.width) + 7U) / 8U;
        const uint32_t expectedLength = rowBytes * item.height;
        const uint64_t end = static_cast<uint64_t>(item.dataOffset) + item.dataLength;
        if(item.cardIndex >= g_manifest.cardCount ||
           item.width < 1 || item.width > FT02_FIELD_MANUAL_MEDIA_MAX_WIDTH ||
           item.height < 1 || item.height > FT02_FIELD_MANUAL_MEDIA_MAX_HEIGHT ||
           item.dataLength < 1 || item.dataLength > FT02_FIELD_MANUAL_MEDIA_BUFFER_SIZE ||
           item.dataLength != expectedLength || end > dataFileSize)
        {
            Serial.print("[FIELD-MANUAL] media entry invalid index=");
            Serial.println(mediaIndex);
            FT02_FmPsramFree(g_media);
            g_media = nullptr;
            g_mediaAllocatedBytes = 0;
            return true;
        }
    }

    g_mediaCount = header.mediaCount;
    g_mediaDataFileSize = dataFileSize;
    g_mediaReady = true;
    return true;
}

static void FT02_FmResetMemory()
{
    memset(&g_manifest, 0, sizeof(g_manifest));
    memset(g_categories, 0, sizeof(g_categories));
    if(g_cards != nullptr)
    {
        FT02_FmPsramFree(g_cards);
        g_cards = nullptr;
    }
    g_cardsAllocatedBytes = 0;
    memset(g_quickIndices, 0, sizeof(g_quickIndices));
    memset(g_cardBuffer, 0, sizeof(g_cardBuffer));
    memset(&g_loadedCard, 0, sizeof(g_loadedCard));
    g_cardsDataFileSize = 0;
    g_mediaReady = false;
    g_mediaCount = 0;
    g_mediaDataFileSize = 0;
    if(g_media != nullptr)
    {
        FT02_FmPsramFree(g_media);
        g_media = nullptr;
    }
    g_mediaAllocatedBytes = 0;
    if(g_mediaBuffer != nullptr)
    {
        FT02_FmPsramFree(g_mediaBuffer);
        g_mediaBuffer = nullptr;
    }
    g_mediaBufferAllocatedBytes = 0;
    memset(&g_loadedMedia, 0, sizeof(g_loadedMedia));
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

    g_cards = static_cast<FT02FieldManualCardIndex*>(FT02_FmPsramAlloc(cardIndexBytes));
    if(g_cards == nullptr)
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        Serial.println("[FIELD-MANUAL] card index PSRAM allocation failed");
        return false;
    }
    g_cardsAllocatedBytes = cardIndexBytes;
    memset(g_cards, 0, cardIndexBytes);

    if(!FT02_FmReadExact(FT02_FM_CATEGORIES_PATH, g_categories, categoryBytes) ||
       !FT02_FmReadExactStaged(FT02_FM_INDEX_PATH, g_cards, cardIndexBytes) ||
       !FT02_FmReadExact(FT02_FM_QUICK_PATH, g_quickIndices, quickBytes))
    {
        g_state = FT02_FIELD_MANUAL_IO_ERROR;
        Serial.println("[FIELD-MANUAL] catalog read failed");
        FT02_FmPsramFree(g_cards);
        g_cards = nullptr;
        g_cardsAllocatedBytes = 0;
        return false;
    }

    if(!FT02_FmValidateCatalog())
    {
        g_state = FT02_FIELD_MANUAL_BAD_FORMAT;
        FT02_FmPsramFree(g_cards);
        g_cards = nullptr;
        g_cardsAllocatedBytes = 0;
        return false;
    }

    // Runtime A3 media is optional and deliberately fail-soft: a damaged or
    // absent media pack never prevents the reviewed text manual from loading.
    FT02_FmLoadOptionalMediaCatalog();

    g_state = FT02_FIELD_MANUAL_READY;
    return true;
}

void FT02_FieldManualUnloadMedia()
{
    memset(&g_loadedMedia, 0, sizeof(g_loadedMedia));
    if(g_mediaBuffer != nullptr)
    {
        FT02_FmPsramFree(g_mediaBuffer);
        g_mediaBuffer = nullptr;
    }
    g_mediaBufferAllocatedBytes = 0;
}

void FT02_FieldManualUnloadCard()
{
    FT02_FieldManualUnloadMedia();
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
    if(g_state != FT02_FIELD_MANUAL_READY || g_cards == nullptr || globalIndex < 0 || globalIndex >= g_manifest.cardCount)
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

int FT02_FieldManualLoadedCardGlobalIndex()
{
    if(g_loadedCard.index == nullptr || g_cards == nullptr) return -1;
    const ptrdiff_t index = g_loadedCard.index - g_cards;
    return index >= 0 && index < g_manifest.cardCount ? static_cast<int>(index) : -1;
}

bool FT02_FieldManualMediaReady()
{
    return g_state == FT02_FIELD_MANUAL_READY && g_mediaReady;
}

int FT02_FieldManualMediaCount()
{
    return FT02_FieldManualMediaReady() ? g_mediaCount : 0;
}

int FT02_FieldManualMediaCountForCard(int globalCardIndex)
{
    if(!FT02_FieldManualMediaReady() || globalCardIndex < 0 || globalCardIndex >= g_manifest.cardCount)
    {
        return 0;
    }
    int count = 0;
    for(int mediaIndex = 0; mediaIndex < g_mediaCount; mediaIndex++)
    {
        if(g_media[mediaIndex].cardIndex == globalCardIndex) count++;
    }
    return count;
}

bool FT02_FieldManualLoadMediaForCard(int globalCardIndex, int mediaOrdinal)
{
    FT02_FieldManualUnloadMedia();
    if(!FT02_FieldManualMediaReady() || mediaOrdinal < 0) return false;

    const FT02FieldManualMediaIndex* selected = nullptr;
    int ordinal = 0;
    for(int mediaIndex = 0; mediaIndex < g_mediaCount; mediaIndex++)
    {
        if(g_media[mediaIndex].cardIndex != globalCardIndex) continue;
        if(ordinal == mediaOrdinal)
        {
            selected = &g_media[mediaIndex];
            break;
        }
        ordinal++;
    }
    if(selected == nullptr || selected->dataLength > FT02_FIELD_MANUAL_MEDIA_BUFFER_SIZE) return false;

    g_mediaBuffer = static_cast<uint8_t*>(FT02_FmPsramAlloc(selected->dataLength));
    if(g_mediaBuffer == nullptr)
    {
        Serial.println("[FIELD-MANUAL] media PSRAM image allocation failed");
        return false;
    }
    g_mediaBufferAllocatedBytes = selected->dataLength;
    if(!FT02_FmReadRangeStaged(
        FT02_FM_MEDIA_DATA_PATH,
        selected->dataOffset,
        g_mediaBuffer,
        selected->dataLength
    ))
    {
        FT02_FieldManualUnloadMedia();
        return false;
    }

    const uint32_t crc = FT02_FmCrc32Update(0, g_mediaBuffer, selected->dataLength);
    if(crc != selected->crc32)
    {
        Serial.print("[FIELD-MANUAL] media CRC mismatch card=");
        Serial.print(globalCardIndex);
        Serial.print(" ordinal=");
        Serial.println(mediaOrdinal);
        FT02_FieldManualUnloadMedia();
        return false;
    }

    g_loadedMedia.index = selected;
    g_loadedMedia.data = g_mediaBuffer;
    return true;
}

const FT02FieldManualLoadedMedia* FT02_FieldManualLoadedMediaCurrent()
{
    return g_loadedMedia.index != nullptr ? &g_loadedMedia : nullptr;
}

