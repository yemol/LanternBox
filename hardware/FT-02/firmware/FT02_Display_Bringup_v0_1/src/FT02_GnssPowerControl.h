#pragma once

#include <Arduino.h>

// User-facing GNSS software power policy. LR01 remains the hardware authority;
// FT-02 stores the desired state and reconciles it when Host UART is online.
void FT02_GnssPowerControlBegin();
void FT02_GnssPowerControlPoll();

bool FT02_GnssPowerDesiredEnabled();
bool FT02_GnssPowerActualKnown();
bool FT02_GnssPowerActualEnabled();
bool FT02_GnssPowerSetEnabled(bool enabled);
bool FT02_GnssPowerToggle();

// True while an outdoor-mode GNSS wake request is still inside its startup
// grace window. UI should show "唤醒中" instead of a transient offline alarm.
bool FT02_GnssPowerWakeTransitionActive();

const char* FT02_GnssPowerDisplayText();
