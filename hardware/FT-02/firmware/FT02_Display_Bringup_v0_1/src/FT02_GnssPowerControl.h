#pragma once

#include <Arduino.h>

// User-facing GNSS policy owned by FT-02 system mode. v2.75k92 controls the
// Core driver owns GNSS state/data directly.
void FT02_GnssPowerControlBegin();
void FT02_GnssPowerControlPoll();

bool FT02_GnssPowerDesiredEnabled();
bool FT02_GnssPowerActualKnown();
bool FT02_GnssPowerActualEnabled();
bool FT02_GnssPowerSetEnabled(bool enabled);
bool FT02_GnssPowerToggle();
bool FT02_GnssPowerWakeTransitionActive();
const char* FT02_GnssPowerDisplayText();
