# Open Issues

Status: OI-001 CLOSED; OI-002 PARTIALLY RESOLVED by the2026-09-25 supplied battery source, with explicit child decisions below. Other top-level issues remain OPEN. A blocker is an unmet release condition, not proof of a defective manufactured board.

| ID | Issue | Required closure evidence | Gate |
| --- | --- | --- | --- |
| OI-001 CLOSED | Corrected project /Volumes/yemol_HDDisk/LanternBox is accessible | Local rules and hardware records read; original HEAD and dirty state captured in 808b603 | Closed |
| OI-002 PARTIAL | Source now supports identity/current/discharge limits and protection presence; charging/safety details incomplete | Only close child fields with direct source evidence; see below and BATTERY_SPEC.md | Safe charging/battery path/sample release still blocked |
| OI-003 | Load variants and concurrency not frozen | EPD, SD, PA/speaker, GNSS and Core variants; measured workloads and peak durations | Power/thermal envelope |
| OI-004 | TPS2121 UNFROZEN; board-required continuous/transient load not established | U701 candidate wired with29.8k ILIM; U702 nominal3.30V SYS_RAW cutoff. Prove admitted load, minimum input, current/thermal headroom and transition; pack5A alone neither rejects nor approves it | UPS choice remains OPEN |
| OI-005 | Separate charger cannot measure total board input through its own ILIM | Source-wide limiter and hardware charge shedding; transient timing and reset-safe defaults | Architecture |
| OI-006 | EXT_VALID requires more than unloaded voltage | Defined comparator, hysteresis, qualification, failed-source inhibit/retry, load test and battery-free behavior | Architecture |
| OI-007 | BQ25628E charger-only application wired, not fully qualified | U401 LC/local SYS implemented; R407 DNP charge lock, R408 TS fault. Reverse/sleep/fault, defaults/watchdog and source-priority hardware still need proof | Charger freeze |
| OI-008 | HUSB238A GPIO fixed5V now prioritized; exact boot/source authorization unproved | VSET0ohm,HVDCP900kGND,GPIO strap definitions verified; exact order/fuses and safe current-capability/admission path still missing. GATE/FAULT not accepted as universal current authorization | PD freeze |
| OI-009 | Input OVP must protect low-voltage downstream stages | Threshold tolerances, cutoff delay, overshoot, FET/TVS energy coordination | Protection |
| OI-010 | Charger BAT/UPS IN2 now physically netlisted on R601 load side; complete all-current requirement unproved | Exact boundary-node checks pass; U601 BAT/C601 upstream bias/dynamic bypass and Kelvin input bias explicit. Guard, missing input sheets and internal reverse/fault states not proven. Remains OPEN | Fuel gauge |
| OI-011 | NTC shared sensing cannot be assumed | Actual thermistor and reference circuits; independent sensing or validated noninterfering scheme | Battery safety |
| OI-012 | Complete I2C address/pullup/off-state map missing | Exact board variants and official maps; bus scan, capacitance, phantom-power analysis | Interface |
| OI-013 | TCA9539 0x74 telemetry/control wired, source policy incomplete | P00..P07 collect PG/FG/charger/MUX and charge request. Source controls/keep-alive absent; short IRQ capture and off-state behavior not qualified | Interface |
| OI-014 | Input-loss cause is not always identifiable | UNKNOWN/ambiguous diagnostic state; observed evidence distinct from inferred cause | Firmware semantics |
| OI-015 | Passive values, footprints and assembly constraints not frozen | Datasheet-derived BOM, manufacturer footprints, tolerance/derating checks, DSBGA capability | Schematic release |
| OI-016 | USB-C, board, holes, connectors and EMI spacing not frozen | Enclosure CAD and connector drawings | Mechanical/Phase 2 |
| OI-017 | Lifecycle and two procurement channels not all checked | Dated evidence for exact orderable MPNs and qualified sources; no stock assumptions | Component release |
| OI-018 | Complete electrical schematic/ERC closure outstanding | Full connected13-child-sheet design and retained ERC; partial reference implementation is not completion | Phase 1 exit |
| OI-019 | No-brownout assertion unproved | Worst-case hold-up calculation, source/rail simulation and later instrumented tests | Design proof/sample validation |
| OI-020 | Battery absent/depleted plus low-power source boot unresolved | Boot admission/load gating within authorized input; no negotiation/boot deadlock | Architecture |
| OI-021 | CardKB2 supply rail records conflict | Older wiring says 3.3V; newer power-test plan says 5V; confirm actual module supply node | Power budget |
| OI-022 | SYSTEM HARD OFF series gate wired but not fully qualified | Sheet13 U1301 mechanical default-OFF; prove residual/time on all rails, interface/debug injection, input/thermal/inrush margins and future autonomous charging with Core off | Phase1 exit |

Proceed with independent research and parameterized calculations. Do not replace unresolved battery or source limits with guessed ratings, generate fabrication outputs, or declare Phase 1 passed.

Current schematic checkpoint:127 symbols/363 physical or logical pins checked;49 regression tests across netlist(40),battery(4),load inventory(5);ERC4 errors/6 warnings. Sheet13 series gate is wired and exact boundaries checked. Sheets01/03/05 remain NOT IMPLEMENTED. Sheet02 TPS259474L still cannot guarantee5.25V acceptance; OI-009 remains OPEN. The load inventory is complete as a list, not a qualified current envelope. No additional issue is closed by this checkpoint; electrical implementation work remains, not only missing battery documentation.

## OI-002 Source-Level Decisions

Evidence: [unchanged supplied source record](FT02_BATTERY_SPEC_955565_955465.md) and user's explicit adoption. CLOSED means this parameter is supported, not that the circuit is qualified.

| Child | Status | Field / evidence boundary |
| --- | --- | --- |
| OI-002A | CLOSED | 955565/955465 Polymer Li-ion,5000mAh,3.8V(3.7V compatible),9.5x55x65mm,76g |
| OI-002B | CLOSED | Standard2.5A,fast5A,maximum charge5A; no requirement to build5A charger |
| OI-002C | CLOSED | Maximum discharge5A,cutoff2.75V,discharge-20C to+60C; not a guaranteed board/loaded-voltage rating |
| OI-002D | CLOSED | Presence of pack overcharge/overdischarge/overcurrent/short-circuit/overtemperature protection |
| OI-002E | OPEN | CC-to4.25V then CV4.2V ambiguity; charge setpoint/tolerance not approved |
| OI-002F | OPEN | Charge-temperature range; discharge range is not transferable |
| OI-002G | OPEN | Exact protection thresholds,tolerance,delay,recovery |
| OI-002H | OPEN | NTC presence/value/Beta/tolerance/wiring |
| OI-002I | OPEN | Connector/polarity/wire gauge/length |
| OI-002J | OPEN | Minimum loaded pack-output voltage/impedance;2.75V cutoff is not this quantity |
| OI-002K | OPEN | Peak discharge above1C and allowed duration/repetition |

No parent architecture,thermal,NTC,protection or full battery-path issue closes merely because5A is now known.
