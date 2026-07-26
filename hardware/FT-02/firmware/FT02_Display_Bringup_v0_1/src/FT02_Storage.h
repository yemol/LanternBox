#pragma once

#include <Arduino.h>

enum FT02StorageState
{
    FT02_STORAGE_STATE_NOT_STARTED = 0,
    FT02_STORAGE_STATE_SCANNING,
    FT02_STORAGE_STATE_READY,
    FT02_STORAGE_STATE_NO_CARD,
    FT02_STORAGE_STATE_ERROR
};

enum FT02StorageError
{
    FT02_STORAGE_ERROR_NONE = 0,
    FT02_STORAGE_ERROR_SET_PINS_FAILED,
    FT02_STORAGE_ERROR_MOUNT_FAILED,
    FT02_STORAGE_ERROR_CARD_NONE,
    FT02_STORAGE_ERROR_RW_FAILED
};

enum FT02StorageRwResult
{
    FT02_STORAGE_RW_NOT_RUN = 0,
    FT02_STORAGE_RW_WRITE_OPEN_FAILED,
    FT02_STORAGE_RW_WRITE_FAILED,
    FT02_STORAGE_RW_READ_OPEN_FAILED,
    FT02_STORAGE_RW_READ_MISMATCH,
    FT02_STORAGE_RW_OK
};

void FT02_StorageBegin();

FT02StorageState FT02_StorageStateCurrent();
FT02StorageError FT02_StorageLastError();

bool FT02_StorageIsReady();

uint8_t FT02_StorageCardDetectRaw();
uint8_t FT02_StorageD3Raw();

unsigned long FT02_StorageCardSizeMB();
unsigned long FT02_StorageTotalMB();
unsigned long FT02_StorageUsedMB();
unsigned long FT02_StorageFreeMB();

FT02StorageRwResult FT02_StorageRwResultCurrent();
const char* FT02_StorageRwResultText();

const char* FT02_StorageCardTypeText();
const char* FT02_StorageStateText();
const char* FT02_StorageErrorText();

bool FT02_StoragePollCardDetectChanged();
