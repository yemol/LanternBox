#include "FT02_Storage.h"
#include "FT02_StorageConfig.h"

#include <SPI.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "diskio.h"
#include "diskio_impl.h"
#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_vfs_fat.h"
#include "ff.h"
}

// FT-02 Storage v2.0 / firmware v2.34
// Frozen production backend:
//   SD: FSPI / SPI Mode 3 / 40 MHz
//   Pins: SCK42, MOSI2, MISO1, CS41
//   I/O: CMD18 read, ACMD23 + CMD25 write, 16 KiB chunks
//   Mount point: /sdcard
// The display is assigned to HSPI in main.cpp, so storage and e-paper never
// share a hardware SPI controller.

SPIClass g_ft02SdSpi(FSPI);

class Mode3SdCard {
public:
    bool begin(uint32_t operatingHz) {
        operatingHz_ = operatingHz;
        initialized_ = false;
        highCapacity_ = false;
        sectorCount_ = 0;

        if (!busStarted_) {
            if (!g_ft02SdSpi.begin(FT02StorageConfig::SD_SCK, FT02StorageConfig::SD_MISO,
                             FT02StorageConfig::SD_MOSI, FT02StorageConfig::SD_CS)) {
                Serial.println("[SDM3] g_ft02SdSpi.begin failed");
                return false;
            }
            busStarted_ = true;
        }

        pinMode(FT02StorageConfig::SD_CS, OUTPUT);
        digitalWrite(FT02StorageConfig::SD_CS, HIGH);

        sendPowerUpClocks();

        uint8_t r1 = 0xFF;
        bool idle = false;
        for (uint8_t attempt = 0; attempt < 5; ++attempt) {
            if (command(0, 0, 0x95, r1, nullptr, 0, true) && r1 == 0x01) {
                idle = true;
                break;
            }
            delay(20);
        }
        if (!idle) {
            Serial.printf("[SDM3] CMD0 failed, last R1=0x%02X\n", r1);
            return false;
        }

        uint8_t r7[4] = {0};
        const bool cmd8Transport = command(8, 0x000001AAUL, 0x87,
                                           r1, r7, sizeof(r7), false);
        const bool isV2 = cmd8Transport && r1 == 0x01 &&
                          r7[2] == 0x01 && r7[3] == 0xAA;
        if (!isV2 && r1 != 0x05 && r1 != 0x01) {
            Serial.printf("[SDM3] CMD8 unexpected R1=0x%02X\n", r1);
            return false;
        }

        const uint32_t acmd41Arg = isV2 ? 0x40000000UL : 0UL;
        const uint32_t start = millis();
        do {
            uint8_t appR1 = 0xFF;
            if (!command(55, 0, 0x01, appR1, nullptr, 0, false)) {
                return false;
            }
            if (appR1 > 0x01) {
                Serial.printf("[SDM3] CMD55 failed R1=0x%02X\n", appR1);
                return false;
            }
            if (!command(41, acmd41Arg, 0x01, r1, nullptr, 0, false)) {
                return false;
            }
            if (r1 == 0x00) {
                break;
            }
            delay(10);
        } while (millis() - start < FT02StorageConfig::INIT_TIMEOUT_MS);

        if (r1 != 0x00) {
            Serial.printf("[SDM3] ACMD41 timeout, R1=0x%02X\n", r1);
            return false;
        }

        uint8_t ocr[4] = {0};
        if (!command(58, 0, 0x01, r1, ocr, sizeof(ocr), false) || r1 != 0x00) {
            Serial.printf("[SDM3] CMD58 failed R1=0x%02X\n", r1);
            return false;
        }
        highCapacity_ = isV2 && ((ocr[0] & 0x40U) != 0U);

        if (!highCapacity_) {
            if (!command(16, 512, 0x01, r1, nullptr, 0, false) || r1 != 0x00) {
                Serial.printf("[SDM3] CMD16 failed R1=0x%02X\n", r1);
                return false;
            }
        }

        // Disable optional CRC checking. CMD0 and CMD8 CRC values above remain
        // valid and sufficient for entering SPI mode.
        (void)command(59, 0, 0x01, r1, nullptr, 0, false);

        uint8_t csd[16] = {0};
        if (!readRegister(9, csd, sizeof(csd))) {
            Serial.println("[SDM3] CSD read failed");
            return false;
        }
        sectorCount_ = parseSectorCount(csd);
        if (sectorCount_ == 0) {
            Serial.println("[SDM3] Invalid CSD capacity");
            return false;
        }

        initialized_ = true;
        return true;
    }

    bool readBlock(uint32_t sector, uint8_t* dst) {
        if (!initialized_ || dst == nullptr) {
            return false;
        }

        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint32_t address = highCapacity_ ? sector : sector * 512UL;
        const uint8_t r1 = sendCommandSelected(17, address, 0x01);
        bool ok = false;
        if (r1 == 0x00) {
            ok = readDataSelected(dst, 512);
        }
        deselectCard();
        endBusTransaction();
        return ok;
    }

    bool writeBlock(uint32_t sector, const uint8_t* src) {
        if (!initialized_ || src == nullptr) {
            return false;
        }

        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint32_t address = highCapacity_ ? sector : sector * 512UL;
        const uint8_t r1 = sendCommandSelected(24, address, 0x01);
        bool ok = false;
        if (r1 == 0x00 && waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS)) {
            g_ft02SdSpi.transfer(0xFE);
            g_ft02SdSpi.writeBytes(const_cast<uint8_t*>(src), 512);
            g_ft02SdSpi.transfer(0xFF);
            g_ft02SdSpi.transfer(0xFF);
            const uint8_t response = g_ft02SdSpi.transfer(0xFF);
            ok = ((response & 0x1FU) == 0x05U) &&
                 waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS);
        }
        deselectCard();
        endBusTransaction();
        return ok;
    }

    bool readBlocks(uint32_t sector, uint8_t* dst, uint32_t count) {
        if (!initialized_ || dst == nullptr || count == 0) {
            return false;
        }
        if (count == 1) {
            return readBlock(sector, dst);
        }

        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint32_t address = highCapacity_ ? sector : sector * 512UL;
        const uint8_t r1 = sendCommandSelected(18, address, 0x01);
        bool ok = r1 == 0x00;

        if (ok) {
            for (uint32_t block = 0; block < count; ++block) {
                if (!readDataSelected(dst + static_cast<size_t>(block) * 512U,
                                      512U)) {
                    ok = false;
                    break;
                }
                if ((block & 0x0FU) == 0U) {
                    yield();
                }
            }
        }

        // CMD12 terminates CMD18. SPI mode inserts one mandatory stuff byte
        // before the R1 response, so it needs a dedicated decoder.
        const uint8_t stopR1 = sendStopTransmissionSelected();
        const bool stopped = stopR1 == 0x00 &&
                             waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS);
        deselectCard();
        endBusTransaction();
        return ok && stopped;
    }

    bool writeBlocks(uint32_t sector, const uint8_t* src, uint32_t count) {
        if (!initialized_ || src == nullptr || count == 0) {
            return false;
        }
        if (count == 1) {
            return writeBlock(sector, src);
        }

        // ACMD23 is optional in SPI mode. A successful pre-erase hint can
        // reduce internal flash-management stalls during CMD25.
        (void)setPreEraseCount(count);

        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint32_t address = highCapacity_ ? sector : sector * 512UL;
        const uint8_t r1 = sendCommandSelected(25, address, 0x01);
        bool ok = r1 == 0x00;

        if (ok) {
            for (uint32_t block = 0; block < count; ++block) {
                if (!waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS)) {
                    ok = false;
                    break;
                }

                g_ft02SdSpi.transfer(0xFF);
                g_ft02SdSpi.transfer(0xFC);  // multi-block data token
                g_ft02SdSpi.writeBytes(
                    const_cast<uint8_t*>(
                        src + static_cast<size_t>(block) * 512U),
                    512U);
                g_ft02SdSpi.transfer(0xFF);  // dummy CRC
                g_ft02SdSpi.transfer(0xFF);

                const uint8_t response = g_ft02SdSpi.transfer(0xFF);
                if ((response & 0x1FU) != 0x05U) {
                    ok = false;
                    break;
                }
                if ((block & 0x07U) == 0U) {
                    yield();
                }
            }
        }

        // Stop token is required even when a block failed, so the next command
        // starts from a known state.
        if (waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS)) {
            g_ft02SdSpi.transfer(0xFD);
        } else {
            ok = false;
            g_ft02SdSpi.transfer(0xFD);
        }
        const bool stopped = waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS);
        deselectCard();
        endBusTransaction();
        return ok && stopped;
    }

    bool sync() {
        if (!initialized_) {
            return false;
        }
        beginBusTransaction();
        const bool selected = selectCard();
        const bool ready = selected && waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS);
        if (selected) {
            deselectCard();
        }
        endBusTransaction();
        return ready;
    }

    uint32_t sectorCount() const { return sectorCount_; }
    uint32_t operatingHz() const { return operatingHz_; }
    bool highCapacity() const { return highCapacity_; }
    bool initialized() const { return initialized_; }

private:
    bool busStarted_ = false;
    bool initialized_ = false;
    bool highCapacity_ = false;
    uint32_t sectorCount_ = 0;
    uint32_t operatingHz_ = FT02StorageConfig::RAW_BASELINE_HZ;

    void beginBusTransaction(uint32_t hz = 0) {
        const uint32_t frequency = hz == 0 ? operatingHz_ : hz;
        g_ft02SdSpi.beginTransaction(SPISettings(frequency, MSBFIRST, SPI_MODE3));
    }

    void endBusTransaction() {
        g_ft02SdSpi.endTransaction();
    }

    void sendPowerUpClocks() {
        digitalWrite(FT02StorageConfig::SD_CS, HIGH);
        beginBusTransaction(FT02StorageConfig::SD_INIT_HZ);
        for (uint8_t i = 0; i < 20; ++i) {
            g_ft02SdSpi.transfer(0xFF);
        }
        endBusTransaction();
        delay(2);
    }

    bool waitReadySelected(uint32_t timeoutMs) {
        const uint32_t start = millis();
        do {
            if (g_ft02SdSpi.transfer(0xFF) == 0xFF) {
                return true;
            }
            yield();
        } while (millis() - start < timeoutMs);
        return false;
    }

    bool selectCard() {
        digitalWrite(FT02StorageConfig::SD_CS, LOW);
        g_ft02SdSpi.transfer(0xFF);
        if (!waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS)) {
            digitalWrite(FT02StorageConfig::SD_CS, HIGH);
            g_ft02SdSpi.transfer(0xFF);
            return false;
        }
        return true;
    }

    void deselectCard() {
        digitalWrite(FT02StorageConfig::SD_CS, HIGH);
        g_ft02SdSpi.transfer(0xFF);
    }

    static uint8_t commandCrc(uint8_t commandIndex) {
        if (commandIndex == 0) return 0x95;
        if (commandIndex == 8) return 0x87;
        return 0x01;
    }

    uint8_t sendCommandSelected(uint8_t commandIndex,
                                uint32_t argument,
                                uint8_t crc) {
        g_ft02SdSpi.transfer(static_cast<uint8_t>(0x40U | commandIndex));
        g_ft02SdSpi.transfer(static_cast<uint8_t>(argument >> 24U));
        g_ft02SdSpi.transfer(static_cast<uint8_t>(argument >> 16U));
        g_ft02SdSpi.transfer(static_cast<uint8_t>(argument >> 8U));
        g_ft02SdSpi.transfer(static_cast<uint8_t>(argument));
        g_ft02SdSpi.transfer(crc);

        for (uint8_t i = 0; i < 16; ++i) {
            const uint8_t value = g_ft02SdSpi.transfer(0xFF);
            if ((value & 0x80U) == 0U) {
                return value;
            }
        }
        return 0xFF;
    }

    uint8_t sendStopTransmissionSelected() {
        g_ft02SdSpi.transfer(static_cast<uint8_t>(0x40U | 12U));
        g_ft02SdSpi.transfer(0x00);
        g_ft02SdSpi.transfer(0x00);
        g_ft02SdSpi.transfer(0x00);
        g_ft02SdSpi.transfer(0x00);
        g_ft02SdSpi.transfer(0x01);

        (void)g_ft02SdSpi.transfer(0xFF);  // mandatory CMD12 stuff byte
        for (uint8_t i = 0; i < 16; ++i) {
            const uint8_t value = g_ft02SdSpi.transfer(0xFF);
            if ((value & 0x80U) == 0U) {
                return value;
            }
        }
        return 0xFF;
    }

    bool setPreEraseCount(uint32_t count) {
        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint8_t appR1 = sendCommandSelected(55, 0, 0x01);
        const uint8_t eraseR1 = appR1 <= 0x01
            ? sendCommandSelected(23, count, 0x01)
            : 0xFF;
        deselectCard();
        endBusTransaction();
        return eraseR1 == 0x00;
    }

    bool command(uint8_t commandIndex,
                 uint32_t argument,
                 uint8_t crc,
                 uint8_t& r1,
                 uint8_t* extra,
                 size_t extraLength,
                 bool skipInitialReadyWait) {
        beginBusTransaction(FT02StorageConfig::SD_INIT_HZ);
        digitalWrite(FT02StorageConfig::SD_CS, LOW);
        g_ft02SdSpi.transfer(0xFF);

        if (!skipInitialReadyWait &&
            !waitReadySelected(FT02StorageConfig::COMMAND_TIMEOUT_MS)) {
            deselectCard();
            endBusTransaction();
            r1 = 0xFF;
            return false;
        }

        r1 = sendCommandSelected(commandIndex, argument,
                                 crc == 0 ? commandCrc(commandIndex) : crc);
        if (r1 != 0xFF && extra != nullptr) {
            for (size_t i = 0; i < extraLength; ++i) {
                extra[i] = g_ft02SdSpi.transfer(0xFF);
            }
        }

        deselectCard();
        endBusTransaction();
        return r1 != 0xFF;
    }

    bool readDataSelected(uint8_t* dst, size_t length) {
        const uint32_t start = millis();
        uint8_t token = 0xFF;
        do {
            token = g_ft02SdSpi.transfer(0xFF);
            if (token == 0xFE) {
                break;
            }
            if (token != 0xFF) {
                return false;
            }
            yield();
        } while (millis() - start < FT02StorageConfig::COMMAND_TIMEOUT_MS);

        if (token != 0xFE) {
            return false;
        }

        g_ft02SdSpi.transferBytes(nullptr, dst, length);
        g_ft02SdSpi.transfer(0xFF);
        g_ft02SdSpi.transfer(0xFF);
        return true;
    }

    bool readRegister(uint8_t commandIndex, uint8_t* dst, size_t length) {
        beginBusTransaction();
        if (!selectCard()) {
            endBusTransaction();
            return false;
        }

        const uint8_t r1 = sendCommandSelected(commandIndex, 0,
                                                commandCrc(commandIndex));
        const bool ok = r1 == 0x00 && readDataSelected(dst, length);
        deselectCard();
        endBusTransaction();
        return ok;
    }

    static uint32_t parseSectorCount(const uint8_t* csd) {
        if ((csd[0] >> 6U) == 0x01U) {
            const uint32_t cSize =
                (static_cast<uint32_t>(csd[7] & 0x3FU) << 16U) |
                (static_cast<uint32_t>(csd[8]) << 8U) |
                static_cast<uint32_t>(csd[9]);
            return (cSize + 1UL) << 10U;
        }

        uint32_t size =
            (static_cast<uint32_t>(csd[6] & 0x03U) << 10U) |
            (static_cast<uint32_t>(csd[7]) << 2U) |
            ((static_cast<uint32_t>(csd[8]) & 0xC0U) >> 6U);
        size += 1UL;
        size <<= (((csd[9] & 0x03U) << 1U) |
                  ((csd[10] & 0x80U) >> 7U)) + 2U;
        size <<= csd[5] & 0x0FU;
        return size >> 9U;
    }
};

static Mode3SdCard sdCard;

enum class DiskTransferMode : uint8_t {
    SingleBlock,
    MultiBlock,
};

static DiskTransferMode diskTransferMode = DiskTransferMode::MultiBlock;
static uint32_t diskMultiChunkBlocks = FT02StorageConfig::TRANSFER_CHUNK_BLOCKS;

static void configureDiskTransfer(DiskTransferMode mode,
                                  uint32_t multiChunkBlocks) {
    diskTransferMode = mode;
    if (multiChunkBlocks == 0U) {
        multiChunkBlocks = 1U;
    }
    if (multiChunkBlocks > FT02StorageConfig::MAX_MULTI_BLOCKS) {
        multiChunkBlocks = FT02StorageConfig::MAX_MULTI_BLOCKS;
    }
    diskMultiChunkBlocks = multiChunkBlocks;
}

static const char* transferModeName(DiskTransferMode mode) {
    return mode == DiskTransferMode::MultiBlock ? "MULTI" : "SINGLE";
}

// ------------------------------- FatFs bridge -------------------------------
static uint8_t fatDrive = 0xFF;
static bool fatRegistered = false;
static bool fatMounted = false;
static bool fatVfsRegistered = false;
static FATFS* fatFs = nullptr;
static DSTATUS fatStatus = STA_NOINIT;

static DSTATUS mode3DiskInitialize(uint8_t pdrv) {
    if (pdrv != fatDrive || !sdCard.initialized()) {
        return STA_NOINIT;
    }
    fatStatus = 0;
    return fatStatus;
}

static DSTATUS mode3DiskStatus(uint8_t pdrv) {
    if (pdrv != fatDrive || !sdCard.initialized()) {
        return STA_NOINIT;
    }
    return fatStatus;
}

static DRESULT mode3DiskRead(uint8_t pdrv, uint8_t* buffer,
                             DWORD sector, UINT count) {
    if (pdrv != fatDrive || buffer == nullptr || count == 0 ||
        (fatStatus & STA_NOINIT)) {
        return RES_NOTRDY;
    }

    UINT completed = 0;
    while (completed < count) {
        const UINT remaining = count - completed;
        UINT chunk = 1;
        if (diskTransferMode == DiskTransferMode::MultiBlock) {
            chunk = static_cast<UINT>(diskMultiChunkBlocks);
            if (chunk > remaining) chunk = remaining;
        }

        const uint32_t currentSector =
            static_cast<uint32_t>(sector + completed);
        uint8_t* currentBuffer =
            buffer + static_cast<size_t>(completed) * 512U;
        const bool ok = chunk > 1
            ? sdCard.readBlocks(currentSector, currentBuffer, chunk)
            : sdCard.readBlock(currentSector, currentBuffer);
        if (!ok) {
            return RES_ERROR;
        }
        completed += chunk;
        yield();
    }
    return RES_OK;
}

static DRESULT mode3DiskWrite(uint8_t pdrv, const uint8_t* buffer,
                              DWORD sector, UINT count) {
    if (pdrv != fatDrive || buffer == nullptr || count == 0 ||
        (fatStatus & STA_NOINIT)) {
        return RES_NOTRDY;
    }

    UINT completed = 0;
    while (completed < count) {
        const UINT remaining = count - completed;
        UINT chunk = 1;
        if (diskTransferMode == DiskTransferMode::MultiBlock) {
            chunk = static_cast<UINT>(diskMultiChunkBlocks);
            if (chunk > remaining) chunk = remaining;
        }

        const uint32_t currentSector =
            static_cast<uint32_t>(sector + completed);
        const uint8_t* currentBuffer =
            buffer + static_cast<size_t>(completed) * 512U;
        const bool ok = chunk > 1
            ? sdCard.writeBlocks(currentSector, currentBuffer, chunk)
            : sdCard.writeBlock(currentSector, currentBuffer);
        if (!ok) {
            return RES_ERROR;
        }
        completed += chunk;
        yield();
    }
    return RES_OK;
}

static DRESULT mode3DiskIoctl(uint8_t pdrv, uint8_t command, void* buffer) {
    if (pdrv != fatDrive) {
        return RES_PARERR;
    }

    switch (command) {
        case CTRL_SYNC:
            return sdCard.sync() ? RES_OK : RES_ERROR;
        case GET_SECTOR_COUNT:
            if (buffer == nullptr) return RES_PARERR;
            *static_cast<DWORD*>(buffer) = sdCard.sectorCount();
            return RES_OK;
        case GET_SECTOR_SIZE:
            if (buffer == nullptr) return RES_PARERR;
            *static_cast<WORD*>(buffer) = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            if (buffer == nullptr) return RES_PARERR;
            *static_cast<DWORD*>(buffer) = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

static const ff_diskio_impl_t mode3DiskImpl = {
    .init = &mode3DiskInitialize,
    .status = &mode3DiskStatus,
    .read = &mode3DiskRead,
    .write = &mode3DiskWrite,
    .ioctl = &mode3DiskIoctl,
};

static void unmountFat() {
    if (fatMounted && fatDrive != 0xFF) {
        char drive[3] = {
            static_cast<char>('0' + fatDrive), ':', '\0'
        };
        f_mount(nullptr, drive, 0);
        fatMounted = false;
    }

    if (fatVfsRegistered) {
        esp_vfs_fat_unregister_path(FT02StorageConfig::MOUNT_POINT);
        fatVfsRegistered = false;
        fatFs = nullptr;
    }

    if (fatRegistered && fatDrive != 0xFF) {
        ff_diskio_register(fatDrive, nullptr);
        fatRegistered = false;
    }

    fatDrive = 0xFF;
    fatStatus = STA_NOINIT;
}

static bool mountFat() {
    unmountFat();

    if (ff_diskio_get_drive(&fatDrive) != ESP_OK || fatDrive == 0xFF) {
        Serial.println("[FAT] No free FatFs drive");
        return false;
    }

    ff_diskio_register(fatDrive, &mode3DiskImpl);
    fatRegistered = true;
    fatStatus = 0;

    char drive[3] = {
        static_cast<char>('0' + fatDrive), ':', '\0'
    };

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
    const esp_err_t registerResult = esp_vfs_fat_register(
        FT02StorageConfig::MOUNT_POINT, drive, FT02StorageConfig::MAX_OPEN_FILES, &fatFs);
#else
    esp_vfs_fat_conf_t conf = {
        .base_path = FT02StorageConfig::MOUNT_POINT,
        .fat_drive = drive,
        .max_files = FT02StorageConfig::MAX_OPEN_FILES,
    };
    const esp_err_t registerResult = esp_vfs_fat_register(&conf, &fatFs);
#endif

    if (registerResult != ESP_OK) {
        Serial.printf("[FAT] esp_vfs_fat_register failed: 0x%X\n",
                      static_cast<unsigned>(registerResult));
        unmountFat();
        return false;
    }
    fatVfsRegistered = true;

    const FRESULT mountResult = f_mount(fatFs, drive, 1);
    if (mountResult != FR_OK) {
        Serial.printf("[FAT] f_mount failed: %u\n",
                      static_cast<unsigned>(mountResult));
        unmountFat();
        return false;
    }

    fatMounted = true;
    return true;
}


static FT02StorageState g_storageState = FT02_STORAGE_STATE_NOT_STARTED;
static FT02StorageError g_storageError = FT02_STORAGE_ERROR_NONE;
static uint8_t g_cardType = 0;
static unsigned long g_cardSizeMB = 0;
static unsigned long g_totalMB = 0;
static unsigned long g_usedMB = 0;

static bool FT02_BuildSdPath(const char* path, char* output, size_t outputSize)
{
    if(path == nullptr || output == nullptr || outputSize == 0) return false;

    int written = 0;
    if(path[0] == '/')
    {
        written = snprintf(output, outputSize, "%s%s", FT02StorageConfig::MOUNT_POINT, path);
    }
    else
    {
        written = snprintf(output, outputSize, "%s/%s", FT02StorageConfig::MOUNT_POINT, path);
    }
    return written > 0 && static_cast<size_t>(written) < outputSize;
}

static bool FT02_EnsureParentDirs(const char* fullPath)
{
    if(fullPath == nullptr) return false;

    char path[160];
    const size_t length = strnlen(fullPath, sizeof(path));
    if(length == 0 || length >= sizeof(path)) return false;
    memcpy(path, fullPath, length + 1);

    for(char* cursor = path + strlen(FT02StorageConfig::MOUNT_POINT) + 1;
        *cursor != 0; ++cursor)
    {
        if(*cursor != '/') continue;
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
    return static_cast<unsigned long>(bytes / (1024ULL * 1024ULL));
}

static void FT02_UpdateFsUsage()
{
    if(fatDrive == 0xFF) return;

    char drive[3] = { static_cast<char>('0' + fatDrive), ':', '\0' };
    DWORD freeClusters = 0;
    FATFS* currentFs = nullptr;
    if(f_getfree(drive, &freeClusters, &currentFs) != FR_OK || currentFs == nullptr)
    {
        return;
    }

    const uint64_t bytesPerCluster = static_cast<uint64_t>(currentFs->csize) * 512ULL;
    const uint64_t totalBytes = static_cast<uint64_t>(currentFs->n_fatent - 2U) * bytesPerCluster;
    const uint64_t freeBytes = static_cast<uint64_t>(freeClusters) * bytesPerCluster;
    g_totalMB = FT02_ToMB(totalBytes);
    g_usedMB = FT02_ToMB(totalBytes - freeBytes);
}

static const char* FT02_CardTypeName(uint8_t cardType)
{
    if(cardType == 2) return "SDSC";
    if(cardType == 3) return "SDHC/SDXC";
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
    unmountFat();

    pinMode(FT02StorageConfig::SD_CS, OUTPUT);
    digitalWrite(FT02StorageConfig::SD_CS, HIGH);
    delay(10);

    Serial.println("FT02 Storage v2.0 begin");
    Serial.print("Profile: ");
    Serial.println(FT02StorageConfig::PROFILE_NAME);
    Serial.printf(
        "Pins: SCK=%d MOSI=%d MISO=%d CS=%d\n",
        FT02StorageConfig::SD_SCK,
        FT02StorageConfig::SD_MOSI,
        FT02StorageConfig::SD_MISO,
        FT02StorageConfig::SD_CS
    );
    Serial.printf(
        "SD FSPI Mode3 @ %lu kHz, multi-block chunk=%lu KiB\n",
        static_cast<unsigned long>(FT02StorageConfig::FREQUENCY_KHZ),
        static_cast<unsigned long>(FT02StorageConfig::TRANSFER_CHUNK_BLOCKS / 2U)
    );

    configureDiskTransfer(
        DiskTransferMode::MultiBlock,
        FT02StorageConfig::TRANSFER_CHUNK_BLOCKS
    );

    if(!sdCard.begin(FT02StorageConfig::SD_FREQUENCY_HZ))
    {
        g_storageState = FT02_STORAGE_STATE_ERROR;
        g_storageError = FT02_STORAGE_ERROR_MOUNT_FAILED;
        Serial.println("SD raw Mode3 initialization failed");
        return;
    }

    if(!mountFat())
    {
        g_storageState = FT02_STORAGE_STATE_ERROR;
        g_storageError = FT02_STORAGE_ERROR_MOUNT_FAILED;
        Serial.println("SD FAT mount failed");
        return;
    }

    g_cardType = sdCard.highCapacity() ? 3 : 2;
    g_cardSizeMB = FT02_ToMB(
        static_cast<uint64_t>(sdCard.sectorCount()) * 512ULL
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

FT02StorageState FT02_StorageStateCurrent() { return g_storageState; }
FT02StorageError FT02_StorageLastError() { return g_storageError; }
bool FT02_StorageIsReady() { return g_storageState == FT02_STORAGE_STATE_READY; }
unsigned long FT02_StorageCardSizeMB() { return g_cardSizeMB; }
unsigned long FT02_StorageTotalMB() { return g_totalMB; }
unsigned long FT02_StorageUsedMB() { return g_usedMB; }

unsigned long FT02_StorageFreeMB()
{
    if(g_totalMB <= g_usedMB) return 0;
    return g_totalMB - g_usedMB;
}

const char* FT02_StorageCardTypeText() { return FT02_CardTypeName(g_cardType); }

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

const char* FT02_StorageProfileText() { return FT02StorageConfig::PROFILE_NAME; }
uint32_t FT02_StorageFrequencyKHz() { return FT02StorageConfig::FREQUENCY_KHZ; }
int FT02_StorageClockPin() { return FT02StorageConfig::SD_SCK; }
int FT02_StorageCommandPin() { return FT02StorageConfig::SD_MOSI; }
int FT02_StorageD0Pin() { return FT02StorageConfig::SD_MISO; }

bool FT02_StorageFileExists(const char* path)
{
    if(!FT02_StorageIsReady() || path == nullptr) return false;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return false;
    struct stat info {};
    return stat(fullPath, &info) == 0 && S_ISREG(info.st_mode);
}

bool FT02_StorageFileSize(const char* path, uint64_t& bytes)
{
    bytes = 0;
    if(!FT02_StorageIsReady() || path == nullptr) return false;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return false;
    struct stat info {};
    if(stat(fullPath, &info) != 0 || !S_ISREG(info.st_mode)) return false;
    bytes = static_cast<uint64_t>(info.st_size);
    return true;
}

FILE* FT02_StorageOpenReadFile(const char* path)
{
    if(!FT02_StorageIsReady() || path == nullptr) return nullptr;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return nullptr;
    return fopen(fullPath, "rb");
}

FILE* FT02_StorageOpenWriteFile(const char* path, bool truncate)
{
    if(!FT02_StorageIsReady() || path == nullptr) return nullptr;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return nullptr;
    if(!FT02_EnsureParentDirs(fullPath)) return nullptr;
    return fopen(fullPath, truncate ? "wb+" : "ab+");
}

bool FT02_StorageSyncFile(FILE* file)
{
    if(file == nullptr) return false;
    if(fflush(file) != 0) return false;
    const bool ok = fsync(fileno(file)) == 0;
    if(ok) FT02_UpdateFsUsage();
    return ok;
}

bool FT02_StorageDeleteFile(const char* path)
{
    if(!FT02_StorageIsReady() || path == nullptr) return false;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return false;
    const bool ok = unlink(fullPath) == 0 || errno == ENOENT;
    if(ok) FT02_UpdateFsUsage();
    return ok;
}

bool FT02_StorageRenameFile(const char* fromPath, const char* toPath)
{
    if(!FT02_StorageIsReady() || fromPath == nullptr || toPath == nullptr) return false;
    char fromFullPath[160];
    char toFullPath[160];
    if(!FT02_BuildSdPath(fromPath, fromFullPath, sizeof(fromFullPath)) ||
       !FT02_BuildSdPath(toPath, toFullPath, sizeof(toFullPath))) return false;
    if(!FT02_EnsureParentDirs(toFullPath)) return false;
    return rename(fromFullPath, toFullPath) == 0;
}

bool FT02_StorageAppendLine(const char* path, const char* line)
{
    if(!FT02_StorageIsReady() || path == nullptr || line == nullptr) return false;
    char fullPath[160];
    if(!FT02_BuildSdPath(path, fullPath, sizeof(fullPath))) return false;
    if(!FT02_EnsureParentDirs(fullPath)) return false;

    FILE* file = fopen(fullPath, "ab");
    if(file == nullptr) return false;
    const size_t length = strlen(line);
    bool ok = fwrite(line, 1, length, file) == length;
    ok = fwrite("\n", 1, 1, file) == 1 && ok;
    ok = fflush(file) == 0 && ok;
    ok = fsync(fileno(file)) == 0 && ok;
    ok = fclose(file) == 0 && ok;
    if(ok) FT02_UpdateFsUsage();
    return ok;
}
