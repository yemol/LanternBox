# RF Fragment A2.3 - Control And Clock Audit

Status: PASS
Date: 2026-08-14
Sources:
- `reference/SX1268MB1xAS_E512V01A_Altium_Package.zip`
- `reference/e512_imported/e512_imported.kicad_pcb`
- `spec/LOCKED_BOM.csv`

No RF geometry was extracted, cropped, translated, routed, refilled, or modified. This audit only resolves the `ANT_SW`, `TCXO_EN`, Q1, and Q2 control/clock questions using the official E512 Altium source and imported PCB connectivity.

## Source Notes

The official package contains `SX1268_E512V01A.SchDoc` and `SX1268_E512V01A.PcbDoc`; no separate E512 BOM file is present in the archive. The SchDoc was inspected through its embedded Altium records; the PcbDoc-derived KiCad import was used for pad-net connectivity. The SchDoc includes source-visible records such as:

- `LIBREFERENCE=SX126x`, `COMPONENTDESCRIPTION=SX1261/2 - Ultra low power RF transceiver`, pins `DIO2`, `DIO3`, `XTA`, `XTB`, `NRESET`, `MISO`, `MOSI`, `SCK`, `NSS`.
- `LIBREFERENCE=PE4259 RF Switch`, `COMPONENTDESCRIPTION=PE4259 Peregrine RF Switch`, pins `RF1`, `RF2`, `RFC`, `CTRL`, and `\C\T\R\L\`.
- `TEXT=Optional TCXO footprint:` near the Q2 TCXO records.
- Q1 records: `LIBREFERENCE=Xtal GND`, `COMPONENTDESCRIPTION=Crystal with GND pins; 4 pins`, `TEXT=32MHz`, current PCB model `nd2520da`.
- Q2 records: `LIBREFERENCE=TCXO`, `COMPONENTDESCRIPTION=TCXO`, `TEXT=32.0MHz`, current PCB model `TCXO 2.0x1.6mm`.

The locked LR01 BOM identifies the intended production crystal as `RF_Q1`, NDK `EXS00A-CS06465`, NX2016SA, `32MHz +/-10ppm CL10pF`. The official E512 SchDoc itself does not expose an exact Q1 manufacturer part number beyond the 32 MHz crystal symbol/model family.

## PE4259 / ANT_SW Connectivity

| Item | Official/imported evidence | Result |
|---|---|---|
| PE4259 device | SchDoc: `PE4259 RF Switch`; imported footprint U2 value `PE4259 RF Switch`, SC70-6 | RF switch is present inside the RF fragment. |
| U2 pin 4 | SchDoc pin name `CTRL`; PCB net `NetC15_1` | Control input side. |
| U2 pin 6 | SchDoc pin name `\C\T\R\L\`; PCB net `NetC14_1` | Complementary control input side. |
| DIO2 path | U1 pad 12 `DIO2` -> R3 pad 2; R3 pad 1 -> `NetC15_1`; U2 pad 4 -> `NetC15_1` | SX1268 DIO2 drives PE4259 `CTRL` through R3. |
| ANT_SW path | J2 pad 1 `ANT_SW` -> R4 pad 2; R4 pad 1 -> `NetC14_1`; U2 pad 6 -> `NetC14_1` | `ANT_SW` drives PE4259 complementary control through R4. |
| Local PE4259 control shunts | C15 from `NetC15_1` to GND, C14 from `NetC14_1` to GND; both SchDoc value `1nF` | Local control filtering is part of the frozen RF fragment. |

### ANT_SW Decision

`ANT_SW` is REQUIRED as a future LR01 interface unless engineering intentionally approves a different PE4259 control strategy. It is not an RF signal path, but it is a real PE4259 control net in the official/imported E512 design. The boundary may cross it as a low-frequency/control net, and the right-side fragment geometry for R4/U2/C14 must remain frozen.

## Q1 Crystal Path

| Item | Official/imported evidence | Result |
|---|---|---|
| Q1 identity in E512 SchDoc | `LIBREFERENCE=Xtal GND`, `COMPONENTDESCRIPTION=Crystal with GND pins; 4 pins`, value `32MHz` | Q1 is the 32 MHz crystal position. |
| Q1 footprint in imported PCB | `SX126x_e407v02a:nd2520da` at X=163.4261, Y=98.3036 | Current imported footprint is a 2.5 x 2.0 mm crystal family footprint. |
| Q1 pad connectivity | Q1.1 `XTB`, Q1.3 `XTA`, Q1.2/Q1.4 `GND` | Q1 is directly connected to SX1268 XTA/XTB. |
| SX1268 clock pins | U1.3 `XTA`, U1.4 `XTB` | Matches Q1 direct crystal path. |
| Load/support capacitors | C25.2 `XTA` to C25.1 GND; C13.2 `XTB` to C13.1 GND | Complete local crystal support network is inside the fragment. |
| Locked LR01 BOM | `RF_Q1`, NDK `EXS00A-CS06465`, NX2016SA, `32MHz +/-10ppm CL10pF` | Production LR01 crystal source of truth for MPN/CL/tolerance. |

### Q1 Decision

Q1 is the active locked LR01 clock architecture: 32 MHz crystal on XTA/XTB, with local support capacitors C13 and C25 inside the frozen RF fragment. The XTA/XTB crystal network does not cross `X = 153.5000 mm`; it stays wholly to the right of the boundary.

## Q2 Optional TCXO Path

| Item | Official/imported evidence | Result |
|---|---|---|
| Schematic note | SchDoc text: `Optional TCXO footprint:` | Q2 area is explicitly optional in the E512 schematic source. |
| Q2 identity in E512 SchDoc | `LIBREFERENCE=TCXO`, value `32.0MHz`, pins `GND`, `GND`, `OUT`, `VCC` | Q2 is an alternate TCXO footprint, not the Q1 crystal. |
| Q2 imported footprint | `SX126x_e383v01a:TCXO 2.0x1.6mm` at X=162.6761, Y=95.7286 | Optional TCXO geometry exists in the imported RF fragment. |
| Q2 pad connectivity | Q2.1/Q2.2 GND, Q2.3 `NetC27_1`, Q2.4 `NetC28_2` | Q2 output/supply structures are local to optional TCXO circuitry. |
| TCXO output path | Q2.3 `NetC27_1` -> C27.1; C27.2 -> R5.1; R5.2 -> `XTA` | TCXO output would reach XTA only through the optional coupling/component path. |
| TCXO supply/control path | `TCXO_EN` -> FB1.1 and R6.1; FB1.2 -> `NetC28_2` -> Q2.4/C28.2; R6.2 -> `NetR6_2` -> U1.6 DIO3 | TCXO enable/supply path is present as optional local circuitry. |
| FB1 value | SchDoc value `NC` | The visible source marks this TCXO supply feed component as NC. |
| Q2 exact MPN | No exact manufacturer part number was exposed in the official SchDoc strings, no separate E512 BOM file is present in the archive, and Q2 is absent from the locked LR01 BOM | Treat Q2 as optional/not fitted for the locked LR01 Q1-crystal implementation; no Q2 production MPN is required. |

### TCXO_EN Decision

`TCXO_EN` is NOT REQUIRED for the locked LR01 Q1-crystal implementation. It crosses the proposed boundary as an optional/low-frequency control line only. Because the official E512 schematic calls Q2 optional, FB1 is marked `NC`, and the locked LR01 BOM selects Q1 NDK `EXS00A-CS06465` with no Q2 line item, LR01 should not depend on TCXO_EN unless engineering separately approves a TCXO variant.

The optional Q2/TCXO pads and local support geometry remain part of the frozen RF fragment region; their presence does not require a future LR01 control interface for the locked crystal design.

## Boundary Control Nets Summary

| Boundary net | Required for locked LR01? | Reason |
|---|---|---|
| NSS | Yes | SX1268 SPI chip select. |
| SCK | Yes | SX1268 SPI clock. |
| MOSI | Yes | SX1268 SPI data into radio. |
| MISO | Yes | SX1268 SPI data out of radio. |
| SX_NRESET | Yes | SX1268 reset. |
| BUSY | Yes | SX1268 busy status. |
| DIO1 | Yes | SX1268 interrupt. |
| ANT_SW | Yes | PE4259 complementary control path through R4/U2 pin 6. |
| TCXO_EN | No for locked Q1-crystal LR01; optional only for an approved TCXO variant | Q2 is explicitly optional and the locked BOM selects Q1 crystal. |
| GND | Yes | RF/reference return interface. |
| VDD_RADIO / 3V3 radio supply | Yes | SX1268 VBAT/VBAT_IO supply rail. |
| NetC16_2 / VDD_IN support node | Preserve/interface as needed | SX1268 VDD_IN/local supply support copper crossing; not RF matching. |

## A2.3 Result

A2.3 CONTROL/CLOCK = PASS.

Authoritative source evidence resolves the final control/clock questions:

- `ANT_SW` is a real PE4259 control interface and should be carried into LR01 unless engineering approves another switch-control implementation.
- `TCXO_EN` is not required for the locked Q1-crystal LR01 implementation; it belongs to the optional TCXO path.
- Q1 is the locked 32 MHz crystal architecture and is directly connected to SX1268 XTA/XTB with its support network inside the fragment.
- Q2 is an optional/not-fitted TCXO footprint/path for the locked LR01 build; the source archive does not provide a Q2 production MPN, and none is required unless engineering approves a TCXO variant.

No RF signal, RF matching/tuning structure, XTA/XTB resonator copper, SMA launch geometry, or critical RF ground-return structure crosses the boundary.
