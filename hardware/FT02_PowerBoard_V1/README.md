# FT02 PowerBoard V1

Phase 1 work in progress. NOT FOR FABRICATION.

This independent power-board design follows the user's FT02 PowerBoard V1 task brief. Existing FT-02 firmware, LR01 hardware and user changes are not part of these commits.

Initial repository HEAD: `9b2b18d5c1fc886b769d72a9dbecfce2d816d4c9` on `main`.
The repository was dirty before this work. A machine-readable baseline is retained in `review/git_baseline.json`; it records status and source hashes without committing unrelated files.

Design status, evidence and open items are recorded in the repository `docs/` PowerBoard documents and `FT02_PowerBoard_V1_Phase1_Review.md`. A parseable project or a clean ERC alone does not establish an electrically complete or releasable board.

No PCB routing, Gerbers, drill files, placement files, assembly release or Rev A tag is authorized by the current review status.

## Current Implementation

BQ27220/common-shunt,TPS63802,BQ25628E,candidate TPS2121/UVLO,LT3120,TPS259474L EXT guard,sensor filter,TCA9539 interface,19 test points and sheet13 TPS22953 master gate are wired references. U701 now feeds UPS_RAW; only the mechanical series gate feeds SYS_RAW. Three sheets01/03/05 remain explicitly unimplemented. The independent checker covers127 symbols/363 pins with40 tests, plus4 battery-contract and5 load-budget tests (49 total). The partial BOM has no approved footprints and must not be used for procurement/assembly. R601 Kelvin names and U1301 EP11 are logical mappings pending physical footprint review. No board file is created before all Phase1 gates pass.

R407 charge-arm link is DNP; TS is locked to fault. The charger local SYS has no system load. Current-path validation proves only the installed boundary, not the missing pack/guard or all internal fault paths. TPS2121 is UNFROZEN; board load/thermal/transition and safe source qualification are not established. `wired_reference_sheets` is deliberately separate from the empty `qualified_sheets` list.

The supplied955565/955465 battery limits are adopted in `docs/BATTERY_SPEC.md`. Charge voltage remains OPEN (CC4.25V then CV4.2V); charge temperature, precise protection, NTC, connector/wire, loaded voltage and >1C pulse data also remain OPEN. Sheet06 makes the gauge's upstream supply-current bypass explicit; it does not close the literal all-current measurement requirement.

## Reproduce Checks

Run from the repository root with Python3 and KiCad10.0.5 (`kicad-cli` on PATH):

```sh
python3 hardware/FT02_PowerBoard_V1/tools/build_reference.py
kicad-cli sch export netlist --format kicadxml -o hardware/FT02_PowerBoard_V1/review/reference.net.xml hardware/FT02_PowerBoard_V1/FT02_PowerBoard_V1.kicad_sch
kicad-cli sch export svg -o hardware/FT02_PowerBoard_V1/review/svg/ hardware/FT02_PowerBoard_V1/FT02_PowerBoard_V1.kicad_sch
kicad-cli sch erc --format json --severity-all --exit-code-violations -o hardware/FT02_PowerBoard_V1/review/erc.json hardware/FT02_PowerBoard_V1/FT02_PowerBoard_V1.kicad_sch
python3 hardware/FT02_PowerBoard_V1/tools/verify_reference.py
python3 -B hardware/FT02_PowerBoard_V1/tools/test_verify_reference.py -v
python3 hardware/FT02_PowerBoard_V1/tools/calculate_examples.py
python3 -B hardware/FT02_PowerBoard_V1/tools/test_battery_contract.py -v
python3 -B hardware/FT02_PowerBoard_V1/tools/load_budget.py
python3 -B hardware/FT02_PowerBoard_V1/tools/test_load_budget.py -v
```

Current ERC command intentionally exits5 because4 errors/6 warnings remain. Run subsequent checks separately, not behind shell success chaining. The topology check passes only the reference connections and explicitly reports the full design gate FAIL. KiCad's default ignored rules are disclosed in the report; no project-specific exclusions were added.

`build_reference.py` regenerates all schematic/project/library files and the partial BOM. Do not rerun after manual schematic edits without reconciling those edits into the generator. `capture_baseline.py` is a one-time provenance tool, not a reset or backup restoration command.
