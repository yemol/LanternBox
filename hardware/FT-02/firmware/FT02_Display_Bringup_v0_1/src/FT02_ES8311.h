#pragma once

#include <Arduino.h>
#include <Wire.h>

// Minimal FT-02 ES8311 control driver. Audio samples continue to use the ESP-IDF
// I2S standard driver; this class only configures and verifies the codec over I2C.
class FT02ES8311
{
public:
    bool begin(TwoWire& wire, bool digitalMic);
    bool setDacMuted(bool muted);
    bool prepareDacForPaShutdown();
    bool powerDownDac();
    bool isDacMuted() const;

    uint8_t address() const;
    uint8_t chipId1() const;
    uint8_t chipId2() const;
    uint8_t chipVersion() const;

private:
    bool probe(uint8_t address);
    bool readRegister(uint8_t reg, uint8_t& value);
    bool writeRegister(uint8_t reg, uint8_t value, bool verify = true);
    bool writeStep(const char* label, uint8_t reg, uint8_t value, bool verify = true);

    TwoWire* wire_ = nullptr;
    uint8_t address_ = 0;
    uint8_t chipId1_ = 0;
    uint8_t chipId2_ = 0;
    uint8_t chipVersion_ = 0;
    uint8_t dacReg31_ = 0x60;
    bool dacMuted_ = true;
    bool dacPowered_ = false;
};
