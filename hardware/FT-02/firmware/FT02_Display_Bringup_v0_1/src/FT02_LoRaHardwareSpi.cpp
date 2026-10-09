#include "FT02_LoRaHardwareSpi.h"

namespace FT02_LoRaHardwareSpi {
namespace {
SPIClass g_spi(HSPI);
bool g_ready = false;
}

bool begin() {
    if (g_ready) return true;

    pinMode(NSS, OUTPUT);
    digitalWrite(NSS, HIGH);
    pinMode(BUSY, INPUT);
    pinMode(DIO1, INPUT);

    // ESP32-S3 GPIO matrix allows the HSPI peripheral to be routed to the
    // already-wired FT02 Core1262 pins. EPD no longer owns HSPI in this build.
    const bool ok = g_spi.begin(SCK, MISO, MOSI, NSS);
    g_ready = ok;
    return ok;
}

void end() {
    if (!g_ready) return;
    g_spi.end();
    g_ready = false;
}

bool ready() { return g_ready; }
SPIClass& bus() { return g_spi; }
SPISettings settings() { return SPISettings(CLOCK_HZ, MSBFIRST, SPI_MODE0); }
}
