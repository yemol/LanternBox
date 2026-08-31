#include "FT02_ES8311.h"
#include "FT02_I2CBus.h"

namespace
{
constexpr uint8_t ES8311_ADDRESS_CANDIDATES[] = {0x18, 0x19};

constexpr uint8_t ES8311_RESET_REG00 = 0x00;
constexpr uint8_t ES8311_CLK_MANAGER_REG01 = 0x01;
constexpr uint8_t ES8311_CLK_MANAGER_REG02 = 0x02;
constexpr uint8_t ES8311_CLK_MANAGER_REG03 = 0x03;
constexpr uint8_t ES8311_CLK_MANAGER_REG04 = 0x04;
constexpr uint8_t ES8311_CLK_MANAGER_REG05 = 0x05;
constexpr uint8_t ES8311_CLK_MANAGER_REG06 = 0x06;
constexpr uint8_t ES8311_CLK_MANAGER_REG07 = 0x07;
constexpr uint8_t ES8311_CLK_MANAGER_REG08 = 0x08;
constexpr uint8_t ES8311_SDPIN_REG09 = 0x09;
constexpr uint8_t ES8311_SDPOUT_REG0A = 0x0A;
constexpr uint8_t ES8311_SYSTEM_REG0B = 0x0B;
constexpr uint8_t ES8311_SYSTEM_REG0C = 0x0C;
constexpr uint8_t ES8311_SYSTEM_REG0D = 0x0D;
constexpr uint8_t ES8311_SYSTEM_REG0E = 0x0E;
constexpr uint8_t ES8311_SYSTEM_REG10 = 0x10;
constexpr uint8_t ES8311_SYSTEM_REG11 = 0x11;
constexpr uint8_t ES8311_SYSTEM_REG12 = 0x12;
constexpr uint8_t ES8311_SYSTEM_REG13 = 0x13;
constexpr uint8_t ES8311_SYSTEM_REG14 = 0x14;
constexpr uint8_t ES8311_ADC_REG15 = 0x15;
constexpr uint8_t ES8311_ADC_REG16 = 0x16;
constexpr uint8_t ES8311_ADC_REG17 = 0x17;
constexpr uint8_t ES8311_ADC_REG1B = 0x1B;
constexpr uint8_t ES8311_ADC_REG1C = 0x1C;
constexpr uint8_t ES8311_DAC_REG31 = 0x31;
constexpr uint8_t ES8311_DAC_REG32 = 0x32;
constexpr uint8_t ES8311_DAC_REG37 = 0x37;
constexpr uint8_t ES8311_GPIO_REG44 = 0x44;
constexpr uint8_t ES8311_GP_REG45 = 0x45;
constexpr uint8_t ES8311_CHIP_ID1_REGFD = 0xFD;
constexpr uint8_t ES8311_CHIP_ID2_REGFE = 0xFE;
constexpr uint8_t ES8311_CHIP_VERSION_REGFF = 0xFF;

constexpr uint8_t FT02_ES8311_I2C_RETRIES = 3;

// FT-02 audio remains 44.1 kHz, 16-bit, two 16-bit I2S slots. Therefore:
// BCLK = 44100 * 16 * 2 = 1.4112 MHz. ES8311 uses BCLK/SCLK as its
// internal master-clock source and the 1.4112 MHz / 44.1 kHz coefficient.
constexpr uint8_t FT02_ES8311_CLOCK_REG01 = 0xBF;
constexpr uint8_t FT02_ES8311_CLOCK_REG02 = 0x18;
constexpr uint8_t FT02_ES8311_CLOCK_REG03 = 0x10;
constexpr uint8_t FT02_ES8311_CLOCK_REG04 = 0x10;
constexpr uint8_t FT02_ES8311_CLOCK_REG05 = 0x00;
constexpr uint8_t FT02_ES8311_CLOCK_REG06 = 0x03;
constexpr uint8_t FT02_ES8311_CLOCK_REG07 = 0x00;
constexpr uint8_t FT02_ES8311_CLOCK_REG08 = 0xFF;

// Speaker baseline trim: REG32 uses 0.5 dB/step with 0xBF = 0 dB.
// User requested roughly 2/3 of the current output-voltage amplitude. -3.5 dB
// gives 10^(-3.5/20) = 0.668x, so use 0xB8 while preserving the existing
// software 0..10 volume curve and limiter.
constexpr uint8_t FT02_ES8311_DAC_VOLUME = 0xB8;
}

bool FT02ES8311::probe(uint8_t address)
{
    if(wire_ == nullptr || !FT02_I2CBusLock(50)) return false;
    wire_->beginTransmission(address);
    const uint8_t error = wire_->endTransmission(true);
    FT02_I2CBusUnlock();
    return error == 0;
}

bool FT02ES8311::readRegister(uint8_t reg, uint8_t& value)
{
    if(wire_ == nullptr || address_ == 0) return false;

    for(uint8_t attempt = 0; attempt < FT02_ES8311_I2C_RETRIES; ++attempt)
    {
        if(!FT02_I2CBusLock(50))
        {
            delay(2);
            continue;
        }

        bool ok = false;
        wire_->beginTransmission(address_);
        wire_->write(reg);
        if(wire_->endTransmission(false) == 0 &&
           wire_->requestFrom(address_, static_cast<uint8_t>(1), true) == 1 &&
           wire_->available() > 0)
        {
            value = wire_->read();
            ok = true;
        }
        FT02_I2CBusUnlock();
        if(ok) return true;
        delay(2);
    }
    return false;
}

bool FT02ES8311::writeRegister(uint8_t reg, uint8_t value, bool verify)
{
    if(wire_ == nullptr || address_ == 0) return false;

    for(uint8_t attempt = 0; attempt < FT02_ES8311_I2C_RETRIES; ++attempt)
    {
        if(!FT02_I2CBusLock(50))
        {
            delay(2);
            continue;
        }

        wire_->beginTransmission(address_);
        wire_->write(reg);
        wire_->write(value);
        const bool writeOk = wire_->endTransmission(true) == 0;
        FT02_I2CBusUnlock();

        if(writeOk)
        {
            if(!verify) return true;

            // Verification is a separate complete transaction. Never keep the
            // shared-bus mutex across the nested read or CardKB2 can starve.
            uint8_t actual = 0;
            if(readRegister(reg, actual) && actual == value) return true;
        }
        delay(2);
    }
    return false;
}

bool FT02ES8311::writeStep(const char* label, uint8_t reg, uint8_t value, bool verify)
{
    const bool ok = writeRegister(reg, value, verify);
    Serial.printf(
        "[AUDIO-CODEC] %-27s reg=0x%02X value=0x%02X %s\n",
        label,
        static_cast<unsigned int>(reg),
        static_cast<unsigned int>(value),
        ok ? "OK" : "FAIL"
    );
    return ok;
}

bool FT02ES8311::begin(TwoWire& wire, bool digitalMic)
{
    wire_ = &wire;
    address_ = 0;
    dacReg31_ = 0x60;
    dacMuted_ = true;

    for(uint8_t candidate : ES8311_ADDRESS_CANDIDATES)
    {
        if(probe(candidate))
        {
            address_ = candidate;
            break;
        }
    }
    if(address_ == 0)
    {
        Serial.println("[AUDIO-CODEC] ES8311 not found at 0x18/0x19");
        return false;
    }

    (void)readRegister(ES8311_CHIP_ID1_REGFD, chipId1_);
    (void)readRegister(ES8311_CHIP_ID2_REGFE, chipId2_);
    (void)readRegister(ES8311_CHIP_VERSION_REGFF, chipVersion_);
    Serial.printf(
        "[AUDIO-CODEC] ES8311 addr=0x%02X id=%02X:%02X version=%02X mic=%s clock=SCLK/no-MCLK\n",
        static_cast<unsigned int>(address_),
        static_cast<unsigned int>(chipId1_),
        static_cast<unsigned int>(chipId2_),
        static_cast<unsigned int>(chipVersion_),
        digitalMic ? "digital" : "analog"
    );

    bool ok = true;
    auto step = [&ok, this](const char* label, uint8_t reg, uint8_t value, bool verify = true)
    {
        const bool result = writeStep(label, reg, value, verify);
        ok = ok && result;
    };

    // Espressif's current ES8311 driver writes GPIO44 twice before the rest
    // of codec setup. The first control write can occasionally fail and 0x08
    // improves ES8311 I2C noise immunity. Keep both writes unverified here,
    // then retain normal register verification for the actual codec setup.
    step("I2C noise immunity 1", ES8311_GPIO_REG44, 0x08, false);
    step("I2C noise immunity 2", ES8311_GPIO_REG44, 0x08, false);

    // Establish a known powered-down state before setting the clock tree.
    step("general purpose", ES8311_GP_REG45, 0x00);
    step("clock reset", ES8311_CLK_MANAGER_REG01, 0x30);
    step("clock pre-divider reset", ES8311_CLK_MANAGER_REG02, 0x00);
    step("ADC oversampling reset", ES8311_CLK_MANAGER_REG03, 0x10);
    step("ADC coarse gain", ES8311_ADC_REG16, 0x24);
    step("DAC oversampling reset", ES8311_CLK_MANAGER_REG04, 0x10);
    step("ADC/DAC divider reset", ES8311_CLK_MANAGER_REG05, 0x00);
    step("system 0B", ES8311_SYSTEM_REG0B, 0x00);
    step("system 0C", ES8311_SYSTEM_REG0C, 0x00);
    step("system 10", ES8311_SYSTEM_REG10, 0x1F);
    step("system 11", ES8311_SYSTEM_REG11, 0x7F);
    step("codec reset", ES8311_RESET_REG00, 0x80, false);
    delay(80);

    // Codec is an I2S slave. Bit 7 selects BCLK/SCLK as its master-clock
    // source, so the module's MCLK pin is intentionally not connected.
    step("I2S slave", ES8311_RESET_REG00, 0x80);
    step("analog power", ES8311_SYSTEM_REG0D, 0x01);
    step("SCLK master source", ES8311_CLK_MANAGER_REG01, FT02_ES8311_CLOCK_REG01);
    step("44.1k pre-mult", ES8311_CLK_MANAGER_REG02, FT02_ES8311_CLOCK_REG02);
    step("44.1k ADC OSR", ES8311_CLK_MANAGER_REG03, FT02_ES8311_CLOCK_REG03);
    step("44.1k DAC OSR", ES8311_CLK_MANAGER_REG04, FT02_ES8311_CLOCK_REG04);
    step("44.1k ADC/DAC div", ES8311_CLK_MANAGER_REG05, FT02_ES8311_CLOCK_REG05);
    step("44.1k BCLK div", ES8311_CLK_MANAGER_REG06, FT02_ES8311_CLOCK_REG06);
    step("44.1k LRCK high", ES8311_CLK_MANAGER_REG07, FT02_ES8311_CLOCK_REG07);
    step("44.1k LRCK low", ES8311_CLK_MANAGER_REG08, FT02_ES8311_CLOCK_REG08);

    // Philips I2S, 16-bit input and output. The ESP32 sends two 16-bit slots;
    // ES8311 is mono and uses the valid mono data duplicated into both slots.
    step("SDIN I2S 16-bit", ES8311_SDPIN_REG09, 0x0C);
    step("SDOUT I2S 16-bit", ES8311_SDPOUT_REG0A, 0x0C);

    // Keep DAC muted while the analogue path starts. I2S is already sending
    // zero samples, which avoids presenting an undefined level to NS4150B.
    step("DAC soft mute", ES8311_DAC_REG31, 0x60);
    step("DAC volume zero", ES8311_DAC_REG32, 0x00);
    step("DAC power", ES8311_SYSTEM_REG12, 0x00);
    step("output driver", ES8311_SYSTEM_REG13, 0x10);
    step(
        digitalMic ? "digital microphone" : "analog microphone",
        ES8311_SYSTEM_REG14,
        digitalMic ? 0x5A : 0x1A
    );
    step("ADC analog path", ES8311_SYSTEM_REG0E, 0x02);
    step("ADC ramp/DMIC sense", ES8311_ADC_REG15, 0x40);
    step("ADC HPF stage 1", ES8311_ADC_REG1B, 0x0A);
    step("ADC HPF stage 2", ES8311_ADC_REG1C, 0x6A);
    step("ADC digital volume", ES8311_ADC_REG17, 0xBF);
    step("DAC ramp/EQ bypass", ES8311_DAC_REG37, 0x08);
    step("GPIO normal path", ES8311_GPIO_REG44, 0x08);

    if(!ok) return false;

    delay(20);
    // Idle/recording state follows the proven ES8311 full mute sequence used
    // by Espressif's ADF driver: soft-mute, zero DAC volume, power DAC down.
    // The ADC remains active for recording.
    step("DAC idle mute", ES8311_DAC_REG31, 0x60);
    step("DAC idle volume zero", ES8311_DAC_REG32, 0x00);
    step("DAC idle power down", ES8311_SYSTEM_REG12, 0x02);
    dacReg31_ = 0x60;
    dacMuted_ = true;
    dacPowered_ = false;
    if(!ok) return false;

    Serial.println("[AUDIO-CODEC] ES8311 configuration verified OK; DAC=IDLE_POWERDOWN");
    return true;
}

bool FT02ES8311::prepareDacForPaShutdown()
{
    if(wire_ == nullptr || address_ == 0) return false;

    const uint8_t mutedReg31 =
        static_cast<uint8_t>(dacReg31_ | 0x60u);
    bool ok = true;

    // Keep the analogue DAC powered while the external BTL PA is still on.
    // First engage codec soft mute, then walk the DAC gain down in coarse
    // steps. The PCM stream is already zero, so this only removes register/
    // bias discontinuities from the subsequent PA shutdown edge.
    ok = writeRegister(ES8311_DAC_REG31, mutedReg31, false) && ok;
    delay(4);
    static const uint8_t kRamp[] = {0xA8, 0x98, 0x78, 0x58, 0x38, 0x18, 0x00};
    for(uint8_t value : kRamp)
    {
        ok = writeRegister(ES8311_DAC_REG32, value, false) && ok;
        delay(3);
    }

    if(ok)
    {
        dacReg31_ = mutedReg31;
        dacMuted_ = true;
        dacPowered_ = true;
        Serial.println("[AUDIO-CODEC] DAC soft-mute+volume-ramp ready for PA shutdown");
    }
    return ok;
}

bool FT02ES8311::powerDownDac()
{
    if(wire_ == nullptr || address_ == 0) return false;
    if(!dacPowered_) return true;

    const bool ok = writeRegister(ES8311_SYSTEM_REG12, 0x02, false);
    if(ok)
    {
        dacPowered_ = false;
        dacMuted_ = true;
        Serial.println("[AUDIO-CODEC] DAC powered down after PA shutdown");
    }
    return ok;
}

bool FT02ES8311::setDacMuted(bool muted)
{
    if(wire_ == nullptr || address_ == 0) return false;

    const uint8_t mutedReg31 =
        static_cast<uint8_t>(dacReg31_ | 0x60u);
    const uint8_t unmutedReg31 =
        static_cast<uint8_t>(dacReg31_ & static_cast<uint8_t>(~0x60u));

    bool ok = true;
    if(muted)
    {
        if(dacMuted_ && !dacPowered_) return true;
        ok = prepareDacForPaShutdown();
        if(ok) ok = powerDownDac();
        if(ok)
        {
            Serial.println("[AUDIO-CODEC] DAC mute+volume0+powerdown OK");
        }
    }
    else
    {
        if(!dacMuted_ && dacPowered_) return true;
        // Restore DAC power/level while external PA is still in shutdown.
        ok = writeRegister(ES8311_SYSTEM_REG12, 0x00, false) && ok;
        delay(8);
        ok = writeRegister(ES8311_DAC_REG32, FT02_ES8311_DAC_VOLUME, false) && ok;
        ok = writeRegister(ES8311_DAC_REG31, unmutedReg31, false) && ok;
        if(ok)
        {
            dacReg31_ = unmutedReg31;
            dacMuted_ = false;
            dacPowered_ = true;
            Serial.println("[AUDIO-CODEC] DAC powerup+volume+unmute OK");
        }
    }

    if(!ok)
    {
        Serial.printf(
            "[AUDIO-CODEC] DAC %s transition FAIL\n",
            muted ? "idle-powerdown" : "playback-powerup"
        );
    }
    return ok;
}

bool FT02ES8311::isDacMuted() const
{
    return dacMuted_;
}

uint8_t FT02ES8311::address() const
{
    return address_;
}

uint8_t FT02ES8311::chipId1() const
{
    return chipId1_;
}

uint8_t FT02ES8311::chipId2() const
{
    return chipId2_;
}

uint8_t FT02ES8311::chipVersion() const
{
    return chipVersion_;
}
