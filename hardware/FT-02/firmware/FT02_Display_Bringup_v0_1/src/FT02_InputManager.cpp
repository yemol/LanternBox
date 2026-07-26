#include "FT02_InputManager.h"

#include <Wire.h>

static uint8_t g_ft02CardKbAddress = 0x5F;
static char g_ft02LastRawKey = 0;
static uint32_t g_ft02LastRawKeyMillis = 0;

static bool g_ft02CardKbAvailable = false;
static uint32_t g_ft02LastProbeMillis = 0;
static uint32_t g_ft02LastReadFailLogMillis = 0;
static uint8_t g_ft02ReadFailCount = 0;

static int g_ft02InputSdaPin = -1;
static int g_ft02InputSclPin = -1;

static const uint32_t FT02_INPUT_PROBE_INTERVAL_MS = 3000;
static const uint32_t FT02_INPUT_FAIL_LOG_INTERVAL_MS = 3000;
static const uint8_t FT02_INPUT_MAX_READ_FAILS = 3;
static const uint32_t FT02_INPUT_SAME_KEY_REPEAT_MS = 180;

static bool FT02_ProbeCardKb()
{
    Wire.beginTransmission(
        g_ft02CardKbAddress
    );

    uint8_t error = Wire.endTransmission();

    return error == 0;
}

static void FT02_UpdateCardKbAvailability(
    bool forceProbe
)
{
    uint32_t now = millis();

    if(!forceProbe)
    {
        if(now - g_ft02LastProbeMillis < FT02_INPUT_PROBE_INTERVAL_MS)
        {
            return;
        }
    }

    g_ft02LastProbeMillis = now;

    bool available = FT02_ProbeCardKb();

    if(available != g_ft02CardKbAvailable)
    {
        g_ft02CardKbAvailable = available;

        if(g_ft02CardKbAvailable)
        {
            Serial.print("FT02 InputManager: CardKB2 detected at 0x");
            Serial.print(g_ft02CardKbAddress, HEX);
            Serial.print(" SDA=");
            Serial.print(g_ft02InputSdaPin);
            Serial.print(" SCL=");
            Serial.println(g_ft02InputSclPin);
        }
        else
        {
            Serial.println("FT02 InputManager: CardKB2 not detected, keyboard polling paused");
        }
    }
}

static char FT02_ReadCardKbRawKey()
{
    if(!g_ft02CardKbAvailable)
    {
        FT02_UpdateCardKbAvailability(false);
        return 0;
    }

    uint8_t received = Wire.requestFrom(
        g_ft02CardKbAddress,
        static_cast<uint8_t>(1)
    );

    if(received < 1 || Wire.available() <= 0)
    {
        g_ft02ReadFailCount++;

        uint32_t now = millis();

        if(now - g_ft02LastReadFailLogMillis >= FT02_INPUT_FAIL_LOG_INTERVAL_MS)
        {
            g_ft02LastReadFailLogMillis = now;
            Serial.println("FT02 InputManager: CardKB2 read failed");
        }

        if(g_ft02ReadFailCount >= FT02_INPUT_MAX_READ_FAILS)
        {
            g_ft02CardKbAvailable = false;
            g_ft02ReadFailCount = 0;
            Serial.println("FT02 InputManager: CardKB2 polling paused, will re-probe fixed pins");
        }

        return 0;
    }

    g_ft02ReadFailCount = 0;

    char raw = static_cast<char>(Wire.read());

    if(raw == 0)
    {
        return 0;
    }

    return raw;
}

static FT02InputKey FT02_MapRawKey(
    char raw
)
{
    // Current software navigation layout:
    //
    //     D
    //   Z X C
    //
    // D = Up, Z = Left, X = Down, C = Right.
    //
    // This is an input-layer mapping only. UI pages consume FT02InputKey and
    // do not depend on the physical key source.
    if(raw == 'd' || raw == 'D') return FT02_KEY_UP;
    if(raw == 'z' || raw == 'Z') return FT02_KEY_LEFT;
    if(raw == 'x' || raw == 'X') return FT02_KEY_DOWN;
    if(raw == 'c' || raw == 'C') return FT02_KEY_RIGHT;

    if(raw == '\r' || raw == '\n' || raw == ' ')
    {
        return FT02_KEY_SELECT;
    }

    if(raw == 'h' || raw == 'H')
    {
        return FT02_KEY_HELP;
    }

    if(raw == 0x08 || raw == 0x1B || raw == 'b' || raw == 'B')
    {
        return FT02_KEY_BACK;
    }

    return FT02_KEY_CHAR;
}

void FT02_InputBegin(
    int sdaPin,
    int sclPin,
    uint8_t cardKbAddress
)
{
    g_ft02CardKbAddress = cardKbAddress;
    g_ft02InputSdaPin = sdaPin;
    g_ft02InputSclPin = sclPin;

    g_ft02LastRawKey = 0;
    g_ft02LastRawKeyMillis = 0;
    g_ft02ReadFailCount = 0;
    g_ft02LastReadFailLogMillis = 0;
    g_ft02LastProbeMillis = 0;
    g_ft02CardKbAvailable = false;

    Wire.begin(
        g_ft02InputSdaPin,
        g_ft02InputSclPin
    );

    Wire.setClock(
        100000
    );

    Serial.print("FT02 InputManager: fixed CardKB2 pins SDA=");
    Serial.print(g_ft02InputSdaPin);
    Serial.print(" SCL=");
    Serial.print(g_ft02InputSclPin);
    Serial.print(" addr=0x");
    Serial.println(g_ft02CardKbAddress, HEX);

    FT02_UpdateCardKbAvailability(true);
}

FT02InputEvent FT02_InputPoll()
{
    FT02InputEvent event;
    event.key = FT02_KEY_NONE;
    event.raw = 0;

    char raw = FT02_ReadCardKbRawKey();

    if(raw == 0)
    {
        g_ft02LastRawKey = 0;
        return event;
    }

    uint32_t now = millis();

    if(raw == g_ft02LastRawKey)
    {
        if(now - g_ft02LastRawKeyMillis < FT02_INPUT_SAME_KEY_REPEAT_MS)
        {
            return event;
        }
    }

    g_ft02LastRawKey = raw;
    g_ft02LastRawKeyMillis = now;

    event.raw = raw;
    event.key = FT02_MapRawKey(raw);

    return event;
}

const char* FT02_InputKeyName(
    FT02InputKey key
)
{
    switch(key)
    {
        case FT02_KEY_UP: return "UP";
        case FT02_KEY_DOWN: return "DOWN";
        case FT02_KEY_LEFT: return "LEFT";
        case FT02_KEY_RIGHT: return "RIGHT";
        case FT02_KEY_SELECT: return "SELECT";
        case FT02_KEY_BACK: return "BACK";
        case FT02_KEY_HELP: return "HELP";
        case FT02_KEY_CHAR: return "CHAR";
        case FT02_KEY_NONE:
        default:
            return "NONE";
    }
}
