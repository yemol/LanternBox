#pragma once
#include <Arduino.h>

struct FT02GrayFontIndexRecord
{
    uint32_t codepoint;
    uint32_t offset;
};

struct FT02GrayFontPack
{
    const FT02GrayFontIndexRecord* index;
    uint16_t count;
    const uint8_t* data;
    uint32_t dataLength;
};
