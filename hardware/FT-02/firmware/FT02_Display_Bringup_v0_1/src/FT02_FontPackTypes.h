#pragma once
#include <Arduino.h>

struct FT02FontIndexRecord
{
    uint32_t codepoint;
    uint32_t offset;
};

struct FT02FontPack
{
    const FT02FontIndexRecord* index;
    uint16_t count;
    const uint8_t* data;
    uint32_t dataLength;
};
