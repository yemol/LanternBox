#include "FT02_WifiProfileStore.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "FT02_Storage.h"

namespace
{
static constexpr uint32_t FT02_WIFI_PROFILE_MAGIC = 0x32465746UL; // FWF2
static constexpr uint16_t FT02_WIFI_PROFILE_VERSION_A1 = 1;
static constexpr uint16_t FT02_WIFI_PROFILE_VERSION_A2 = 2;

static const char* FT02_WIFI_PROFILE_PATH = "/wifi/wifi_profiles.bin";
static const char* FT02_WIFI_PROFILE_TEMP = "/wifi/wifi_profiles.tmp";
static const char* FT02_WIFI_PROFILE_BACKUP = "/wifi/wifi_profiles.bak";

struct FT02WifiProfileFileHeader
{
    uint32_t magic;
    uint16_t version;
    uint16_t count;
};

struct FT02WifiProfileA1
{
    char ssid[FT02_WIFI_PROFILE_SSID_LEN];
    char password[FT02_WIFI_PROFILE_PASSWORD_LEN];
    int16_t priority;
    uint32_t lastConnected;
};

static FT02WifiProfile g_profiles[FT02_WIFI_PROFILE_MAX_COUNT] = {};
static uint8_t g_profileCount = 0;
static bool g_loaded = false;

static uint32_t nowSeconds()
{
    const time_t t = time(nullptr);
    if(t >= 1704067200) return static_cast<uint32_t>(t);
    return millis() / 1000UL;
}

static void sanitizeProfile(FT02WifiProfile& profile)
{
    profile.ssid[FT02_WIFI_PROFILE_SSID_LEN - 1] = '\0';
    profile.password[FT02_WIFI_PROFILE_PASSWORD_LEN - 1] = '\0';
    if(profile.lastSuccess == 0) profile.lastSuccess = profile.lastConnected;
}
}

void FT02_WifiProfileStoreBegin()
{
    g_profileCount = 0;
    g_loaded = false;
}

bool FT02_WifiProfileStoreIsLoaded()
{
    return g_loaded;
}

void FT02_WifiProfileStoreSort()
{
    for(uint8_t i = 0; i < g_profileCount; ++i)
    {
        for(uint8_t j = static_cast<uint8_t>(i + 1); j < g_profileCount; ++j)
        {
            const uint32_t lhsRecent = g_profiles[i].lastSuccess != 0 ? g_profiles[i].lastSuccess : g_profiles[i].lastConnected;
            const uint32_t rhsRecent = g_profiles[j].lastSuccess != 0 ? g_profiles[j].lastSuccess : g_profiles[j].lastConnected;
            const bool better =
                g_profiles[j].priority > g_profiles[i].priority ||
                (g_profiles[j].priority == g_profiles[i].priority && rhsRecent > lhsRecent);
            if(better)
            {
                FT02WifiProfile tmp = g_profiles[i];
                g_profiles[i] = g_profiles[j];
                g_profiles[j] = tmp;
            }
        }
    }
}

bool FT02_WifiProfileStoreLoad()
{
    g_profileCount = 0;
    g_loaded = true;

    FILE* file = FT02_StorageOpenReadFile(FT02_WIFI_PROFILE_PATH);
    if(file == nullptr) return false;

    FT02WifiProfileFileHeader header = {};
    if(fread(&header, 1, sizeof(header), file) != sizeof(header) ||
       header.magic != FT02_WIFI_PROFILE_MAGIC ||
       (header.version != FT02_WIFI_PROFILE_VERSION_A1 && header.version != FT02_WIFI_PROFILE_VERSION_A2))
    {
        fclose(file);
        return false;
    }

    const uint16_t count = header.count < FT02_WIFI_PROFILE_MAX_COUNT
        ? header.count
        : FT02_WIFI_PROFILE_MAX_COUNT;

    for(uint16_t i = 0; i < count; ++i)
    {
        FT02WifiProfile profile = {};
        if(header.version == FT02_WIFI_PROFILE_VERSION_A1)
        {
            FT02WifiProfileA1 oldProfile = {};
            if(fread(&oldProfile, 1, sizeof(oldProfile), file) != sizeof(oldProfile)) break;
            snprintf(profile.ssid, sizeof(profile.ssid), "%s", oldProfile.ssid);
            snprintf(profile.password, sizeof(profile.password), "%s", oldProfile.password);
            profile.priority = oldProfile.priority;
            profile.lastConnected = oldProfile.lastConnected;
            profile.lastSuccess = oldProfile.lastConnected;
            profile.autoConnect = true;
        }
        else
        {
            if(fread(&profile, 1, sizeof(profile), file) != sizeof(profile)) break;
        }
        sanitizeProfile(profile);
        if(profile.ssid[0] == '\0') continue;
        g_profiles[g_profileCount++] = profile;
    }

    fclose(file);
    FT02_WifiProfileStoreSort();
    return g_profileCount > 0;
}

bool FT02_WifiProfileStoreSave()
{
    if(!FT02_StorageIsReady()) return false;

    FT02_WifiProfileStoreSort();
    (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_TEMP);
    FILE* file = FT02_StorageOpenWriteFile(FT02_WIFI_PROFILE_TEMP, true);
    if(file == nullptr) return false;

    FT02WifiProfileFileHeader header = {};
    header.magic = FT02_WIFI_PROFILE_MAGIC;
    header.version = FT02_WIFI_PROFILE_VERSION_A2;
    header.count = g_profileCount;

    bool ok = fwrite(&header, 1, sizeof(header), file) == sizeof(header);
    for(uint8_t i = 0; ok && i < g_profileCount; ++i)
    {
        ok = fwrite(&g_profiles[i], 1, sizeof(g_profiles[i]), file) == sizeof(g_profiles[i]);
    }
    if(ok) ok = FT02_StorageSyncFile(file);
    fclose(file);

    if(!ok)
    {
        (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_TEMP);
        return false;
    }

    (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_BACKUP);
    const bool hadProfile = FT02_StorageFileExists(FT02_WIFI_PROFILE_PATH);
    if(hadProfile && !FT02_StorageRenameFile(FT02_WIFI_PROFILE_PATH, FT02_WIFI_PROFILE_BACKUP))
    {
        (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_TEMP);
        return false;
    }
    if(!FT02_StorageRenameFile(FT02_WIFI_PROFILE_TEMP, FT02_WIFI_PROFILE_PATH))
    {
        if(hadProfile) (void)FT02_StorageRenameFile(FT02_WIFI_PROFILE_BACKUP, FT02_WIFI_PROFILE_PATH);
        (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_TEMP);
        return false;
    }
    (void)FT02_StorageDeleteFile(FT02_WIFI_PROFILE_BACKUP);
    return true;
}

uint8_t FT02_WifiProfileStoreCount()
{
    if(!g_loaded) FT02_WifiProfileStoreLoad();
    return g_profileCount;
}

bool FT02_WifiProfileStoreGet(uint8_t index, FT02WifiProfile& profile)
{
    if(!g_loaded) FT02_WifiProfileStoreLoad();
    if(index >= g_profileCount) return false;
    profile = g_profiles[index];
    sanitizeProfile(profile);
    return true;
}

int FT02_WifiProfileStoreFind(const char* ssid)
{
    if(ssid == nullptr || ssid[0] == '\0') return -1;
    if(!g_loaded) FT02_WifiProfileStoreLoad();
    for(uint8_t i = 0; i < g_profileCount; ++i)
    {
        if(strcmp(g_profiles[i].ssid, ssid) == 0) return i;
    }
    return -1;
}

bool FT02_WifiProfileStoreUpsert(const char* ssid, const char* password, bool connectedNow)
{
    if(ssid == nullptr || ssid[0] == '\0') return false;
    if(!g_loaded) FT02_WifiProfileStoreLoad();

    int index = FT02_WifiProfileStoreFind(ssid);
    if(index < 0)
    {
        if(g_profileCount >= FT02_WIFI_PROFILE_MAX_COUNT)
        {
            FT02_WifiProfileStoreSort();
            index = FT02_WIFI_PROFILE_MAX_COUNT - 1;
        }
        else
        {
            index = g_profileCount++;
        }
        g_profiles[index].priority = 100;
        g_profiles[index].autoConnect = true;
    }

    snprintf(g_profiles[index].ssid, sizeof(g_profiles[index].ssid), "%s", ssid);
    snprintf(g_profiles[index].password, sizeof(g_profiles[index].password), "%s", password != nullptr ? password : "");
    g_profiles[index].lastConnected = nowSeconds();
    if(connectedNow) g_profiles[index].lastSuccess = g_profiles[index].lastConnected;
    g_profiles[index].autoConnect = true;
    return FT02_WifiProfileStoreSave();
}

bool FT02_WifiProfileStoreDelete(const char* ssid)
{
    const int index = FT02_WifiProfileStoreFind(ssid);
    if(index < 0) return false;
    for(uint8_t i = static_cast<uint8_t>(index); i + 1 < g_profileCount; ++i)
    {
        g_profiles[i] = g_profiles[i + 1];
    }
    --g_profileCount;
    return FT02_WifiProfileStoreSave();
}
