# FT02 PowerBoard V1 Design Review

Review date:2026-09-25. Decision: **FAIL / HOLD PHASE 2**. This is an incomplete Phase1 checkpoint, not the requested completed electrical design. PASS below means only the stated evidence passed, never board release.

## Verification Evidence

- KiCad10.0.5 parses root plus12 child sheets and exports XML/SVG.
- Sheets04/07/09/10/11 now have charger, candidate UPS/UVLO,5V, sensor and interface wiring; sheet08 enable is supervised. Sheets01/02/03/05 remain NOT IMPLEMENTED. None is a qualified subsystem; the main input/pack chain is still incomplete.
- Independent exported-netlist check passes305 pins/107 symbols.29 connectivity/boundary/candidate-limit tests pass, including deliberate charger/UPS shunt bypass, charge-arm stuffing, UVLO bypass, wrong IC, NC-to-live-net, missing unused-input bias and hidden power-output mutations.
- Four battery-contract regression tests also pass: adopted limits, unresolved parameters remaining null, over5A examples forbidden and only source-backed issue children closed. Source/electrical evidence hashes match the retained records. These are software/document checks, not electrical qualification.
- ERC:3 errors,7 warnings. Undriven EXT_BUS_5V/GND/BAT_GUARDED correspond to missing upstream implementation. Seven isolated test-point labels are individually reviewed in the root Phase1 review, not waived. Duplicate internally common output pins now have one power_out plus one passive pin, with equality enforced by the netlist contract. No PWR_FLAG/exclusions were added; default ignored checks are disclosed.
- Empty sheets returning zero ERC violations do not prove completeness. No simulation, complete battery-path audit, physical footprint review or hardware test has passed.
- Full-aspect previews of the newly wired pages were inspected; sheet09 label collision was corrected. No schematic rendering is evidence of electrical qualification.

## Required Thirty Answers

| # | Question | Finding | Evidence / closure |
| --- | --- | --- | --- |
| 1 | EXT_VALID forbids BAT feeding System? | OPEN | Required invariant; no completed UPS/validity circuit,OI-004/006 |
| 2 | Hidden battery supplement absent? | OPEN | Must inspect every conducting/backfeed path; empty sheets are not proof,OI-005/010 |
| 3 | Charger SYS incorrectly tied to system? | PASS installed net boundary / OPEN device states | CHG_SYS_LOCAL only U401:9,L401:2,C406:1; internal reverse/fault behavior remains OI-007 |
| 4 | All charge/discharge through one shunt? | OPEN | U401 BAT/U701 IN2 connected on post-shunt side; exact node audit passes. Guard absent; gauge supply/C601/sense-bias exceptions explicit,OI-010 |
| 5 | Weak USB reduces charging first? | OPEN | Branch ILIM cannot enforce source-wide priority,OI-005 |
| 6 | Collapsed external invalidates and transfers? | OPEN | Hardware qualification, latch/retry and timing missing,OI-006/019 |
| 7 | Low-load keep-alive solution? | OPEN | DNP pulse policy/calculation exists; no implemented gating/pulse limiter or bank test |
| 8 | Keep-alive defaults OFF? | PASS requirement / OPEN circuit | Reset and stuck-control proof still required |
| 9 | PD only requests5V? | OPEN | HUSB238A exact order/fuse/reset behavior unqualified,OI-008 |
| 10 | USB-A to C scope? | OPEN | DCP/CDP/SDP policy documented, detection/current/boot behavior untested |
| 11 | Current5000mAh frozen spec implemented? | PARTIAL | Source model/3.8V(3.7V compatible)/size/mass/current/discharge/protection-presence adopted;4.25V CC vs4.2V CV, charge temperature and safety/interface details OPEN,OI-002 |
| 12 | Future10000mAh without PCB change? | OPEN | Gauge capacity range suitable; shared current/connector/thermal envelope not frozen |
| 13 | 3V3 stable full-to-low battery? | OPEN | Partial reference connectivity passes; loaded battery floor/transient/thermal proof absent |
| 14 | 5V stable on both sources? | OPEN | LT3120 TA07 circuit wired, default-off with UVLO AND gate; lowest-input load/loop/thermal not qualified |
| 15 | ESP32 avoids transfer brownout? | OPEN | No worst-case timing/hold-up/rail proof or scope traces,OI-019 |
| 16 | USB5V/2A maximum system load? | OPEN | Nominal source10W, delivered rail power less after losses; no approved numeric rating |
| 17 | EXT5V/3A maximum system load? | OPEN | Nominal15W, losses/reserve/thermal reduce available rail power |
| 18 | Battery continuous system power? | OPEN | Pack maximum5A known; loaded minimum,path and thermal envelope missing; TPS2121 limits still constrain board |
| 19 | Current battery continuous discharge? | PASS source ceiling / OPEN board qualification | Maximum1C=5A source-backed; no >5A pulse/duration or full current-temperature qualification inferred |
| 20 | Real remaining mAh available? | PASS candidate capability / OPEN calibration | BQ27220 coulomb-informed model estimate; profile/learning not qualified |
| 21 | Signed battery current available? | PASS candidate capability / OPEN implementation | BQ27220 supports shunt current; actual shunt/gain/sign calibration required |
| 22 | State from measurements not voltage guessing? | PASS protocol policy / OPEN integration | Measured current plus gauge/charger flags, validity/age/deadband required |
| 23 | I2C conflicts? | OPEN | Named verified address subset has none; expansion/straps/off-state/pullups still unqualified |
| 24 | Main Core GPIO count? | PARTIAL implementation | Logical connector and TCA9539 0x74 use existing SDA47/SCL21 plus PWR_INT/5V_EN; no GPIO4/new MCU. Source-policy controls and pulse capture incomplete |
| 25 | Idle overhead? | OPEN | Six-mode accounting present; UPS alone about300uA typical enabled bias, total not closed |
| 26 | 5V-off saving? | OPEN | Must compare same-load input power; no numeric saving asserted |
| 27 | All critical parts active/new-design? | OPEN | TI TPS2121/TPS63802/BQ25628E ACTIVE; ADI LT3120 recommended; full exact-MPN BOM audit absent |
| 28 | Packages assemble locally? | OPEN | BQ27220 DSBGA process and all footprints unapproved,OI-015 |
| 29 | Two reliable channels per part? | OPEN | No complete dated exact-MPN dual-source audit,OI-017 |
| 30 | Miniaturization creates thermal risk? | OPEN | Dual mux losses substantial; enclosure/PCB thermal evidence absent,OI-016 |

## Next Electrical Work

1. Preserve adopted pack fields; clarify CC4.25V/CV4.2V ambiguity, charge temperature, exact PCM/NTC/connector/lead ratings, loaded minimum and >1C pulse data. Keep charging inhibited.
2. Close source-wide limiting, autonomous charge shedding, external qualification/retry and battery-free boot as a pin-level hardware design. Verify switching timing before selecting hold-up capacitance.
3. Resolve exact5V-only PD configuration and charger-only application/defaults. Do not silently replace candidates.
4. Complete protected battery/common-shunt topology and all return/bias paths, NTC, sensing/calibration and low-battery margin.
5. Finish remaining four electrical sheets and qualify all wired references, device footprints/passives, real load/thermal envelope and all ERC violations; independently review before PCB. TPS2121 remains UNFROZEN until board-required continuous/transient currents are established, not by comparing the pack5A ceiling alone.

No PCB, production files or `ft02-powerboard-v1-revA` tag. A later zero-error ERC still requires independent electrical and safety review. Engineering-sample authorization is a later decision, never production readiness.
