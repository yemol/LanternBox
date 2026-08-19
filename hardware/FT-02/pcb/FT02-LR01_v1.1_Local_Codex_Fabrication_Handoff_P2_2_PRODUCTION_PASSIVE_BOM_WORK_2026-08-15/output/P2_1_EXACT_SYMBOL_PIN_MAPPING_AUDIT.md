# P2.1 Exact Symbol / Pin Mapping Audit - Updated by P2.2

Status: BLOCKED
Date: 2026-08-15

Local symbol library remains established at `cad/FT02-LR01.kicad_sym`. Symbol SVG validation remains PASS. P2.2 closed BMI270 pin authority and passive BOM blockers, but exact symbol pin mapping remains blocked by two authority items.

`EXACT_SYMBOL_PIN_MAPPING = BLOCKED`

## P2.2 Updates

| Prior P2.1 Item | P2.2 Result |
|---|---|
| Missing exact production passives | CLOSED for BOM authority; see `output/P2_2_PRODUCTION_PASSIVE_BOM_AUDIT.md` |
| BMI270 official pin-function cross-check | CLOSED / PASS; see `output/P2_2_REMAINING_PIN_AUTHORITY_CLOSURE.md` |
| Y1 terminal/case pin authority | STILL BLOCKED; Tier A manufacturer pin-function table missing |
| ESP32 GNSS_PPS `GPIO47` mapping | NEW BLOCKER / DESIGN DECISION REQUIRED; QFN56 package does not expose a `GPIO47` package pin according to Espressif authority |

## Current Blocking Items

1. `Y1 PIN AUTHORITY = BLOCKED`: exact manufacturer terminal/case/NC pad table required.
2. `U1 GPIO47 / GNSS_PPS = DESIGN DECISION REQUIRED`: current locked GPIO contract cannot be mapped to ESP32-S3FH4R2 QFN56 physical pin.

No frozen footprint, RF geometry, PCB placement, or routing was modified.
