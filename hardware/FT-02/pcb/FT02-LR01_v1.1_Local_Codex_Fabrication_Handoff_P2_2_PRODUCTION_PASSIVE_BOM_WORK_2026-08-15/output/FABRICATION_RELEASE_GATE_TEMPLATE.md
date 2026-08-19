# FT02-LR01 v1.1 FABRICATION RELEASE GATE

Overall: BLOCKED

| Gate | Status | Evidence |
|---|---|---|
| Exact manufacturer footprints | PASS | output/B2_FINAL_EXACT_FOOTPRINT_GATE.md |
| Schematic ERC | BLOCKED | logs/ERC.txt |
| PCB DRC | BLOCKED | logs/DRC.txt |
| Schematic parity | BLOCKED | logs/DRC.txt |
| E512 RF preservation | PASS | output/RF_FRAGMENT_TRANSLATION_AUDIT.md |
| USB routing | BLOCKED | |
| M2/RF collision | BLOCKED | output/MECHANICAL_AUDIT.md |
| GNSS DNP | BLOCKED | output/DNP_AUDIT.md |
| BOM exactness | BLOCKED | output/FT02-LR01_v1.1_BOM.csv |
| CPL correctness | BLOCKED | output/FT02-LR01_v1.1_CPL.csv |
| Firmware 4MB/2MB build | BLOCKED | logs/FIRMWARE_BUILD.txt |
| Gerber/Drill | BLOCKED | output/gerber + output/drill |

Overall may become PASS only if every mandatory gate is PASS.
