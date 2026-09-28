# FT-02 Power Module V2-PD-R0

2026-09-27. Editable schematic review draft, not a manufacturing release.
**Static checks PASS; complete Phase 1 gate FAIL / OPEN. Charging is physically inhibited.**

## Current Deliverables

- `FT02_PowerModule_V2.kicad_pro` and `.kicad_sch`: root plus eight populated sheets.
- `review/FT02_PowerModule_V2_PD_Review.pdf`: exported schematic, nine pages.
- `review/erc.json`: actual KiCad ERC, zero errors and zero warnings at this checkpoint.
- `review/reference.net.xml`: actual KiCad exported connectivity.
- `review/schematic_checks.json`: connectivity checks, not a hardware safety certificate.
- `review/SCHEMATIC_REVIEW.md`: decisions, source references, unresolved gates and verification limits.
- `review/pd_budget.json`: tolerance calculations and conditional power envelope.
- `review/pd_nvm_profile.json`: logical programming specification, NOT a tested NVM image.
- `review/review_bom.csv`: schematic inventory, NOT a production BOM or pick-and-place file.

## Simplified Power Path

```text
USB-C -> fuse/ESD -> single input eFuse -> BQ25638 NVDC -> SYS_AON
          |                                  |             |
      STUSB4500                           protected 1S      TPS22953
   9V preferred / 5V fallback                battery         hardware OFF
                                            |              |
                               MAX17055 + 5mohm shunt      SYS_SW
                                                        /         \
                                              TPS63802             TPS61022
                                              3V3 / 2A             5V / 1.5A
                                                 |
                                          filtered 3V3_SENS
```

No EXT input, no external-priority MUX, no UPS MUX, no PD source role, no extra MCU.
The input eFuse is protection, not source arbitration. The charger handles USB/system/battery arbitration.
The 5V converter is a **boost**, not a buck, because a single-cell battery is below 5V.
The physical switch controls only the terminal power gate; charger, pack protection and gauge remain upstream.

## Power Policy

The user-approved policy is shared total power, system priority, charge current yielding.
**8W combined output is a proposed qualification envelope, not a measured terminal requirement or a guaranteed rating.**
At 3V3/2A, this leaves 5V/0.28A; at 5V/1.5A, it leaves 3V3/0.152A.
No simultaneous 14.1W rating is claimed. The real FT-02 load concurrency remains a release gate.

STUSB4500 requests 9V/3A preferentially, then 9V/2A; the charger deliberately uses only about
1.28..1.60A under the stated resistor/KILIM assumptions. A 27W adapter does not mean this board uses 27W.
The restricted input bounds the charger's 5A recommended SW current at the proposed loaded SYS floor.
At nominal conditions, an 8W terminal load leaves roughly 0.5A charging at 4.2V; worst-case margin can approach zero.
Default 2A charging is a future setting, **not enabled in this draft**.

5V fallback is deliberately low-power, with a sub-100mA resistor-derived estimate that has NOT been
qualified at that range. It cannot supply an 8W terminal or guarantee dead-battery/no-battery startup.
Closing usable 5V fallback is still an architecture-level gate; do not present it as completed functionality.
No source advertisement, PD contract, or input current entitlement is inferred from cable attachment alone.

## Battery And Safety Boundary

The source-backed current ceiling remains 5A for the current 5000mAh protected pack.
Capacity upgrade to 10000mAh does not imply a higher current rating, identical chemistry profile or identical connector.
The source's 4.25V CC / 4.2V CV conflict remains OPEN; 4.2V default in the IC is not approval for this pack.
Charge temperature, NTC, PCM thresholds, connector/wire gauge, minimum loaded voltage and >1C peak duration remain OPEN.
R302 is DNP and R303 forces TS fault. Do not remove either lock until the charge profile and real TS network are approved.

## Reproduction

From this directory, using KiCad 10 and Python 3:

`python3 tools/check_and_export.py` performs the full sequence below and records `review/test_results.txt`.

```sh
python3 tools/build_schematic.py
kicad-cli sch export netlist --format kicadxml -o review/reference.net.xml FT02_PowerModule_V2.kicad_sch
kicad-cli sch erc --format json -o review/erc.json FT02_PowerModule_V2.kicad_sch
python3 tools/verify_schematic.py
python3 tools/pd_budget.py
python3 -m unittest discover -s tools -p 'test_*.py' -v
kicad-cli sch export pdf -o review/FT02_PowerModule_V2_PD_Review.pdf FT02_PowerModule_V2.kicad_sch
```

The generator is the authoring source for this checkpoint; hand edits to generated sheets must be ported back before regeneration.
No PCB, Gerber, drill, placement or RevA output is authorized or created.
V1 remains historical WIP, unchanged. `review/archive/V2_5V_Architecture.md`, the old block SVG,
`tools/power_budget.py`, its tests and `review/power_budget.json` are superseded 5V-only comparisons.
