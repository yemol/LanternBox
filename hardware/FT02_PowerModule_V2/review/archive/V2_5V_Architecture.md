# FT-02 Power Module V2 - SUPERSEDED 5V-only proposal

Historical comparison only. The active design is ../../README.md and the V2-PD-R0 KiCad project.
The old block diagram, power_budget.json and power_budget.py describe this superseded proposal, not the PD circuit.

Architecture proposal, 2026-09-27. **NOT RELEASED. No V2 schematic/ERC/PCB pass is claimed.**

This is a new, simplified design direction, not another layer on PowerBoard V1.
V1 at `358b365` remains historical WIP. Its schematic, protection circuit,
reports and unresolved issues are not silently adopted or marked closed here.

## 1. System Diagram

![V2 architecture](FT02_PowerModule_V2_Block_Diagram.svg)

```text
USB-C 5V/3A -> ESD + input OVP/OCP/inrush protection -> NVDC charger
                  ^                                   |      |
      USB-only CC current authorization                SYS    BAT
                                                       |      |
                                              hardware gate  protected 1S pack
                                                       |      |
                                            +----------+      PACK_NEG
                                            |          |      |
                                       3V3 buck-boost  5V     5mohm Kelvin shunt
                                            |        boost    |
                                       all 3V3 loads  loads   GND

MAX17055: BATT=pack positive; CSP=PACK_NEG; CSN=GND.
Charger, pack protection and gauge stay alive when the hardware gate is OFF.
```

The 5V converter must be **boost**, not buck: a 1S battery/SYS supply can be
below 5V. The diagram is a functional proposal, not a pin-connected schematic.

## 2. Power-Path Decisions

- Use one NVDC charger for USB-to-SYS, USB-to-battery and battery-to-SYS.
  BQ25628 supports this topology; an extra source/UPS MUX is not required by
  the stated two-source use case.
- USB power goes to the system first; input DPM reduces charging as input
  power runs out. NVDC can then **supplement from the battery**. It does not
  guarantee zero battery discharge whenever a cable is attached.
- This explicitly supersedes V1's strict external-valid/no-battery-supplement
  policy. Persistent supplementation is overload, not free extra USB power.
- User-approved policy: the two rail ratings share total power. Their separate
  capabilities do not promise simultaneous 3V3/2A plus 5V/1.5A operation.
- USB removal transfers to the internal BATFET. Continuity is a design target,
  not yet a tested result. Scope SYS, both rails and ESP reset at worst load,
  minimum qualified battery voltage, cable bounce and temperature corners.
- The only system hard gate is downstream of SYS. No USB or battery bypass
  may reach a terminal rail. OFF must dominate firmware and regulator EN.
- No added always-on MCU. Charge safety cannot depend on a running ESP32.

## 3. Remove / Retain

| V1 function | V2 disposition |
| --- | --- |
| USB PD controller, 9V/12V negotiation | Remove |
| EXT_5V input and external-source selection | Remove; USB-C is the sole external input |
| TPS2121/UPS MUX, external power ORing | Remove from V2, not rejected merely on the pack's 5A rating |
| Separate charger-only SYS island | Remove; charger SYS becomes the main always-on source |
| GPIO expander, keep-alive and active multi-rail discharge network | Remove |
| Battery fuel gauge | Retain function; use MAX17055 plus 4-terminal shunt |
| System hardware switch | Retain as one downstream load gate |
| 3V3_SENS | Passive filtered branch of switched 3V3; no new power controller |
| Pack protection | Retain qualified pack-level protection; never support bare cells |
| V1 duplicate board battery guard | Do not carry forward by default; pack qualification is a release gate |
| USB ESD, input protection and CC detection | Retain; CC current detection is not USB PD |
| Hardware undervoltage shutdown | Retain minimum necessary supervisor function |

Removing V1's guard does not establish that an unknown pack PCM is sufficient.
Thresholds, delay, recovery and reverse-connection behavior still need review.

## 4. Main Chips

Selections below are architecture choices, **not a production-frozen BOM**.

| Function | V2 choice | Decision / boundary |
| --- | --- | --- |
| Standalone NVDC charger | **BQ25616RTWR**, non-J, primary candidate | Hardware charge settings and autonomous OFF charging; 3.2A recommended SW output requires the joint load/charge gate below |
| Fuel gauge | **MAX17055ETB+T**, TDFN | SOC, voltage and signed current; 5mohm 4-terminal shunt |
| 3V3 buck-boost | **TPS63802DLAR** | 2A target; qualify low-input current, ripple, thermal and noise |
| 5V boost | **TPS61022RWUR** | 1.5A target; its switch current is not an output-current rating |
| System load gate | **TPS22953DQCR** | Entire terminal downstream; gate leakage included in OFF budget |
| USB-C CC detector | **TUSB320LAIRWBR** candidate | Sink-only, GPIO mode, USB-powered; no PD stack |
| Input protection / CC supply / UV supervisor | Selection not frozen | Only these essential support functions; exact parts and thresholds need pin-level review |

### Why Not Freeze BQ25628?

BQ25628 can provide the required NVDC path, and has lower battery-only
quiescent current. It is not rejected on topology or battery capacity.
However its POR charge current is **320mA**. Ideal CC-only time is 15.625h
for 5000mAh and 31.25h for 10000mAh. Its Rev C timer prose and register
tables disagree (12h versus 14.5h default; table also offers 28h).
Neither is an adequate basis for guaranteeing autonomous 10000mAh charging
after POR with the ESP32 held OFF. Do not assume an earlier I2C setup survived
battery replacement or watchdog/reset behavior.

The BQ25616 alternative solves the **configuration dependency**, without
adding another MCU or MUX. It does not itself close all current and timer gates.
It trades higher OFF current and a 3.2A SW recommendation for hardware setup.

Candidate RICHG=372ohm, 1% gives 1.820A nominal. Conservative KICHG/tolerance
arithmetic gives 1.701..1.941A, below the current pack's 2.5A standard rate.
At the minimum computed current, 10000mAh takes 5.88h of ideal CC charging.
The safety timer is **8h minimum / 10h typical / 12h maximum**, leaving only
2.12h in that idealized check for CV and other effects. Precharge has its own
2h timer. These figures are not proof that either real pack will finish.
Test deep-depletion recovery, CV tail, temperature/DPM and timer behavior for
each pack. Do not disable safety timing to claim compatibility.

Charge remains physically inhibited until the battery profile is approved.
VSET open selects the nominal 4.2V family, not an approved cell voltage.
VSET shorted to ground selects 4.35V and is **not** a safe default here.

Higher-current BQ25895 was also screened, not selected: its 5A recommended
SW output improves current headroom, but BATFET-on standby is 32uA typical /
60uA maximum under specified conditions. Substitution alone raises the
hibernate allocation to 102uA. Its default high-voltage handshakes and
unknown-adapter 500mA limit also require attention. Do not substitute it
solely on its 5A headline, or reintroduce forbidden high-voltage negotiation.

BQ25896 is the more relevant high-current follow-up: it uses PSEL/ILIM
rather than D+/D- negotiation, supports 5A recommended SW output, and defaults
to 2.048A autonomous charging. Unlike BQ25616, its enabled ILIM clamp applies
as the lower of hardware and register limits. However, its BATFET-on
standby maximum is also 60uA in the specified condition, yielding the same
102uA hibernate allocation with the present surrounding circuit. Neither a
smaller pack-PCM allowance without evidence nor the ship-mode current may
be used to turn this into a pass. This is a bounded fallback, not an additional
charger to populate. Final charger selection remains V2-04 OPEN.

## 5. Maximum Power / Current Budget

| Quantity | Value | Evidence class |
| --- | --- | --- |
| 3V3 rail target | 2A / 6.6W | User requirement, individually |
| 5V rail target | 1.5A / 7.5W | User requirement, individually |
| Both maxima summed | 14.1W | Arithmetic, not admitted shared load |
| USB nominal input | 15W | 5V x 3A, only after source authorization |
| USB low-voltage example | 14.25W | 4.75V x 3A, before losses |
| Starting aggregate output target | **8W** | Proposed validation envelope, not a measured rating or user-supplied load |
| Rail efficiency floor | 85% | Qualification assumption; not guaranteed by a headline datasheet figure |
| USB-mode loaded SYS floor | 3.4V | Qualification assumption; no-load SYSMIN is not proof |
| Miscellaneous SYS allowance | 0.1W | Provisional allocation |
| Battery loaded connector voltage | 3.0V | Calculation example, not a pack specification or final UV threshold |
| Pack-to-converter path resistance | 0.100ohm maximum | Qualification target, includes shunt, wiring, BATFET and gate |

At the proposed 8W envelope, 3V3/2A leaves at most 5V/0.28A. Conversely,
5V/1.5A leaves at most 3V3/0.152A. Real peripheral concurrency must fit this
envelope or the charger/converter choice must be revised. Do not silently
drop required terminal loads to make the budget pass.

At 8W and the above assumptions, SYS load is 2.798A at 3.4V. A 10% design
reserve on BQ25616's 3.2A SW recommendation leaves 2.88A total, or just
0.082A charging headroom. **Fixed 1.82A charging plus this load fails**:
4.618A is above even the 3.2A recommendation.

For each operating point check:

```text
Psys = P3V3/eta3V3 + P5V/eta5V + Paux
Isys = Psys / Vsys
Ichg_allowed = min(Ichg_set,
                  max(0, Isw_budget - Isys),
                  max(0, (Vin * Iin_authorized * eta_charger - Psys) / Vbat))
```

This is an **acceptance calculation**, not an implemented charge controller.
Input DPM does not explicitly guarantee the selected SW-current reserve.
The proposed 8W envelope is therefore NOT yet approved for USB operation
with charging enabled. Input-current selection, DPM and worst-case charge
current must be proven jointly; peak cycle-by-cycle OCP is not a substitute
for recommended continuous operation. If this cannot be bounded simply,
replace the charger with a suitably rated autonomous NVDC device; do not add
a power MUX to conceal this problem.

Battery-only arithmetic uses `I * (Vpack - I*Rpath) = Psys`. At 3.0V,
0.100ohm and 8W it gives approximately 3.60A and 2.64V at the converter.
Validate converter current/ripple at this lower voltage, not at 3.0V.
The path's 5A design target is independent of 5000/10000mAh capacity.
The present pack's **5A source ceiling remains binding**; no >5A burst is
authorized. Path copper, connectors and protection are not qualified by this
calculation. The 10000mAh pack needs its own current/impedance qualification.

No transient allowance is frozen. Characterize audio peaks, simultaneous
radio/display startup, output capacitance charging and input transitions.
Include inductor ripple and tolerances when proving battery current stays
within 5A. Output overload protection must also prevent sustained or repeated
pack abuse; the gauge is not a fast protective current limiter.

## 6. OFF / Storage Budget

Scope: battery-only, physical switch OFF, no externally driven I/O/debug,
charger OTG disabled, gauge still measuring. This is not the USB input current
while charging. USB-connected charger bias can be mA even with the system OFF.

| Contributor | Budget (uA) | Status / conditions |
| --- | ---: | --- |
| BQ25616 battery standby | 15 | Datasheet maximum at specified battery/Tj conditions, not every board corner |
| MAX17055 hibernate | 12 | Datasheet maximum, TA <=50C, excluding external thermistor |
| TPS22953 bias + off-input leakage | 11 | Conservative allocation from 3.3V/5V table points; full operating range still to verify |
| UV supervisor | 2 | Unselected part allocation |
| Pack PCM | 10 | Allocation; supplier current unknown |
| Input reverse leakage and signal leakage | 5 | Allocation; circuit not yet qualified |
| Remaining control resistors | 2 | Allocation; final pin states must prove this |
| **Allocated total, hibernate** | **57** | Not a measured or guaranteed maximum |
| **Allocated total, gauge active** | **75** | Uses 30uA active-gauge maximum under its stated temperature condition |

Named-device typical subtotal is approximately 20.5uA with gauge hibernating,
before unqualified support/pack leakage. This suggests <100uA may be viable;
**neither <100uA nor <50uA has passed**. Full -20C..+60C discharge-range
qualification cannot reuse gauge limits specified only through +50C.
Do not use gauge shutdown mode to meet the number: it stops measurement.

50uA consumes 438mAh/year; 100uA consumes 876mAh/year, excluding self-discharge
and aging. HARD OFF is not indefinite storage isolation. Low-voltage hardware
must stop terminal loads before unsafe depletion; pack PCM is the final guard.
Long storage requires an approved storage state and periodic inspection, or
service disconnection of the pack. No extra transport-mode chip is proposed.

## 7. Abnormal Conditions

| Event | Required behavior / remaining evidence |
| --- | --- |
| USB insertion | CC detection and protection before high-current admission; no battery/ESP dependency for source detection; scope inrush and startup |
| USB removal / cable bounce | BATFET supplies SYS; rail capacitors and converter control prevent reset within qualified load range; prove by scope |
| Weak / default-current / 1.5A USB source | Never assume 3A; inhibit high-power input or apply a proven lower limit; no indefinite battery supplement presented as normal charging |
| Battery depleted / absent | USB must support cold-start with staged rail enables; no-battery startup and 200mA charger soft-start phase need load validation |
| Low battery / load rebound | Hardware UV shutdown with hysteresis or restart control; no repeated boot/deplete cycle; exact threshold remains OPEN |
| HARD OFF | Gate and downstream converter EN physically held OFF; charging and gauge remain alive; verify every external connector for backfeed |
| Long storage | Budget includes gauge, charger and pack PCM; no claim that OFF removes cell self-discharge |
| NTC open / short / excessive temperature | Charge inhibited; use actual battery temperature, not a dummy fixed divider |
| Charge timer / regulation fault | Stop charging and expose fault via STAT; no automatic reset loop that bypasses safety timer |
| Output short / hot enclosure | Converter current limit plus qualified pack/input protection; validate repeated retries and thermal stress |
| Reversed battery / external rail injection | Not yet qualified; keyed connector alone is not electrical reverse protection |

## 8. Updated Schematic Proposal

Use five functional pages rather than carrying forward fourteen V1 sheets:

1. **USB_INPUT**: connector, ESD, OVP/OCP/inrush, CC detector, USB-only detector
   supply, source-current admission. No PD and no EXT_5V.
2. **CHARGER_NVDC**: charger, single inductor, decoupling, hardware charge
   profile, physical charge inhibit, NTC, PG/STAT. SYS is the main source.
3. **BATTERY_GAUGE**: protected pack connector, one Kelvin shunt, MAX17055,
   service/test points that cannot bridge the shunt.
4. **SYSTEM_RAILS**: mechanical hard-OFF, UV supervisor, one load gate,
   3V3 buck-boost and 5V boost. Passive sensor filtering only.
5. **HOST_INTERFACE**: switched-rail outputs, I2C/ALRT/PG/STAT and terminal
   connectors. Define pin polarity and backfeed restrictions explicitly.

### Pin-Level Handoff

This is the wiring contract to implement, not an exported netlist.

| Device | Connections to implement / review |
| --- | --- |
| BQ25616 | VBUS24 and VAC1 to protected input if no external ACFET; ACDRV2 NC in that case. PMID23 locally decoupled, never used as a terminal rail. SW19/20 to inductor, inductor to SYS15/16. BTST21 to SW bootstrap; REGN22 locally bypassed. BAT13/14 to PACK_POS. GND17/18 and EP to system GND. |
| BQ25616 control | OTG6 hard GND. CE9 active-low: hardware default inhibit and unpopulated charge-arm link until battery approved. ICHG10 candidate 372ohm to GND. VSET12 unresolved profile, never default short. TS11 real pack temperature network pending. PG7/STAT5 open-drain to switched 3V3 pullups. D+3/D-4/ILIM8 require source-policy closure below. |
| MAX17055 TDFN | BATT10=PACK_POS, 0.1uF to CSP. CSP6 and EP=PACK_NEG. CSN5=GND through separate Kelvin trace to system-side shunt terminal. REG7=0.47uF to CSP. AIN2=CSP for internal-temperature arrangement; THRM9 unused pending final pin audit. NC1 unconnected. SCL3/SDA4/ALRT8 use switched 3V3 pullups. |
| TPS22953 | IN and BIAS=SYS_AON; OUT=SYS_SW; SNS=OUT. Unused PG connection per manufacturer guidance. Hardware OFF forces EN low independently of ESP32. UV condition must inhibit ON. No terminal output ahead of this gate. |
| Converters | Both VIN from SYS_SW; EN defaults low until SYS_SW valid. TPS63802 to 3V3, TPS61022 to 5V. Inductor saturation, ceramic DC bias, compensation/reference networks and inrush are not frozen by the block diagram. |

The mechanical switch carries the enable signal, not several amperes of load.
All load-connected pullups and any rail-bleed resistors are downstream of the
gate. No always-on power LED. Hard-OFF decay time must be measured; add a
targeted discharge element only if the passive arrangement fails a real need.
For quiet 3V3, evaluate forced-PWM noise against standby cost, use appropriate
layout/filtering and measure GNSS/audio/radio interference. Do not promise
low noise from the buck-boost part number alone.

### Gauge Current-Path Contract

The pack connector negative is a dedicated `PACK_NEG`, not board GND.
The **only external high-current connection** between PACK_NEG and GND is
the 5mohm shunt. Charger ground, USB ground, system gate/converters, detector,
supervisor, debug ground and output return all belong on GND.

Charge loop: USB -> charger BAT -> pack -> PACK_NEG -> shunt -> GND -> USB.
Discharge loop: pack -> BATFET/SYS -> loads -> GND -> shunt -> PACK_NEG -> pack.
Charge and discharge system current therefore cross once in this proposal.

Intentional exceptions: gauge self-supply flows PACK_POS -> gauge -> CSP and
does not cross the shunt; pack-internal PCM/self-discharge also does not cross.
Kelvin and digital-interface leakage is not exactly zero and needs an error
budget. Do not describe the gauge as measuring those currents. Check sensor,
shield, mounting, USB/debug and battery-NTC returns for unintended bridges.
**OI-010 stays OPEN** until the actual netlist proves the intended topology;
the block diagram and arithmetic tests do not prove it.

The 5mohm shunt gives 25mV at 5A; worst 1% resistance gives 25.25mV and
0.12625W. Choose a pulse/temperature-rated 4-terminal part, provisionally
>=0.5W. MAX17055's +/-51.2mV range is not a permission to exceed pack current.
At this shunt value, current resolution is 312.5uA and capacity resolution
1mAh; both 5000 and 10000mAh fit the representation.

Gauge POR requires capacity/profile initialization; do not report valid SOC
before initialization or after an unhandled pack change. Gauge startup
requires approximately 3.0V even though its operating range extends lower.
Charging and hard-OFF must still work while host/gauge initialization is
unavailable. Validate signed current using known charge/discharge fixtures.
Combine measured direction with PG/STAT; USB present does not mean charging,
and STAT high alone does not establish a full battery.

### USB Current Authorization

TUSB320LAI in sink-only GPIO mode reports default, 1.5A or 3A advertisement.
Use its internal sink termination as specified; do not blindly parallel
another pair of 5.1kohm resistors. Its recommended VDD maximum is 5.0V:
use a small **USB-only 3.3V supply**, not direct raw 5.25V or switched host 3V3.
The VBUS_DET input requires the manufacturer's series resistor arrangement.
Dead-battery behavior and unattached output states must be checked.

BQ25616's ILIM resistor controls current only for an **unknown** detected
adapter; D+/D- classifications can select other limits. RILIM=187ohm, 1%
would give about 2.43..2.70A in the ILIM-controlled case, but is NOT a final
selection, nor proof of SW-current safety. Leaving D+/D- floating is not
accepted here as unverified proof that all sources enter that case.

Target a compliant C-to-C 5V/3A source. Default/1.5A sources are not promised
full terminal performance. Admission must fail safe on attach, detach and
advertisement downgrade without needing the ESP32. Input protection and
all upstream auxiliary currents must fit the authorized source current.
Exact CC-to-input-limit wiring remains a schematic gate, not a closed feature.

## Release Gates

| ID | Status | Required evidence |
| --- | --- | --- |
| V2-01 | RESOLVED direction | No PD, no EXT, no external/UPS MUX; single NVDC path |
| V2-02 | RESOLVED policy | User explicitly approved shared total power, system priority |
| V2-03 | OPEN | Actual concurrent peripheral load/peaks; validate or revise proposed 8W envelope |
| V2-04 | OPEN | Joint charger SW current, input authorization and charge-current bounds; BQ25616 remains conditional |
| V2-05 | OPEN | Input-protection/CC/UV exact parts, pin wiring, fault/leakage limits |
| V2-06 | OPEN | Charge voltage, temperature, NTC, timer and recovery qualification for each pack |
| V2-07 / OI-010 | OPEN | Netlist proves no unintended shunt bypass; qualify gauge profile and direction |
| V2-08 | OPEN | Hard-OFF leakage/backfeed/decay, UV/rebound behavior, thermal and transient bench tests |
| V2-09 | NOT IMPLEMENTED | V2 KiCad pages, zero ERC errors and reviewed warnings |

Canonical battery evidence remains [BATTERY_SPEC.md](../../docs/BATTERY_SPEC.md).
Its source-backed 5000mAh pack is 3.8V nominal, 3.7V compatible; the new
3.7V-family requirement is not new charge-voltage evidence. Keep OPEN:
CC4.25V/CV4.2V conflict, charge temperature, exact PCM thresholds, NTC,
connector/wire gauge, minimum loaded voltage and >1C duration. No item has
been closed by inference. Charging is inhibited until the relevant evidence
is available. Do not ask the user to re-provide already adopted current and
capacity data; only missing supplier facts or actual load measurements help.

**No PCB, Gerber, drill, pick-and-place or RevA production release.**
Architecture review can continue while charge is physically locked out.
Phase 1 may pass only after actual schematic/netlist/ERC and relevant
electrical gates are satisfied, not after this proposal's arithmetic tests.

## Verification / Sources

Run `python3 -m unittest discover -s hardware/FT02_PowerModule_V2/tools -v`
and `python3 hardware/FT02_PowerModule_V2/tools/power_budget.py` from repo root.
The latter regenerates `review/power_budget.json`. The tests check arithmetic,
including expected failures, not electrical safety or schematic completeness.

Checks completed for this checkpoint: 14 arithmetic tests pass; generated
JSON matches the calculator; SVG parses and its rendered layout was visually
reviewed. The canonical battery-source SHA-256 remains
`409e5458b481c0d98fdc5805732ae342b2d3e72082190b7b517fd8355301663e`.
No V1 tracked design, firmware or battery-source file was modified.

Primary manufacturer references checked for this proposal:

- [TI BQ25616, SLUSDF7A](https://www.ti.com/lit/ds/symlink/bq25616.pdf), sections 7, 8.3, 8.5/8.6, 9.3.3, 9.3.5-9.3.9.
- [TI BQ25628, SLUSEG4C](https://www.ti.com/lit/ds/symlink/bq25628.pdf), recommended current, battery IQ, charge timer and POR register tables.
- [TI BQ25895, SLUSC88C](https://www.ti.com/lit/ds/symlink/bq25895.pdf), recommended SW current, BATFET-on standby, source detection and REG02 defaults; screened, not selected.
- [TI BQ25896, SLUSC76C](https://www.ti.com/lit/ds/symlink/bq25896.pdf), PSEL/ILIM, 5A recommended SW output, POR charge and standby; fallback screening only.
- [ADI MAX17055, 19-8726 Rev 0](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17055.pdf), electrical characteristics, TDFN pins and current/capacity registers.
- [TI TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf), operating range and output-current conditions.
- [TI TPS61022](https://www.ti.com/lit/ds/symlink/tps61022.pdf), switch-current limits, shutdown and application design.
- [TI TPS22953](https://www.ti.com/lit/ds/symlink/tps22953.pdf), bias/off-input leakage and application connections.
- [TI TUSB320LAI, SLLSEQ8D](https://www.ti.com/lit/ds/symlink/tusb320lai.pdf), VDD limits, sink GPIO current advertisement and VBUS_DET.
