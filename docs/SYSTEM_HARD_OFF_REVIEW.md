# System Hard Off Review

Status: **series gate wired; full HARD OFF qualification FAIL / OPEN**. Not TOTAL_ISO, BAT_ISO or shipping mode. No MCU or firmware shutdown is substituted for the power gate.

## Actual Circuit

`U701 OUT -> UPS_RAW -> U1301 TPS22953DQCR -> SYS_RAW -> U801 / U901 -> 3V3 /5V`;3V3_SENS remains a filtered3V3 branch. C703 moves to UPS_RAW. J1301 is the external maintained SPDT switch interface: common2 to1 ON, common2 to3 OFF; open/unplugged defaults OFF through R1302. R1301 limits switch-wire current. Connector/ESD/contact specifications remain OPEN. U1301 IN1/2 and BIAS3 are upstream; OUT9/10 and SNS8 downstream; unused PG7 grounded; CT6 uses10nF50V. Logical EP11 is grounded but its physical footprint pad mapping is not released.

[TI TPS22953 RevD](https://www.ti.com/lit/gpn/TPS22953), tables5/6/7 and sections9.3/11, was checked against the actual pin diagram. TPS22953 supports disabled-state reverse blocking, not QOD; TPS22954 is not an equivalent substitution. The datasheet's5A ceiling is not a qualified board rating. Independent tests reject bypassing the series gate, connecting a regulator upstream, moving the charger downstream, MCU control of MASTER_EN, wrong device variant, absent pulldown and inadequate CT rating. Internally common OUT9/10 use one power_out plus one passive pin, with both nets and types enforced, not hidden independent drivers.

## OFF And Charging

| Item | Actual result / outstanding proof |
| --- | --- |
| Mechanical OFF with valid upstream supply | MASTER_EN pulled low by R1302; OFF throw drives DISCHARGE_EN high without MCU/I2C |
| Switch unplugged |100k pulldown; selected100nA leakage limit gives10.1mV worst resistor corner, below560mV minimum falling threshold |
| SYS_RAW | Input disconnected by candidate gate; R1307/Q1301 active discharge plus4.7k fallback; complete residual/time not qualified |
| 3V3_MAIN | R1308/Q1302 active discharge plus4.7k fallback; regulator reverse paths and decay not qualified |
| 5V_MAIN | R1309/Q1303 active discharge plus4.7k fallback;220uF zero-injection example about125ms after FET conduction, about4s if active bias absent |
| 3V3_SENS | Discharges through ferrite/3V3 bleeder; attached module/cable injection not bounded |
| PD / EXT / charger / gauge / protection | Not intentionally downstream of master gate; incomplete input/guard sheets still prevent full-chain proof |
| Charging while OFF | Architecture retains charger power, but charging remains physically DISABLED by R407 DNP and TS lock. Normal OFF-state charging is NOT demonstrated or authorized |
| USB/EXT insertion while OFF | Cannot command EN high through defined gate-control nets; full auxiliary/interface injection audit still OPEN |
| Reverse current | Disabled-state blocking is specified; finite leakage and typical-only response are not zero-current guarantees. Enabled-state reverse blocking is NOT claimed |

## Backfeed Audit

- Positive upstream power nodes have no netlisted direct bridge to SYS_RAW around U1301; exact UPS_RAW/SYS_RAW/control terminal sets are checked independently.
- Gauge SDA/SCL/GPOUT and charger status connect across powered/unpowered domains. Present pullups use3V3_MAIN, not an always-on rail; this alone does not prove zero leakage. Verify IC pad structures and every attached bus pullup.
- MainBoard USB/debug/external peripheral power remains a potential bypass of the system boundary; connector contract and isolation are not defined. A reverse-blocking master gate can prevent current returning upstream without preventing an externally injected downstream rail from powering Core. OI-022 remains OPEN.
- Gate bias and switch-divider current originate after the shunt in battery mode. Existing gauge BAT/C601 and Kelvin-bias exceptions are unchanged; no new upstream-shunt system load was introduced. OI-010 remains OPEN until sheet05 and fault/DNP audit are complete.

## Power And Timing

Three4.7k bleeders consume about12.96mW at RAW5V/3V3/5V, excluding conversions. They remain necessary passive fallback when upstream bias disappears or the switch is unplugged. No instant-off claim.

## Mechanically Selected Active Discharge

J1301 pin2 now receives UPS_RAW through R1301=10k; pin1 is MASTER_EN, pin3 is DISCHARGE_EN. This is a changed switch-harness electrical assignment, not a compatible replacement for the previous grounded OFF throw. R1302 and new R1306 are separate100k pulldowns. A maintained break-before-make SPDT is required; exact switch/connector ratings remain OPEN. No powered logic inverter or new MCU is required.

Q1301/Q1302/Q1303 are DMN2056U-7 candidates, each with gate on DISCHARGE_EN, source at GND, and drain through its own150ohm1% resistor to SYS_RAW/3V3_MAIN/5V_MAIN respectively. Drain nets are separate; body diodes point from GND toward the drain. The symbol uses standard SOT23 G1/S2/D3; actual footprint release is still prohibited. R1307/8/9 carry a0.5W reference requirement, not an approved resistor MPN.

[Diodes DMN2056U DS38480 Rev2-2](https://www.diodes.com/assets/Datasheets/DMN2056U.pdf), pages1-3, supplies the top-view terminal identities, exact orderable and25C electrical limits: RDS maximum85mohm at VGS1.5V, gate leakage100nA magnitude, drain leakage1uA at VGS0/VDS20V. These are selected conditions, not guaranteed hot leakage or a guaranteed ON-state leakage budget at a small nonzero gate voltage.

With UPS_RAW=2.5V, R1301 high/R1306 low1% corners and three100nA gate leakages, calculated gate drive remains above2.26V. This is above the1.5V RDS table condition; it does not use threshold voltage as a full-conduction rating. With no switch drive, three100nA leaks and R1306 high give30.3mV; hot leakage, Miller coupling and gate transitions remain unqualified.

At an assumed rail maximum5.5V and minimum148.5ohm series resistance, each active branch draws at most37.04mA and initially dissipates at most203.7mW in the resistor, ignoring positive FET resistance conservatively. A220uF capacitor at5.5V stores3.3275mJ. Resistor pulse/temperature derating and sustained external injection must still be checked. If both throws momentarily connect, or FET gates retain charge during transition, the series resistors bound rail loading; zero overlap is NOT assumed. All three branches may load simultaneously, so the board budget must include their additional conversion losses and current.

For nominal220uF,150ohm,4.7k and the selected25C85mohm RDS corner, the5V-to0.1V RC example is about125ms after FET conduction. This is not elapsed switch-to-OFF time and excludes extra load capacitance, converter energy transfer, tolerances, hot RDS, bounce and injection. Absent UPS bias, only the approximately4.045s passive example applies. Neither example guarantees Core is electrically unpowered when a debug source backfeeds a rail.

At an illustrative3A and the selected2.5V/105C datasheet27mohm corner, gate drop is81mV and dissipation243mW. Using the DQC datasheet65.2C/W gives15.84C rise as an example, NOT an enclosure or full voltage-range guarantee. Source/regulator losses and steady/pulse loads are still unknown. Input range, transition dip, CT slew/inrush, bounce and effective low-profile capacitor selection must pass before freezing U1301. No large electrolytic is approved.

Reproducible selected-corner arithmetic is retained in `reference_validation.json -> candidate_operating_limits -> master_gate_candidate`. All four system rails are intended OFF; **none has a fully proven numerical residual/time limit yet**.
