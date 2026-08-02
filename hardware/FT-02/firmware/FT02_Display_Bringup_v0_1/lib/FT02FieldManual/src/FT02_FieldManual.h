#pragma once

#include <Arduino.h>
#include <stdint.h>

static const int FT02_FIELD_MANUAL_MAX_CATEGORIES = 16;
static const int FT02_FIELD_MANUAL_MAX_CARDS = 64;
static const int FT02_FIELD_MANUAL_MAX_QUICK = 32;
static const int FT02_FIELD_MANUAL_MAX_SECTIONS = 8;

enum FT02FieldManualState
{
    FT02_FIELD_MANUAL_NOT_STARTED = 0,
    FT02_FIELD_MANUAL_READY,
    FT02_FIELD_MANUAL_STORAGE_NOT_READY,
    FT02_FIELD_MANUAL_PACK_NOT_FOUND,
    FT02_FIELD_MANUAL_BAD_FORMAT,
    FT02_FIELD_MANUAL_CHECKSUM_FAILED,
    FT02_FIELD_MANUAL_IO_ERROR
};

#pragma pack(push, 1)
struct FT02FieldManualCategory
{
    char id[4];
    char name[48];
    char summary[144];
    uint8_t icon;
    uint16_t firstCard;
    uint16_t cardCount;
    uint8_t reserved[3];
};

struct FT02FieldManualCardIndex
{
    char id[12];
    char category[4];
    char title[96];
    char summary[192];
    uint32_t dataOffset;
    uint32_t dataLength;
    uint8_t quickAccess;
    uint8_t priority;
    uint8_t reserved[2];
};
#pragma pack(pop)

struct FT02FieldManualSection
{
    const char* title;
    const char* text;
};

struct FT02FieldManualLoadedCard
{
    const FT02FieldManualCardIndex* index;
    uint8_t sectionCount;
    FT02FieldManualSection sections[FT02_FIELD_MANUAL_MAX_SECTIONS];
};

bool FT02_FieldManualBegin(bool forceReload = false);
void FT02_FieldManualUnloadCard();

FT02FieldManualState FT02_FieldManualStateCurrent();
const char* FT02_FieldManualStateText();
const char* FT02_FieldManualContentVersion();

int FT02_FieldManualCategoryCount();
int FT02_FieldManualCardCount();
int FT02_FieldManualQuickCount();

const FT02FieldManualCategory* FT02_FieldManualCategoryAt(int index);
const FT02FieldManualCardIndex* FT02_FieldManualCardAt(int globalIndex);
int FT02_FieldManualCategoryCardGlobalIndex(int categoryIndex, int localIndex);
int FT02_FieldManualQuickGlobalIndex(int localIndex);

bool FT02_FieldManualLoadCard(int globalIndex);
const FT02FieldManualLoadedCard* FT02_FieldManualLoadedCardCurrent();
