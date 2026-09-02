#pragma once

#include <Arduino.h>
#include <stdint.h>

static constexpr uint8_t FT02_CORE_PORTAL_MAX_ITEMS = 6;
static constexpr size_t FT02_CORE_PORTAL_ID_LEN = 24;
static constexpr size_t FT02_CORE_PORTAL_LABEL_LEN = 34;

enum FT02CorePortalItemType : uint8_t
{
    FT02_CORE_PORTAL_ITEM_PAGE = 0,
    FT02_CORE_PORTAL_ITEM_ACTION,
    FT02_CORE_PORTAL_ITEM_STATUS,
    FT02_CORE_PORTAL_ITEM_PROGRESS,
    FT02_CORE_PORTAL_ITEM_CONFIRM
};

struct FT02CorePortalItem
{
    char id[FT02_CORE_PORTAL_ID_LEN];
    char label[FT02_CORE_PORTAL_LABEL_LEN];
    FT02CorePortalItemType type;
    int16_t value;
};

struct FT02CorePortalPage
{
    char page[FT02_CORE_PORTAL_ID_LEN];
    char title[FT02_CORE_PORTAL_LABEL_LEN];
    char status[64];
    FT02CorePortalItem items[FT02_CORE_PORTAL_MAX_ITEMS];
    uint8_t itemCount;
    uint8_t selected;
};
