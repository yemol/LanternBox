# Fuel Gauge Design - Preliminary

BQ27220 remains a candidate. Remaining mAh and SOC are model-based estimates informed by coulomb counting, not directly observable absolute chemical capacity. Changing to 10000mAh requires an appropriate battery profile and initialization/learning validation, not a UI offset.

Manufacturer evidence: system-side high-side sensing is documented; shunt differential operating range is +/-80mV; the standard 10mohm option is pre-calibrated; I2C address is 0x55; normal/sleep typical consumption is 50/9uA. [Datasheet](https://www.ti.com/lit/ds/symlink/bq27220.pdf)

## Shunt constraints

```text
Ipeak * Rnom * (1 + tolerance + TCR_delta) + error_margin < 80mV
Pcontinuous = Irms^2 * Rhot
Eshort = integral(I(t)^2 * Rhot dt)
Vloss = I * Rhot
current_offset_error = input_offset_voltage / Rshunt
```

At candidate 10mohm: 5A gives 50mV and 0.25W; 8A gives 80mV and 0.64W with zero measurement margin. These are arithmetic examples, not approved battery currents. Final resistance requires frozen continuous/peak currents, pulse duration, uncertainty budget and resistor thermal/pulse curves. A changed shunt needs the manufacturer's gain/configuration calibration process, not an arbitrary SOC correction.

Use a four-terminal low-TCR part where practical. Route separate Kelvin pairs from its pads. All external charging and system discharging branches join on the load side. Include reverse-protection, test connectors and future expansion in a graph-level bypass audit.

## Literal all-current requirement

Gauge BAT supply/sense, protection bias and any pack-side auxiliary load can draw current upstream of the measurement resistor in reference topologies. Internal pack PCM consumption is also outside an external board shunt. These paths must be explicitly accounted for; do not silently claim that every electron is measured. Whether a fully measured arrangement can meet gauge common-mode/power requirements remains OI-010. Main charging/discharging paths must never bypass the shunt, regardless of the eventual auxiliary-current treatment.

## Readout

BQ27220 TRM defines Current 0x0C, RemainingCapacity 0x10, FullChargeCapacity 0x12, TimeToEmpty 0x16, TimeToFull 0x18, CycleCount 0x2A, SOC 0x2C and SOH 0x2E. Current is signed and updated each second. Time estimates of 65535 mean not applicable, not 65535 minutes. [TRM sections 2.8-2.22](https://www.ti.com/lit/ug/sluubd4a/sluubd4a.pdf)

The gauge's update speed is insufficient to prove absence of microsecond battery supplement. Verify that invariant using a current probe on the UPS battery branch, not just net battery current, which can hide simultaneous charge/discharge branches. Sample age, validity, pack presence and profile validity belong in Core telemetry.

A four-wire shunt is necessary but not sufficient: voltage sense must not share switching-current copper, gauge return must follow the reference topology, and charger termination error from the selected shunt location must be included.
