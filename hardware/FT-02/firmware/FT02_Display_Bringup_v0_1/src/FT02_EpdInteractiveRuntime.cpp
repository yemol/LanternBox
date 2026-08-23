#include "FT02_EpdInteractiveRuntime.h"

namespace
{
constexpr uint8_t QUEUE_CAPACITY = 24;
constexpr uint32_t BUSY_POLL_INTERVAL_MS = 8u;
constexpr uint32_t PARTIAL_HOT_IDLE_MS = 2500u;

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
            "[EPD-INTERACTIVE-A1] queue overflow dropped=%lu pending=%u\n",
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
        "[EPD-INTERACTIVE-A1] partial source=%s elapsed=%lums panel=hot queued=%u\n",
        source != nullptr ? source : "unknown",
        static_cast<unsigned long>(g_lastPartialCommitMs - startedMs),
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
    Serial.print("[EPD-INTERACTIVE-A1] commit complete; powerOff source=");
    Serial.println(source != nullptr ? source : "unknown");
}

bool FT02_EpdInteractiveTakeQueuedInput(FT02InputEvent& event)
{
    if(g_count == 0) return false;
    event = g_queue[g_head];
    g_head = static_cast<uint8_t>((g_head + 1u) % QUEUE_CAPACITY);
    --g_count;
    return true;
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
        "[EPD-INTERACTIVE-A1] idle powerOff elapsed=%lums queued=%u\n",
        static_cast<unsigned long>(endedMs - startedMs),
        static_cast<unsigned>(g_count)
    );
}

