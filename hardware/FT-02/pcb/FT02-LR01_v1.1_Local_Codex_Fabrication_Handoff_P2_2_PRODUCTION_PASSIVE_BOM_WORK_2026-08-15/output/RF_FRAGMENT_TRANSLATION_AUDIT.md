# RF Fragment Translation Audit

Status: PASS
Date: 2026-08-15
Source: `reference/e512_imported/e512_imported.kicad_pcb`
Target PCB: `cad/FT02-LR01_v1.1_Fabrication_Release.kicad_pcb`

## Required Translation

| Coordinate | Source | Target | Measured delta |
|---|---:|---:|---:|
| Right edge X | 193.0011 mm | 66.0000 mm | -127.0011 mm |
| Cut X | 153.5000 mm | 26.4989 mm | -127.0011 mm |
| Top Y | 76.0036 mm | 0.0000 mm | -76.0036 mm |
| Bottom Y | 134.0036 mm | 58.0000 mm | -76.0036 mm |

## Mechanical Proofs

| Check | Maximum measured error | Result |
|---|---:|---|
| Per-footprint DX/DY equals required translation | 0.000000000 mm | PASS |
| Pairwise footprint relative coordinates unchanged | 0.000000000 mm | PASS |
| Per-track endpoint DX/DY equals required translation after boundary extraction | 0.000000000 mm | PASS |
| Internal, unclipped track lengths unchanged | 0.000000000 mm | PASS |
| RF-class internal trace lengths unchanged | 0.000000000 mm | PASS |
| Per-via DX/DY equals required translation | 0.000000000 mm | PASS |
| Sampled via pairwise relative coordinates unchanged | 0.000000000 mm | PASS |

## RF Geometry Proof

- RF signal/matching/crystal/SMA traces do not cross the locked boundary and were translated as complete internal segments.
- Boundary-crossing tracks are limited to approved control/supply/reference nets and were clipped only at the physical extraction boundary.
- The SMA launch footprint `RFIO` and adjacent RF matching footprints are translated by the same DX/DY as U1/U2/Q1.
- Q1, C13, C25, XTA, and XTB geometry remains internal to the fragment; no XTA/XTB copper crosses the boundary.
- PE4259 U2, R3/R4, C14/C15, and the RF switch/matching geometry are translated rigidly; PE4259 complementary-control topology is preserved.
- Zones were not refilled or regenerated into optimized shapes. Crossing electrical/protected zones were clipped to the approved fragment boundary; all protected-side polygon coordinates use the same DX/DY. Exactly 3 approved non-electrical E512 board-edge artifacts were excluded; protected electrical zone hash is unchanged before/after deletion.

## Boundary Interfaces Retained

Required interfaces available at the left edge of the frozen fragment: GND, VDD_RADIO/radio 3V3, NetC16_2/VDD_IN support as required, NSS, SCK, MOSI, MISO, SX_NRESET, BUSY, DIO1, and ANT_SW. TCXO_EN remains optional/not required for the locked Q1 crystal build and is not an MCU interface requirement.

## Result

RF_FRAGMENT_TRANSLATION = PASS.

The protected RF fragment uses the required rigid translation DX = -127.0011 mm, DY = -76.0036 mm, and protected relative RF geometry is unchanged. Engineering-approved deletion removed only the 3 non-electrical imported E512 board-edge artifacts; protected electrical inventories and zone geometry remain unchanged. Overall fabrication release remains BLOCKED.
