# B2 Final Exact Footprint Gate

Status: PASS
Date: 2026-08-15

ALL EXACT MANUFACTURER FOOTPRINTS FROZEN.

Any future footprint geometry modification must reopen the exact-footprint gate. Overall fabrication release remains BLOCKED pending later gates.

| Ref | Exact MPN | Local footprint | Manufacturer authority | Geometry status | KiCad validation | PASS |
|---|---|---|---|---|---|---|
| U1 | ESP32-S3FH4R2 | ESP32-S3FH4R2_QFN56_7x7mm_P0.4mm_EP5.7x5.7mm.kicad_mod | Espressif ESP32-S3FH4R2 package/land-pattern authority retained from exact audit | FROZEN | PASS | PASS |
| J1 | USB4105-GF-A | GCT_USB4105-GF-A_16P_TopMnt_Horizontal.kicad_mod | GCT USB4105-GF-A official product drawing/PCB footprint retained from exact audit | FROZEN | PASS | PASS |
| Y1 | 7D040000I01 / C648973 | HD_7D040000I01_SMD2016-4P.kicad_mod | Exact C648973 / 7D040000I01 datasheet/footprint geometry retained from exact audit | FROZEN | PASS | PASS |
| GNSS1 | DNP TH carrier | GNSS_5xTH_2.54_DNP_13.1x15.7.kicad_mod | Locked GNSS 5-pin DNP carrier contract | FROZEN | PASS | PASS |
| J2 | FH34D-10S-0.5SH(50) / CL0580-1270-0-50 | Hirose_FH34D-10S-0.5SH_50.kicad_mod | Hirose FH34D official recommended PCB mounting pattern and metal mask dimensions | FROZEN after B2.3 | PASS | PASS |
| U4 | Bosch BMI270 / 0 273 017 008 | Bosch_BMI270_LGA-14_3x2.5mm_P0.5mm.kicad_mod | Bosch BST-BMI270-DS000-08 Rev. 1.6 section 8.3 landing pattern | FROZEN after B2.3 | PASS | PASS |
| U5 | RV-3028-C7-32.768KHZ-1PPM-TA-QA | MicroCrystal_RV-3028-C7_SON-8_1.5x3.2mm_P0.9mm.kicad_mod | Micro Crystal RV-3028-C7 datasheet/application-manual geometry retained from exact audit | FROZEN | PASS | PASS |
| U6 | TPD2EUSB30DRTR / DRT0003A | TI_TPD2EUSB30DRTR_DRT-3_SOT-9X3.kicad_mod | TI official TIDA-010057/TIDM594A CAD/CAE DRT0003A footprint, U18/U19 | FROZEN after B2.4 | PASS | PASS |
| SW1/SW2 | Omron B3U-1000P | Omron_B3U-1000P_SPST_Tactile.kicad_mod | Omron official B3U-1000P top-actuated without ground terminal/boss PCB pad top view | FROZEN after B2.3 | PASS | PASS |

## Final Footprint SHA256

| Footprint | SHA256 |
|---|---|
| `Bosch_BMI270_LGA-14_3x2.5mm_P0.5mm.kicad_mod` | `cc93a2bab441946dbc8836e5e6041ddf024f628599d50e3a4bd1a7c4ba288cfc` |
| `ESP32-S3FH4R2_QFN56_7x7mm_P0.4mm_EP5.7x5.7mm.kicad_mod` | `edc68e2b213fc80aaaf52ebd539b977bd586b7b014392db3b57778e40cdfc3a2` |
| `GCT_USB4105-GF-A_16P_TopMnt_Horizontal.kicad_mod` | `d8ecd98d521a923948e12d1122d92baf72f23e8a5951e2f4085d45dfcc53587e` |
| `GNSS_5xTH_2.54_DNP_13.1x15.7.kicad_mod` | `60506253a4bf96be75fec4e10bafbb6ee9dd153110d5cd832e72ecf3b36c6f64` |
| `HD_7D040000I01_SMD2016-4P.kicad_mod` | `fbf8ebf7d12163a45917b95e69ce96de1715e7e70fd9842b52c109a9c8d82967` |
| `Hirose_FH34D-10S-0.5SH_50.kicad_mod` | `1e7ca585e6e7aad0b84e008bbc7ca72a7280afbd4c765976aa0c7e8cef0e6976` |
| `MicroCrystal_RV-3028-C7_SON-8_1.5x3.2mm_P0.9mm.kicad_mod` | `0f93f7e6d56808736b2eb87a521c4c08ef438006737e1e6f997ef117e7e48b05` |
| `Omron_B3U-1000P_SPST_Tactile.kicad_mod` | `4742939d2df96dfb4ab82f28a9421cef6896800d9058d50202bc6d36e68a073e` |
| `TI_TPD2EUSB30DRTR_DRT-3_SOT-9X3.kicad_mod` | `516d38cc855eecd60bab7b9d4a8d53425036db63ddffdb7055ab569a5e3d8193` |

## Evidence

- `output/B2_3_OFFICIAL_DRAWING_GEOMETRY_CLOSURE.md`
- `output/B2_4_TI_DRT0003A_FINAL_AUTHORITY_CLOSURE.md`
- `output/footprint_svg_previews/`

## Stop Statement

B2 exact footprint gate is complete. Phase 2 full schematic capture, digital placement, routing, ERC, DRC, BOM/CPL, firmware, and Gerber/drill remain pending next phase and were not started in B2.4.
