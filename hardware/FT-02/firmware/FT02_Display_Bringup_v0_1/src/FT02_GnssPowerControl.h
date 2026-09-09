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

const char* FT02_GnssPowerDisplayText();
