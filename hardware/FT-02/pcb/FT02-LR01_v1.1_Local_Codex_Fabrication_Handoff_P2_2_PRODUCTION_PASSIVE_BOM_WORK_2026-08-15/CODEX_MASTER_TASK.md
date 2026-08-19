# CODEX MASTER TASK

## Phase 0 - Read and checkpoint
- Read AGENTS.md and all files in spec/.
- Inspect reference/.
- Initialize Git if needed and commit untouched handoff state.
- Create output/WORKLOG.md.

## Phase 1 - Exact library audit
Resolve and verify exact symbols/footprints for:
- ESP32-S3FH4R2
- HD 7D040000I01
- BMI270
- RV-3028-C7
- USB4105-GF-A
- FH34D-10S-0.5SH(50)
- B3U-1000P
- TPD2EUSB30DRTR
- all RF footprints from official E512 source
Write `output/FOOTPRINT_AUDIT.md`.

## Phase 2 - Full schematic
Create the complete real KiCad schematic from locked specifications.
Every IC pin must be intentionally connected, DNC, or NC-marked.
Add fields: Manufacturer, MPN, JLCPCB, DNP, Notes.
GNSS1 must be DNP.
Run ERC until all real violations are solved. Save `logs/ERC.txt`.

## Phase 3 - Production PCB
- 66 x 58 mm, 2 layer, 1.0 mm
- all SMT Top
- GNSS TH Bottom
- four M2 NPTH + keepouts
- import official E512V01A protected RF region
- do NOT recreate RF from memory
- if CLI cannot safely import Altium, write `MANUAL_RF_IMPORT_REQUIRED` to output/BLOCKERS.md and give exact KiCad GUI import steps; after user stores imported KiCad board in reference/e512_imported/, continue
- place/rout digital portion with real footprints
- USB differential pair over continuous GND
- refill zones

Run DRC with schematic parity and violations-as-exit-code. Save `logs/DRC.txt`.

## Phase 4 - Audits
Generate:
- output/ORIENTATION_AUDIT.md
- output/RF_REFERENCE_AUDIT.md
- output/MECHANICAL_AUDIT.md
- output/POWER_AUDIT.md
- output/DNP_AUDIT.md

Check U1/U4/U5/J1/J2 Pin1/orientation, GNSS silk order, M2/RF clearance, USB, RF geometry.

## Phase 5 - Firmware gate
Search for firmware source.
If present, actually compile for ESP32-S3FH4R2 and record:
- 4MB Flash
- 2MB PSRAM
- partition
- USB Serial/JTAG
- locked GPIO map
- binary/app size
Save logs/FIRMWARE_BUILD.txt.

If firmware source is absent, mark this gate BLOCKED. Do not fake a build.

## Phase 6 - Automated export
After ERC/DRC are clean run:
`bash scripts/03_RUN_FAB_CHECKS_AND_EXPORT.sh`

## Phase 7 - Release gate
Create output/FABRICATION_RELEASE_GATE.md using the template.
Overall may be PASS only when every mandatory gate is PASS.
Before finalizing, self-review all changes.

Only after gate PASS run:
`bash scripts/04_BUILD_RELEASE_ZIP.sh`
