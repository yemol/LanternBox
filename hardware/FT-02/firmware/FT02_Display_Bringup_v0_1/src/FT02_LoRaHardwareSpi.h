#pragma once

#include <Arduino.h>
#include <SPI.h>

// FT-02 Core1262 hardware SPI owner.
// Physical wiring is unchanged from the former software-SPI path.
namespace FT02_LoRaHardwareSpi {
constexpr int SCK = 38;
constexpr int MOSI = 39;
constexpr int MISO = 40;
constexpr int NSS = 7;
constexpr int BUSY = 13;
constexpr int DIO1 = 48;
constexpr uint32_t CLOCK_HZ = 4000000u;

bool begin();
void end();
bool ready();
SPIClass& bus();
SPISettings settings();
}
