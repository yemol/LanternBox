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
