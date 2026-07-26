#include "FT02_Storage.h"

#include <FS.h>
#include <SD_MMC.h>

// FT-02 Storage v0.3 / firmware v1.90
// SDMMC 1-bit file R/W smoke based on v1.87.
//
// Locked wiring:
//   SD CLK -> GPIO15
//   SD CMD -> GPIO16
//   SD D0  -> GPIO17
//   SD CD  -> GPIO7
//   SD D3  -> NC
//   SD D1  -> NC
//   SD D2  -> NC
//
// This module uses SD_MMC in 1-bit mode.
// It does not use the Arduino SPI bus or SD.begin().

static const int FT02_SD_CLK = 15;
static const int FT02_SD_CMD = 16;
static const int FT02_SD_D0  = 17;
static const int FT02_SD_CD  = 7;

// D3 is not part of the 1-bit bus. GPIO6 is sampled only as a diagnostic.
static const int FT02_SD_D3_DIAG = 6;

static FT02StorageState g_storageState = FT02_STORAGE_STATE_NOT_STARTED;
static FT02StorageError g_storageError = FT02_STORAGE_ERROR_NONE;

static uint8_t g_cdRaw = HIGH;
static uint8_t g_d3Raw = HIGH;

static uint8_t g_cardType = CARD_NONE;
static unsigned long g_cardSizeMB = 0;
static unsigned long g_totalMB = 0;
static unsigned long g_usedMB = 0;

static FT02StorageRwResult g_rwResult = FT02_STORAGE_RW_NOT_RUN;

static int g_lastCdPollRaw = -1;
static uint32_t g_lastCdPollMillis = 0;

static unsigned long FT02_ToMB(uint64_t bytes)
{
    return (unsigned long)(bytes / (1024ULL * 1024ULL));
}

static const char* FT02_CardTypeName(uint8_t cardType)
{
    if(cardType == CARD_MMC) return "MMC";
    if(cardType == CARD_SD) return "SDSC";
    if(cardType == CARD_SDHC) return "SDHC";
    if(cardType == CARD_NONE) return "NONE";
    return "UNKNOWN";
}

static void FT02_ResetStorageRuntime()
{
    g_storageState = FT02_STORAGE_STATE_SCANNING;
    g_storageError = FT02_STORAGE_ERROR_NONE;

    g_cdRaw = HIGH;
    g_d3Raw = HIGH;
    g_cardType = CARD_NONE;

    g_cardSizeMB = 0;
    g_totalMB = 0;
    g_usedMB = 0;

    g_rwResult = FT02_STORAGE_RW_NOT_RUN;
}


static void FT02_RunFileRwSmokeTest()
{
    static const char* testPath = "/ft02_rw_test.txt";
    static const char* markerText = "FT02_RW_TEST_V1_90";

    g_rwResult = FT02_STORAGE_RW_NOT_RUN;

    Serial.print("File R/W smoke path: ");
    Serial.println(testPath);
    Serial.flush();

    if(SD_MMC.exists(testPath))
    {
        SD_MMC.remove(testPath);
        delay(20);
    }

    File writeFile = SD_MMC.open(
        testPath,
        FILE_WRITE
    );

    if(!writeFile)
    {
        g_rwResult = FT02_STORAGE_RW_WRITE_OPEN_FAILED;
        Serial.println("File R/W smoke FAILED: open write failed");
        Serial.flush();
        return;
    }

    size_t written = writeFile.println(markerText);
    writeFile.print("millis=");
    writeFile.println((unsigned long)millis());
    writeFile.close();

    if(written == 0)
    {
        g_rwResult = FT02_STORAGE_RW_WRITE_FAILED;
        Serial.println("File R/W smoke FAILED: write returned 0");
        Serial.flush();
        return;
    }

    File readFile = SD_MMC.open(
        testPath,
        FILE_READ
    );

    if(!readFile)
    {
        g_rwResult = FT02_STORAGE_RW_READ_OPEN_FAILED;
        Serial.println("File R/W smoke FAILED: open read failed");
        Serial.flush();
        return;
    }

    char buffer[96];
    size_t count = readFile.readBytes(
        buffer,
        sizeof(buffer) - 1
    );
    readFile.close();

    buffer[count] = 0;

    if(strstr(buffer, markerText) == nullptr)
    {
        g_rwResult = FT02_STORAGE_RW_READ_MISMATCH;
        Serial.print("File R/W smoke FAILED: read mismatch: ");
        Serial.println(buffer);
        Serial.flush();
        return;
    }

    g_rwResult = FT02_STORAGE_RW_OK;

    Serial.print("File R/W smoke OK: ");
    Serial.println(buffer);
    Serial.flush();
}

void FT02_StorageBegin()
{
    FT02_ResetStorageRuntime();

    Serial.println("FT02 Storage v1.90 begin: SDMMC 1-bit file R/W smoke from v1.87");
    Serial.println("Pins: CLK=15 CMD=16 D0=17 CD=7; D3/D1/D2 NC");
    Serial.println("No Arduino SPI bus, no SD.begin. File R/W smoke test enabled.");
    Serial.flush();

    pinMode(FT02_SD_CD, INPUT_PULLUP);
    pinMode(FT02_SD_D3_DIAG, INPUT_PULLUP);

    pinMode(FT02_SD_CMD, INPUT_PULLUP);
    pinMode(FT02_SD_D0, INPUT_PULLUP);

    delay(50);

    g_cdRaw = (uint8_t)digitalRead(FT02_SD_CD);
    g_d3Raw = (uint8_t)digitalRead(FT02_SD_D3_DIAG);

    Serial.print("Pre-begin raw: CD7=");
    Serial.print(g_cdRaw);
    Serial.print(" D3diag6=");
    Serial.print(g_d3Raw);
    Serial.print(" CMD16=");
    Serial.print(digitalRead(FT02_SD_CMD));
    Serial.print(" D0_17=");
    Serial.println(digitalRead(FT02_SD_D0));
    Serial.flush();

    SD_MMC.end();
    delay(50);

    bool pinsOk = SD_MMC.setPins(
        FT02_SD_CLK,
        FT02_SD_CMD,
        FT02_SD_D0,
        -1,
        -1,
        -1
    );

    if(!pinsOk)
    {
        g_storageState = FT02_STORAGE_STATE_ERROR;
        g_storageError = FT02_STORAGE_ERROR_SET_PINS_FAILED;
        Serial.println("SD_MMC.setPins FAILED");
        Serial.flush();
        return;
    }

    Serial.println("SD_MMC.setPins OK");

    bool mounted = SD_MMC.begin(
        "/sdcard",
        true,
        false
    );

    if(!mounted)
    {
        if(g_cdRaw != LOW)
        {
            g_storageState = FT02_STORAGE_STATE_NO_CARD;
        }
        else
        {
            g_storageState = FT02_STORAGE_STATE_ERROR;
        }

        g_storageError = FT02_STORAGE_ERROR_MOUNT_FAILED;
        Serial.println("SD_MMC.begin FAILED");
        Serial.flush();
        return;
    }

    Serial.println("SD_MMC.begin OK");

    g_cardType = SD_MMC.cardType();

    if(g_cardType == CARD_NONE)
    {
        g_storageState = FT02_STORAGE_STATE_NO_CARD;
        g_storageError = FT02_STORAGE_ERROR_CARD_NONE;
        Serial.println("SD mounted but card type is NONE");
        Serial.flush();
        return;
    }

    g_cardSizeMB = FT02_ToMB(SD_MMC.cardSize());
    g_totalMB = FT02_ToMB(SD_MMC.totalBytes());
    g_usedMB = FT02_ToMB(SD_MMC.usedBytes());

    FT02_RunFileRwSmokeTest();

    if(g_rwResult != FT02_STORAGE_RW_OK)
    {
        g_storageState = FT02_STORAGE_STATE_ERROR;
        g_storageError = FT02_STORAGE_ERROR_RW_FAILED;
        Serial.println("FT02 SDMMC 1-bit file R/W smoke FAILED");
        Serial.flush();
        return;
    }

    // Refresh used/free after the test file has been written.
    g_usedMB = FT02_ToMB(SD_MMC.usedBytes());

    g_storageState = FT02_STORAGE_STATE_READY;
    g_storageError = FT02_STORAGE_ERROR_NONE;

    Serial.print("SD card type: ");
    Serial.println(FT02_CardTypeName(g_cardType));
    Serial.print("SD card size MB: ");
    Serial.println(g_cardSizeMB);
    Serial.print("SD total MB: ");
    Serial.println(g_totalMB);
    Serial.print("SD used MB: ");
    Serial.println(g_usedMB);
    Serial.print("SD free MB: ");
    Serial.println(FT02_StorageFreeMB());
    Serial.print("SD file R/W result: ");
    Serial.println(FT02_StorageRwResultText());
    Serial.println("FT02 SDMMC 1-bit READY + FILE RW OK");
    Serial.flush();
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

uint8_t FT02_StorageCardDetectRaw()
{
    return g_cdRaw;
}

uint8_t FT02_StorageD3Raw()
{
    return g_d3Raw;
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


FT02StorageRwResult FT02_StorageRwResultCurrent()
{
    return g_rwResult;
}

const char* FT02_StorageRwResultText()
{
    switch(g_rwResult)
    {
        case FT02_STORAGE_RW_NOT_RUN: return "NOT_RUN";
        case FT02_STORAGE_RW_WRITE_OPEN_FAILED: return "WRITE_OPEN_FAILED";
        case FT02_STORAGE_RW_WRITE_FAILED: return "WRITE_FAILED";
        case FT02_STORAGE_RW_READ_OPEN_FAILED: return "READ_OPEN_FAILED";
        case FT02_STORAGE_RW_READ_MISMATCH: return "READ_MISMATCH";
        case FT02_STORAGE_RW_OK: return "RW_OK";
        default: return "UNKNOWN";
    }
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
        case FT02_STORAGE_ERROR_SET_PINS_FAILED: return "SET_PINS_FAILED";
        case FT02_STORAGE_ERROR_MOUNT_FAILED: return "MOUNT_FAILED";
        case FT02_STORAGE_ERROR_CARD_NONE: return "CARD_NONE";
        case FT02_STORAGE_ERROR_RW_FAILED: return "RW_FAILED";
        default: return "UNKNOWN";
    }
}

bool FT02_StoragePollCardDetectChanged()
{
    uint32_t now = millis();

    if(now - g_lastCdPollMillis < 500)
    {
        return false;
    }

    g_lastCdPollMillis = now;

    int raw = digitalRead(FT02_SD_CD);
    if(raw == g_lastCdPollRaw)
    {
        return false;
    }

    g_lastCdPollRaw = raw;
    g_cdRaw = (uint8_t)raw;

    Serial.print("SD CD changed: raw=");
    Serial.print(g_cdRaw);
    Serial.print(" state=");
    Serial.print(FT02_StorageStateText());
    Serial.print(" error=");
    Serial.println(FT02_StorageErrorText());
    Serial.flush();

    return true;
}
