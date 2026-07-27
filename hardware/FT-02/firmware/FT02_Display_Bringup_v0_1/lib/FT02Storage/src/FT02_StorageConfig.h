#pragma once

#include <Arduino.h>

// FT-02 stable SD profile validated by Codex and hardware testing.
// SDMMC 1-bit, 5 MHz:
//   CLK -> GPIO35
//   CMD -> GPIO2
//   D0  -> GPIO1
// CD/D1/D2/D3 are not connected in the current production profile.
// Future 4-bit work must use a separate laboratory profile.
namespace FT02StorageConfig
{
    static const int CLK = 35;
    static const int CMD = 2;
    static const int D0 = 1;

    static const uint32_t FREQUENCY_KHZ = 5000U;
    static const uint8_t MAX_OPEN_FILES = 5;

    static const char* const PROFILE_NAME =
        "SDMMC1Bit5MHz_CLK35_CMD2_D0_GPIO1";
}
