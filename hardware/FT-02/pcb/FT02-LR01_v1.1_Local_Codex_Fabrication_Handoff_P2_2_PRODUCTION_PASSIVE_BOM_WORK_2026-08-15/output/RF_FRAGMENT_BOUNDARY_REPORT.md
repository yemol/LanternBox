# RF Fragment Boundary Report

Status: ENGINEERING APPROVAL REQUIRED
Date: 2026-08-14
Source board: `reference/e512_imported/e512_imported.kicad_pcb`

No RF geometry was cut, translated, cropped, or modified. This report proposes a candidate boundary only.

## Official E512 Board Bounding Box

| Item | Value |
|---|---:|
| Left X | 103.9761 mm |
| Top Y | 75.9786 mm |
| Right X | 193.0261 mm |
| Bottom Y | 134.0286 mm |
| Width | 89.0500 mm |
| Height | 58.0500 mm |

## Proposed Boundary

| Item | Value |
|---|---:|
| Proposed vertical cut X | 153.5000 mm |
| Cut offset from E512 left edge | 49.5239 mm |
| Candidate RF fragment width | 39.5261 mm |
| Candidate RF fragment height | 58.0500 mm |

The proposed fragment is `X >= 153.5000 mm` in the imported E512 coordinate system.

## Boundary Rationale

The candidate cut is to the left of the complete RF subsystem, including SX1268, PE4259, the 32 MHz crystal area, the SX1268 DC-DC support area, RF matching/tuning parts, SMA connector/launch, associated local copper, stitching vias, and local RF/GND zones.

The cut crosses only low-frequency digital/control/power/reference structures. It does not cross the RF output/matching path, RF DNP tuning pads, SMA launch geometry, XTA/XTB crystal nets, or local RF matching structures.

## Footprints Inside Candidate Fragment

Definition used here: footprint origin at or to the right of the proposed cut. Count: 53.

| Ref | X mm | Y mm | Layer | Value |
|---|---:|---:|---|---|
| Logo Semtech S | 154.3511 | 87.2036 | Top Layer | Logo symbol |
| Shield | 154.3511 | 110.4536 | Top Layer | Logo symbol |
| L7 | 156.6761 | 103.1286 | Top Layer | Inductor |
| P2 | 157.8211 | 115.7436 | Top Layer | Header 4X2 |
| L2 | 158.6011 | 100.6536 | Top Layer | Inductor |
| Logo ESD | 158.7647 | 133.1036 | Top Layer | Logo symbol |
| C17 | 160.0011 | 103.1536 | Top Layer | Cap |
| C18 | 160.0011 | 106.0536 | Top Layer | Cap |
| R6 | 160.0511 | 101.1536 | Top Layer | Resistor |
| C13 | 161.0511 | 96.8536 | Top Layer | Cap |
| Q2 | 162.6761 | 95.7286 | Top Layer | TCXO |
| P3 | 162.9661 | 88.8036 | Top Layer | Header 2 |
| U1 | 163.2011 | 105.0036 | Top Layer | SX1261/2 |
| Q1 | 163.4261 | 98.3036 | Top Layer | Xtal |
| FB1 | 163.6511 | 93.8286 | Top Layer | Ferrite beads |
| C27 | 164.2261 | 95.7786 | Top Layer | Cap |
| R5 | 165.1011 | 95.7786 | Top Layer | Resistor |
| C28 | 165.7011 | 93.8286 | Top Layer | Cap |
| C25 | 166.0761 | 97.2536 | Top Layer | Cap |
| L6 | 166.4511 | 105.0036 | Top Layer | Inductor |
| L1 | 166.7761 | 102.9536 | Top Layer | Inductor |
| C16 | 166.8761 | 100.8036 | Top Layer | Cap |
| C1 | 167.7511 | 101.8536 | Top Layer | Cap |
| C12 | 167.9511 | 105.5536 | Top Layer | Cap |
| C11 | 168.1761 | 104.4536 | Top Layer | Cap |
| C2 | 168.6261 | 101.8536 | Top Layer | Cap |
| C3 | 168.7511 | 103.4786 | Top Layer | Cap |
| Logo_LoRA2 | 169.0263 | 130.8176 | Top Layer | Logo symbol |
| C7 | 169.5761 | 105.5286 | Top Layer | Cap |
| C19 | 170.4011 | 101.8786 | Top Layer | Cap |
| L3 | 170.9511 | 103.3036 | Top Layer | Inductor |
| C4 | 170.9511 | 104.1536 | Top Layer | Cap |
| C5 | 172.6011 | 102.9536 | Top Layer | Cap |
| L4 | 174.1011 | 103.5036 | Top Layer | Inductor |
| GND | 174.4011 | 119.1036 | Top Layer | Test Pin |
| C6 | 176.1261 | 103.5036 | Top Layer | Cap |
| Logo Bot | 176.9011 | 120.9036 | Bottom Layer | Logo symbol |
| C47 | 177.5261 | 102.9536 | Top Layer | Cap |
| R3 | 179.5011 | 107.3036 | Top Layer | Resistor |
| U2 | 179.5261 | 104.3286 | Top Layer | PE4259 RF Switch |
| R4 | 179.5511 | 101.3536 | Top Layer | Resistor |
| C14 | 180.4511 | 101.3536 | Top Layer | Cap |
| C15 | 180.4511 | 107.3036 | Top Layer | Cap |
| C8 | 182.0511 | 104.3286 | Top Layer | Cap |
| C9 | 184.8511 | 104.8786 | Top Layer | Cap |
| L5 | 186.4011 | 104.3286 | Top Layer | Inductor |
| R915 | 187.3511 | 124.4036 | Top Layer | Resistor |
| R868 | 187.3511 | 125.5536 | Top Layer | Resistor |
| R490 | 187.3511 | 126.7036 | Top Layer | Resistor |
| R434 | 187.3511 | 127.8536 | Top Layer | Resistor |
| Other | 187.3511 | 129.0036 | Top Layer | Resistor |
| C10 | 187.9011 | 104.8786 | Top Layer | Cap |
| RFIO | 190.6511 | 104.3286 | Top Layer | SMA |

## Boundary Intersections

### Track Nets Crossing Proposed Cut

Count: 9 tracks.

| Net | Layer | Start X,Y mm | End X,Y mm | Width mm |
|---|---|---:|---:|---:|
| DIO1 | Top Layer | 151.8011,116.0036 | 155.4011,116.0036 | 0.2000 |
| BUSY | Top Layer | 151.6561,116.3536 | 158.0661,116.3536 | 0.2000 |
| MISO | Top Layer | 151.6011,116.8036 | 161.0115,116.8036 | 0.2000 |
| MOSI | Top Layer | 151.2510,117.1536 | 161.1564,117.1536 | 0.2000 |
| SCK | Top Layer | 151.0011,117.5036 | 161.3014,117.5036 | 0.2000 |
| SX_NRESET | Top Layer | 153.2611,125.8036 | 160.3611,118.7036 | 0.2000 |
| ANT_SW | Top Layer | 130.1261,78.8036 | 174.8261,78.8036 | 0.2500 |
| NSS | Top Layer | 150.8562,117.8536 | 162.2761,117.8536 | 0.2000 |
| TCXO_EN | Top Layer | 152.5511,80.8536 | 160.4761,88.7786 | 0.2000 |

Crossing track net counts: {'DIO1': 1, 'BUSY': 1, 'MISO': 1, 'MOSI': 1, 'SCK': 1, 'SX_NRESET': 1, 'ANT_SW': 1, 'NSS': 1, 'TCXO_EN': 1}

### Zone Nets Crossing Proposed Cut

Count: 6 zones.

| Zone index | Net | Layer | Bounding box mm |
|---:|---|---|---|
| 7 | GND | Top Layer | 104.0011,76.0036 to 193.0011,134.0036 |
| 14 | NetC16_2 | Top Layer | 147.3761,97.4286 to 158.1011,100.5036 |
| 22 | VDD_RADIO | Top Layer | 151.5261,103.1536 to 160.0761,105.9036 |
| 52 | <no net> | Top Layer | 103.8540,133.8536 to 193.1482,134.1536 |
| 60 | <no net> | Top Layer | 103.8540,75.8536 to 193.1482,76.1536 |
| 69 | GND | Top Layer | 104.0011,76.0036 to 192.9761,134.0036 |

Crossing zone net counts: {'GND': 2, 'NetC16_2': 1, 'VDD_RADIO': 1, '<no net>': 2}

### Pads Crossing Proposed Cut

None.

### Crossing Net Pad Audit

- `ANT_SW`: R4.2, J2.1
- `BUSY`: P2.1, U1.14, J1.4
- `DIO1`: P2.2, U1.13, J1.6
- `MISO`: P2.4, U1.16, J2.5, J2_DISP.5
- `MOSI`: P2.6, U1.17, J2.4, J2_DISP.4
- `NSS`: P2.8, U1.19, J1.8
- `SCK`: P2.5, U1.18, J2.6, J2_DISP.6
- `SX_NRESET`: P2.3, U1.15, J4.1
- `TCXO_EN`: FB1.1, R6.1, P3.2, J1.1
- `GND`: C7.1, R868.1, R868.2, C5.1, C21.1, C20.1, C25.1, C26.1, U2.2, C12.1, Other.1, Other.2, R15.1, C13.1, R434.1, R434.2, RFIO.2, RFIO.2, OPT1.1, C1.1, C9.1, Shield.1, C14.2, FR1.1, R16.1, C17.1, C22.1, C28.1, P2.7, C18.1, C16.1, C2.1, R915.1, R915.2, C47.1, Q1.2, Q1.4, Q2.1, Q2.2, C23.1, SX1262.1, R490.1, R490.2, C15.2, GND.1, C24.1, C10.1, P3.1, C19.2, U1.2, U1.5, U1.8, U1.20, U1.25, J3_DISP.6, J3_DISP.7, J3.6, J3.7, J2.7, J2_DISP.7
- `NetC16_2`: P1.2, R18.1, C16.2, L2.2, U1.1
- `VDD_RADIO`: R1.1, C20.2, P1.1, R18.2, C18.2, VDD_RADIO.2, U1.10, U1.11

## RF Non-Crossing Confirmation

Confirmed for this candidate boundary:

- No track on the RF output/matching/SMA path crosses X=153.5000 mm.
- No RF matching net such as `NetC3_*`, `NetC4_1`, `NetC6_*`, `NetC7_2`, `NetC8_*`, `NetC10_2`, `NetC11_2`, `NetC12_2`, `NetC14_1`, or `NetC15_1` crosses the proposed boundary.
- No crystal resonator nets `XTA` or `XTB` cross the proposed boundary.
- `TCXO_EN` crosses as a control line only; it is not the crystal resonator RF/clock pair.
- `ANT_SW` crosses as an RF switch control line only; it is not the RF signal path.
- `NetC16_2` crosses as a zone associated with U1/C16/L2/P1/R18 supply/support connectivity, not as an RF matching trace.

## Contained Object Counts

| Object type | Count | Definition |
|---|---:|---|
| Footprints | 53 | Footprint origin X >= cut X |
| Tracks fully inside | 191 | Both segment endpoints X >= cut X |
| Tracks crossing boundary | 9 | Segment spans cut X |
| Vias inside | 684 | Via position X >= cut X |
| Zones fully inside | 59 | Zone bounding-box left X >= cut X |
| Zones crossing boundary | 6 | Zone bounding box spans cut X |
| Pads crossing boundary | 0 | Pad bounding box spans cut X |

## Footprint Graphics Touching Boundary

These are not counted as inside by origin, but their footprint bounding boxes intersect the proposed cut due to non-pad graphics/text:

- J4_DISP: origin left of cut, footprint bounding box intersects due to graphics/text; pads do not cross cut.

## Image Evidence

- `output/RF_FRAGMENT_BOUNDARY_TOP.png`
- `output/RF_FRAGMENT_BOUNDARY_BOTTOM.png`

Engineering approval is required before any physical RF extraction, translation, or integration into LR01 production CAD.
