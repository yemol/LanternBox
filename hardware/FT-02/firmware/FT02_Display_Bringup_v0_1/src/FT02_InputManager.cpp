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

static char FT02_NormalizeCommandChar(
    char raw
)
{
    const uint8_t code = static_cast<uint8_t>(raw);

    // CardKB2 I2C mode returns the character after Aa/Sym/Fn processing.
    // FT-02 hotkeys must therefore not rely on receiving the base letter.
    // Keep event.raw untouched; this second channel is only for UI commands.
    switch(code)
    {
        // Sym-layer aliases for FT-02 command keys.
        case '^': return 'd'; // D / Up
        case '{': return 'a'; // A / auto track
        case '\\': return 'r'; // R / reconnect or record
        case '=': return 'p'; // P / point or playback
        case '"': return 'h'; // H / help
        case ':': return 'l'; // L / location log
        case '>': return 'b'; // B / back
        case '/': return 't'; // T / delete
        case '[': return 'f'; // F / map cache rebuild
        default:
            break;
    }

    if(code >= 'A' && code <= 'Z')
    {
        return static_cast<char>(code - 'A' + 'a');
    }

    // CardKB v1.1 modifier-layer codes are non-ASCII and intentionally have
    // no ASCII command character. Normal physical arrows are mapped below;
    // Fn-layer arrows are consumed by feature-specific handlers from raw.
    if(code >= 0x80) return 0;

    return static_cast<char>(code);
}

static FT02InputKey FT02_MapRawKey(
    char raw,
    char command
)
{
    const uint8_t code = static_cast<uint8_t>(raw);

    // Native CardKB v1.1 physical direction bytes (normal layer).
    // Official firmware maps LEFT/UP/DOWN/RIGHT to 180..183 (0xB4..0xB7).
    // Fn+LEFT/Fn+RIGHT are different bytes (0x98/0xA5) and are intentionally
    // left as raw feature shortcuts rather than global navigation keys.
    if(code == 0xB4) return FT02_KEY_LEFT;
    if(code == 0xB5) return FT02_KEY_UP;
    if(code == 0xB6) return FT02_KEY_DOWN;
    if(code == 0xB7) return FT02_KEY_RIGHT;

    // Current FT-02 software navigation layout:
    //
    //     D
    //   Z X C
    //
    // command is already normalized across Aa/Sym state, so Sym+D ('^')
    // still produces UP instead of being dropped as an unrelated character.
    if(command == 'd') return FT02_KEY_UP;
    if(command == 'z') return FT02_KEY_LEFT;
    if(command == 'x') return FT02_KEY_DOWN;
    if(command == 'c') return FT02_KEY_RIGHT;

    if(raw == '\r' || raw == '\n' || raw == ' ')
    {
        return FT02_KEY_SELECT;
    }

    if(command == 'h')
    {
        return FT02_KEY_HELP;
    }

    if(code == 0x08 || code == 0x1B || command == 'b')
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

bool FT02_InputCardKbAvailable()
{
    return g_ft02CardKbAvailable;
}

FT02InputEvent FT02_InputPoll()
{
    FT02InputEvent event;
    event.key = FT02_KEY_NONE;
    event.raw = 0;
    event.command = 0;

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
    event.command = FT02_NormalizeCommandChar(raw);
    event.key = FT02_MapRawKey(raw, event.command);

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

char FT02_InputCurrentRawKey()
{
    return g_ft02LastRawKey;
}
