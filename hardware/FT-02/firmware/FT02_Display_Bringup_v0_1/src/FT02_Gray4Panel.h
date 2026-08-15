#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>

constexpr uint16_t FT02_GRAY4_WIDTH = 800;
constexpr uint16_t FT02_GRAY4_HEIGHT = 480;
constexpr size_t FT02_GRAY4_FRAME_PACKED_BYTES =
    static_cast<size_t>(FT02_GRAY4_WIDTH) * FT02_GRAY4_HEIGHT / 4U;

enum FT02Gray4Level : uint8_t
{
    FT02_GRAY4_BLACK = 0,
    FT02_GRAY4_DARK = 1,
    FT02_GRAY4_LIGHT = 2,
    FT02_GRAY4_WHITE = 3
};

struct FT02Gray4PanelReport
{
    bool success;
    bool frameValid;
    bool panelInitialized;
    bool refreshCompleted;
    bool panelQuiesced;
    uint32_t elapsedMs;
    const char* message;
};

uint8_t* FT02_AllocateGray4Framebuffer();
void FT02_FreeGray4Framebuffer(uint8_t* frame);

FT02Gray4PanelReport FT02_DisplayGray4Framebuffer(
    SPIClass& spi,
    uint32_t spiHz,
    int pwrPin,
    int busyPin,
    int rstPin,
    int dcPin,
    int csPin,
    const uint8_t* packed2bpp,
    size_t packedBytes
);
