# P2.2 Production Passive BOM Gate

Status: BLOCKED
Date: 2026-08-15

| Gate Item | Status | Evidence |
|---|---|---|
| Remaining symbol pin authority | BLOCKED | `output/P2_2_REMAINING_PIN_AUTHORITY_CLOSURE.md` |
| Production passive BOM | PASS | `output/P2_2_PRODUCTION_PASSIVE_BOM_AUDIT.md`; `cad/FT02-LR01_v1.1_PRODUCTION_BOM.csv` |
| Power / decoupling requirements | PASS | `output/P2_2_POWER_DECOUPLING_CLOSURE.md` |
| Passive package compatibility | PASS | `output/P2_2_PRODUCTION_PASSIVE_BOM_AUDIT.md` |
| All added populated passives exact MPN selected | PASS | `cad/FT02-LR01_v1.1_PRODUCTION_BOM.csv` |
| Manufacturer-vs-locked conflicts | BLOCKED | ESP32-S3FH4R2 QFN56 `GPIO47` / `GNSS_PPS` contract conflict |

`P2.2 PRODUCTION PASSIVE BOM GATE = BLOCKED`

P2.2 cannot PASS until engineering resolves the locked `GPIO47 = GNSS_PPS` conflict and provides/approves Tier A Y1 pin-function authority for `7D040000I01`.

Full schematic capture, PCB placement, routing, PCB DRC, Gerber, CPL, and ERC closure were not started.
