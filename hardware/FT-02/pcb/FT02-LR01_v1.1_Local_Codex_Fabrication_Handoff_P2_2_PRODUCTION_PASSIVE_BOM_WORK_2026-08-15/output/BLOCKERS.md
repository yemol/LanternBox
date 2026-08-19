# FT02-LR01 v1.1 Blockers

Status: BLOCKED
Date: 2026-08-15

## Closed Gates

| Gate | Status | Evidence |
|---|---|---|
| B1 RF protected-region boundary / extraction | CLOSED / PASS | `output/RF_FRAGMENT_EXTRACTION_AUDIT.md`; `output/RF_FRAGMENT_TRANSLATION_AUDIT.md` |
| B2 Exact manufacturer footprints | CLOSED / PASS | `output/B2_FINAL_EXACT_FOOTPRINT_GATE.md`; `output/B2_4_TI_DRT0003A_FINAL_AUTHORITY_CLOSURE.md` |
| P2.2 Production passive BOM content | CLOSED / PASS | `output/P2_2_PRODUCTION_PASSIVE_BOM_AUDIT.md` |
| P2.2 Power/decoupling requirements | CLOSED / PASS | `output/P2_2_POWER_DECOUPLING_CLOSURE.md` |
| P2.2 BMI270 pin authority | CLOSED / PASS | `output/P2_2_REMAINING_PIN_AUTHORITY_CLOSURE.md` |

## Open Blockers

| Blocker | Status | Evidence |
|---|---|---|
| U1 ESP32-S3FH4R2 QFN56 `GPIO47 = GNSS_PPS` locked-contract conflict | OPEN / DESIGN DECISION REQUIRED | `output/P2_2_REMAINING_PIN_AUTHORITY_CLOSURE.md` |
| Y1 7D040000I01 terminal/case/NC pad authority | OPEN / TIER A AUTHORITY REQUIRED | `output/P2_2_REMAINING_PIN_AUTHORITY_CLOSURE.md` |
| Exact symbol pin mapping gate | OPEN / BLOCKED | `output/P2_1_EXACT_SYMBOL_PIN_MAPPING_AUDIT.md` |
| Full production schematic capture | NOT STARTED | Pending blocker resolution |
| ERC closure | NOT RUN | Pending full schematic capture |

Frozen RF fragment, frozen footprints, board outline, mounting holes, placement, and routing remain untouched.
