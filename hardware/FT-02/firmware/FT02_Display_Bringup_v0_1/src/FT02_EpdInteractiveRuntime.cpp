#include "FT02_EpdInteractiveRuntime.h"

namespace
{
constexpr uint8_t QUEUE_CAPACITY = 24;
constexpr uint32_t BUSY_POLL_INTERVAL_MS = 8u;
// A19: keep the generic post-partial hot tail short.  The previous 2500 ms
// lifecycle tail made real-device interaction feel as if the keyboard stayed
// unavailable long after the visible refresh completed.  350 ms plus the
// measured ~140 ms SSD1677 power-off tail remains below the requested 0.5 s
// post-refresh budget, while BUSY-time key capture remains active.
constexpr uint32_t PARTIAL_HOT_IDLE_MS = 350u;

FT02InputEvent g_queue[QUEUE_CAPACITY] = {};
uint8_t g_head = 0;
uint8_t g_count = 0;
uint32_t g_dropped = 0;
uint32_t g_lastPollMs = 0;
bool g_captureActive = false;
bool g_waitForInitialRelease = false;
bool g_hotPowerOffPending = false;
uint32_t g_lastPartialCommitMs = 0;

bool pushEvent(const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_NONE) return true;
    if(g_count >= QUEUE_CAPACITY)
    {
        ++g_dropped;
        return false;
    }
    const uint8_t tail = static_cast<uint8_t>((g_head + g_count) % QUEUE_CAPACITY);
    g_queue[tail] = event;
    ++g_count;
    return true;
}

void busyCallback(const void*)
{
    if(!g_captureActive)
    {
        delay(1);
        return;
    }

    const uint32_t now = millis();
    if(static_cast<uint32_t>(now - g_lastPollMs) < BUSY_POLL_INTERVAL_MS)
    {
        delay(1);
        return;
    }
    g_lastPollMs = now;

    const FT02InputEvent event = FT02_InputPoll();
    const char currentRaw = FT02_InputCurrentRawKey();

    // The key that initiated this refresh can remain held while BUSY. Wait for
    // one real release before accepting subsequent presses into the queue.
    if(g_waitForInitialRelease)
    {
        if(currentRaw == 0) g_waitForInitialRelease = false;
        delay(1);
        return;
    }

    if(event.key != FT02_KEY_NONE) (void)pushEvent(event);
    delay(1);
}
}

void FT02_EpdInteractiveReset()
{
    g_head = 0;
    g_count = 0;
    g_dropped = 0;
    g_lastPollMs = 0;
    g_captureActive = false;
    g_waitForInitialRelease = false;
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
}

void FT02_EpdInteractiveBeginBusyCapture(FT02Display& display)
{
    g_lastPollMs = 0;
    g_waitForInitialRelease = FT02_InputCurrentRawKey() != 0;
    g_captureActive = true;
    display.epd2.setBusyCallback(busyCallback);
}

void FT02_EpdInteractiveEndBusyCapture(FT02Display& display)
{
    g_captureActive = false;
    g_waitForInitialRelease = false;
    display.epd2.setBusyCallback(nullptr);
    if(g_dropped > 0)
    {
        Serial.printf(
            "[EPD-INTERACTIVE-A2] queue overflow dropped=%lu pending=%u\n",
            static_cast<unsigned long>(g_dropped),
            static_cast<unsigned>(g_count)
        );
        g_dropped = 0;
    }
}

void FT02_EpdInteractiveFinishPartial(
    FT02Display& display,
    const char* source,
    uint32_t startedMs
)
{
    display.setFullWindow();
    g_hotPowerOffPending = true;
    g_lastPartialCommitMs = millis();
    Serial.printf(
        "[EPD-INTERACTIVE-A2] partial source=%s elapsed=%lums panel=hot hotIdle=%lums queued=%u\n",
        source != nullptr ? source : "unknown",
        static_cast<unsigned long>(g_lastPartialCommitMs - startedMs),
        static_cast<unsigned long>(PARTIAL_HOT_IDLE_MS),
        static_cast<unsigned>(g_count)
    );
}

void FT02_EpdInteractivePowerOffNow(
    FT02Display& display,
    const char* source
)
{
    display.epd2.powerOff();
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
    Serial.print("[EPD-INTERACTIVE-A2] commit complete; powerOff source=");
    Serial.println(source != nullptr ? source : "unknown");
}

bool FT02_EpdInteractivePeekQueuedInput(FT02InputEvent& event)
{
    if(g_count == 0) return false;
    event = g_queue[g_head];
    return true;
}

bool FT02_EpdInteractiveTakeQueuedInput(FT02InputEvent& event)
{
    if(!FT02_EpdInteractivePeekQueuedInput(event)) return false;
    g_head = static_cast<uint8_t>((g_head + 1u) % QUEUE_CAPACITY);
    --g_count;
    return true;
}

uint8_t FT02_EpdInteractiveQueuedInputCount()
{
    return g_count;
}

void FT02_EpdInteractiveService(FT02Display& display, uint32_t nowMs)
{
    if(!g_hotPowerOffPending) return;
    if(static_cast<uint32_t>(nowMs - g_lastPartialCommitMs) < PARTIAL_HOT_IDLE_MS) return;

    const uint32_t startedMs = millis();
    FT02_EpdInteractiveBeginBusyCapture(display);
    display.epd2.powerOff();
    FT02_EpdInteractiveEndBusyCapture(display);
    const uint32_t endedMs = millis();
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
    Serial.printf(
        "[EPD-INTERACTIVE-A2] idle powerOff elapsed=%lums queued=%u\n",
        static_cast<unsigned long>(endedMs - startedMs),
        static_cast<unsigned>(g_count)
    );
}

