#pragma once

#include <Arduino.h>

// FT-02 hardware control for the dedicated Meshtastic radio coprocessor.
// Prototype wiring:
//   FT-02 GPIO6 -> Wireless Stick Lite V3 J3-14 RST / CHIP_PU
//
// GPIO6 is used open-drain. HIGH means released (high impedance); LOW means
// reset asserted. The WSL V3 board supplies the reset pull-up.

void FT02_LoRaCoprocessorBegin();
void FT02_LoRaCoprocessorReset(const char* reason);
uint32_t FT02_LoRaCoprocessorLastReleaseMs();
uint32_t FT02_LoRaCoprocessorResetCount();
