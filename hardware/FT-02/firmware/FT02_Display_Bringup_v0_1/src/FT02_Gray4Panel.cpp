#include "FT02_Gray4Panel.h"


#include <esp_heap_caps.h>
#include <stdlib.h>

namespace
{
constexpr size_t FT02_GRAY4_PLANE_BYTES =
    static_cast<size_t>(FT02_GRAY4_WIDTH) * FT02_GRAY4_HEIGHT / 8U;
constexpr uint32_t FT02_GRAY4_BUSY_TIMEOUT_MS = 20000UL;

// Waveshare's tested SSD1677 four-gray waveform for the 4.26-inch T82 panel.
// Layout: 105 bytes LUT + VGH/VSH1/VSH2/VSL/VCOM parameters.
const uint8_t FT02_GRAY4_LUT[112] PROGMEM = {
    0x80, 0x48, 0x4A, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x48, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA8, 0x48, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x07, 0x1E, 0x1C, 0x02, 0x00,
    0x05, 0x01, 0x05, 0x01, 0x02,
    0x08, 0x01, 0x01, 0x04, 0x04,
    0x00, 0x02, 0x00, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01,
    0x22, 0x22, 0x22, 0x22, 0x22,
    0x17, 0x41, 0xA8, 0x32, 0x30,
    0x00, 0x00
};

class FT02Gray4Panel
{
public:
    FT02Gray4Panel(
        SPIClass& spi,
        uint32_t spiHz,
        int pwrPin,
        int busyPin,
        int rstPin,
        int dcPin,
        int csPin
    ) :
        spi_(spi),
        settings_(spiHz, MSBFIRST, SPI_MODE0),
        pwrPin_(pwrPin),
        busyPin_(busyPin),
        rstPin_(rstPin),
        dcPin_(dcPin),
        csPin_(csPin)
    {
    }

    bool begin()
    {
        pinMode(csPin_, OUTPUT);
        pinMode(rstPin_, OUTPUT);
        pinMode(dcPin_, OUTPUT);
        pinMode(busyPin_, INPUT);
        pinMode(pwrPin_, OUTPUT);

        digitalWrite(pwrPin_, HIGH);
        digitalWrite(csPin_, HIGH);
        digitalWrite(dcPin_, LOW);
        digitalWrite(rstPin_, HIGH);
        delay(60);

        reset();
        if(!waitBusy("reset")) return false;

        command(0x12); // software reset
        if(!waitBusy("software-reset")) return false;

        commandData(0x18, 0x80); // internal temperature sensor

        command(0x0C); // booster soft start
        data(0xAE);
        data(0xC7);
        data(0xC3);
        data(0xC0);
        data(0x80);

        command(0x01); // driver output control: 480 gates
        data(static_cast<uint8_t>((FT02_GRAY4_HEIGHT - 1U) & 0xFFU));
        data(static_cast<uint8_t>((FT02_GRAY4_HEIGHT - 1U) >> 8));
        data(0x02);

        commandData(0x3C, 0x01); // border waveform
        commandData(0x11, 0x01); // X+, Y-

        setWindow(0, FT02_GRAY4_HEIGHT - 1U, FT02_GRAY4_WIDTH - 1U, 0);
        setCursor(0, 0);
        if(!waitBusy("address-window")) return false;

        command(0x32);
        beginDataStream();
        for(size_t i = 0; i < 105U; ++i)
        {
            spi_.transfer(pgm_read_byte(FT02_GRAY4_LUT + i));
        }
        endDataStream();

        commandData(0x03, pgm_read_byte(FT02_GRAY4_LUT + 105));
        command(0x04);
        data(pgm_read_byte(FT02_GRAY4_LUT + 106));
        data(pgm_read_byte(FT02_GRAY4_LUT + 107));
        data(pgm_read_byte(FT02_GRAY4_LUT + 108));
        commandData(0x2C, pgm_read_byte(FT02_GRAY4_LUT + 109));

        return true;
    }

    bool display(const uint8_t* packed2bpp, size_t packedBytes)
    {
        if(packed2bpp == nullptr || packedBytes != FT02_GRAY4_FRAME_PACKED_BYTES)
        {
            return false;
        }

        // The SSD1677 combines RAM 0x24 and RAM 0x26 as two bit planes.
        // Decoded level values are: 0 black, 1 dark gray, 2 light gray, 3 white.
        if(!writePlane(0x24, packed2bpp, true)) return false;
        if(!writePlane(0x26, packed2bpp, false)) return false;

        commandData(0x22, 0xC7);
        command(0x20);
        if(!waitBusy("four-gray-refresh")) return false;

        // The GDEQ0426T82 is bistable. Once the refresh waveform is complete,
        // leaving the SSD1677 and panel power rail active can continue biasing
        // the charged particles and make the whole four-gray frame drift toward
        // gray. Follow the Waveshare lifecycle: enter deep sleep, then physically
        // remove panel power. The image remains on the panel without power.
        delay(120);
        return deepSleepAndCutPower();
    }

private:
    SPIClass& spi_;
    SPISettings settings_;
    int pwrPin_;
    int busyPin_;
    int rstPin_;
    int dcPin_;
    int csPin_;

    bool deepSleepAndCutPower()
    {
        // Official epd4in26 lifecycle uses DEEP_SLEEP (0x10, 0x01) before
        // module power-down. Keep CS inactive and all control pins in a quiet
        // state so no later one-bit transaction can leak into the gray frame.
        command(0x10);
        data(0x01);
        delay(2000);

        digitalWrite(csPin_, HIGH);
        digitalWrite(dcPin_, LOW);
        digitalWrite(rstPin_, LOW);
        digitalWrite(pwrPin_, LOW);
        delay(80);
        return true;
    }

    void reset()
    {
        digitalWrite(rstPin_, HIGH);
        delay(20);
        digitalWrite(rstPin_, LOW);
        delay(4);
        digitalWrite(rstPin_, HIGH);
        delay(20);
    }

    bool waitBusy(const char* stage)
    {
        const uint32_t started = millis();
        while(digitalRead(busyPin_) == HIGH)
        {
            if(millis() - started >= FT02_GRAY4_BUSY_TIMEOUT_MS)
            {
                Serial.print("[GRAY4] busy timeout stage=");
                Serial.println(stage != nullptr ? stage : "unknown");
                return false;
            }
            delay(20);
        }
        delay(20);
        return true;
    }

    void command(uint8_t value)
    {
        spi_.beginTransaction(settings_);
        digitalWrite(dcPin_, LOW);
        digitalWrite(csPin_, LOW);
        spi_.transfer(value);
        digitalWrite(csPin_, HIGH);
        spi_.endTransaction();
    }

    void data(uint8_t value)
    {
        spi_.beginTransaction(settings_);
        digitalWrite(dcPin_, HIGH);
        digitalWrite(csPin_, LOW);
        spi_.transfer(value);
        digitalWrite(csPin_, HIGH);
        spi_.endTransaction();
    }

    void commandData(uint8_t cmd, uint8_t value)
    {
        command(cmd);
        data(value);
    }

    void beginDataStream()
    {
        spi_.beginTransaction(settings_);
        digitalWrite(dcPin_, HIGH);
        digitalWrite(csPin_, LOW);
    }

    void endDataStream()
    {
        digitalWrite(csPin_, HIGH);
        spi_.endTransaction();
    }

    void setWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
    {
        command(0x44);
        data(static_cast<uint8_t>(xStart & 0xFFU));
        data(static_cast<uint8_t>((xStart >> 8) & 0x03U));
        data(static_cast<uint8_t>(xEnd & 0xFFU));
        data(static_cast<uint8_t>((xEnd >> 8) & 0x03U));

        command(0x45);
        data(static_cast<uint8_t>(yStart & 0xFFU));
        data(static_cast<uint8_t>((yStart >> 8) & 0x03U));
        data(static_cast<uint8_t>(yEnd & 0xFFU));
        data(static_cast<uint8_t>((yEnd >> 8) & 0x03U));
    }

    void setCursor(uint16_t x, uint16_t y)
    {
        command(0x4E);
        data(static_cast<uint8_t>(x & 0xFFU));
        data(static_cast<uint8_t>((x >> 8) & 0x03U));

        command(0x4F);
        data(static_cast<uint8_t>(y & 0xFFU));
        data(static_cast<uint8_t>((y >> 8) & 0x03U));
    }

    static uint8_t pixelLevel(uint8_t packed, uint8_t pixel)
    {
        const uint8_t shift = static_cast<uint8_t>(6U - pixel * 2U);
        return static_cast<uint8_t>((packed >> shift) & 0x03U);
    }

    bool writePlane(uint8_t ramCommand, const uint8_t* packed2bpp, bool lowBitPlane)
    {
        command(ramCommand);
        beginDataStream();

        size_t source = 0;
        for(size_t output = 0; output < FT02_GRAY4_PLANE_BYTES; ++output)
        {
            const uint8_t first = packed2bpp[source++];
            const uint8_t second = packed2bpp[source++];
            uint8_t planeByte = 0;

            for(uint8_t pixel = 0; pixel < 8U; ++pixel)
            {
                const uint8_t packed = pixel < 4U ? first : second;
                const uint8_t localPixel = pixel < 4U ? pixel : static_cast<uint8_t>(pixel - 4U);
                const uint8_t level = pixelLevel(packed, localPixel);
                const bool planeBit = lowBitPlane
                    ? ((level & 0x01U) == 0U)
                    : ((level & 0x02U) == 0U);
                planeByte = static_cast<uint8_t>((planeByte << 1U) | (planeBit ? 1U : 0U));
            }

            spi_.transfer(planeByte);
        }

        endDataStream();
        return source == FT02_GRAY4_FRAME_PACKED_BYTES;
    }
};

}

uint8_t* FT02_AllocateGray4Framebuffer()
{
    uint8_t* buffer = static_cast<uint8_t*>(heap_caps_malloc(
        FT02_GRAY4_FRAME_PACKED_BYTES,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    ));
    if(buffer == nullptr)
    {
        buffer = static_cast<uint8_t*>(malloc(FT02_GRAY4_FRAME_PACKED_BYTES));
    }
    return buffer;
}

void FT02_FreeGray4Framebuffer(uint8_t* frame)
{
    free(frame);
}

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
)
{
    FT02Gray4PanelReport report = {};
    report.message = "not-started";
    report.frameValid = packed2bpp != nullptr && packedBytes == FT02_GRAY4_FRAME_PACKED_BYTES;
    const uint32_t started = millis();

    if(!report.frameValid)
    {
        report.message = "invalid-four-gray-frame";
        report.elapsedMs = millis() - started;
        return report;
    }

    FT02Gray4Panel panel(spi, spiHz, pwrPin, busyPin, rstPin, dcPin, csPin);
    report.panelInitialized = panel.begin();
    if(!report.panelInitialized)
    {
        report.message = "panel-four-gray-init-failed";
        report.elapsedMs = millis() - started;
        return report;
    }

    report.refreshCompleted = panel.display(packed2bpp, packedBytes);
    // display() returns only after the refresh has completed and the panel has
    // entered deep sleep with EPD_PWR low. Treat this as one atomic commit.
    report.panelQuiesced = report.refreshCompleted;
    report.success = report.refreshCompleted && report.panelQuiesced;
    report.message = report.success ? "ok" : "panel-four-gray-refresh-or-sleep-failed";
    report.elapsedMs = millis() - started;
    return report;
}
