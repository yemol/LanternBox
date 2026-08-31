#pragma once

#include <Arduino.h>
#include "FT02_FontPackRenderer.h"

// Close the panel's internal high-voltage drive immediately after each
// completed GxEPD2 transaction. E-paper retains the image without power.
// Keeping this operation in one helper prevents individual UI pages from
// forgetting to end the display lifecycle.
inline void FT02_EpdPowerOffAfterCommit(
    FT02Display& display,
    const char* source
)
{
    display.epd2.powerOff();

    Serial.print("[EPD] commit complete; powerOff source=");
    Serial.println(source != nullptr ? source : "unknown");
}

// Keep the e-paper high-voltage domain warm across a sequence of small
// playback-timer partial updates. The panel image still commits normally,
// but we deliberately avoid powerOff()/powerOn() cycling while the speaker
// PA is active. The normal audio-list redraw after playback owns the final
// safe power-off once the PA has already been shut down.
inline void FT02_EpdKeepHotAfterCommit(
    FT02Display& display,
    const char* source
)
{
    (void)display;
    Serial.print("[EPD] commit complete; keepHot source=");
    Serial.println(source != nullptr ? source : "unknown");
}
