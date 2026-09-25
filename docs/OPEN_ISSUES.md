# Open Issues

Status: OI-001 CLOSED by corrected repository path and baseline 808b603. All remaining issues OPEN. A blocker is an unmet release condition, not proof of a defective manufactured board.

| ID | Issue | Required closure evidence | Gate |
| --- | --- | --- | --- |
| OI-001 CLOSED | Corrected project /Volumes/yemol_HDDisk/LanternBox is accessible | Local rules and hardware records read; original HEAD and dirty state captured in 808b603 | Closed |
| OI-002 | Current 5000mAh electrical/thermal/PCM limits not found in inspected project records | Current frozen pack document with model and revision; see BATTERY_SPEC.md | Battery path, charge settings, sample release |
| OI-003 | Load variants and concurrency not frozen | EPD, SD, PA/speaker, GNSS and Core variants; measured workloads and peak durations | Power/thermal envelope |
| OI-004 | TPS2121 battery operating margin and current limit unproved | Lowest loaded IN2 >=2.8V in guaranteed conditions; limit tolerance, hot resistance and thermal calculation | UPS choice |
| OI-005 | Separate charger cannot measure total board input through its own ILIM | Source-wide limiter and hardware charge shedding; transient timing and reset-safe defaults | Architecture |
| OI-006 | EXT_VALID requires more than unloaded voltage | Defined comparator, hysteresis, qualification, failed-source inhibit/retry, load test and battery-free behavior | Architecture |
| OI-007 | BQ25628E charger-only application not fully qualified | Official support for local unloaded SYS with required LC/caps; reverse/sleep/fault proof; defaults and watchdog audit | Charger freeze |
| OI-008 | HUSB238A I2C cold-start 5V-only behavior unproved | Correct A-variant register documentation, ordering/fuse configuration and reset/attach traces | PD freeze |
| OI-009 | Input OVP must protect low-voltage downstream stages | Threshold tolerances, cutoff delay, overshoot, FET/TVS energy coordination | Protection |
| OI-010 | Common battery shunt and all auxiliary returns unproved | Complete connectivity audit including gauge supply/sense, protection bias, NTC, debug and connector paths | Fuel gauge |
| OI-011 | NTC shared sensing cannot be assumed | Actual thermistor and reference circuits; independent sensing or validated noninterfering scheme | Battery safety |
| OI-012 | Complete I2C address/pullup/off-state map missing | Exact board variants and official maps; bus scan, capacitance, phantom-power analysis | Interface |
| OI-013 | Low-GPIO telemetry/control not fully implemented | Concrete I2C GPIO/monitor solution or verified existing outputs for mux ST, rail PG, charging permit and keep-alive | Interface |
| OI-014 | Input-loss cause is not always identifiable | UNKNOWN/ambiguous diagnostic state; observed evidence distinct from inferred cause | Firmware semantics |
| OI-015 | Passive values, footprints and assembly constraints not frozen | Datasheet-derived BOM, manufacturer footprints, tolerance/derating checks, DSBGA capability | Schematic release |
| OI-016 | USB-C, board, holes, connectors and EMI spacing not frozen | Enclosure CAD and connector drawings | Mechanical/Phase 2 |
| OI-017 | Lifecycle and two procurement channels not all checked | Dated evidence for exact orderable MPNs and qualified sources; no stock assumptions | Component release |
| OI-018 | Complete electrical schematic/ERC closure outstanding | Full connected 12-sheet design and retained ERC; partial reference implementation is not completion | Phase 1 exit |
| OI-019 | No-brownout assertion unproved | Worst-case hold-up calculation, source/rail simulation and later instrumented tests | Design proof/sample validation |
| OI-020 | Battery absent/depleted plus low-power source boot unresolved | Boot admission/load gating within authorized input; no negotiation/boot deadlock | Architecture |
| OI-021 | CardKB2 supply rail records conflict | Older wiring says 3.3V; newer power-test plan says 5V; confirm actual module supply node | Power budget |

Proceed with independent research and parameterized calculations. Do not replace unresolved battery or source limits with guessed ratings, generate fabrication outputs, or declare Phase 1 passed.
