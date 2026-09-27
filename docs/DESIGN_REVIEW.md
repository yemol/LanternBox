# FT02 PowerBoard V1 Design Review

Review date:2026-09-27. Decision: **FAIL / HOLD PHASE 2**. This is an incomplete Phase1 checkpoint, not the requested completed electrical design. PASS below means only the stated evidence passed, never board release.

## Verification Evidence

- KiCad10.0.5 parses root plus13 child sheets and exports XML/SVG.
- Sheets02/04/05/07/09/10/11 have candidate input, charger, battery guard, UPS/UVLO,5V, sensor and interface wiring; sheet08 is supervised. Sheets01/03 remain NOT IMPLEMENTED. No subsystem is qualified; full main-chain closure is outstanding.
- Independent exported-netlist check passes418 pins/145 symbols.60 connectivity/boundary/candidate-limit tests,5 load-inventory tests and4 battery-contract tests pass:69 total. New mutations cover battery return/OC/gate/bias errors and power-pin suppression; recovery-window calculations are regression checked.
- Four battery-contract regression tests also pass: adopted limits, unresolved parameters remaining null, over5A examples forbidden and only source-backed issue children closed. Source/electrical evidence hashes match the retained records. These are software/document checks, not electrical qualification.
- ERC:6 errors,4 warnings. U501 BAT/VSS add real power-input checks; external supply/passive-path models and the missing03 output remain unresolved. BAT_PLUS/BAT_MINUS warnings disappear with actual wiring; USB_VBUS,USB_5V,KEEPALIVE,EXT_VALID warnings remain, not waived. Existing common output-pin models are enforced; no PWR_FLAG/exclusions added. Root Phase1 review lists every violation.
- EXT guard static OV trip5.148..5.453V exposes a5.25V acceptance failure; transient/negative-spike/current/thermal qualification also remains OPEN. This is a recorded counterexample, not an approved narrowing of source specifications.
- Empty sheets returning zero ERC violations do not prove completeness. No simulation, complete battery-path audit, physical footprint review or hardware test has passed.
- Sheet13 now connects a genuine mechanically controlled TPS22953 series gate between UPS_RAW and SYS_RAW. Exact control/input/output terminal sets pass; full off-state zero-rail and backfeed proof do not. Charger/gauge remain upstream and charge lock stays fitted. [Detailed HARD OFF review](SYSTEM_HARD_OFF_REVIEW.md).
- HUSB238A GPIO5V strap definitions are verified, but current authorization is not: GATE/FAULT alone cannot yet justify a source-current setting. [PD review](USB_PD_POLICY.md). BQ25628E explicitly has no OTG per TI table5-1; its local-SYS boundary is preserved, without claiming all charger-only/fault states qualified.
- [LOAD_BUDGET](LOAD_BUDGET.md) contains21 entries including board-guard bias separately from series losses. Reference peaks remain separate from unknown guarantees. Whole-board current/duration and TPS2121/current/thermal margins are still OPEN.
- Sheet13 adds three resistor-limited DMN2056U discharge branches selected by the mechanical OFF throw; open switch/lost UPS bias retains passive bleeders. The approximately125ms active RC example is conditional, not a switch-to-OFF guarantee. Pin2 switch wiring changed; contact/temperature/resistor derating/rail injection remain OPEN. EXT OVP resistor-only tuning is infeasible under the documented reverse-injection assumptions; a replacement circuit is still required.
- Full-aspect previews of the newly wired pages were inspected; sheet09 label collision was corrected. No schematic rendering is evidence of electrical qualification.

## Required Thirty Answers

| # | Question | Finding | Evidence / closure |
| --- | --- | --- | --- |
| 1 | EXT_VALID forbids BAT feeding System? | OPEN | Required invariant; no completed UPS/validity circuit,OI-004/006 |
| 2 | Hidden battery supplement absent? | OPEN | Must inspect every conducting/backfeed path; empty sheets are not proof,OI-005/010 |
| 3 | Charger SYS incorrectly tied to system? | PASS installed net boundary / OPEN device states | CHG_SYS_LOCAL only U401:9,L401:2,C406:1; internal reverse/fault behavior remains OI-007 |
| 4 | All charge/discharge through one shunt? | OPEN | Installed main paths and new05 guard node audit pass; U501 supply/filter/gate/VM and gauge exceptions explicit. Full IC fault/leakage proof absent,OI-010 |
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

Latest30-gate task brief: gates1-12 fail full-chain completion (01/03 absent,05/13 wired not qualified);13 no-supplement OPEN;14 zero-rail OFF OPEN;15 normal charging while OFF blocked by charge lock/profile;16 full shunt/fault proof OPEN;17 ERC fails6 errors;18 four warnings reviewed but not waived;19 no PWR_FLAG/exclusions added;20 load inventory exists but envelope OPEN;21 calculations updated but thermal proof OPEN;22 passives/protection not qualified;23/24 statuses and battery unknowns preserved;25 charging disabled;26 review synchronized;27/28 netlist and mutation tests pass;29 full-aspect rendering checked, empty pages not complete;30 no prohibited production output. **Overall FAIL.**

1. Preserve adopted pack fields; clarify CC4.25V/CV4.2V ambiguity, charge temperature, exact PCM/NTC/connector/lead ratings, loaded minimum and >1C pulse data. Keep charging inhibited.
2. Close source-wide limiting, autonomous charge shedding, external qualification/retry and battery-free boot as a pin-level hardware design. Verify switching timing before selecting hold-up capacitance.
3. Resolve exact5V-only PD configuration and charger-only application/defaults. Do not silently replace candidates.
4. Complete protected battery/common-shunt topology and all return/bias paths, NTC, sensing/calibration and low-battery margin.
5. Finish remaining two empty sheets and qualify all wired references, footprints/passives, real load/thermal envelope and ERC. Sheet05 now requires startup/early-trip/reverse/thermal closure, not merely connection. TPS2121 remains UNFROZEN until actual board currents and durations are established, not by comparing pack5A alone.

No PCB, production files or `ft02-powerboard-v1-revA` tag. A later zero-error ERC still requires independent electrical and safety review. Engineering-sample authorization is a later decision, never production readiness.
