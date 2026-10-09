#include "FT02_EpdInteractiveRuntime.h"

namespace
{
constexpr uint8_t QUEUE_CAPACITY = 24;
constexpr uint32_t BUSY_POLL_INTERVAL_MS = 8u;
// Keep the panel powered briefly after partial refreshes so rapid navigation
// avoids repeated shutdown/startup cycles while BUSY-time input remains active.
constexpr uint32_t PARTIAL_HOT_IDLE_MS = 350u;

FT02InputEvent g_queue[QUEUE_CAPACITY] = {};
uint8_t g_head = 0;
uint8_t g_count = 0;
uint32_t g_lastPollMs = 0;
bool g_captureActive = false;
bool g_hotPowerOffPending = false;
uint32_t g_lastPartialCommitMs = 0;

bool pushEvent(const FT02InputEvent& event)
{
    if(event.key == FT02_KEY_NONE) return true;
    if(g_count >= QUEUE_CAPACITY) return false;
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

    // InputManager A2 owns held-repeat suppression. Do not add a second
    // synthetic "wait for release" gate here, because CardKB2 I2C has no
    // release frames and that gate used to swallow legitimate rapid re-presses
    // while the EPD was BUSY.
    const FT02InputEvent event = FT02_InputPoll();
    if(event.key != FT02_KEY_NONE) (void)pushEvent(event);
    delay(1);
}
}

void FT02_EpdInteractiveReset()
{
    g_head = 0;
    g_count = 0;
    g_lastPollMs = 0;
    g_captureActive = false;
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
}

void FT02_EpdInteractiveBeginBusyCapture(FT02Display& display)
{
    g_lastPollMs = 0;
    g_captureActive = true;
    display.epd2.setBusyCallback(busyCallback);
}

void FT02_EpdInteractiveEndBusyCapture(FT02Display& display)
{
    g_captureActive = false;
    display.epd2.setBusyCallback(nullptr);
}

void FT02_EpdInteractiveFinishPartial(FT02Display& display)
{
    display.setFullWindow();
    g_hotPowerOffPending = true;
    g_lastPartialCommitMs = millis();
}

void FT02_EpdInteractivePowerOffNow(FT02Display& display)
{
    display.epd2.powerOff();
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
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

    FT02_EpdInteractiveBeginBusyCapture(display);
    display.epd2.powerOff();
    FT02_EpdInteractiveEndBusyCapture(display);
    g_hotPowerOffPending = false;
    g_lastPartialCommitMs = 0;
}

