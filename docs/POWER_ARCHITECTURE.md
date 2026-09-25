# Power Architecture - Requirements and Closure Plan

Status: proposed functional architecture, not an electrical schematic.

```text
USB-C -> CC/PD + D+/D- legacy detector -> protected/limited USB_5V --+
                                                                  +-> external mux -> EXT_BUS_5V
regulated EXT_5V -> reverse/OVP/OCP protection --------------------+

EXT_BUS_5V -> autonomous priority/validity control -> UPS IN1
EXT_BUS_5V -> charge branch -> charger local LC/SYS -> charger BAT --+
                                                                  +-> BAT_PROTECTED load-side node
pack -> pack protection/fuse/reverse guard -> common gauge shunt ---+
BAT_PROTECTED -----------------------------------------------------> UPS IN2

UPS OUT -> SYS_RAW -> 3.3V buck-boost -> 3V3_MAIN -> filter -> 3V3_SENS
                  -> 5.0V buck-boost -> 5V_MAIN

USB protected branch -> optional pulse resistor/MOS -> GND
```

Precise ordering of battery protection, fuse, gauge taps and reverse-polarity elements requires a full pin-level audit. The drawing fixes energy-path intent only.

## Invariants

1. While EXT_VALID is true, the battery-to-UPS load switch is off. No intentional battery supplement, parallel diode sharing or charger-SYS bridge is allowed.
2. Charger SYS has only its local conversion network, not MainBoard loads. Check all alternate backfeed paths, including debug USB and output rails.
3. Both charger BAT and UPS battery input connect to the system side of the same shunt. No charging or system-load branch may leave the pack side.
4. Source loss, selection and recovery must work with the ESP32 halted, resetting, asleep or absent.
5. Total USB current includes system, charger, PD/protection bias and keep-alive. Charger branch current regulation is not source-wide regulation.

## The input-control gap

Engineering deduction from the branch topology: an input-current limit inside the charger only measures that branch. It cannot reserve source current for the independent UPS input. A software current budget alone cannot guarantee behavior during fast load steps, reset or a hung Core.

Required implementation: a source-wide current ceiling matched to the allowed source current, hardware charging reduction/inhibit before system undervoltage, and enough hold-up for the comparator/charger response. Candidate implementations must show the complete control chain, tolerance and startup state. Adding a fixed 3A mux limit does not solve a 500mA/1.5A/default-current source case. Do not mark this architecture closed until a concrete circuit is selected.

If system demand alone exceeds source capability after charging is zero, external becomes INVALID and the hardware changes to battery. With no usable battery, admission control or orderly power-down is necessary; uninterrupted full operation cannot be promised from an insufficient source.

## Validity and recovery

EXT_PRESENT means a physical/electrical source indication. EXT_VALID means selected source, permitted voltage, no blocking fault, and ability to supply the admitted load. Charger PG and a mux status bit do not individually prove this predicate.

Proposed states: ABSENT, QUALIFYING, VALID, INVALID_LATCHED, RECOVERY_TEST. On failure, inhibit charging and select battery in hardware. A weak source may rebound immediately after unloading; voltage hysteresis alone can still create repeated transfers. Qualification and retry/latch behavior must be implemented without relying on firmware for UPS continuity.

Battery must exit when hardware qualification declares VALID, not after a firmware polling interval. Retry qualification must not conceal continuous battery assist. USB collapse must permit evaluation of healthy EXT even when USB is physically attached; a stuck USB-priority decision is unacceptable.

## Threshold design, not guessed resistor values

Define external sense location first. Determine minimum loaded connector voltage, cable/contact loss, protection/mux voltage drops, transient allowance and converter constraints. Require:

```text
VEXT_LOW_min > required downstream operating voltage + remaining path drops
VEXT_HIGH_max < lowest valid loaded external voltage at the sense location
VEXT_HIGH_min - VEXT_LOW_max > required noise/rebound margin
VBAT_min_loaded - battery upstream losses >= mux guaranteed minimum input
```

If these inequalities have no margin, revise the qualified source/load envelope or the selected parts. A divider using a comparator reference has nominal trip `Vref * (1 + Rtop/Rbottom)`, but reference error, resistor tolerance, bias current and feedback hysteresis must be included. No EXT_VALID resistor values are frozen here.

## Hold-up

For a constant-power SYS_RAW load, `Ceff >= 2*Praw*tgap/(Vstart^2 - Vend^2)`. For a rail current step, `deltaV ~= deltaI*ESR + deltaI*tresponse/Ceff`. Include tolerance, DC-bias loss, aging, inrush, converter mode transitions and worst-case timing. A typical mux transfer figure is not a guaranteed no-reset proof. Values cannot close without the real loads and battery floor.

TI's [SLVAFJ1](https://www.ti.com/lit/pdf/SLVAFJ1) VCOMP example uses100mA/320uF and100us in its dip calculation; this mode lacks fast switchover. Do not transplant those component values into ampere-level FT02 loads or treat highest-voltage selection as qualified external-priority logic.
