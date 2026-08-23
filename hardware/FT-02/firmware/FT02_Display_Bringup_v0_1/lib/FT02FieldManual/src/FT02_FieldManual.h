#pragma once

#include <Arduino.h>
#include <stdint.h>

static const int FT02_FIELD_MANUAL_MAX_CATEGORIES = 16;
static const int FT02_FIELD_MANUAL_MAX_CARDS = 256;
static const int FT02_FIELD_MANUAL_MAX_QUICK = 32;
static const int FT02_FIELD_MANUAL_MAX_SECTIONS = 8;
static const int FT02_FIELD_MANUAL_MAX_MEDIA = 256;
static const int FT02_FIELD_MANUAL_MEDIA_MAX_WIDTH = 688;
static const int FT02_FIELD_MANUAL_MEDIA_MAX_HEIGHT = 184;
static const int FT02_FIELD_MANUAL_MEDIA_BUFFER_SIZE = 24576;
static const uint8_t FT02_FIELD_MANUAL_MEDIA_FLAG_CAPTION_BELOW = 0x01;

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

// FieldManual Runtime A3 optional image-page index entry.
// media.idx is optional: A2 text-only packs remain valid without it.
struct FT02FieldManualMediaIndex
{
    uint16_t cardIndex;
    uint8_t order;
    uint8_t flags;
    uint16_t width;
    uint16_t height;
    uint32_t dataOffset;
    uint32_t dataLength;
    uint32_t crc32;
    char caption[64];
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

struct FT02FieldManualLoadedMedia
{
    const FT02FieldManualMediaIndex* index;
    const uint8_t* data;
};

bool FT02_FieldManualBegin(bool forceReload = false);
void FT02_FieldManualUnloadCard();
void FT02_FieldManualUnloadMedia();

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
int FT02_FieldManualLoadedCardGlobalIndex();

// Runtime A3 image pages. If media.idx/media.dat are absent or invalid,
// text cards still work and these functions simply report zero/no media.
bool FT02_FieldManualMediaReady();
int FT02_FieldManualMediaCount();
int FT02_FieldManualMediaCountForCard(int globalCardIndex);
bool FT02_FieldManualLoadMediaForCard(int globalCardIndex, int mediaOrdinal);
const FT02FieldManualLoadedMedia* FT02_FieldManualLoadedMediaCurrent();
