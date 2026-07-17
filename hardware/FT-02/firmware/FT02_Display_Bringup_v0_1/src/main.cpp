#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>

// FT-02 e-paper pin mapping
constexpr int EPD_PWR  = 6;
constexpr int EPD_BUSY = 7;
constexpr int EPD_RST  = 8;
constexpr int EPD_DC   = 9;
constexpr int EPD_CS   = 10;
constexpr int EPD_MOSI = 11;
constexpr int EPD_SCK  = 12;

// Waveshare 4.26-inch e-Paper
// Panel: GDEQ0426T82
// Controller: SSD1677
// Physical resolution: 480 x 800
// Landscape resolution with rotation 2: 800 x 480
GxEPD2_BW<
    GxEPD2_426_GDEQ0426T82,
    GxEPD2_426_GDEQ0426T82::HEIGHT
> display(
    GxEPD2_426_GDEQ0426T82(
        EPD_CS,
        EPD_DC,
        EPD_RST,
        EPD_BUSY
    )
);

void drawStartupScreen()
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        display.setTextSize(3);
        display.setCursor(70, 100);
        display.println("LanternBox");

        display.setTextSize(2);
        display.setCursor(70, 160);
        display.println("FT-02 DISPLAY TEST");

        display.setCursor(70, 215);
        display.println("ESP32-S3 ONLINE");

        display.setCursor(70, 270);
        display.println("800 x 480 E-PAPER");

        display.drawRect(40, 45, 720, 330, GxEPD_BLACK);

        display.drawLine(40, 320, 760, 320, GxEPD_BLACK);

        display.setTextSize(1);
        display.setCursor(70, 350);
        display.println("DISPLAY BRING-UP v0.1");

    }
    while (display.nextPage());
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("[FT02] Boot");
    Serial.println("[FT02] Enabling display power");

    pinMode(EPD_PWR, OUTPUT);
    digitalWrite(EPD_PWR, HIGH);
    delay(200);

    Serial.println("[FT02] Starting SPI");

    SPI.begin(
        EPD_SCK,
        -1,
        EPD_MOSI,
        EPD_CS
    );

    Serial.println("[FT02] Initializing e-paper");

    display.init(
        115200,
        true,
        2,
        false
    );

    display.setRotation(2);

    Serial.printf(
        "[FT02] Display size: %d x %d\n",
        display.width(),
        display.height()
    );

    drawStartupScreen();

    Serial.println("[FT02] Display refresh complete");

    display.hibernate();

    Serial.println("[FT02] Display hibernating");
}

void loop()
{
    delay(1000);
}