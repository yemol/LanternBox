# RF Fragment A2.2 - Zone Interface Audit

Status: PASS
Date: 2026-08-14
Source board: `reference/e512_imported/e512_imported.kicad_pcb`
Boundary under audit: `X = 153.5000 mm`

No RF geometry was extracted, cropped, translated, routed, refilled, or modified. This audit only classifies imported zones that cross the provisionally accepted RF fragment boundary.

## Locked Context

| Item | Value |
|---|---:|
| True E512 left edge centerline X | 104.0011 mm |
| True E512 right edge centerline X | 193.0011 mm |
| True E512 top edge centerline Y | 76.0036 mm |
| True E512 bottom edge centerline Y | 134.0036 mm |
| True E512 physical width | 89.0000 mm |
| True E512 physical height | 58.0000 mm |
| Candidate RF boundary | X = 153.5000 mm |
| Resulting RF fragment width | 39.5011 mm |
| Resulting RF fragment height | 58.0000 mm |

A2.1 is engineering approved and locked. This A2.2 audit uses those centerline outline dimensions.

## Method

The imported KiCad board was parsed without changing it. Zones whose polygon bounding boxes span `X = 153.5000 mm` were enumerated, classified electrically, and checked against imported pad-net connectivity. Segment crossings at the same boundary were also listed to ensure the zone crossings are interpreted with the already-approved signal crossings.

## Boundary-Crossing Tracks

These are the only imported copper track segments crossing `X = 153.5000 mm`:

| Net | Layer | Segment | Width | Y at boundary | Classification |
|---|---|---:|---:|---:|---|
| ANT_SW | F.Cu | 130.1261,78.8036 -> 174.8261,78.8036 | 0.2500 mm | 78.8036 mm | RF switch control |
| TCXO_EN | F.Cu | 152.5511,80.8536 -> 160.4761,88.7786 | 0.2000 mm | 81.8025 mm | optional TCXO/control |
| DIO1 | F.Cu | 151.8011,116.0036 -> 155.4011,116.0036 | 0.2000 mm | 116.0036 mm | SX1268 digital control |
| BUSY | F.Cu | 151.6561,116.3536 -> 158.0661,116.3536 | 0.2000 mm | 116.3536 mm | SX1268 digital control |
| MISO | F.Cu | 151.6011,116.8036 -> 161.0115,116.8036 | 0.2000 mm | 116.8036 mm | SPI digital |
| MOSI | F.Cu | 151.2510,117.1536 -> 161.1564,117.1536 | 0.2000 mm | 117.1536 mm | SPI digital |
| SCK | F.Cu | 151.0011,117.5036 -> 161.3014,117.5036 | 0.2000 mm | 117.5036 mm | SPI digital |
| NSS | F.Cu | 150.8562,117.8536 -> 162.2761,117.8536 | 0.2000 mm | 117.8536 mm | SPI digital/chip select |
| SX_NRESET | F.Cu | 153.2611,125.8036 -> 160.3611,118.7036 | 0.2000 mm | 125.5647 mm | SX1268 reset control |

No imported RF signal track, RF matching trace, RF tuning structure, XTA/XTB resonator trace, SMA launch trace, or critical RF matching copper track crosses the boundary.

## Boundary-Crossing Zones

| Zone index | Layer | Net | BBox min X,Y -> max X,Y | Type | Boundary contact / interval | Classification | Future LR01 interface handling |
|---:|---|---|---|---|---|---|---|
| 16 | F.Cu | GND | 104.0011,76.0036 -> 193.0011,134.0036 | copper zone | multiple local copper openings along boundary | Global/reference ground copper | Preserve all fragment-side GND copper and via return geometry. Future LR01 GND may connect at the boundary as the approved reference/interface only; do not regenerate RF-side GND geometry. |
| 23 | F.Cu | NetC16_2 | 147.3761,97.4286 -> 158.1011,100.5036 | copper zone | Y ~= 97.4286-98.9173 mm and 99.0536-99.1181 mm at X=153.5000 | SX1268 VDD_IN / local supply support node | Preserve the fragment-side local supply copper. Future LR01 supply interface may connect to `NetC16_2` at the existing boundary-crossing copper window if this node is retained; no RF-side copper refill or reshaping. |
| 31 | F.Cu | VDD_RADIO | 151.5261,103.1536 -> 160.0761,105.9036 | copper zone | Y ~= 103.1536-105.5536 mm at X=153.5000 | SX1268 VBAT/VBAT_IO radio supply rail | Preserve the fragment-side supply copper. Future LR01 3V3/radio supply interface should connect to `VDD_RADIO` at this boundary region; no RF-side copper regeneration. |
| 61 | non-copper imported keepout/rule area | no net | 103.8540,133.8536 -> 193.1482,134.1536 | keepout/rule-area artifact | Y ~= 133.8536-134.1536 mm | Board-edge imported rule area, not electrical copper | No electrical interface required. Preserve or handle mechanically during later outline work; do not treat as RF copper. |
| 69 | non-copper imported keepout/rule area | no net | 103.8540,75.8536 -> 193.1482,76.1536 | keepout/rule-area artifact | Y ~= 75.8536-76.1536 mm | Board-edge imported rule area, not electrical copper | No electrical interface required. Preserve or handle mechanically during later outline work; do not treat as RF copper. |
| 78 | B.Cu | GND | 104.0011,76.0036 -> 192.9761,134.0036 | copper zone | multiple local copper openings along boundary | Bottom global/reference ground copper | Preserve all fragment-side bottom GND copper and RF return geometry. Future LR01 GND may connect at the boundary as the approved reference/interface only; do not regenerate RF-side GND geometry. |

## Electrical Pad Evidence For Crossing Supply Zones

| Net | Imported pads on net | Interpretation |
|---|---|---|
| NetC16_2 | P1.2, R18.1, C16.2, L2.2, U1.1 | SX1268 `VDD_IN` local supply/support node. This is not an RF signal or RF matching net. |
| VDD_RADIO | R1.1, C20.2, P1.1, R18.2, C18.2, VDD_RADIO.2, U1.10, U1.11 | SX1268 `VBAT`/`VBAT_IO` radio supply rail. This is not an RF signal or RF matching net. |
| GND | RF ground pads, shield, SMA/RFIO ground, SX1268 ground pins, RF switch ground, crystal ground pins, and local decoupling grounds | Reference/return copper. It must remain frozen inside the fragment. |

## Counts At Or To The Right Of Boundary

These counts are from the imported board geometry and are used only as an audit inventory for the frozen RF-side region:

| Item | Count / note |
|---|---:|
| Footprints with origin X >= 153.5000 mm | 53 |
| Track segments with at least one endpoint X >= 153.5000 mm | 180 |
| Vias with center X >= 153.5000 mm | 684 |
| Zones crossing X = 153.5000 mm | 6 total: 4 electrical copper zones plus 2 non-copper keepout/rule-area artifacts |
| Total imported zones on board | 78 |

## A2.2 Result

A2.2 ZONE INTERFACE = PASS.

Every zone crossing `X = 153.5000 mm` has been classified. The electrical crossing zones are `GND`, `NetC16_2`, and `VDD_RADIO`; all are ground/reference or low-frequency supply/support interfaces. The two no-net crossings are imported board-edge keepout/rule-area artifacts, not electrical copper. No RF signal zone, RF matching/tuning structure, XTA/XTB crystal network copper, or SMA launch copper crosses the boundary.

Future LR01 copper must connect only through explicit boundary interfaces for GND/reference and radio supply/support nets. All copper, tracks, vias, footprints, and zones at or to the right of the approved boundary must remain frozen until physical extraction is separately approved.
