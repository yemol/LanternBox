# FT-02 Core ↔ LR01 Host Interface Pin Lock

**Status:** current v2.75k91 runtime contract

## Purpose

This document records the UART link that the current FT-02 firmware actually uses for the LR01 host connection. Older prototype reset/pin maps must not be treated as current firmware contracts unless they are re-verified against hardware and source.

## Current UART connection

| Net | FT-02 Core ESP32-S3 | Direction | Role |
|---|---:|---|---|
| `LR01_HOST_RX` | GPIO7 | LR01 → Core | Core UART receive |
| `LR01_HOST_TX` | GPIO13 | Core → LR01 | Core UART transmit |
| `GND` | GND | common | signal reference |

UART settings:

```text
115200 baud
8 data bits
No parity
1 stop bit
3.3 V logic
```

Source of truth: `src/FT02_LR01HostRuntime.cpp`:

```cpp
constexpr int FT02_LR01_RX_PIN = 7;
constexpr int FT02_LR01_TX_PIN = 13;
constexpr uint32_t FT02_LR01_BAUD = 115200;
```

## Reset/control line

The current v2.75k91 `FT02_LR01HostRuntime` does not define or drive a dedicated LR01 reset GPIO. Older documents that assign Core GPIO6 as an LR01 reset signal describe an earlier architecture and are not a current software guarantee. Any future PCB use of a hardware reset line must be re-locked when LoRa/GNSS ownership is migrated back to Core.

## Power-management boundary

The current host runtime can pause/resume UART ingestion around FT-02 Sleep, but this is not equivalent to powering down LR01, LoRa or GNSS. Deep power ownership is intentionally deferred until the planned Core migration.
