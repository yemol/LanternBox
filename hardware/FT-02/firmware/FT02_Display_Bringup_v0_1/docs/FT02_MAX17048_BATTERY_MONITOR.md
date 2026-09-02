# FT-02 MAX17048 Battery Monitor A1

## Hardware assumption

- Fuel gauge: MAX17048 / MRS182A-compatible breakout.
- Battery: 1S Li-ion/LiPo, nominal 3.7 V, full 4.2 V.
- Shared FT-02 I2C bus: SDA GPIO47 / SCL GPIO21.
- MAX17048 address: `0x36`.
- A1 does not require the INT or QStart pins.

The monitor board is a fuel gauge, not a load switch or charger controller. The two battery ports on common MAX17048 breakouts are electrically equivalent pass-through connections. Firmware power policy can warn the user, but this module alone cannot disconnect the FT-02 load or stop charging.

## Firmware behavior

- Poll every 1 second when audio is idle so charge/battery-power transitions reach the status bar promptly.
- Uses the existing `FT02_I2CBus` mutex so CardKB2 / ES8311 / MAX17048 cannot collide on `Wire`.
- Reads VCELL, SOC, CRATE, VERSION, CHIPID and hibernate state.
- Status bar shows real rounded SOC instead of the old fixed `86%`.
- Status bar state text is limited to two valid power-source states:
  - charging: `充电中`
  - battery powered: `供电中`
  - unavailable: `未检测` / `异常`
- E-paper battery block is refreshed only if rounded SOC or state text changes.
- Battery partial refresh is suppressed on map, FieldManual/Library and while audio is busy; cache is drawn at the next normal full frame.
- Device Status / System Self-Test reports actual SOC, voltage and charge/discharge rate.

## Gauge policy

A1 intentionally does **not** issue reset or QuickStart at boot and does not force MAX17048 sleep/hibernate. The ModelGauge estimate is therefore not disturbed by an automatic recalibration command under load. MAX17048 default automatic hibernation remains active.

## Low-battery control scope

The 20% and 10% thresholds are software warning bands only. A1 does not automatically disable WiFi, LoRa, GNSS, audio, recording or the display, because those functions may be mission-critical. If a future hardware load switch is added, an explicit shutdown/power-domain policy can be built on top of `FT02_BatteryBandCurrent()`.
