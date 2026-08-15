#pragma once

#include <Arduino.h>

// FT-02 <-> Meshtastic radio transport.
// Frozen UART wiring:
//   FT-02 RX = GPIO7  <- Wireless Stick Lite V3 TX / GPIO17
//   FT-02 TX = GPIO13 -> Wireless Stick Lite V3 RX / GPIO18
//   FT-02 RST= GPIO6  -> Wireless Stick Lite V3 J3-14 RST / CHIP_PU
// Serial module on the radio: PROTO @ 115200 baud.
//
// The wire protocol and want_config frame intentionally retain the v2.66b
// golden baseline that has repeatedly passed on real hardware.

void FT02_LoRaTransportBegin();
void FT02_LoRaTransportPoll();

bool FT02_LoRaTransportLinkUp();
bool FT02_LoRaTransportConfigComplete();
uint32_t FT02_LoRaTransportFrameCount();
uint32_t FT02_LoRaTransportResetCount();

// Send an already-encoded ToRadio protobuf over the frozen 0x94/0xC3 serial framing.
// Returns false unless the initial NodeDB sync is fully ready.
bool FT02_LoRaTransportSendToRadio(const uint8_t* protobuf, size_t length);

// User/health-triggered deterministic recovery: reset only the WSL coprocessor,
// then rerun the proven v2.66b full-sync cycle.
void FT02_LoRaTransportForceResync(const char* reason);
