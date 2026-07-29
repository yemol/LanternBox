#pragma once

#include <Arduino.h>

// FT-02 frozen SD SPI production profile, validated by the v0.9 baseline test.
// SD card uses a dedicated FSPI host and the display uses a dedicated HSPI host.
//
// SD SPI wiring:
//   D3 / CS    -> GPIO41
//   CLK / SCK  -> GPIO42
//   CMD / MOSI -> GPIO2
//   D0 / MISO  -> GPIO1
//
// Frozen transport:
//   SPI Mode 3
//   40 MHz operating clock
//   CMD18 multi-block read
//   ACMD23 + CMD25 multi-block write
//   16 KiB transfer chunks (32 sectors)
namespace FT02StorageConfig
{
    static const int SD_CS = 41;
    static const int SD_SCK = 42;
    static const int SD_MOSI = 2;
    static const int SD_MISO = 1;

    static const uint32_t SD_INIT_FREQUENCY_HZ = 400000U;
    static const uint32_t SD_INIT_HZ = SD_INIT_FREQUENCY_HZ;
    static const uint32_t RAW_BASELINE_HZ = 4000000U;
    static const uint32_t SD_FREQUENCY_HZ = 40000000U;
    static const uint32_t FREQUENCY_KHZ = SD_FREQUENCY_HZ / 1000U;

    static const uint32_t TRANSFER_CHUNK_BLOCKS = 32U;
    static const uint32_t MAX_MULTI_BLOCKS = TRANSFER_CHUNK_BLOCKS;
    static const uint32_t COMMAND_TIMEOUT_MS = 500U;
    static const uint32_t INIT_TIMEOUT_MS = 3000U;
    static const uint8_t MAX_OPEN_FILES = 5;

    static const char* const MOUNT_POINT = "/sdcard";
    static const char* const PROFILE_NAME =
        "FSPI_Mode3_40MHz_SCK42_MOSI2_MISO1_CS41_MULTI16KiB";
}
