# RF Fragment Extraction Audit

Status: PASS
Date: 2026-08-15
Source: `reference/e512_imported/e512_imported.kicad_pcb`
Target PCB: `cad/FT02-LR01_v1.1_Fabrication_Release.kicad_pcb`

A2 final engineering approval was granted. The locked RF fragment was physically extracted from the official E512 import and placed on the LR01 66.0000 mm x 58.0000 mm board using the approved rigid translation only.

## Locked Source And Target

| Item | Value |
|---|---:|
| Source fragment X | 153.5000 mm to 193.0011 mm |
| Source fragment Y | 76.0036 mm to 134.0036 mm |
| Source fragment size | 39.5011 mm x 58.0000 mm |
| Target fragment X | 26.4989 mm to 66.0000 mm |
| Target fragment Y | 0.0000 mm to 58.0000 mm |
| Target board size | 66.0000 mm x 58.0000 mm |
| DX | -127.0011 mm |
| DY | -76.0036 mm |

## Extracted Object Counts

| Object class | Source fragment count | Target fragment count | Notes |
|---|---:|---:|---|
| Footprints | 53 | 53 | Footprints with source origin X >= 153.5000 mm. |
| Tracks/segments | 180 | 180 | Includes approved boundary-interface segments clipped at X=153.5000; RF-internal segments are not clipped. |
| Vias | 684 | 684 | Vias with source center X >= 153.5000 mm. |
| Zones | 62 | 62 | Electrical/protected zones at/right of boundary after approved exclusion of 3 non-electrical E512 board-edge artifacts; crossing electrical zones are clipped at boundary without refill/regeneration. |
| Graphic/mechanical items | 95 | 95 | Protected-side graphics/mechanical geometry plus new LR01 outline/boundary marker. |

Boundary clipping was applied only to objects that crossed the approved boundary: 9 control/supply/reference track segments, 6 zones, 4 graphic lines, and 0 graphic polygons. This is the physical boundary cut; no RF signal/matching/crystal/SMA launch trace crosses the boundary. After engineering approval, exactly 3 no-net/no-layer E512 board-edge rule/keepout artifacts were excluded; no protected electrical zone was deleted or modified.

## Protected Footprints

| Ref | Value | Footprint | Source X | Source Y | Target X | Target Y | DX | DY |
|---|---|---|---:|---:|---:|---:|---:|---:|
| Logo Semtech S | Logo symbol | `SX1280_e325v02a:logo_semtech_horiz_size3` | 154.3511 | 87.2036 | 27.3500 | 11.2000 | -127.0011 | -76.0036 |
| P3 | Header 2 | `Miscellaneous Connectors:HDR1X2` | 162.9661 | 88.8036 | 35.9650 | 12.8000 | -127.0011 | -76.0036 |
| FB1 | Ferrite beads | `SX126x_e383v01a:RLC_402_SMD` | 163.6511 | 93.8286 | 36.6500 | 17.8250 | -127.0011 | -76.0036 |
| C28 | Cap | `SX126x_e383v02a:RLC_402_SMD` | 165.7011 | 93.8286 | 38.7000 | 17.8250 | -127.0011 | -76.0036 |
| Q2 | TCXO | `SX126x_e383v01a:TCXO 2.0x1.6mm` | 162.6761 | 95.7286 | 35.6750 | 19.7250 | -127.0011 | -76.0036 |
| C27 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 164.2261 | 95.7786 | 37.2250 | 19.7750 | -127.0011 | -76.0036 |
| R5 | Resistor | `SX126x_e407v02a:rlc_402_smd` | 165.1011 | 95.7786 | 38.1000 | 19.7750 | -127.0011 | -76.0036 |
| C13 | Cap | `SX126x_e407v02a:rlc_402_smd` | 161.0511 | 96.8536 | 34.0500 | 20.8500 | -127.0011 | -76.0036 |
| C25 | Cap | `SX126x_e407v02a:rlc_402_smd` | 166.0761 | 97.2536 | 39.0750 | 21.2500 | -127.0011 | -76.0036 |
| Q1 | Xtal | `SX126x_e407v02a:nd2520da` | 163.4261 | 98.3036 | 36.4250 | 22.3000 | -127.0011 | -76.0036 |
| L2 | Inductor | `SX126x_e407v02a:rlc_402_smd` | 158.6011 | 100.6536 | 31.6000 | 24.6500 | -127.0011 | -76.0036 |
| C16 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 166.8761 | 100.8036 | 39.8750 | 24.8000 | -127.0011 | -76.0036 |
| R6 | Resistor | `SX126x_e407v02a:rlc_402_smd` | 160.0511 | 101.1536 | 33.0500 | 25.1500 | -127.0011 | -76.0036 |
| R4 | Resistor | `SX1280_e325v01a:RLC_402_SMD` | 179.5511 | 101.3536 | 52.5500 | 25.3500 | -127.0011 | -76.0036 |
| C14 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 180.4511 | 101.3536 | 53.4500 | 25.3500 | -127.0011 | -76.0036 |
| C1 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 167.7511 | 101.8536 | 40.7500 | 25.8500 | -127.0011 | -76.0036 |
| C2 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 168.6261 | 101.8536 | 41.6250 | 25.8500 | -127.0011 | -76.0036 |
| C19 | Cap | `SX126x_e407v02a:rlc_402_smd` | 170.4011 | 101.8786 | 43.4000 | 25.8750 | -127.0011 | -76.0036 |
| L1 | Inductor | `SX1280_e325v01a:RLC_402_SMD` | 166.7761 | 102.9536 | 39.7750 | 26.9500 | -127.0011 | -76.0036 |
| C5 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 172.6011 | 102.9536 | 45.6000 | 26.9500 | -127.0011 | -76.0036 |
| C47 | Cap | `SX126x_e407v02a:rlc_402_smd` | 177.5261 | 102.9536 | 50.5250 | 26.9500 | -127.0011 | -76.0036 |
| L7 | Inductor | `SX126x_e383v01a:rlc_805_smd enlarged` | 156.6761 | 103.1286 | 29.6750 | 27.1250 | -127.0011 | -76.0036 |
| C17 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 160.0011 | 103.1536 | 33.0000 | 27.1500 | -127.0011 | -76.0036 |
| L3 | Inductor | `SX126x_e383v01a:RLC_402_SMD` | 170.9511 | 103.3036 | 43.9500 | 27.3000 | -127.0011 | -76.0036 |
| C3 | Cap | `SX126x_e407v02a:rlc_402_smd` | 168.7511 | 103.4786 | 41.7500 | 27.4750 | -127.0011 | -76.0036 |
| L4 | Inductor | `SX126x_e383v01a:RLC_402_SMD` | 174.1011 | 103.5036 | 47.1000 | 27.5000 | -127.0011 | -76.0036 |
| C6 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 176.1261 | 103.5036 | 49.1250 | 27.5000 | -127.0011 | -76.0036 |
| C4 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 170.9511 | 104.1536 | 43.9500 | 28.1500 | -127.0011 | -76.0036 |
| U2 | PE4259 RF Switch | `SX126x_e383v02a:SC70-6` | 179.5261 | 104.3286 | 52.5250 | 28.3250 | -127.0011 | -76.0036 |
| C8 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 182.0511 | 104.3286 | 55.0500 | 28.3250 | -127.0011 | -76.0036 |
| L5 | Inductor | `SX126x_e383v01a:RLC_402_SMD` | 186.4011 | 104.3286 | 59.4000 | 28.3250 | -127.0011 | -76.0036 |
| RFIO | SMA | `SX126x_e407v01a:SMA_end_launch_1.0mm_9.52mm` | 190.6511 | 104.3286 | 63.6500 | 28.3250 | -127.0011 | -76.0036 |
| C11 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 168.1761 | 104.4536 | 41.1750 | 28.4500 | -127.0011 | -76.0036 |
| C9 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 184.8511 | 104.8786 | 57.8500 | 28.8750 | -127.0011 | -76.0036 |
| C10 | Cap | `SX1280_e325v01a:RLC_402_SMD` | 187.9011 | 104.8786 | 60.9000 | 28.8750 | -127.0011 | -76.0036 |
| U1 | SX1261/2 | `SX1280_e325v01a:VQFN24_4X4MM` | 163.2011 | 105.0036 | 36.2000 | 29.0000 | -127.0011 | -76.0036 |
| L6 | Inductor | `SX126x_e383v01a:RLC_402_SMD` | 166.4511 | 105.0036 | 39.4500 | 29.0000 | -127.0011 | -76.0036 |
| C7 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 169.5761 | 105.5286 | 42.5750 | 29.5250 | -127.0011 | -76.0036 |
| C12 | Cap | `SX1280_e363v01a:RLC_402_SMD` | 167.9511 | 105.5536 | 40.9500 | 29.5500 | -127.0011 | -76.0036 |
| C18 | Cap | `SX126x_e383v01a:RLC_402_SMD` | 160.0011 | 106.0536 | 33.0000 | 30.0500 | -127.0011 | -76.0036 |
| R3 | Resistor | `SX126x_e383v02a:RLC_402_SMD` | 179.5011 | 107.3036 | 52.5000 | 31.3000 | -127.0011 | -76.0036 |
| C15 | Cap | `SX126x_e383v02a:RLC_402_SMD` | 180.4511 | 107.3036 | 53.4500 | 31.3000 | -127.0011 | -76.0036 |
| Shield | Logo symbol | `SX126x_e383v01a:BMIS-209-F - SHIELD` | 154.3511 | 110.4536 | 27.3500 | 34.4500 | -127.0011 | -76.0036 |
| P2 | Header 4X2 | `Miscellaneous Connectors:HDR2X4` | 157.8211 | 115.7436 | 30.8200 | 39.7400 | -127.0011 | -76.0036 |
| GND | Test Pin | `SX1280_e325v01a:pin_0.9mm` | 174.4011 | 119.1036 | 47.4000 | 43.1000 | -127.0011 | -76.0036 |
| Logo Bot | Logo symbol | `SX1280_e325v01a:logo_semtech_horiz_size4` | 176.9011 | 120.9036 | 49.9000 | 44.9000 | -127.0011 | -76.0036 |
| R915 | Resistor | `SX126x_e383v01a:RLC_402_SMD` | 187.3511 | 124.4036 | 60.3500 | 48.4000 | -127.0011 | -76.0036 |
| R868 | Resistor | `SX126x_e383v01a:RLC_402_SMD` | 187.3511 | 125.5536 | 60.3500 | 49.5500 | -127.0011 | -76.0036 |
| R490 | Resistor | `SX126x_e407v01a:rlc_402_smd` | 187.3511 | 126.7036 | 60.3500 | 50.7000 | -127.0011 | -76.0036 |
| R434 | Resistor | `SX126x_e383v01a:RLC_402_SMD` | 187.3511 | 127.8536 | 60.3500 | 51.8500 | -127.0011 | -76.0036 |
| Other | Resistor | `SX126x_e383v01a:RLC_402_SMD` | 187.3511 | 129.0036 | 60.3500 | 53.0000 | -127.0011 | -76.0036 |
| Logo_LoRA2 | Logo symbol | `SX126x_e383v01a:Logo LoRa Size3` | 169.0263 | 130.8176 | 42.0252 | 54.8140 | -127.0011 | -76.0036 |
| Logo ESD | Logo symbol | `SX1276_e311v01a:Logo ESD` | 158.7647 | 133.1036 | 31.7636 | 57.1000 | -127.0011 | -76.0036 |

## Protected Subsystem Checks

| Subsystem | Evidence | Result |
|---|---|---|
| SX1268 | U1: source (163.2011, 105.0036) -> target (36.2000, 29.0000) | Preserved by rigid translation. |
| PE4259 | U2: source (179.5261, 104.3286) -> target (52.5250, 28.3250) | Preserved by rigid translation. |
| SMA launch | RFIO: source (190.6511, 104.3286) -> target (63.6500, 28.3250) | Preserved by rigid translation. |
| Q1 crystal | Q1: source (163.4261, 98.3036) -> target (36.4250, 22.3000); C13/C25 load capacitors remain in translated protected footprint table | Q1/XTA/XTB geometry preserved. |
| Optional TCXO area | Q2: source (162.6761, 95.7286) -> target (35.6750, 19.7250); TCXO_EN not connected to MCU in this extraction | Optional/not-fitted geometry kept untouched inside fragment. |
| Matching/tuning network | RF matching footprints and DNP tuning pads remain in the protected footprint table with identical relative coordinates | Preserved by rigid translation. |

## Extraction Result

RF_FRAGMENT_EXTRACTION = PASS.

Engineering approval granted exclusion of exactly three imported E512 no-net/no-layer board-edge keepout/rule-area artifacts that protruded outside the locked LR01 outline. Their removal is not considered a protected RF geometry modification.

Post-deletion protected electrical inventories are unchanged: footprints = 53, copper tracks/segments = 180, vias = 684. Electrical/protected zones are unchanged; non-artifact zone SHA256 before and after deletion is `8adb4ddd6e5d472319da3c4ec07fbc61a0dcff222c5d6abe2496d3dcf5fa00c2`. No object now extends outside the 66.0000 mm x 58.0000 mm LR01 outline.

Overall fabrication release remains BLOCKED until every remaining manufacturing gate passes.
