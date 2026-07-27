#include "FT02_Storage.h"
#include "FT02_StorageConfig.h"

#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <ff.h>
#include <sdmmc_cmd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// FT-02 Storage v1.0 / firmware v1.98
// Stable production profile:
//   SDMMC 1-bit @ 5 MHz
//   CLK -> GPIO35
//   CMD -> GPIO2
//   D0  -> GPIO1
// Uses the native ESP-IDF VFS SDMMC backend validated by Codex.

static FT02StorageState g_storageState = FT02_STORAGE_STATE_NOT_STARTED;
static FT02StorageError g_storageError = FT02_STORAGE_ERROR_NONE;

static const char* FT02_SD_MOUNT_POINT = "/sdcard";

static sdmmc_card_t* g_sdCard = nullptr;
static uint8_t g_cardType = 0;
static unsigned long g_cardSizeMB = 0;
static unsigned long g_totalMB = 0;
static unsigned long g_usedMB = 0;

static bool FT02_BuildSdPath(
    const char* path,
    char* output,
    size_t outputSize
)
{
    if(path == nullptr || output == nullptr || outputSize == 0)
    {
        return false;
    }

    int written = 0;

    if(path[0] == '/')
    {
        written = snprintf(
            output,
            outputSize,
            "%s%s",
            FT02_SD_MOUNT_POINT,
            path
        );
    }
    else
    {
        written = snprintf(
            output,
            outputSize,
            "%s/%s",
            FT02_SD_MOUNT_POINT,
            path
        );
    }

    return written > 0 && (size_t)written < outputSize;
}

static bool FT02_EnsureParentDirs(const char* fullPath)
{
    if(fullPath == nullptr)
    {
        return false;
    }

    char path[160];
    size_t length = strnlen(fullPath, sizeof(path));

    if(length == 0 || length >= sizeof(path))
    {
        return false;
    }

    memcpy(path, fullPath, length + 1);

    for(
        char* cursor = path + strlen(FT02_SD_MOUNT_POINT) + 1;
        *cursor != 0;
        cursor++
    )
    {
        if(*cursor != '/')
        {
            continue;
        }

        *cursor = 0;

        if(mkdir(path, 0775) != 0 && errno != EEXIST)
        {
            *cursor = '/';
            return false;
        }

        *cursor = '/';
    }

    return true;
}

static unsigned long FT02_ToMB(uint64_t bytes)
{
    return (unsigned long)(bytes / (1024ULL * 1024ULL));
}

static void FT02_UpdateFsUsage()
{
    DWORD freeClusters = 0;
    FATFS* fatfs = nullptr;

    if(f_getfree(FT02_SD_MOUNT_POINT, &freeClusters, &fatfs) != FR_OK)
    {
        return;
    }

    if(fatfs == nullptr)
    {
        return;
    }

    uint64_t bytesPerCluster = (uint64_t)fatfs->csize * 512ULL;
    uint64_t totalBytes =
        (uint64_t)(fatfs->n_fatent - 2U) * bytesPerCluster;
    uint64_t freeBytes = (uint64_t)freeClusters * bytesPerCluster;

    g_totalMB = FT02_ToMB(totalBytes);
    g_usedMB = FT02_ToMB(totalBytes - freeBytes);
}

static const char* FT02_CardTypeName(uint8_t cardType)
{
    if(cardType == 1) return "MMC";
    if(cardType == 2) return "SDSC";
    if(cardType == 3) return "SDHC";
    if(cardType == 0) return "NONE";
    return "UNKNOWN";
}

static void FT02_ResetStorageRuntime()
{
    g_storageState = FT02_STORAGE_STATE_SCANNING;
    g_storageError = FT02_STORAGE_ERROR_NONE;

    g_cardType = 0;
    g_cardSizeMB = 0;
    g_totalMB = 0;
    g_usedMB = 0;
}

void FT02_StorageBegin()
{
    FT02_ResetStorageRuntime();

    Serial.println("FT02 Storage v1.0 begin");
    Serial.print("Profile: ");
    Serial.println(FT02StorageConfig::PROFILE_NAME);
    Serial.print("Pins: CLK=");
    Serial.print(FT02StorageConfig::CLK);
    Serial.print(" CMD=");
    Serial.print(FT02StorageConfig::CMD);
    Serial.print(" D0=");
    Serial.println(FT02StorageConfig::D0);
    Serial.print("SDMMC 1-bit @ ");
    Serial.print(FT02StorageConfig::FREQUENCY_KHZ);
    Serial.println(" kHz");

    pinMode(FT02StorageConfig::CMD, INPUT_PULLUP);
    pinMode(FT02StorageConfig::D0, INPUT_PULLUP);
    delay(50);

    if(g_sdCard != nullptr)
    {
        esp_vfs_fat_sdcard_unmount(
            FT02_SD_MOUNT_POINT,
            g_sdCard
        );

        g_sdCard = nullptr;
        delay(50);
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_1;
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = FT02StorageConfig::FREQUENCY_KHZ;
    host.command_timeout_ms = 5000;

    sdmmc_slot_config_t slotConfig = SDMMC_SLOT_CONFIG_DEFAULT();
    slotConfig.clk = (gpio_num_t)FT02StorageConfig::CLK;
    slotConfig.cmd = (gpio_num_t)FT02StorageConfig::CMD;
    slotConfig.d0 = (gpio_num_t)FT02StorageConfig::D0;
    slotConfig.d1 = GPIO_NUM_NC;
    slotConfig.d2 = GPIO_NUM_NC;
    slotConfig.d3 = GPIO_NUM_NC;
    slotConfig.d4 = GPIO_NUM_NC;
    slotConfig.d5 = GPIO_NUM_NC;
    slotConfig.d6 = GPIO_NUM_NC;
    slotConfig.d7 = GPIO_NUM_NC;
    slotConfig.width = 1;
    slotConfig.flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_mount_config_t mountConfig = {
        .format_if_mount_failed = false,
        .max_files = FT02StorageConfig::MAX_OPEN_FILES,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t mountResult = esp_vfs_fat_sdmmc_mount(
        FT02_SD_MOUNT_POINT,
        &host,
        &slotConfig,
        &mountConfig,
        &g_sdCard
    );

    if(mountResult != ESP_OK)
    {
        g_storageState = FT02_STORAGE_STATE_ERROR;
        g_storageError = FT02_STORAGE_ERROR_MOUNT_FAILED;

        Serial.print("SD mount failed: 0x");
        Serial.println((uint32_t)mountResult, HEX);
        return;
    }

    if(g_sdCard == nullptr || !g_sdCard->is_mem)
    {
        g_storageState = FT02_STORAGE_STATE_NO_CARD;
        g_storageError = FT02_STORAGE_ERROR_CARD_NONE;
        Serial.println("SD mounted but card is unavailable");
        return;
    }

    if(g_sdCard->is_mmc)
    {
        g_cardType = 1;
    }
    else if(g_sdCard->csd.capacity > (4U * 1024U * 1024U))
    {
        g_cardType = 3;
    }
    else
    {
        g_cardType = 2;
    }

    g_cardSizeMB = FT02_ToMB(
        (uint64_t)g_sdCard->csd.capacity
            * (uint64_t)g_sdCard->csd.sector_size
    );

    FT02_UpdateFsUsage();

    g_storageState = FT02_STORAGE_STATE_READY;
    g_storageError = FT02_STORAGE_ERROR_NONE;

    Serial.print("SD ready: type=");
    Serial.print(FT02_CardTypeName(g_cardType));
    Serial.print(" cardMB=");
    Serial.print(g_cardSizeMB);
    Serial.print(" freeMB=");
    Serial.println(FT02_StorageFreeMB());
}

FT02StorageState FT02_StorageStateCurrent()
{
    return g_storageState;
}

FT02StorageError FT02_StorageLastError()
{
    return g_storageError;
}

bool FT02_StorageIsReady()
{
    return g_storageState == FT02_STORAGE_STATE_READY;
}

unsigned long FT02_StorageCardSizeMB()
{
    return g_cardSizeMB;
}

unsigned long FT02_StorageTotalMB()
{
    return g_totalMB;
}

unsigned long FT02_StorageUsedMB()
{
    return g_usedMB;
}

unsigned long FT02_StorageFreeMB()
{
    if(g_totalMB <= g_usedMB)
    {
        return 0;
    }

    return g_totalMB - g_usedMB;
}

const char* FT02_StorageCardTypeText()
{
    return FT02_CardTypeName(g_cardType);
}

const char* FT02_StorageStateText()
{
    switch(g_storageState)
    {
        case FT02_STORAGE_STATE_NOT_STARTED: return "NOT_STARTED";
        case FT02_STORAGE_STATE_SCANNING: return "SCANNING";
        case FT02_STORAGE_STATE_READY: return "READY";
        case FT02_STORAGE_STATE_NO_CARD: return "NO_CARD";
        case FT02_STORAGE_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* FT02_StorageErrorText()
{
    switch(g_storageError)
    {
        case FT02_STORAGE_ERROR_NONE: return "NONE";
        case FT02_STORAGE_ERROR_MOUNT_FAILED: return "MOUNT_FAILED";
        case FT02_STORAGE_ERROR_CARD_NONE: return "CARD_NONE";
        default: return "UNKNOWN";
    }
}

const char* FT02_StorageProfileText()
{
    return FT02StorageConfig::PROFILE_NAME;
}

uint32_t FT02_StorageFrequencyKHz()
{
    return FT02StorageConfig::FREQUENCY_KHZ;
}

int FT02_StorageClockPin()
{
    return FT02StorageConfig::CLK;
}

int FT02_StorageCommandPin()
{
    return FT02StorageConfig::CMD;
}

int FT02_StorageD0Pin()
{
    return FT02StorageConfig::D0;
}

bool FT02_StorageFileExists(const char* path)
{
    if(!FT02_StorageIsReady() || path == nullptr)
    {
        return false;
    }

    char fullPath[160];

    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath)))
    {
        return false;
    }

    struct stat info;
    return stat(fullPath, &info) == 0 && S_ISREG(info.st_mode);
}

FILE* FT02_StorageOpenReadFile(const char* path)
{
    if(!FT02_StorageIsReady() || path == nullptr)
    {
        return nullptr;
    }

    char fullPath[160];

    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath)))
    {
        return nullptr;
    }

    return fopen(fullPath, "rb");
}

bool FT02_StorageAppendLine(const char* path, const char* line)
{
    if(!FT02_StorageIsReady() || path == nullptr || line == nullptr)
    {
        return false;
    }

    char fullPath[160];

    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath)))
    {
        return false;
    }

    if(!FT02_EnsureParentDirs(fullPath))
    {
        return false;
    }

    FILE* file = fopen(fullPath, "ab");

    if(file == nullptr)
    {
        return false;
    }

    size_t length = strlen(line);
    bool ok = fwrite(line, 1, length, file) == length;
    ok = fwrite("\n", 1, 1, file) == 1 && ok;
    ok = fflush(file) == 0 && ok;
    fsync(fileno(file));
    ok = fclose(file) == 0 && ok;

    return ok;
}
