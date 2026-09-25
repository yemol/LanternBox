# USB / Power Bank Policy - Preliminary

HUSB238A has official legacy SDP/CDP/DCP detection and I2C mode address selection. It shares SDA/SCL with analog voltage/current straps in GPIO mode. The inspected document also describes fuse-dependent voltage selection. Therefore a GPIO-mode 5V resistor example must not be copied onto the I2C bus. Verify the correct part's boot/reset/fuse behavior before claiming 5V-only operation. Disable HVDCP for this 5V-only design and whitelist only 5V fixed PDO requests. [HUSB238A datasheet, tables 6-9 and legacy detection](https://www.hynetek.com/uploadfiles/site/219/news/24322c00-2276-4d74-98db-c74432318210.pdf)

USB data is not exposed to MainBoard, but D+/D- may still be needed locally for legacy charging detection. USB-C must retain correct CC circuitry, dead-battery attach behavior, connector ESD protection and hardware reverse isolation. Input OVP is required independently of the requested PD voltage.

## Source authorization

Use actual negotiated/advertised capability, not the supply label. A 5V/2A target is not a universal USB fallback. The exact limits for default Type-C, SDP before/after enumeration, CDP, DCP and vendor protocols must be verified against the applicable protocol before release. A power-only SDP connection must not assume that USB enumeration occurred.

Start with a hardware-safe limited load. Charging is inhibited until source and battery profile are qualified. Keep the combined input below the permitted limit, including detection bias, rail inrush and keep-alive. Contract loss and Core reset return to safe limits. Low-current source plus absent/depleted battery requires a boot/load admission strategy.

## Keep-alive

Proposed path: protected USB supply after the total USB current-sensing/limiting boundary, before source mux isolation, through a pulse resistor and default-off MOSFET. Prove that EXT and battery cannot energize it by reverse paths. Gate condition includes USB-valid, USB actually selected for system power, pulse authorization and available source margin. Include a hardware maximum on-time so a stalled Core cannot turn the optional pulse load into a permanent dummy load. Part values are not frozen.

For the requested 100-200mA exploratory pulse at nominal 5V, resistance lies between 50 and 25ohm and pulse resistor power between 0.5 and 1W. With an illustrative 100ms pulse every 10s, average resistor power is 5-10mW; pulse energy is 0.05-0.1J. These timing values are arithmetic examples, not a recommended compatibility setting. Recalculate at maximum permitted USB voltage and resistor tolerance, and select from the resistor's repetitive pulse-energy curve. MOS conduction is `Ipulse^2*Rds_on` at minimum gate voltage and maximum junction temperature, plus switching/gate losses.

Default OFF; DNP-compatible; TP_KEEPALIVE provided in final schematic. Core hooks: `powerbank_keepalive_enable(bool)` and `powerbank_keepalive_pulse()`. A hardware fault or source invalidation overrides the requested state. No extra MCU.

## Diagnostics and observability

Record CC attachment, PD contract/faults, advertised current, measured VBUS, limiter/fault events, source selection, load history and timestamps. Distinguish observations from inferred causes. Cable removal and power-bank auto-off can produce identical telemetry, particularly on USB-A-to-C. Use UNKNOWN_SOURCE_LOSS or a confidence-tagged diagnosis when evidence cannot distinguish them; never label all losses as charging failure.

Compatibility matrix must cover actual bank/cable pairs: C-to-C PD, C-to-C non-PD, A-to-C SDP/CDP/DCP, battery full, idle, battery absent, weak source, reattach, keep-alive enabled/disabled and 5V-only reset behavior. No universal bank compatibility is claimed.
