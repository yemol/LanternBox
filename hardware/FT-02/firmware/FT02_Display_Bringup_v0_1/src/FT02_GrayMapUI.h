#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>

struct FT02GrayMapReport
{
    bool success;
    bool mapReady;
    bool frameAllocated;
    bool frameRendered;
    bool panelInitialized;
    bool refreshCompleted;
    bool panelQuiesced;
    int zoom;
    size_t segmentCount;
    size_t labelCount;
    size_t outlinedBuildingSegments;
    uint32_t renderMs;
    uint32_t elapsedMs;
    const char* message;
};

FT02GrayMapReport FT02_DrawGrayMapScreen(
    SPIClass& spi,
    uint32_t spiHz,
    int pwrPin,
    int busyPin,
    int rstPin,
    int dcPin,
    int csPin
);
