# Battery Board Guard - Wired Candidate, Not Qualified

Sheet05 now contains J501, U501 BQ29700DSER, Q501/Q502 AON3402, R501-R506 and C501. It is no longer an empty page. This does not approve the candidate, close OI-010, authorize assembly or resolve any pack-source ambiguity.

## Actual Paths

```text
J501:1 BAT_PLUS -> R501 -> BAT_GUARDED -> R601 I1/I2 -> BAT_PROTECTED
                                                     |-> U401 BAT (charge locked)
                                                     |-> U701 IN2 -> UPS_RAW -> master gate
J501:2 BAT_MINUS <- Q501 S/D <- R506 <- Q502 D/S <- GND
```

R501 is a positive high-current link, not a bypass around the negative-side switch. Its exact current-rated MPN remains OPEN. R506 is a25mohm1%2W candidate series sensing resistance, not a second gauge shunt. It is between the two MOSFET drains. No BAT_MINUS-to-GND link is fitted. Connecting a grounded probe, shield or service cable to BAT_MINUS can defeat low-side protection and is not authorized.

U501 pins:1 NC;2 COUT;3 DOUT;4 VSS=BAT_MINUS;5 BAT via330ohm R502 with100nF C501 to BAT_MINUS;6 VM via2.2k R503 to GND. Each gate has5.1M to its own source. Q501/Q502 physical pins1/2/3 source,4 gate,5/6/7/8 drain. The exact exposed-pad footprint mapping is not released. [TI BQ2970 Rev I, pin table and Figure9-1](https://www.ti.com/lit/ds/symlink/bq2970.pdf), [AOS AON3402 Rev2.1, pages1-2](https://aosmd.com/pdfs/datasheet/AON3402.pdf).

## Limits That Prevent Release

- BQ29700 nominal OV4.275V and UV2.8V are board-candidate detector settings, NOT adopted battery PCM thresholds or a charge-voltage decision. UV accuracy is specified at25C; OV accuracy at0..60C does not cover the entire pack discharge range. Keep charging disabled.
- OCD100mV has +/-15mV accuracy over-40..85C and16..24ms delay. Short-circuit delay125..375us does not provide a hard5A peak clamp. No >1C pulse capability is inferred.
- Ignoring bias, TCR and dynamics,115mV/(25mohm*0.99)=4.646A is a conditional detector upper bound, not a board rating. Including two AON3402 maximum26mohm resistances at1.8V gate/25C gives an early-trip example85mV/(25.25+52)mohm=1.10A. At2.5V gate with17mohm each it is1.43A. This exposes a possible load conflict; it does not establish the actual load requirement or full-temperature trip bounds.
- R506 nominal loss at3A is0.225W. FET losses, hot resistance, copper, resistor TCR/pulse/derating and connector losses remain unqualified. AON3402's headline current is not an enclosed-board rating; its low-gate resistance specifications are not hot guarantees. U501 gate-drive behavior also needs checking at the chosen loaded-voltage floor.
- First pack insertion can leave discharge disabled until charger attachment or a momentary VM-to-VSS service action. No automatic startup claim or permanent VM short is allowed. Recovery/OV fault-state limitations and coordination with the pack PCM remain OPEN.
- Opening low-side FETs does not physically isolate BAT_PLUS. Reverse insertion, connector faults, charger-present faults, gauge/input common-mode behavior and bypass via external connections require separate verification. No TOTAL_ISO requirement was added.

## Gauge Exceptions And Checks

Main positive charge/discharge terminal paths join R601's load side exactly once in the installed netlist. U501 supply/filter/gate charging and gate-source bleeders, plus VM fault paths, are explicit unmeasured auxiliary paths upstream of R601. Existing U601 BAT supply, C601, sense-input bias/probe leakage and internal pack PCM exceptions remain. R506 and FET losses are series losses, not extra parallel loads.

Exact node-set checks now cover BAT_PLUS, BAT_MINUS and every guard control/drain node, in addition to the shunt and local charger SYS. Regression mutations reject grounding the pack negative, changing filter return, reversing a source terminal, wrong threshold variant/FET, shorting R506, moving bias across the shunt and changing a power input to passive. These are connection/model checks, not an analog fault simulation or complete leakage proof. OI-010 stays OPEN.

The two new ERC power-source errors are U501 BAT and VSS. J501 is a passive logical connector and R502 a passive filter; the supply declaration has not been falsely replaced with a board power output. Existing U601 BAT source modeling also remains unresolved. See the root Phase1 review for all six errors and four warnings.
