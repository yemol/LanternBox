# V2-PD-R0 Schematic Review

Checkpoint: 2026-09-27. Scope: actual nine-page KiCad schematic and exported XML.
**STATIC CONNECTIVITY PASS. PHASE 1 RELEASE FAIL / OPEN. NO PCB AUTHORIZATION.**
Not a charger-enabled prototype. Do not connect an unqualified battery for charging.

## Implemented Architecture

| Sheet | Implemented circuit |
| --- | --- |
| 01 | USB-C, STUSB4500, VBUS fuse/TVS, CC ESD, private NVM service bus |
| 02 | TPS259474L input protection, PD-controlled enable, independent UV/OV dividers |
| 03 | BQ25638 NVDC, inductor/decoupling, 9V-contract-controlled ILIM, CE/TS locks |
| 04 | MAX17055, protected-pack connector, four-terminal 5mohm shunt |
| 05 | TPS22953 terminal gate, TPS3808 UV supervisor, physical ON/OFF |
| 06 | TPS63802 3V3 buck-boost, filtered sensor branch |
| 07 | TPS61022 5V boost and feedback/feedforward |
| 08 | Terminal interface, switched I2C/status pullups, rail test connector |

BQ25638 replaces the earlier low-current charger candidate rather than adding a MUX.
STUSB4500 is selected for explicit selected-PDO outputs; no PD source role or runtime MCU is added.
TPS25730 was evaluated but not adopted: CAP_MIS is an event-toggle signal, not a steady 9V authorization level.
The charger package is 0.4mm-pitch DSBGA; assembly inspection/rework capability is still a manufacturing gate.

## ERC And Netlist Evidence

`erc.json` has zero errors and zero warnings. No violation exclusions were added.
KiCad's default ignored rule classes are recorded in the JSON: singleton global labels, four-way junctions,
SPICE-model issues and footprint filters. Footprints are not assigned; that is a release gate, not a PCB pass.

The initial ERC issue was the unused open-drain TPS22953 PG pin tied directly to flagged GND,
which TI permits for unused PG. It is now a usable PG probe node with a 100k pullup to switched SYS_SW.
No pin type was weakened and no error was excluded. The CT capacitor was also corrected to 50V;
TI requires at least 25V on CT even in this low-voltage application.

Five PWR_FLAG symbols declare external USB power, fused USB power, external ground return,
protected pack positive and protected pack negative. They add no physical connections.
There are no artificial flags on SYS, regulator outputs, enables or fault lines.

The verifier compares actual exported pin connectivity and population state, and independently checks
the main source path, no-EXT/no-MUX boundary, switched host pullups, charge locks and PACK_NEG whitelist.
Mutation tests cover a return-domain violation, reversed gauge reference, broken input path,
always-on host pullup, installed charge arm, wrong PD voltage and an ERC warning.
These are logical checks, not analogue simulation or proof that custom symbols match every silicon behavior.
The checkpoint has 13 new tests (8 netlist and 5 PD budget), plus 14 superseded-architecture regression tests.
`test_results.txt` records the actual run; the historical tests are not counted as new V2 hardware evidence.

## Gauge Current-Path Proof

Main discharge loop: PACK_POS -> charger BATFET -> SYS_AON -> gate -> converters/load -> GND
-> R401 force terminal 2 -> force terminal 1 -> PACK_NEG.
Main charge loop: charger BAT -> PACK_POS -> cell/PCM -> PACK_NEG -> R401 -> GND -> charger return.
Each main battery current crosses R401 once. USB-only system current need not cross the battery shunt.

Actual PACK_NEG node whitelist: J401.2; R401.1/.3; U401.6/.2/.11; C401.2; C402.2.
Only R401 and U401 span PACK_NEG and GND. U401 is the intentional differential measurement device,
not a second main current conductor. No USB shield, host return, charger capacitor or external ground
connects to PACK_NEG. The checker rejects additions, including an unintended capacitor bypass.

Auxiliary currents: charger BATP sensing and BAT/SYS bias, UV divider/supervisor, gate leakage,
host pullup leakage and reverse input leakage return to GND and therefore cross R401 when battery-fed.
MAX17055 self-supply (BATT to CSP) and its local bypass capacitors intentionally remain pack-referenced;
this bias is not measured by R401. Internal pack PCM current is also unmeasured and not source-qualified.
Do not connect grounded instrumentation to PACK_NEG while another connection grounds system GND.

This establishes no unintended *logical* bypass in this V2 checkpoint. Physical Kelvin routing,
four-pad shunt selection, internal gauge-current accounting and calibration remain OPEN.
V1 OI-010 is not silently closed by evidence from a different V2 circuit.
5mohm causes 25mV at 5A and about 10mV at 2A charge; charger BATP sensing versus board GND must be
included in the final cell-terminal voltage accuracy analysis, not just the IC's nominal VREG.

## PD And Input Policy

NVM manifest: PDO1 5V/0.1A, PDO2 9V/2A, PDO3 9V/3A; POWER_OK_CFG=2.
Both matching 9V open-drain flags feed a PNP/NMOS level interface to the ILIM resistor branch.
The flag pins do not carry ILIM current directly. Q201 is a signal PMOS controlling eFuse EN, not a power MUX.
VBUS_EN_SNK independently controls input enable. POWER_OK signals can retain state after detach;
detach/reattach, hard reset and downgrade require scope verification, not just a static truth table.

Factory NVM includes unwanted 15V/20V profiles. Program and read back the device before functional testing.
The manifest is not a raw image, not a successful programming log and not a field update mechanism.
The independent nominal 10.416V eFuse OVP is a protective boundary, but TVS surge coordination is not qualified.
The private programming connector pin 4 is USB VBUS, NOT a 3.3V logic supply. Fixture pullups need their own 3.3V.
STUSB4500 I2C pins cannot share the host gauge bus because the controller is USB-only powered.

ILIM baseline 47k and switched 2.43k set a conditional 1.28..1.60A 9V input range.
Below 400mA, the manufacturer's 1.2k + 330nF ILIM filter is fitted.
**Sub-100mA 5V operation is not qualified; no reliable dead-battery/no-battery fallback is claimed.**
Do not tie a Type-C 3A-advertisement flag into this permission signal without resolving PD-contract precedence.
The eFuse L variant is a latched circuit breaker, not a precision continuous current clamp.
Its approximately 2A trip and fast-trip overshoot require fuse, connector and source coordination.

## Power And OFF Budgets

See generated `pd_budget.json` for min/max resistor calculations and all assumptions.
8W shared output, 85% rail/path efficiency, 0.1W auxiliary and 3.4V loaded USB SYS floor are qualification targets.
The steady power bound gives about 4.45A SW current at the high input corner, below 5A recommended.
It does not bound inductor ripple, soft start, short circuits, reverse current or thermal dissipation.
Typical charge headroom at full proposed load is roughly 0.5A; worst-case can approach zero.
The lower-tolerance current setting, cable loss and temperature require real validation.

Battery-only OFF allocation: charger 3uA, gauge hibernate 12uA, gate 11uA, supervisor 6uA,
divider/control 2uA, pack PCM 10uA and reverse/I/O leakage 5uA: **49uA allocated, not guaranteed**.
Active gauge adds 18uA. R503 was increased to 470k and R504 to 4.7M: UV asserted now adds at most
about 7.4uA in the calculation, rather than 34.8uA with the original 100k pullup.
The enable high remains above 2.6V with the stated leakage allocation, above the 1.3V design guard.
Active-gauge low-battery allocation is about 74.4uA, not a measured maximum.
Gauge >50C and PCM current remain unknown.
Neither <100uA across all states nor <50uA has passed. Do not shut down the gauge to claim compliance.
At 100uA, annual drain is 876mAh before self-discharge; HARD OFF is not indefinite storage isolation.

## Abnormal-State Review

| Condition | Implemented behavior | Remaining evidence |
| --- | --- | --- |
| USB attach | PD plus eFuse, conservative initial ILIM | Cold/dead-pack startup and inrush scope capture |
| USB removal | NVDC battery supplement, downstream rails stay on if pack supports load | Rail droop and audio/radio transient capture |
| Low battery | TPS3808 opens terminal gate near nominal 3.30V SYS | Divider/input-current tolerance, rebound cycling and loaded pack floor |
| Physical OFF | SPDT grounds gate EN; rail bleeders discharge outputs | External I/O backfeed and OFF leakage over temperature |
| Long storage | Terminal off, protection/gauge remain active | Storage interval/PCM quiescent drain; service disconnect policy |
| Charge fault | CE arm absent and TS fault block all charging in this draft | Approved cell profile, actual NTC and charge timer validation |
| Output short | Converter protection, gate supervision and pack PCM exist | Peak/retry current and temperature; pack thresholds unknown |
| Unsupported PD voltage | Input OVP blocks charger path | Fault clamp overshoot; 12V TVS/fuse final selection |

## Release Gates Still OPEN

1. Battery source conflict 4.25V CC / 4.2V CV; charge temperature; actual NTC; PCM thresholds;
   connector/wire gauge; loaded pack minimum; >1C peak duration. No inferred closures.
2. Usable 5V fallback and PD programming/readback, downgrade and startup sequencing.
3. Actual load concurrency, inrush and peak power; shared 8W envelope is only a proposal.
4. Full-temperature OFF budget, including UV asserted, active gauge and pack protection.
5. Final TVS/fuse/shunt/connector/passive MPNs, tolerances, DC bias, pulse ratings and all footprint maps.
6. Real TS network and charging release, 5000/10000mAh timer/profile qualification. R302 remains DNP.
7. Thermal and transient proof; voltage-noise targets and radio/audio/display interactions.

These gates prevent complete Phase 1 PASS and PCB entry. A clean ERC is not a reason to waive them.

## Primary Sources

- [STUSB4500 datasheet](https://www.st.com/resource/en/datasheet/stusb4500.pdf): QFN pin map, PDO status, NVM defaults, dead-battery and I2C behavior.
- [BQ25638 datasheet](https://www.ti.com/lit/ds/symlink/bq25638.pdf): Rev B, pin map, NVDC, 5A recommendation, ILIM filter, reset defaults and charge timers.
- [TPS25947 datasheet](https://www.ti.com/lit/ds/symlink/tps25947.pdf): input protection, divider limits and circuit-breaker behavior.
- [MAX17055 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17055.pdf): gauge return domains and self-supply conditions.
- [TPS22953 datasheet](https://www.ti.com/lit/ds/symlink/tps22953.pdf): gate pin map, PG use, CT rating and off leakage.
- [TPS3808 datasheet](https://www.ti.com/lit/ds/symlink/tps3808.pdf): threshold tolerance, sense leakage, reset delay and current.
- [TPS63802 datasheet](https://www.ti.com/lit/ds/symlink/tps63802.pdf): 3V3 reference circuit and operating limits.
- [TPS61022 datasheet](https://www.ti.com/lit/ds/symlink/tps61022.pdf): 5V boost reference, compensation and switching-current limits.

Battery source documents under repository docs are preserved unchanged; they are not superseded by IC defaults.
