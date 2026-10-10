#include "FT02_PinyinEngineAdapter.h"

#include <stdio.h>
#include <string.h>

#include "FT02_PinyinIme.h"
#include "FT02_Storage.h"

namespace
{
constexpr char INDEX_MAGIC[8] = {'F','T','2','P','Y','I','1','\0'};
constexpr char DATA_MAGIC[8]  = {'F','T','2','P','Y','D','1','\0'};
constexpr size_t INDEX_KEY_BYTES = FT02_PINYIN_MAX_INPUT + 1u;

enum class ProbeState : uint8_t
{
    Unknown = 0,
    Ready,
    MissingOrInvalid
};

#pragma pack(push, 1)
struct IndexHeaderDisk
{
    char magic[8];
    uint16_t version;
    uint16_t recordSize;
    uint32_t groupCount;
    uint32_t candidateCount;
    uint32_t reserved[3];
};

struct IndexRecordDisk
{
    char key[INDEX_KEY_BYTES];
    uint8_t keyLen;
    uint16_t candidateCount;
    uint32_t dataOffset;
    uint32_t reserved;
};

struct DataHeaderDisk
{
    char magic[8];
    uint16_t version;
    uint16_t headerSize;
    uint32_t groupCount;
    uint32_t candidateCount;
    uint32_t reserved[3];
};
#pragma pack(pop)

static_assert(sizeof(IndexHeaderDisk) == 32u, "A5 index header layout drift");
static_assert(sizeof(IndexRecordDisk) == 36u, "A5 index record layout drift");
static_assert(sizeof(DataHeaderDisk) == 32u, "A5 data header layout drift");

struct CachedCandidate
{
    char utf8[FT02_PINYIN_MAX_CANDIDATE_BYTES + 1u];
    uint32_t frequency;
};

ProbeState g_probeState = ProbeState::Unknown;
IndexHeaderDisk g_indexHeader = {};
DataHeaderDisk g_dataHeader = {};
char g_cachedKey[FT02_PINYIN_MAX_INPUT + 1u] = {};
CachedCandidate g_cache[FT02_PINYIN_A5_MAX_CANDIDATES] = {};
size_t g_cacheCount = 0;
bool g_cacheValid = false;

bool validKey(const char* key)
{
    if(key == nullptr || key[0] == '\0') return false;
    const size_t n = strlen(key);
    if(n == 0 || n > FT02_PINYIN_MAX_INPUT) return false;
    for(size_t i = 0; i < n; ++i)
        if(key[i] < 'a' || key[i] > 'z') return false;
    return true;
}

bool readExact(FILE* file, void* dst, size_t bytes)
{
    return file != nullptr && dst != nullptr && bytes > 0u && fread(dst, 1u, bytes, file) == bytes;
}

bool validateIndexHeader(const IndexHeaderDisk& header)
{
    return memcmp(header.magic, INDEX_MAGIC, sizeof(INDEX_MAGIC)) == 0 &&
           header.version == FT02_PINYIN_A5_FORMAT_VERSION &&
           header.recordSize == sizeof(IndexRecordDisk) &&
           header.groupCount > 0u;
}

bool validateDataHeader(const DataHeaderDisk& header, const IndexHeaderDisk& indexHeader)
{
    return memcmp(header.magic, DATA_MAGIC, sizeof(DATA_MAGIC)) == 0 &&
           header.version == FT02_PINYIN_A5_FORMAT_VERSION &&
           header.headerSize == sizeof(DataHeaderDisk) &&
           header.groupCount == indexHeader.groupCount &&
           header.candidateCount == indexHeader.candidateCount;
}

void clearCache()
{
    g_cachedKey[0] = '\0';
    g_cacheCount = 0;
    g_cacheValid = false;
}

bool probeDictionary()
{
    if(g_probeState == ProbeState::Ready) return true;
    if(g_probeState == ProbeState::MissingOrInvalid) return false;

    // Do not freeze the state to "missing" while storage is still coming up.
    if(!FT02_StorageIsReady()) return false;

    if(!FT02_StorageFileExists(FT02_PINYIN_A5_INDEX_PATH) ||
       !FT02_StorageFileExists(FT02_PINYIN_A5_DATA_PATH))
    {
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }

    FILE* indexFile = FT02_StorageOpenReadFile(FT02_PINYIN_A5_INDEX_PATH);
    if(indexFile == nullptr)
    {
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }
    const bool indexOk = readExact(indexFile, &g_indexHeader, sizeof(g_indexHeader)) &&
                         validateIndexHeader(g_indexHeader);
    fclose(indexFile);
    if(!indexOk)
    {
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }

    FILE* dataFile = FT02_StorageOpenReadFile(FT02_PINYIN_A5_DATA_PATH);
    if(dataFile == nullptr)
    {
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }
    const bool dataOk = readExact(dataFile, &g_dataHeader, sizeof(g_dataHeader)) &&
                        validateDataHeader(g_dataHeader, g_indexHeader);
    fclose(dataFile);
    if(!dataOk)
    {
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }

    clearCache();
    g_probeState = ProbeState::Ready;
    return true;
}

bool readIndexRecord(FILE* file, uint32_t index, IndexRecordDisk& record)
{
    const uint64_t offset = static_cast<uint64_t>(sizeof(IndexHeaderDisk)) +
                            static_cast<uint64_t>(index) * sizeof(IndexRecordDisk);
    if(offset > 0x7FFFFFFFu) return false;
    if(fseek(file, static_cast<long>(offset), SEEK_SET) != 0) return false;
    if(!readExact(file, &record, sizeof(record))) return false;

    // Make corrupted records harmless before strcmp/strlen can walk outside.
    record.key[INDEX_KEY_BYTES - 1u] = '\0';
    if(record.keyLen == 0u || record.keyLen > FT02_PINYIN_MAX_INPUT) return false;
    if(record.key[record.keyLen] != '\0') return false;
    if(strlen(record.key) != record.keyLen) return false;
    return validKey(record.key);
}

bool findIndexRecord(const char* key, IndexRecordDisk& result)
{
    FILE* file = FT02_StorageOpenReadFile(FT02_PINYIN_A5_INDEX_PATH);
    if(file == nullptr) return false;

    uint32_t lo = 0;
    uint32_t hi = g_indexHeader.groupCount;
    bool found = false;
    while(lo < hi)
    {
        const uint32_t mid = lo + (hi - lo) / 2u;
        IndexRecordDisk record = {};
        if(!readIndexRecord(file, mid, record)) break;
        const int cmp = strcmp(key, record.key);
        if(cmp == 0)
        {
            result = record;
            found = true;
            break;
        }
        if(cmp < 0) hi = mid;
        else lo = mid + 1u;
    }

    fclose(file);
    return found;
}

bool loadKey(const char* key)
{
    if(!validKey(key) || !probeDictionary()) return false;
    if(g_cacheValid && strcmp(key, g_cachedKey) == 0) return g_cacheCount > 0u;

    clearCache();
    snprintf(g_cachedKey, sizeof(g_cachedKey), "%s", key);
    g_cacheValid = true; // cache negative lookups too

    IndexRecordDisk record = {};
    if(!findIndexRecord(key, record)) return false;
    if(record.candidateCount == 0u) return false;

    FILE* file = FT02_StorageOpenReadFile(FT02_PINYIN_A5_DATA_PATH);
    if(file == nullptr)
    {
        clearCache();
        return false;
    }

    bool ok = true;
    if(record.dataOffset < sizeof(DataHeaderDisk) ||
       fseek(file, static_cast<long>(record.dataOffset), SEEK_SET) != 0)
    {
        ok = false;
    }

    const size_t wanted = record.candidateCount < FT02_PINYIN_A5_MAX_CANDIDATES
                            ? record.candidateCount
                            : FT02_PINYIN_A5_MAX_CANDIDATES;
    for(size_t i = 0; ok && i < wanted; ++i)
    {
        uint32_t frequency = 0;
        uint8_t utf8Bytes = 0;
        if(!readExact(file, &frequency, sizeof(frequency)) ||
           !readExact(file, &utf8Bytes, sizeof(utf8Bytes)))
        {
            ok = false;
            break;
        }
        if(utf8Bytes == 0u || utf8Bytes > FT02_PINYIN_MAX_CANDIDATE_BYTES)
        {
            ok = false;
            break;
        }
        if(!readExact(file, g_cache[i].utf8, utf8Bytes))
        {
            ok = false;
            break;
        }
        g_cache[i].utf8[utf8Bytes] = '\0';
        g_cache[i].frequency = frequency;
        ++g_cacheCount;
    }
    fclose(file);

    if(!ok)
    {
        // Treat format/data corruption as a non-fatal smart-engine failure for
        // this boot. The legacy built-in dictionary remains available.
        clearCache();
        g_probeState = ProbeState::MissingOrInvalid;
        return false;
    }
    return g_cacheCount > 0u;
}
}

bool FT02_PinyinEngineAdapterAvailable()
{
    return probeDictionary();
}

size_t FT02_PinyinEngineAdapterCandidateCount(const char* pinyinKey)
{
    if(!loadKey(pinyinKey)) return 0u;
    return g_cacheCount;
}

bool FT02_PinyinEngineAdapterCandidate(
    const char* pinyinKey,
    size_t index,
    char* outUtf8,
    size_t outSize,
    uint32_t* baseFrequency
)
{
    if(outUtf8 == nullptr || outSize == 0u) return false;
    outUtf8[0] = '\0';
    if(baseFrequency != nullptr) *baseFrequency = 0u;
    if(!loadKey(pinyinKey) || index >= g_cacheCount) return false;

    const size_t n = strlen(g_cache[index].utf8);
    if(n + 1u > outSize) return false;
    memcpy(outUtf8, g_cache[index].utf8, n + 1u);
    if(baseFrequency != nullptr) *baseFrequency = g_cache[index].frequency;
    return true;
}
