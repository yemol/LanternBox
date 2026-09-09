#pragma once

#include "FT02_FontPackRenderer.h"

// Power down the e-paper high-voltage domain after a completed commit.
inline void FT02_EpdPowerOffAfterCommit(FT02Display& display)
{
    display.epd2.powerOff();
}
