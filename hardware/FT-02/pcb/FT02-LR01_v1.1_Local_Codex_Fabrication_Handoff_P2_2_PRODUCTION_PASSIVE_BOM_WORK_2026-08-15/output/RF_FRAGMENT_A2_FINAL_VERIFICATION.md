# RF Fragment A2 Final Verification

Status: A2 FINAL VERIFICATION COMPLETE / ENGINEERING REVIEW REQUIRED
Date: 2026-08-14
Source board: `reference/e512_imported/e512_imported.kicad_pcb`
Boundary under review: `X = 153.5000 mm`

No RF geometry was extracted, cropped, translated, routed, refilled, or modified. This package is verification-only and stops here for engineering approval.

## Gate Results

| Gate | Result | Evidence |
|---|---|---|
| A2.1 TRUE OUTLINE | PASS | Engineering approved and locked. True E512 centerline outline is 104.0011 <= X <= 193.0011 mm and 76.0036 <= Y <= 134.0036 mm; physical size 89.0000 x 58.0000 mm. |
| A2.2 ZONE INTERFACE | PASS | `output/RF_FRAGMENT_A2_2_ZONE_INTERFACE_AUDIT.md` classifies every zone crossing X=153.5000 mm. Crossings are GND/reference, VDD_RADIO, NetC16_2 supply/support, and two no-net board-edge keepout/rule-area artifacts. |
| A2.3 CONTROL/CLOCK | PASS | `output/RF_FRAGMENT_A2_3_CONTROL_CLOCK_AUDIT.md` resolves ANT_SW as required PE4259 control, TCXO_EN as optional/not required for locked Q1-crystal LR01, Q1 as the locked 32 MHz crystal path, and Q2 as optional/not-fitted TCXO footprint/path for the locked LR01 build. |

## Locked Boundary Geometry

| Item | Value |
|---|---:|
| Candidate cut X | 153.5000 mm |
| True E512 left edge centerline X | 104.0011 mm |
| True E512 right edge centerline X | 193.0011 mm |
| True E512 top edge centerline Y | 76.0036 mm |
| True E512 bottom edge centerline Y | 134.0036 mm |
| True E512 physical width | 89.0000 mm |
| True E512 physical height | 58.0000 mm |
| Resulting RF fragment width | 39.5011 mm |
| Resulting RF fragment height | 58.0000 mm |
| Future LR01-right-edge alignment left X, if later approved | 26.4989 mm |
| Future LR01-right-edge alignment right X, if later approved | 66.0000 mm |

## Approved Boundary Crossing Classes

The X=153.5000 mm boundary may cross only these low-frequency/control/reference classes observed in the source data:

| Class | Nets |
|---|---|
| SPI/control | NSS, SCK, MOSI, MISO, SX_NRESET, BUSY, DIO1 |
| RF switch control | ANT_SW |
| Optional control only | TCXO_EN |
| Supply/reference | GND, VDD_RADIO, NetC16_2 |
| Non-electrical imported artifacts | top/bottom no-net board-edge keepout/rule-area strips |

The audit confirms that the boundary does not cross any RF signal trace, RF matching/tuning structure, XTA/XTB crystal resonator copper, SMA launch geometry, or critical local RF ground-return structure.

## Required Future Handling If Engineering Approves Extraction Later

- Keep all RF-side copper, tracks, vias, footprints, zones, RF matching/tuning pads, SMA launch geometry, crystal network, and local RF/GND return structures frozen.
- Connect future LR01 copper only through explicit boundary interfaces for approved low-frequency/control/supply/reference nets.
- Carry `ANT_SW` as a required LR01 control interface for the PE4259 path unless engineering explicitly approves a different switch-control scheme.
- Do not require `TCXO_EN` for the locked LR01 Q1-crystal implementation; keep optional/not-fitted Q2/TCXO geometry frozen unless engineering later approves a TCXO variant.
- Do not crop, translate, or route the RF fragment until engineering approval is given for this A2 final verification package.

## Overall Manufacturing Closure State

Overall fabrication release remains BLOCKED.

Reason: A2 boundary verification is complete for engineering review, but physical RF extraction has not been approved or performed, and the remaining manufacturing closure gates outside A2 have not all passed. This report intentionally stops before any geometry modification.
