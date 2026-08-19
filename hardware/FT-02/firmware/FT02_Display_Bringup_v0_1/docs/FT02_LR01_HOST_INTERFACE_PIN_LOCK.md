# FT-02 Core ↔ LR01 Host Interface Pin Lock

**Status:** LOCKED CURRENT HARDWARE/FIRMWARE BASELINE  
**Date:** 2026-08-17  
**Firmware baseline inspected:** FT-02 v2.74o (host pin definitions unchanged)

## 1. Purpose

This document is the authoritative pin reference for the digital control link between the FT-02 Core MCU and the current LoRa coprocessor / future FT02-LR01 MCU. It exists specifically to prevent older prototype pin maps from being reused in PCB work.

## 2. Locked connection table

| Net | FT-02 Core ESP32-S3 | LR01 / current WSL V3 side | Direction | Electrical role |
|---|---:|---:|---|---|
| `LR01_HOST_RX` | **GPIO7** | **GPIO17 / UART TX** | LR01 → Core | UART receive at Core |
| `LR01_HOST_TX` | **GPIO13** | **GPIO18 / UART RX** | Core → LR01 | UART transmit from Core |
| `LR01_RESET_N` | **GPIO6** | **RST / CHIP_PU / EN** | Core → LR01 | Active-low hardware reset |
| `GND` | GND | GND | common | Signal reference |

UART parameters:

```text
115200 baud
8 data bits
No parity
1 stop bit
3.3 V logic
```

Physical signal relationship:

```text
FT-02 Core                         LR01 / WSL V3

GPIO13  UART TX  ----------------> GPIO18 UART RX
GPIO7   UART RX  <---------------- GPIO17 UART TX
GPIO6   RESET_N  ----------------> RST / CHIP_PU / EN
GND              ----------------- GND
```

## 3. Reset electrical behavior

Current firmware defines Core GPIO6 as the LoRa coprocessor reset control. The implementation uses open-drain output behavior:

```text
GPIO6 LOW      -> assert reset
GPIO6 released -> LR01/WSL pull-up returns CHIP_PU/EN high
```

The current reset pulse is generated in `src/FT02_LoRaCoprocessor.cpp`. The future LR01 PCB shall preserve the ability for the Core to hard-reset the LR01 MCU even when UART/software communication is unavailable.

## 4. Source-of-truth evidence

The pin lock is derived directly from the current firmware tree:

### `src/FT02_LoRaTransport.cpp`

```cpp
constexpr int FT02_LORA_RX_PIN = 7;
constexpr int FT02_LORA_TX_PIN = 13;
```

### `src/FT02_LoRaTransport.h`

```text
FT-02 RX = GPIO7  <- Wireless Stick Lite V3 TX / GPIO17
FT-02 TX = GPIO13 -> Wireless Stick Lite V3 RX / GPIO18
FT-02 RST= GPIO6  -> Wireless Stick Lite V3 J3-14 RST / CHIP_PU
```

### `src/FT02_LoRaCoprocessor.cpp`

```cpp
constexpr int FT02_LORA_RESET_PIN = 6;
```

The same runtime baseline prints `UART2 started RX=7 TX=13 baud=115200`.

## 5. Explicitly obsolete historical mapping

The following older prototype/mainboard mapping is **OBSOLETE** for new PCB work:

```text
Core GPIO4 = LoRa UART RX   [OBSOLETE]
Core GPIO5 = LoRa UART TX   [OBSOLETE]
Core GPIO6 = AUX            [OBSOLETE]
Core GPIO7 = RESET          [OBSOLETE]
```

Do not copy it into FT02-LR01 or future Core PCB revisions.

## 6. Change-control rule

A Core ↔ LR01 pin assignment is not considered changed until all of the following are updated together:

1. Firmware pin constants.
2. Related source header comments.
3. This Pin Lock document.
4. Root `README.md`.
5. Schematic/net names and connector table, when PCB files exist.
6. Real-device validation log.

If any of these disagree, **stop PCB release and resolve the conflict before routing or fabrication**.

## 7. Future LR01 naming

For the custom LR01 design, use unambiguous net names:

```text
LR01_HOST_TX   = LR01 MCU TX -> Core GPIO7
LR01_HOST_RX   = LR01 MCU RX <- Core GPIO13
LR01_RESET_N   = Core GPIO6 -> LR01 MCU EN/RESET
```

Do not name connector pins only `TX` / `RX` without a device-side prefix.
