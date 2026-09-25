# Component Selection - Preliminary

No candidate is electrically frozen. References are linked in COMPONENT_DATASHEETS.md. Values below are IC characteristics, not FT02 pack or board ratings.

Lifecycle recheck 2026-09-25: manufacturer pages show [TPS63802](https://www.ti.com/product/TPS63802), [TPS2121](https://www.ti.com/product/TPS2121) and [BQ25628E](https://www.ti.com/product/BQ25628E) ACTIVE; [LT3120](https://www.analog.com/en/products/lt3120.html) RECOMMENDED FOR NEW DESIGNS. Exact ordering suffixes, ancillary parts and two qualified procurement channels remain unverified; not a complete BOM lifecycle pass.

| Function / part | Verified fact | Design implication / remaining gate |
| --- | --- | --- |
| External mux / TPS2121 | 2.8-22V recommended input; programmable 1-4.5A current-limit range; reverse blocking | Suitable range candidate; USB default-current compliance needs source-wide lower-current control, not this limit alone |
| UPS / TPS2121 | Electrical table: enabled-input Iq 300uA typ, 400uA max; 56mohm typ path at specified conditions; fast transfer 5us typ in applicable mode | Cannot certify low-battery operation below 2.8V or use typical transfer time as guaranteed hold-up bound; battery hot R/current/thermal check required |
| 3V3 / TPS63802 | 1.3-5.5V operating, >1.8V startup; 2A at 3.3V for VIN>=2.3V; 11uA operating Iq; PG | Strong candidate; source OVP, tolerance, step response and thermal validation remain |
| 5V / LT3120 | 2.5V startup class, buck-boost; 35uA no-load Iq; 4x5mm LQFN | Keep candidate, not an unconditional 5V/2A guarantee; one published 5V example supplies only 1.25A at VIN=2.5V; configure and validate at actual minimum loaded SYS_RAW |
| Gauge / BQ27220 | Capacity support includes 10000mAh; real shunt measurement with CEDV estimation | Retain candidate; battery profile, shunt, bypass-current audit and DSBGA assembly remain |
| PD / HUSB238A | BC1.2 SDP/CDP/DCP detection; address options 0x42/0x62 in I2C mode | Retain candidate; 5V-only reset/fuse behavior, correct register document and total current limiting remain |
| Optional protection / BQ297xx | External back-to-back FET control; factory-defined thresholds; 4uA normal typical | No suffix or FET frozen until pack safety limits known; no bypass of existing pack PCM |

TPS2121 Iq must use electrical table conditions rather than the differing headline typical values. The two muxes are not both charged to battery-mode overhead if the external mux is truly unpowered and isolated; that isolation needs checking.

## Charger comparison

| Part | Topology | Max charge | Iq evidence | I2C | JEITA / NTC | Package | Availability | Lifecycle | Recommendation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BQ25628E | Buck NVDC, local SYS | 2A | 1.5uA battery-only typ with ADC off; 260uA with ADC on | Yes | Flexible JEITA/NTC | 18-pin 2.5x3mm WQFN | Two channels unverified | ACTIVE family; suffix audit pending | Leading conditional candidate; charger-only support and fail-safe controls open |
| BQ24160 | Buck power-path, dual input | 2.5A | Not yet normalized against required modes | Yes | Yes | 24-pin 4x4mm VQFN option | Two channels unverified | Active on inspected TI ordering page | Does not automatically remove charger-only qualification issue; larger |
| BQ24157 | Buck charge output | 1.55A | Not yet verified | Yes | Battery NTC/JEITA not established by screened source | 20-ball WCSP | Two channels unverified | Active on inspected TI page | Not recommended as drop-in; thermal safety and preferred 2A unmet/unproved |
| BQ25306 | Standalone buck | 3A | 200nA advertised low battery Iq; verify condition | No | Hot/cold NTC; no complete JEITA proof here | 3x3mm WQFN | Two channels unverified | Active on inspected TI page | Reject for mandatory I2C requirement |

For BQ25628E, SYS remains the local buck output with its required inductor and output capacitor even when there is no FT02 system load on it. It cannot be left electrically unimplemented. NVDC capability alone neither proves incompatibility nor proves charger-only qualification. Review its power path under input loss, HIZ, sleep, reset, disabled charging and faults. CE must default to charging inhibited until battery configuration is valid. [BQ25628E datasheet](https://www.ti.com/lit/ds/symlink/bq25628e.pdf)

No charger replacement is authorized as a frozen BOM change by this screening table. Input guards, supervisors, GPIO expansion, ferrite, inductors, capacitors, FETs, connectors and exact shunt MPNs still require selection/calculation.
