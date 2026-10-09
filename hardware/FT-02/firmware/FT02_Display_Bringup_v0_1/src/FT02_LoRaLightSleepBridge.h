#pragma once
// Opt-in GPIO48 light-sleep RX trial. The existing Sleep A1.6 keyboard/UI
// wake semantics are unchanged. Never enable before active-mode RF acceptance.
void FT02_LoRaLightSleepEnter();
void FT02_LoRaLightSleepBeforeSlice();
void FT02_LoRaLightSleepAfterSlice(bool enteredSleep);
void FT02_LoRaLightSleepExit();
