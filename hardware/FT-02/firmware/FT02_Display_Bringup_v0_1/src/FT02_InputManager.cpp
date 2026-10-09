#include "FT02_InputManager.h"
#include "FT02_Log.h"

#include <Wire.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "FT02_I2CBus.h"

static uint8_t g_ft02CardKbAddress = 0x5F;
static char g_ft02LastRawKey = 0;
static uint32_t g_ft02LastRawSeenMillis = 0;
static char g_ft02LastAcceptedRawKey = 0;
static uint32_t g_ft02LastAcceptedRawKeyMillis = 0;

// CardKB2 I2C exposes translated key bytes but not release frames. The factory
// firmware does, however, use a deterministic hold-repeat pattern: first repeat
// after 300 ms, then one every 50 ms. Track that pattern instead of imposing a
// long synthetic release timeout, so fast press-release-press sequences survive.
static uint8_t g_ft02DiscreteTrackedRaw = 0;
static uint32_t g_ft02DiscreteAcceptedMs = 0;
static uint32_t g_ft02DiscreteLastSeenMs = 0;
static bool g_ft02DiscreteRepeatBurst = false;
static char g_ft02PendingRawKey = 0;

static bool g_ft02CardKbAvailable = false;
static uint32_t g_ft02LastProbeMillis = 0;
static uint32_t g_ft02LastReadFailLogMillis = 0;
static uint8_t g_ft02ReadFailCount = 0;
static bool g_ft02LowNoiseMode = false;
static bool g_ft02DiagnosticsEnabled = true;
static uint32_t g_ft02LastPhysicalPollMs = 0;
static uint32_t g_ft02LastRamRegPollMs = 0;
static uint32_t g_ft02LastBusRecoverMs = 0;
static bool g_ft02BusSuspended = false;
static bool g_ft02SclRtcOwned = false;

static int g_ft02InputSdaPin = -1;
static int g_ft02InputSclPin = -1;

static const uint32_t FT02_INPUT_PROBE_INTERVAL_MS = 3000;
static const uint32_t FT02_INPUT_FAIL_LOG_INTERVAL_MS = 3000;
static const uint8_t FT02_INPUT_MAX_READ_FAILS = 3;
static const uint32_t FT02_INPUT_SAME_KEY_REPEAT_MS = 180;
static const uint32_t FT02_INPUT_LOW_NOISE_POLL_MS = 100;
// Human keyboard input does not need an unrestricted loop-rate I2C poll.
// 20 ms = 50 Hz, still faster than CardKB2's 50 ms hold-repeat cadence.
static const uint32_t FT02_INPUT_RAMREG_POLL_MS = 20;
static const uint32_t FT02_INPUT_BUS_RECOVER_GUARD_MS = 500;

// GPIO21/SCL has exactly two owners in RC3:
//   normal runtime: digital IO matrix / Wire master
//   sleep handshake + wake: RTC IO / EXT1
// CardKB2 sleep never uses the digital GPIO interrupt path.
static void FT02_SanitizeKeyboardSclDigitalInterrupt()
{
    if(g_ft02InputSclPin < 0) return;
    const gpio_num_t scl = static_cast<gpio_num_t>(g_ft02InputSclPin);
    (void)gpio_intr_disable(scl);
    (void)gpio_set_intr_type(scl, GPIO_INTR_DISABLE);
}

static bool FT02_ReleaseKeyboardSclToDigitalInput()
{
    if(g_ft02InputSclPin < 0) return false;
    const gpio_num_t scl = static_cast<gpio_num_t>(g_ft02InputSclPin);

    if(rtc_gpio_is_valid_gpio(scl))
    {
        // EXT1 light sleep can leave the RTC pad held. ESP-IDF requires the
        // hold to be released before reconfiguring the wake pad.
        (void)rtc_gpio_hold_dis(scl);
        if(rtc_gpio_deinit(scl) != ESP_OK) return false;
    }
    g_ft02SclRtcOwned = false;

    // Erase any digital interrupt type before Wire takes the pin. This is
    // deliberately targeted to CardKB2 SCL and does not touch other GPIO IRQs.
    FT02_SanitizeKeyboardSclDigitalInterrupt();
    if(gpio_set_direction(scl, GPIO_MODE_INPUT) != ESP_OK) return false;
    if(gpio_set_pull_mode(scl, GPIO_PULLUP_ONLY) != ESP_OK) return false;
    return true;
}

static bool FT02_ClaimKeyboardSclRtc(rtc_gpio_mode_t mode)
{
    if(g_ft02InputSclPin < 0) return false;
    const gpio_num_t scl = static_cast<gpio_num_t>(g_ft02InputSclPin);
    if(!rtc_gpio_is_valid_gpio(scl)) return false;

    // Do not permit a stale digital interrupt configuration to cross the owner
    // boundary. RC3 never enables digital GPIO wake for CardKB2 SCL.
    FT02_SanitizeKeyboardSclDigitalInterrupt();

    if(rtc_gpio_init(scl) != ESP_OK) return false;
    g_ft02SclRtcOwned = true;
    (void)rtc_gpio_hold_dis(scl);

    if(rtc_gpio_pullup_en(scl) != ESP_OK) goto fail;
    if(rtc_gpio_pulldown_dis(scl) != ESP_OK) goto fail;
    if(rtc_gpio_set_direction(scl, mode) != ESP_OK) goto fail;
    return true;

fail:
    (void)rtc_gpio_hold_dis(scl);
    (void)rtc_gpio_deinit(scl);
    g_ft02SclRtcOwned = false;
    return false;
}

static int FT02_ReadKeyboardSclLevel()
{
    if(g_ft02InputSclPin < 0) return LOW;
    if(g_ft02SclRtcOwned)
        return rtc_gpio_get_level(static_cast<gpio_num_t>(g_ft02InputSclPin)) ? HIGH : LOW;
    return digitalRead(g_ft02InputSclPin);
}

// CardKB2 factory firmware timing (M5Unit-CardKB2-UserDemo):
//   first hold repeat = 300 ms, following repeats = 50 ms.
// A real fast re-press before the repeat window must be accepted immediately.
static const uint32_t FT02_INPUT_DISCRETE_BOUNCE_MS = 35;
static const uint32_t FT02_INPUT_DISCRETE_FAST_REPRESS_MS = 270;
static const uint32_t FT02_INPUT_REPEAT_START_MIN_MS = 270;
static const uint32_t FT02_INPUT_REPEAT_START_MAX_MS = 340;
static const uint32_t FT02_INPUT_REPEAT_BURST_GAP_MS = 125;
static const uint8_t FT02_INPUT_REPEAT_DRAIN_MAX = 8;

// CardKB2 real-hardware-proven A1.8 8-byte RAM-register protocol.
static const uint8_t FT02_KB_RAM_BASE = 0x00;
static const uint8_t FT02_KB_RAM_SIZE = 8;
static const uint8_t FT02_KB_RAM_MAGIC = 0xA5;
static const uint8_t FT02_KB_RAM_PROTOCOL = 0x17;
static const uint8_t FT02_KB_STATUS_READY = 0x01;
static const uint8_t FT02_KB_STATUS_SLEEP_ARMED = 0x02;

static const uint32_t FT02_KB_HANDSHAKE_SIGNAL_MS = 120;
static const uint32_t FT02_KB_HANDSHAKE_CANCEL_MS = 260;
static const uint32_t FT02_KB_HANDSHAKE_ACK_TIMEOUT_MS = 350;
static const uint32_t FT02_KB_HANDSHAKE_POST_COMMIT_GUARD_MS = 180;


static uint8_t g_ft02KbLastSeq = 0;
static bool g_ft02KbSeqInitialized = false;

// CurrentRawKey is used only as a coarse activity/release hint by sleep/map/audio
// gates. Repeat filtering itself does not depend on this synthetic release timer.
static const uint32_t FT02_INPUT_ACTIVITY_QUIET_MS = 220;

static bool FT02_IsOneShotKey(FT02InputKey key)
{
    return key == FT02_KEY_UP || key == FT02_KEY_DOWN ||
           key == FT02_KEY_LEFT || key == FT02_KEY_RIGHT ||
           key == FT02_KEY_SELECT || key == FT02_KEY_BACK || key == FT02_KEY_HELP;
}

static uint8_t FT02_KbRamXor(const uint8_t* data, size_t len)
{
    uint8_t v = 0;
    for(size_t i = 0; i < len; ++i) v ^= data[i];
    return v;
}

struct FT02KbRamSnapshot
{
    uint8_t seq;
    uint8_t key;
    uint8_t status;
    uint8_t queueDepth;
};

static bool FT02_ReadCardKbRamSnapshot(FT02KbRamSnapshot* out)
{
    if(!out) return false;
    if(g_ft02BusSuspended) return false;
    if(!FT02_I2CBusLock(30)) return false;

    Wire.beginTransmission(g_ft02CardKbAddress);
    Wire.write(FT02_KB_RAM_BASE);
    const uint8_t selectErr = Wire.endTransmission(false);
    if(selectErr != 0)
    {
        FT02_I2CBusUnlock();
        return false;
    }

    const int got = Wire.requestFrom(
        static_cast<int>(g_ft02CardKbAddress),
        static_cast<int>(FT02_KB_RAM_SIZE),
        static_cast<int>(true));

    if(got != FT02_KB_RAM_SIZE)
    {
        while(Wire.available()) (void)Wire.read();
        FT02_I2CBusUnlock();
        return false;
    }

    uint8_t ram[FT02_KB_RAM_SIZE] = {0};
    for(uint8_t i = 0; i < FT02_KB_RAM_SIZE; ++i)
    {
        if(!Wire.available())
        {
            FT02_I2CBusUnlock();
            return false;
        }
        ram[i] = static_cast<uint8_t>(Wire.read());
    }

    FT02_I2CBusUnlock();

    const bool signatureOk =
        ram[0] == FT02_KB_RAM_MAGIC &&
        ram[1] == FT02_KB_RAM_PROTOCOL;
    const bool keyOk =
        static_cast<uint8_t>(ram[3] ^ 0xFFu) == ram[4];
    const bool checksumOk =
        FT02_KbRamXor(ram, FT02_KB_RAM_SIZE - 1u) ==
        ram[FT02_KB_RAM_SIZE - 1u];

    if(!signatureOk || !keyOk || !checksumOk) return false;

    out->seq = ram[2];
    out->key = ram[3];
    out->status = ram[5];
    out->queueDepth = ram[6];
    return true;
}

static bool FT02_ProbeCardKb()
{
    FT02KbRamSnapshot snapshot = {};
    return FT02_ReadCardKbRamSnapshot(&snapshot) &&
           (snapshot.status & FT02_KB_STATUS_READY) != 0;
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
        if(!available)
        {
            if(g_ft02DiagnosticsEnabled)
                FT02_LOGW_PRINTLN("[WARN][INPUT] CardKB2 RAMREG unavailable; polling paused");
        }
    }
}

static bool FT02_RecoverSharedI2CBus()
{
    const uint32_t now = millis();
    if(static_cast<uint32_t>(now - g_ft02LastBusRecoverMs) <
       FT02_INPUT_BUS_RECOVER_GUARD_MS)
    {
        return false;
    }
    g_ft02LastBusRecoverMs = now;

    if(g_ft02BusSuspended) return false;
    if(g_ft02InputSdaPin < 0 || g_ft02InputSclPin < 0) return false;
    if(!FT02_I2CBusLock(120)) return false;

    const int sdaBefore = digitalRead(g_ft02InputSdaPin);
    const int sclBefore = digitalRead(g_ft02InputSclPin);

    Wire.end();

    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);
    if(!FT02_ReleaseKeyboardSclToDigitalInput())
    {
        FT02_I2CBusUnlock();
        return false;
    }
    delay(3);

    const bool linesReleased =
        digitalRead(g_ft02InputSdaPin) == HIGH &&
        digitalRead(g_ft02InputSclPin) == HIGH;

    bool beginOk = false;
    if(linesReleased)
    {
        beginOk = Wire.begin(g_ft02InputSdaPin, g_ft02InputSclPin);
        if(beginOk)
        {
            Wire.setClock(100000);
            Wire.setTimeOut(20);
        }
    }

    FT02_I2CBusUnlock();

    FT02_LOGW(
        "[WARN][INPUT] I2C recovery sda=%d scl=%d released=%u begin=%u\\n",
        sdaBefore,
        sclBefore,
        static_cast<unsigned>(linesReleased ? 1 : 0),
        static_cast<unsigned>(beginOk ? 1 : 0));

    return beginOk;
}

static char FT02_ReadCardKbRawKeyHardware()
{
    const uint32_t pollNow = millis();
    if(static_cast<uint32_t>(pollNow - g_ft02LastRamRegPollMs) <
       FT02_INPUT_RAMREG_POLL_MS)
    {
        return 0;
    }
    g_ft02LastRamRegPollMs = pollNow;

    if(!g_ft02CardKbAvailable)
    {
        FT02_UpdateCardKbAvailability(false);
        return 0;
    }

    FT02KbRamSnapshot snapshot = {};
    if(!FT02_ReadCardKbRamSnapshot(&snapshot))
    {
        ++g_ft02ReadFailCount;

        const uint32_t now = millis();
        if(now - g_ft02LastReadFailLogMillis >= FT02_INPUT_FAIL_LOG_INTERVAL_MS)
        {
            g_ft02LastReadFailLogMillis = now;
            if(g_ft02DiagnosticsEnabled)
                FT02_LOGW_PRINTLN("[WARN][INPUT] CardKB2 RAMREG read failed");
        }

        if(g_ft02ReadFailCount >= FT02_INPUT_MAX_READ_FAILS)
        {
            const bool recovered = FT02_RecoverSharedI2CBus();

            g_ft02CardKbAvailable = false;
            g_ft02ReadFailCount = 0;

            // Retry discovery shortly after a successful master-bus rebuild
            // instead of waiting for a storm of repeated-start failures.
            if(recovered)
            {
                g_ft02LastProbeMillis =
                    millis() - FT02_INPUT_PROBE_INTERVAL_MS;
            }
        }
        return 0;
    }

    g_ft02ReadFailCount = 0;

    if(!g_ft02KbSeqInitialized)
    {
        g_ft02KbSeqInitialized = true;
        g_ft02KbLastSeq = snapshot.seq;

        // Boot sequence 0 means no pending key. A non-zero sequence means a
        // wake event was staged while FT02 had its bus suspended; consume it.
        if(snapshot.seq == 0 || snapshot.key == 0) return 0;
        return static_cast<char>(snapshot.key);
    }

    if(snapshot.seq == g_ft02KbLastSeq || snapshot.key == 0) return 0;

    g_ft02KbLastSeq = snapshot.seq;
    return static_cast<char>(snapshot.key);
}

static char FT02_ReadCardKbRawKey()
{
    if(g_ft02PendingRawKey != 0)
    {
        const char raw = g_ft02PendingRawKey;
        g_ft02PendingRawKey = 0;
        return raw;
    }
    return FT02_ReadCardKbRawKeyHardware();
}

// When the host was busy outside the normal poll cadence, CardKB2 can have a
// backlog of 50 ms hold-repeat bytes in its 32-byte I2C queue. Probe a bounded
// number of queued bytes. Same-key bytes are stale repeats and are drained; the
// first different key is preserved for the next FT02_InputPoll() call.
static bool FT02_DrainQueuedSameDiscreteRaw(uint8_t raw)
{
    bool sawSame = false;
    for(uint8_t i = 0; i < FT02_INPUT_REPEAT_DRAIN_MAX; ++i)
    {
        const char next = FT02_ReadCardKbRawKeyHardware();
        if(next == 0) break;

        const uint8_t code = static_cast<uint8_t>(next);
        if(code == 0xFFu) continue;

        if(code == raw)
        {
            sawSame = true;
            g_ft02LastRawKey = next;
            g_ft02LastRawSeenMillis = millis();
            continue;
        }

        g_ft02PendingRawKey = next;
        break;
    }
    return sawSame;
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
    g_ft02LastRawSeenMillis = 0;
    g_ft02LastAcceptedRawKey = 0;
    g_ft02LastAcceptedRawKeyMillis = 0;
    g_ft02DiscreteTrackedRaw = 0;
    g_ft02DiscreteAcceptedMs = 0;
    g_ft02DiscreteLastSeenMs = 0;
    g_ft02DiscreteRepeatBurst = false;
    g_ft02PendingRawKey = 0;
    g_ft02ReadFailCount = 0;
    g_ft02LastReadFailLogMillis = 0;
    g_ft02LastProbeMillis = 0;
    g_ft02CardKbAvailable = false;
    g_ft02LowNoiseMode = false;
    g_ft02LastPhysicalPollMs = 0;
    g_ft02LastRamRegPollMs = 0;
    g_ft02LastBusRecoverMs = 0;
    g_ft02BusSuspended = false;
    g_ft02KbLastSeq = 0;
    g_ft02KbSeqInitialized = false;
    g_ft02SclRtcOwned = false;

    // Boot/restart invariant: put GPIO21 back on the digital IO mux and erase
    // any interrupt type before Wire claims the shared I2C bus.
    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);
    (void)FT02_ReleaseKeyboardSclToDigitalInput();

    Wire.begin(
        g_ft02InputSdaPin,
        g_ft02InputSclPin
    );

    Wire.setClock(
        100000
    );
    Wire.setTimeOut(20);
    FT02_I2CBusInit();

    FT02_UpdateCardKbAvailability(true);
    if(g_ft02DiagnosticsEnabled)
    {
        FT02_LOGD_PRINTLN("[INPUT] CardKB2 A1.8 RAMREG / 50Hz poll / SCL2PHASE RC1");
    }
}

bool FT02_InputCardKbAvailable()
{
    return g_ft02CardKbAvailable;
}



static bool FT02_SetWireOnlineLocked()
{
    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);
    if(!FT02_ReleaseKeyboardSclToDigitalInput()) return false;
    delay(3);

    if(digitalRead(g_ft02InputSdaPin) != HIGH ||
       digitalRead(g_ft02InputSclPin) != HIGH)
    {
        return false;
    }

    const bool beginOk =
        Wire.begin(g_ft02InputSdaPin, g_ft02InputSclPin);

    if(beginOk)
    {
        Wire.setClock(100000);
        Wire.setTimeOut(20);
        g_ft02BusSuspended = false;
    }

    return beginOk;
}

static bool FT02_PulseSclOutOfBandLocked(uint32_t lowMs, bool leaveRtcInput)
{
    // Wire must already be ended. SDA is never actively driven during the
    // handshake. A LOW sample aborts the pulse so REQUEST/COMMIT/CANCEL cannot
    // be mistaken for valid I2C bus traffic.
    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);
    if(digitalRead(g_ft02InputSdaPin) != HIGH) return false;

    // Drive only SCL through RTC IO open drain. This removes gpio_config(GPIO21)
    // from the exact RC2 Interrupt-WDT stack and keeps sleep ownership outside
    // the digital GPIO interrupt service.
    if(!FT02_ClaimKeyboardSclRtc(RTC_GPIO_MODE_INPUT_OUTPUT_OD)) return false;
    const gpio_num_t scl = static_cast<gpio_num_t>(g_ft02InputSclPin);

    bool pulseOk = false;
    do
    {
        // Open-drain HIGH is release, never push-pull drive.
        if(rtc_gpio_set_level(scl, 1) != ESP_OK) break;
        if(digitalRead(g_ft02InputSdaPin) != HIGH) break;

        if(rtc_gpio_set_level(scl, 0) != ESP_OK) break;
        const uint32_t lowStart = millis();
        bool sdaStayedHigh = true;
        while(static_cast<uint32_t>(millis() - lowStart) < lowMs)
        {
            if(digitalRead(g_ft02InputSdaPin) != HIGH)
            {
                sdaStayedHigh = false;
                break;
            }
            delay(1);
        }

        // Always release SCL before returning, including protocol aborts.
        if(rtc_gpio_set_level(scl, 1) != ESP_OK) break;
        if(!sdaStayedHigh) break;

        const uint32_t releaseStart = millis();
        while(FT02_ReadKeyboardSclLevel() == LOW)
        {
            if(static_cast<uint32_t>(millis() - releaseStart) >= 300u) break;
            delay(1);
        }
        if(FT02_ReadKeyboardSclLevel() == LOW) break;

        delay(12);
        pulseOk = digitalRead(g_ft02InputSdaPin) == HIGH;
    }
    while(false);

    if(pulseOk && leaveRtcInput)
    {
        // COMMIT hands SCL directly from the RTC open-drain pulse phase to the
        // RTC input wake phase. No intermediate digital GPIO ownership exists.
        if(rtc_gpio_set_direction(scl, RTC_GPIO_MODE_INPUT_ONLY) == ESP_OK)
            return true;
        pulseOk = false;
    }

    const bool releaseOk = FT02_ReleaseKeyboardSclToDigitalInput();
    return pulseOk && releaseOk;
}

static bool FT02_SendCancelAndRestoreWire()
{
    if(!FT02_I2CBusLock(200)) return false;

    if(!g_ft02BusSuspended)
    {
        Wire.end();
        g_ft02BusSuspended = true;
    }

    const bool cancelOk =
        FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_CANCEL_MS, false);

    const bool beginOk = FT02_SetWireOnlineLocked();
    FT02_I2CBusUnlock();

    if(beginOk)
    {
        g_ft02CardKbAvailable = false;
        g_ft02LastProbeMillis = 0;
        FT02_UpdateCardKbAvailability(true);
    }

    return cancelOk && beginOk;
}

bool FT02_InputRequestHostSleep()
{
    if(g_ft02BusSuspended) return true;
    if(g_ft02InputSdaPin < 0 || g_ft02InputSclPin < 0) return false;

    if(!g_ft02CardKbAvailable)
    {
        FT02_UpdateCardKbAvailability(true);
        if(!g_ft02CardKbAvailable) return false;
    }

    // Capture the final key sequence before starting the handshake. Any hold
    // repeat that arrives before CardKB2 accepts REQUEST will be consumed again
    // from the ARMED acknowledgement snapshot below.
    FT02KbRamSnapshot snapshot = {};
    if(FT02_ReadCardKbRamSnapshot(&snapshot))
    {
        g_ft02KbLastSeq = snapshot.seq;
        g_ft02KbSeqInitialized = true;
    }

    // PHASE 1: REQUEST. End Wire, send one physical 120 ms SCL LOW pulse,
    // then restore the exact normal master bus.
    if(!FT02_I2CBusLock(250)) return false;

    Wire.end();
    g_ft02BusSuspended = true;

    const bool requestPulseOk =
        FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, false);
    const bool requestWireOk =
        requestPulseOk && FT02_SetWireOnlineLocked();

    FT02_I2CBusUnlock();

    if(!requestWireOk)
    {
        (void)FT02_SendCancelAndRestoreWire();
        FT02_LOGW_PRINTLN("[WARN][INPUT] CardKB2 REQUEST pulse/restore failed");
        return false;
    }

    // Wait for CardKB2 to acknowledge REQUEST through bit1 of the same
    // real-hardware-passed 8-byte snapshot. No new I2C command format exists.
    bool armed = false;
    const uint32_t ackStart = millis();

    do
    {
        FT02KbRamSnapshot armedSnapshot = {};
        if(FT02_ReadCardKbRamSnapshot(&armedSnapshot))
        {
            if((armedSnapshot.status & FT02_KB_STATUS_SLEEP_ARMED) != 0)
            {
                // Consume the final event sequence at the exact transaction
                // boundary. The future wake key will advance it again.
                g_ft02KbLastSeq = armedSnapshot.seq;
                g_ft02KbSeqInitialized = true;
                armed = true;
                break;
            }
        }
        delay(8);
    }
    while(static_cast<uint32_t>(millis() - ackStart) <
          FT02_KB_HANDSHAKE_ACK_TIMEOUT_MS);

    if(!armed)
    {
        (void)FT02_SendCancelAndRestoreWire();
        FT02_LOGW_PRINTLN("[WARN][INPUT] CardKB2 REQUEST not acknowledged");
        return false;
    }

    // PHASE 2: COMMIT. CardKB2 is already ARMED, so the same pulse now means
    // COMMIT. Leave Wire OFF after this pulse; GPIO21 becomes the wake wire.
    if(!FT02_I2CBusLock(250))
    {
        (void)FT02_SendCancelAndRestoreWire();
        return false;
    }

    Wire.end();
    g_ft02BusSuspended = true;

    const bool commitPulseOk =
        FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, true);

    FT02_I2CBusUnlock();

    if(!commitPulseOk)
    {
        (void)FT02_SendCancelAndRestoreWire();
        FT02_LOGW_PRINTLN("[WARN][INPUT] CardKB2 COMMIT pulse failed");
        return false;
    }

    // Guard the handoff race. A genuinely new key during REQUEST/COMMIT makes
    // CardKB2 send its normal SCL wake pulse. If it happens here, abort sleep,
    // restore Wire, and let the normal input path consume that key.
    const uint32_t guardStart = millis();
    while(static_cast<uint32_t>(millis() - guardStart) <
          FT02_KB_HANDSHAKE_POST_COMMIT_GUARD_MS)
    {
        if(FT02_ReadKeyboardSclLevel() == LOW)
        {
            const uint32_t wakePulseStart = millis();
            while(FT02_ReadKeyboardSclLevel() == LOW)
            {
                if(static_cast<uint32_t>(millis() - wakePulseStart) >= 300u)
                    break;
                delay(1);
            }

            (void)FT02_SendCancelAndRestoreWire();
            FT02_LOGI_PRINTLN("[INPUT] CardKB2 key cancelled sleep during handoff");
            return false;
        }
        delay(1);
    }

    FT02_LOGD("[INPUT] CardKB2 REQUEST/ARMED/COMMIT complete keySeq=%u\n",
              static_cast<unsigned>(g_ft02KbLastSeq));
    return true;
}

bool FT02_InputSuspendBusForSleep()
{
    // COMMIT leaves Wire OFF and transfers SCL directly to RTC input ownership.
    // This method is an invariant check only; it must not route GPIO21 through
    // the digital interrupt path between COMMIT and EXT1 arm.
    if(!g_ft02BusSuspended) return false;
    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);
    if(digitalRead(g_ft02InputSdaPin) != HIGH) return false;

    if(!g_ft02SclRtcOwned)
    {
        // Recovery-only path if COMMIT ownership was unexpectedly lost.
        if(!FT02_ClaimKeyboardSclRtc(RTC_GPIO_MODE_INPUT_ONLY)) return false;
    }
    else
    {
        const gpio_num_t scl = static_cast<gpio_num_t>(g_ft02InputSclPin);
        if(rtc_gpio_set_direction(scl, RTC_GPIO_MODE_INPUT_ONLY) != ESP_OK) return false;
    }

    return FT02_ReadKeyboardSclLevel() == HIGH &&
           digitalRead(g_ft02InputSdaPin) == HIGH;
}

bool FT02_InputSleepSclRtcReady()
{
    return g_ft02BusSuspended && g_ft02SclRtcOwned;
}

bool FT02_InputSleepSclIsLow()
{
    if(!FT02_InputSleepSclRtcReady()) return false;
    return FT02_ReadKeyboardSclLevel() == LOW;
}

bool FT02_InputResumeBusAfterSleep(uint32_t timeoutMs)
{
    if(g_ft02InputSdaPin < 0 || g_ft02InputSclPin < 0) return false;

    // A late sleep-entry abort can run the normal wake lifecycle before Wire
    // was actually suspended. In that case keep the live bus untouched and
    // simply refresh CardKB2 availability.
    if(!g_ft02BusSuspended)
    {
        FT02_UpdateCardKbAvailability(true);
        return g_ft02CardKbAvailable;
    }

    const uint32_t waitStart = millis();
    pinMode(g_ft02InputSdaPin, INPUT_PULLUP);

    // EXT1 light sleep can leave the RTC wake pad held. Release hold, return
    // GPIO21 to the digital mux, and clear its digital interrupt type before
    // normal Wire ownership resumes.
    if(!FT02_ReleaseKeyboardSclToDigitalInput()) return false;

    // CardKB2 deliberately holds SCL LOW for a finite wake pulse. Never
    // restart Wire until that open-drain pulse has been released.
    while(digitalRead(g_ft02InputSclPin) == LOW)
    {
        if(static_cast<uint32_t>(millis() - waitStart) >= timeoutMs)
            return false;
        delay(1);
    }

    // CardKB2 waits a short settle interval after releasing SCL before it
    // restarts its slave peripheral. Give it room to finish that transition.
    delay(20);

    if(!FT02_I2CBusLock(100)) return false;
    const bool beginOk = Wire.begin(g_ft02InputSdaPin, g_ft02InputSclPin);
    if(beginOk)
    {
        Wire.setClock(100000);
        Wire.setTimeOut(20);
    }
    g_ft02BusSuspended = !beginOk;
    FT02_I2CBusUnlock();

    if(!beginOk) return false;

    g_ft02ReadFailCount = 0;
    g_ft02LastReadFailLogMillis = 0;
    g_ft02LastProbeMillis = 0;
    g_ft02LastPhysicalPollMs = 0;
    g_ft02LastRamRegPollMs = 0;
    g_ft02CardKbAvailable = false;

    // Keep g_ft02KbLastSeq from before sleep. The wake key advances CardKB2's
    // sequence while our bus is suspended, so the next InputPoll sees it as a
    // fresh event and the normal wake-release gate can consume it.

    const uint32_t probeStart = millis();
    do
    {
        FT02_UpdateCardKbAvailability(true);
        if(g_ft02CardKbAvailable) return true;
        delay(10);
    }
    while(static_cast<uint32_t>(millis() - probeStart) < timeoutMs);

    // The shared Wire bus is alive even if CardKB2 did not answer yet. Other
    // I2C clients may continue their normal wake lifecycle.
    return false;
}

bool FT02_InputBusSuspended()
{
    return g_ft02BusSuspended;
}

void FT02_InputSetLowNoiseMode(bool enabled)
{
    if(g_ft02LowNoiseMode == enabled) return;
    g_ft02LowNoiseMode = enabled;
    g_ft02LastPhysicalPollMs = 0;
}

void FT02_InputSetDiagnosticsEnabled(bool enabled)
{
    g_ft02DiagnosticsEnabled = enabled;
}

FT02InputEvent FT02_InputPoll()
{
    FT02InputEvent event;
    if(g_ft02LowNoiseMode)
    {
        const uint32_t now = millis();
        if(now - g_ft02LastPhysicalPollMs < FT02_INPUT_LOW_NOISE_POLL_MS)
        {
            event.key = FT02_KEY_NONE;
            event.raw = 0;
            event.command = 0;
            return event;
        }
        g_ft02LastPhysicalPollMs = now;
    }

    event.key = FT02_KEY_NONE;
    event.raw = 0;
    event.command = 0;

    const char raw = FT02_ReadCardKbRawKey();
    const uint32_t now = millis();

    if(raw == 0)
    {
        if(g_ft02LastRawKey != 0 &&
           static_cast<uint32_t>(now - g_ft02LastRawSeenMillis) >= FT02_INPUT_ACTIVITY_QUIET_MS)
        {
            g_ft02LastRawKey = 0;
        }
        return event;
    }

    // 0xFF is not a valid translated CardKB2 key. Ignore shared-bus glitches.
    if(static_cast<uint8_t>(raw) == 0xFFu)
    {
        return event;
    }

    g_ft02LastRawKey = raw;
    g_ft02LastRawSeenMillis = now;

    event.raw = raw;
    event.command = FT02_NormalizeCommandChar(raw);
    event.key = FT02_MapRawKey(raw, event.command);

    if(FT02_IsOneShotKey(event.key))
    {
        const uint8_t code = static_cast<uint8_t>(raw);

        // A different physical/action key is always immediately actionable.
        if(g_ft02DiscreteTrackedRaw != code)
        {
            g_ft02DiscreteTrackedRaw = code;
            g_ft02DiscreteAcceptedMs = now;
            g_ft02DiscreteLastSeenMs = now;
            g_ft02DiscreteRepeatBurst = false;
            g_ft02LastAcceptedRawKey = raw;
            g_ft02LastAcceptedRawKeyMillis = now;
            return event;
        }

        const uint32_t sinceAccepted = static_cast<uint32_t>(now - g_ft02DiscreteAcceptedMs);
        const uint32_t sinceSeen = static_cast<uint32_t>(now - g_ft02DiscreteLastSeenMs);
        g_ft02DiscreteLastSeenMs = now;

        // Electrical/bus duplicate, not a human second press.
        if(sinceAccepted <= FT02_INPUT_DISCRETE_BOUNCE_MS)
        {
            event.key = FT02_KEY_NONE;
            event.raw = 0;
            event.command = 0;
            return event;
        }

        if(g_ft02DiscreteRepeatBurst)
        {
            // Held-key repeats arrive every 50 ms. Keep suppressing while that
            // cadence is alive. After a real quiet gap, probe the CardKB queue
            // once to distinguish a new press from a stale repeat backlog.
            if(sinceSeen <= FT02_INPUT_REPEAT_BURST_GAP_MS)
            {
                event.key = FT02_KEY_NONE;
                event.raw = 0;
                event.command = 0;
                return event;
            }

            if(FT02_DrainQueuedSameDiscreteRaw(code))
            {
                g_ft02DiscreteLastSeenMs = millis();
                event.key = FT02_KEY_NONE;
                event.raw = 0;
                event.command = 0;
                return event;
            }

            // Quiet gap and no repeat backlog: this is a genuine new press.
            g_ft02DiscreteRepeatBurst = false;
            g_ft02DiscreteAcceptedMs = now;
            g_ft02LastAcceptedRawKey = raw;
            g_ft02LastAcceptedRawKeyMillis = now;
            return event;
        }

        // CardKB2 cannot generate a hold repeat before 300 ms. Therefore a
        // second same-key event comfortably before that point is a genuine
        // rapid press-release-press and must not be swallowed.
        if(sinceAccepted < FT02_INPUT_DISCRETE_FAST_REPRESS_MS)
        {
            g_ft02DiscreteAcceptedMs = now;
            g_ft02LastAcceptedRawKey = raw;
            g_ft02LastAcceptedRawKeyMillis = now;
            return event;
        }

        // The first firmware hold repeat is nominally at 300 ms. Suppress this
        // narrow window and enter repeat-burst mode; following 50 ms repeats are
        // then rejected without imposing any 650 ms synthetic release lock.
        if(sinceAccepted >= FT02_INPUT_REPEAT_START_MIN_MS &&
           sinceAccepted <= FT02_INPUT_REPEAT_START_MAX_MS)
        {
            g_ft02DiscreteRepeatBurst = true;
            (void)FT02_DrainQueuedSameDiscreteRaw(code);
            g_ft02DiscreteLastSeenMs = millis();
            event.key = FT02_KEY_NONE;
            event.raw = 0;
            event.command = 0;
            return event;
        }

        // If we were unable to poll while another operation was blocking, a
        // held key leaves a train of 50 ms repeats in CardKB2's I2C queue. A
        // queued same byte is strong evidence of that stale repeat train.
        if(FT02_DrainQueuedSameDiscreteRaw(code))
        {
            g_ft02DiscreteRepeatBurst = true;
            g_ft02DiscreteLastSeenMs = millis();
            event.key = FT02_KEY_NONE;
            event.raw = 0;
            event.command = 0;
            return event;
        }

        // Long enough gap, no repeat cadence, no queue backlog: normal new tap.
        g_ft02DiscreteAcceptedMs = now;
        g_ft02LastAcceptedRawKey = raw;
        g_ft02LastAcceptedRawKeyMillis = now;
        return event;
    }

    // Text entry retains the established short same-character guard. This is
    // separate from navigation/action filtering so UI fixes do not damage IME.
    if(raw == g_ft02LastAcceptedRawKey &&
       static_cast<uint32_t>(now - g_ft02LastAcceptedRawKeyMillis) < FT02_INPUT_SAME_KEY_REPEAT_MS)
    {
        event.key = FT02_KEY_NONE;
        event.raw = 0;
        event.command = 0;
        return event;
    }

    g_ft02LastAcceptedRawKey = raw;
    g_ft02LastAcceptedRawKeyMillis = now;
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
