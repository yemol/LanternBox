#pragma once

#include <Arduino.h>
#include <stdio.h>

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
    FT02_STORAGE_ERROR_MOUNT_FAILED,
    FT02_STORAGE_ERROR_CARD_NONE
};

void FT02_StorageBegin();

FT02StorageState FT02_StorageStateCurrent();
FT02StorageError FT02_StorageLastError();
bool FT02_StorageIsReady();

unsigned long FT02_StorageCardSizeMB();
unsigned long FT02_StorageTotalMB();
unsigned long FT02_StorageUsedMB();
unsigned long FT02_StorageFreeMB();

const char* FT02_StorageCardTypeText();
const char* FT02_StorageStateText();
const char* FT02_StorageErrorText();
const char* FT02_StorageProfileText();

uint32_t FT02_StorageFrequencyKHz();
int FT02_StorageClockPin();
int FT02_StorageCommandPin();
int FT02_StorageD0Pin();

bool FT02_StorageFileExists(const char* path);
FILE* FT02_StorageOpenReadFile(const char* path);
bool FT02_StorageAppendLine(const char* path, const char* line);
