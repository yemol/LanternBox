# RF Reference Audit

Status: BLOCKED
Date: 2026-08-14

## Source

Official Semtech archive:
`reference/SX1268MB1xAS_E512V01A_Altium_Package.zip`

KiCad CLI import:
`reference/e512_imported/e512_imported.kicad_pcb`

Import report:
`reference/e512_imported/e512_import_report.txt`

## Findings

- KiCad CLI 10.0.5 imported `SX1268_E512V01A.PcbDoc` using `pcb import --format altium`.
- Import report: 87 footprints, 373 tracks, 1394 vias, 70 zones.
- Imported board stats: 89.0 x 58.0 mm, 2 copper layers, imported stack thickness 0.24 mm.
- Locked LR01 outline: 66.0 x 58.0 mm, 2 layers, 1.0 mm finished thickness.
- The full official E512 board cannot be translated into the locked LR01 outline without exceeding board width.
- The handoff does not define an authoritative protected RF subregion boundary for extraction from the 89.0 x 58.0 mm E512 board.

## Decision

RF preservation cannot be certified. Do not proceed to production routing or PASS release until the protected E512 RF region boundary is authoritatively defined or supplied as a KiCad fragment.
