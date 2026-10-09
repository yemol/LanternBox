#pragma once

// FT-02 v2.75k92l: NON-TRANSMITTING hardware bring-up probe.
// This is the production Core-side SX1262 hardware probe/control surface.
// This preflight owns only AW9523 reset/RF-path setup and BUSY/DIO1 checks.
// EPD uses FAST software SPI in v2.77, so the SX1262 active backend owns HSPI
// on its existing GPIO38/39/40/7 wiring.
void FT02_LoRaHwProbeBegin();
void FT02_LoRaHwProbePoll();

// The hardware-SPI radio backend can only start after AW reset/RF preflight passes.
bool FT02_LoRaHwProbeReady();

// Once the RadioLib backend starts, periodic probe SPI must stop.
void FT02_LoRaHwProbeSetRadioOwned(bool owned);

// Runtime RF switch control for Core1262-LF. RESET stays released in both modes.
// IDLE: RXEN=0, TXEN=0. RX: RXEN=1, TXEN=0. TX: RXEN=0, TXEN=1.
bool FT02_LoRaHwProbeSetRfIdle();
bool FT02_LoRaHwProbeSetRfReceive();
bool FT02_LoRaHwProbeSetRfTransmit();
bool FT02_LoRaHwProbeRfTransmitActive();
