# AGENTS.md - FT02-LR01 v1.1

## Project isolation
Work ONLY on LanternBox / FT-02 / FT02-LR01 v1.1. Never mix unrelated project context.

## Mission
Turn this frozen handoff into a real manufacturable KiCad project named:
`FT02-LR01_v1.1_Fabrication_Release`.

## NO NEW HARDWARE
Do not add sensors, test points, RTC backup pads, LEDs, connectors, new power paths, new RF protection, or any other feature. Manufacturing closure only.

## Source-of-truth order
1. `spec/LOCKED_DESIGN_SPEC.md`
2. `spec/LOCKED_BOM.csv`
3. `spec/LOCKED_GPIO_CONTRACT.csv`
4. `spec/LOCKED_FPC_CONTRACT.csv`
5. `spec/LOCKED_GNSS_5PIN.csv`
6. `spec/LOCKED_MOUNTING.csv`
7. Official Semtech E512V01A files under `reference/`
8. Manufacturer datasheets/official ECAD for exact footprints
9. Engineering reference PCB/PDF/PNG are visual references only

If sources conflict: STOP and write `output/BLOCKERS.md`. Do not invent.

## Frozen board
- 66.00 x 58.00 mm
- 2-layer FR4
- 1.00 mm finished thickness
- 35 um copper baseline
- ALL SMT on Top
- Bottom: GND + GNSS through-hole carrier only
- 4 x M2: 2.20 mm NPTH, nominal 6.0 mm copper/track keepout
- H2/H4 may move minimally only to protect official E512 RF. Never deform RF.

## Frozen U1
ESP32-S3FH4R2 / JLC C3013940 / QFN56 7x7.
- 4 MB QSPI Flash + 2 MB QSPI PSRAM
- no ESP32 32.768 kHz crystal
- no 2.4GHz antenna, Wi-Fi/BLE disabled in firmware
- use exact Espressif FH4R2 recommended land pattern, including EPAD

## Frozen 40MHz clock
Y1 = HD 7D040000I01 / JLC C648973
- 40.000 MHz
- CL 10 pF
- tolerance +/-10 ppm
- SMD2016-4P
CXT1 = 15 pF C0G/NP0 0402
CXT2 = 15 pF C0G/NP0 0402
LXT1 = 24 nH 0402 in series at XTAL_P
No substitution without explicit user approval.

## Frozen USB
J1 = GCT USB4105-GF-A / JLC C3020560.
Do not substitute -060 or -120.
- data/debug only
- VBUS must NOT power LR01_3V3_SYS
- CC1/CC2 each 5.1k to GND
- D+/D- through TPD2EUSB30DRTR and 22R series resistors
- VBUS sense 100k/100k -> GPIO2
- route D+/D- as an actual differential pair using the real 1.0mm/2-layer fab stackup, continuous GND reference

## Frozen BMI270
U4 = BMI270 / JLC C2836813.
- VDD=3.3V, VDDIO=3.3V
- 100nF close to each supply
- I2C
- CSB -> VDDIO
- SDO/SA0 -> GND, address 0x68
- INT1 -> GPIO6
- INT2 DNC
- unused auxiliary/OIS pins exactly per Bosch datasheet
- correct physical orientation mandatory
- Standard-only / X-ray requirement is accepted; do not redesign to another IMU

## Frozen RTC
U5 = RV-3028-C7-32.768KHZ-1PPM-TA-QA / JLC C3304278.
- INT -> GPIO1
- no external RTC backup source
- no new backup pad
- CLKOUT unused/disabled

## Frozen GNSS
GNSS1 = user-supplied MAX-M10S carrier.
- DNP / DO NOT ASSEMBLE
- excluded from purchasing BOM and CPL
- Bottom side
- 5 x TH, 2.54mm pitch
- approx module 13.1 x 15.7mm
- engineering drill 1.00mm, pad 1.80mm unless physical-module mechanical check demands correction
- pin/silkscreen order MUST be: VCC GND TX RX PPS
- VCC -> GNSS_3V3
- GND -> GND
- TX -> GPIO15 / GNSS_RX
- RX -> GPIO16 / GNSS_TX
- PPS -> GPIO47 / GNSS_PPS

## Frozen host FPC
J2 = Hirose FH34D-10S-0.5SH(50)
- Top side
- 10P, 0.5mm pitch, FPC 0.30mm
- connector opening and cable exit toward PCB bottom edge
- top view, opening toward bottom edge: Pin1 at left
- top-contact insertion
Use `spec/LOCKED_FPC_CONTRACT.csv`.

## Frozen power
Host FT-02 supplies 3V3_AON.
Host contract: 3.3V nominal, design capability >=1A peak.
No onboard buck/mux/charger.
3V3_AON -> dual FPC power contacts -> 0R/ferrite-compatible 0603 -> 150uF + 10uF + 0.1uF -> LR01_3V3_SYS.
GNSS branch: LR01_3V3_SYS -> 0R/ferrite-compatible link -> GNSS_3V3 -> 10uF + 0.1uF.

## Frozen RF
Official Semtech SX1268MB1GAS E512V01A 490MHz is the ONLY RF source of truth.
Preserve exactly:
- SX1268
- PE4259
- radio 32MHz crystal
- matching values
- relative placement
- trace geometry
- GND copper
- stitching vias
- SMA launch
- reference DNP tuning pads
- 2-layer / 1.0mm RF environment

Whole-region translation is allowed. Deformation/rerouting/optimization is forbidden.

RF fragment boundary is locked:
- Source: X=153.5000..193.0011 mm, Y=76.0036..134.0036 mm.
- Target: X=26.4989..66.0000 mm, Y=0.0000..58.0000 mm.
- Rigid translation only: DX=-127.0011 mm, DY=-76.0036 mm.
- Everything at or right of the locked boundary is RF-PROTECTED.
- Do not scale, rotate individual objects, reroute, optimize, shrink, regenerate RF copper, refill frozen fragment copper into a new shape, or replace RF geometry with newly drawn equivalents.

Locked RF controls:
- GPIO7 = ANT_SW. ESP32 GPIO7 / ANT_SW -> R4 -> PE4259 pin6 complementary control input.
- SX1268 DIO2 -> R3 -> PE4259 pin4 CTRL. Do not reroute SX1268 DIO2 to ESP32.
- Do not convert PE4259 to single-pin mode; preserve official complementary-control topology.
- Q1 is populated 32 MHz crystal path; Q2 optional/not fitted; TCXO_EN not required; FB1 NC per official E512 source.


## Required production CAD
Create:
- `cad/FT02-LR01_v1.1_Fabrication_Release.kicad_pro`
- `cad/FT02-LR01_v1.1_Fabrication_Release.kicad_sch`
- `cad/FT02-LR01_v1.1_Fabrication_Release.kicad_pcb`

No engineering-envelope footprints are allowed in release CAD.

## Mandatory gates
Before overall PASS:
- exact footprints audited
- ERC clean with violations-as-exit-code
- DRC clean with violations-as-exit-code
- schematic parity clean
- no unresolved unrouted nets
- zones refilled
- RF reference preservation audited
- USB routing audited
- orientation/pin1 audited
- M2/RF collision audited
- GNSS1 DNP verified
- exact BOM generated
- front-side CPL generated with DNP excluded
- firmware 4MB Flash / 2MB PSRAM build verified
- Gerber + Drill generated

Never fake a PASS. If mandatory work is unavailable or unknown, use BLOCKED.
