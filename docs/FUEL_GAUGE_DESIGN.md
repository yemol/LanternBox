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

The supplied current pack now has a5A maximum discharge ceiling. At candidate10mohm,5A gives50mV and0.25W. The prior8A example gives80mV/0.64W and is expressly OUTSIDE the pack allowance, not an available burst mode. Final resistance requires thermal/pulse qualification and a complete uncertainty budget. A changed shunt needs the manufacturer's gain/configuration calibration process, not an arbitrary SOC correction.

Use a four-terminal low-TCR part where practical. Route separate Kelvin pairs from its pads. All external charging and system discharging branches join on the load side. Include reverse-protection, test connectors and future expansion in a graph-level bypass audit.

## Literal all-current requirement

Gauge BAT supply/sense, protection bias and any pack-side auxiliary load can draw current upstream of the measurement resistor in reference topologies. Internal pack PCM consumption is also outside an external board shunt. These paths must be explicitly accounted for; do not silently claim that every electron is measured. Whether a fully measured arrangement can meet gauge common-mode/power requirements remains OI-010. Main charging/discharging paths must never bypass the shunt, regardless of the eventual auxiliary-current treatment.

## Sheet06 Reference Implemented, Not Qualified

Uses the system-side high-side drawing on BQ27220 datasheet page1 and its pin table, not the pack-side low-side Figure10. `BAT_GUARDED` denotes the future protected-pack/board-guard output; the upstream guard/connector and downstream charger/UPS circuits are NOT implemented. Their eventual main-current branches must join `BAT_PROTECTED`, the load side of R601. Net names alone do not establish protection or prove a complete path.

| Component | Implemented connection / evidence | Remaining boundary |
| --- | --- | --- |
| U601 BQ27220YZFR | BAT=C3 to BAT_GUARDED; VSS=B2 to GND; SRP=C1/SRN=C2 to SENSE_P/N | Gauge BAT bias bypass is visible and unresolved; profile/calibration and common-mode corners require review |
| R601 WSK2512R0100FEA | Four-terminal10mohm1%,1W at70C; full-resistor TCR35ppm/C for this resistance range | I1/I2 are current terminals, E1/E2 sense terminals, NOT assigned footprint pad numbers; no released footprint |
| C601/C602 | BAT1uF reference; VDD=B3 local1.8V output with2.2uF reference to GND | Exact MPN/rating/bias/temperature derating OPEN; VDD effective decoupling must meet TI minimum; no external VDD load |
| R602 | BIN=B1 with fixed10k to GND per embedded-pack guidance | Not a thermistor, not a temperature measurement, not proof of removable-pack presence detection |
| R603 | GPOUT=A1 private FG_GPOUT,10k pullup to3V3_MAIN | Not wired to PWR_INT; wake/SHUTDOWN, off-state and shared interrupt design OPEN |
| R604/R605 | SDA=A2/SCL=A3 with1M pulldowns | Off-state bias reference, not completed phantom-power verification |
| R606/R607 | Optional5.1k bus pullups to3V3_MAIN, both DNP | MainBoard pullup/capacitance audit required before stuffing; never pull I2C to gauge VDD |

At5A,1% high resistance gives50.5mV/0.2525W at reference temperature. With an illustrative45C resistor-temperature change and35ppm/C, the additive tolerance bound is50.57875mV/0.25289375W. This is below the80mV differential limit arithmetically, but excludes offset, ageing, self-heating and fault/pulse conditions; pack ambient limits do not bound resistor element temperature. The1W rating requires its specified PCB/ambient conditions and derating. [Vishay WSK2512, document30108,11-Dec-2023](https://www.vishay.com/docs/30108/wsk2512.pdf)

The independent exported-netlist checks enforce reference pin polarity, distinct shunt sides, private GPOUT, DNP pullups and absence of unreviewed footprints. They do NOT prove the eventual whole-board no-bypass requirement. Charge voltage, charge-temperature/NTC, exact protection and minimum loaded pack voltage remain OPEN; fixed BIN bias must never be interpreted as valid pack temperature. No gauge profile or OTP programming is released.

## Readout

BQ27220 TRM defines Current 0x0C, RemainingCapacity 0x10, FullChargeCapacity 0x12, TimeToEmpty 0x16, TimeToFull 0x18, CycleCount 0x2A, SOC 0x2C and SOH 0x2E. Current is signed and updated each second. Time estimates of 65535 mean not applicable, not 65535 minutes. [TRM sections 2.8-2.22](https://www.ti.com/lit/ug/sluubd4a/sluubd4a.pdf)

The gauge's update speed is insufficient to prove absence of microsecond battery supplement. Verify that invariant using a current probe on the UPS battery branch, not just net battery current, which can hide simultaneous charge/discharge branches. Sample age, validity, pack presence and profile validity belong in Core telemetry.

A four-wire shunt is necessary but not sufficient: voltage sense must not share switching-current copper, gauge return must follow the reference topology, and charger termination error from the selected shunt location must be included.
