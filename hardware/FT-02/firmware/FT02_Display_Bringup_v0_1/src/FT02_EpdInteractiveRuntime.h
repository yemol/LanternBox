#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"
#include "FT02_InputManager.h"

// Shared interactive e-paper runtime for all FT-02 interactive pages.
// Keeps CardKB polling alive while GxEPD2 waits on BUSY and delays panel
// power-off for short partial-refresh bursts so consecutive navigation does
// not repeatedly pay the panel shutdown/startup cost.

void FT02_EpdInteractiveReset();

void FT02_EpdInteractiveBeginBusyCapture(FT02Display& display);
void FT02_EpdInteractiveEndBusyCapture(FT02Display& display);

// Call after a completed partial update. Restores the default full window and
// keeps the panel hot until the idle service powers it off.
void FT02_EpdInteractiveFinishPartial(
    FT02Display& display,
    const char* source,
    uint32_t startedMs
);

// Full-screen commits still power off immediately, but keep BUSY-time input
// capture active through the shutdown transaction.
void FT02_EpdInteractivePowerOffNow(
    FT02Display& display,
    const char* source
);

// Inspect or consume keys captured while another display transaction was
// blocking. The queue is strict FIFO and is the single ordering authority for
// all interactive pages, including FieldManual.
bool FT02_EpdInteractivePeekQueuedInput(FT02InputEvent& event);
bool FT02_EpdInteractiveTakeQueuedInput(FT02InputEvent& event);
uint8_t FT02_EpdInteractiveQueuedInputCount();

// Quiesce a hot panel after a short idle interval. Safe to call every loop.
void FT02_EpdInteractiveService(FT02Display& display, uint32_t nowMs);

