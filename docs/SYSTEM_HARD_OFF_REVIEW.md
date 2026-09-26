# System Hard Off Review

Status: **series gate wired; full HARD OFF qualification FAIL / OPEN**. Not TOTAL_ISO, BAT_ISO or shipping mode. No MCU or firmware shutdown is substituted for the power gate.

## Actual Circuit

`U701 OUT -> UPS_RAW -> U1301 TPS22953DQCR -> SYS_RAW -> U801 / U901 -> 3V3 /5V`;3V3_SENS remains a filtered3V3 branch. C703 moves to UPS_RAW. J1301 is the external maintained SPDT switch interface: common2 to1 ON, common2 to3 OFF; open/unplugged defaults OFF through R1302. R1301 limits switch-wire current. Connector/ESD/contact specifications remain OPEN. U1301 IN1/2 and BIAS3 are upstream; OUT9/10 and SNS8 downstream; unused PG7 grounded; CT6 uses10nF50V. Logical EP11 is grounded but its physical footprint pad mapping is not released.

[TI TPS22953 RevD](https://www.ti.com/lit/gpn/TPS22953), tables5/6/7 and sections9.3/11, was checked against the actual pin diagram. TPS22953 supports disabled-state reverse blocking, not QOD; TPS22954 is not an equivalent substitution. The datasheet's5A ceiling is not a qualified board rating. Independent tests reject bypassing the series gate, connecting a regulator upstream, moving the charger downstream, MCU control of MASTER_EN, wrong device variant, absent pulldown and inadequate CT rating. Internally common OUT9/10 use one power_out plus one passive pin, with both nets and types enforced, not hidden independent drivers.

## OFF And Charging

| Item | Actual result / outstanding proof |
| --- | --- |
| Mechanical OFF with valid upstream supply | EN clamped GND; upstream series switch commanded off without MCU/I2C |
| Switch unplugged |100k pulldown; selected100nA leakage limit gives10.1mV worst resistor corner, below560mV minimum falling threshold |
| SYS_RAW | Input disconnected by candidate gate;4.7k discharge resistor; complete leakage/residual/time not qualified, NOT claimed exactly0V |
| 3V3_MAIN | Downstream of gate,4.7k bleeder; regulator reverse paths and decay not qualified |
| 5V_MAIN | Downstream of gate, default-off EN plus4.7k bleeder;220uF nominal alone can take about4s to0.1V with zero injection |
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

Three4.7k bleeders consume about12.96mW at RAW5V/3V3/5V, excluding conversions. They are explicit discharge provisions, not a hidden keep-alive load. This is a low-power tradeoff awaiting a shutdown-time requirement; an active discharge alternative must also prove operation when upstream power vanishes. No instant-off claim.

At an illustrative3A and the selected2.5V/105C datasheet27mohm corner, gate drop is81mV and dissipation243mW. Using the DQC datasheet65.2C/W gives15.84C rise as an example, NOT an enclosure or full voltage-range guarantee. Source/regulator losses and steady/pulse loads are still unknown. Input range, transition dip, CT slew/inrush, bounce and effective low-profile capacitor selection must pass before freezing U1301. No large electrolytic is approved.

Reproducible selected-corner arithmetic is retained in `reference_validation.json -> candidate_operating_limits -> master_gate_candidate`. All four system rails are intended OFF; **none has a fully proven numerical residual/time limit yet**.
