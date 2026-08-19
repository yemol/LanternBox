# B2.4 TI DRT0003A Final Authority Closure

Status: PASS
Date: 2026-08-15
Scope: U6 `TPD2EUSB30DRTR` only. No other footprint geometry was modified. Frozen RF, board outline, mounting holes, schematic capture, digital placement, and routing were not touched.

## Result

U6 = PASS

EXACT_MANUFACTURER_FOOTPRINTS = PASS

Overall fabrication release remains BLOCKED because later gates are not run: full schematic capture, ERC, digital placement, routing, DRC, BOM/CPL, firmware, and Gerber/drill export.

## TI Official Evidence

| Item | Value |
|---|---|
| Manufacturer | Texas Instruments |
| Exact MPN | TPD2EUSB30DRTR |
| Package | DRT |
| Package family | SOT-9X3 |
| Package reference | DRT0003A |
| Pin count | 3 |
| Primary TI CAD/CAE URL | `https://www.ti.com/lit/zip/tidm594a` |
| Downloaded file | `reference/ti_drt0003a_evidence/download/TIDM594A.ZIP` |
| TIDM594A.ZIP SHA256 | `950fd0e164ace022572bbec4d0b83c42d5d26dd26d56b55288618fa720c7388a` |
| Inner CAD ZIP | `reference/ti_drt0003a_evidence/extracted/CAD_Files rev1/Ultrasound Smart Probe_Ver2 (6-3-2020 6-07-58 PM).zip` |
| Inner CAD ZIP SHA256 | `3af293c8edb56aeb69549dac68ca9dc8ae7125ab124ef32aa2ec6c41bc732950` |
| Altium PCB source | `reference/ti_drt0003a_evidence/extracted/cad_unzipped/TIDA_010057.PcbDoc` |
| KiCad import evidence | `reference/ti_drt0003a_evidence/tida_010057_imported.kicad_pcb` |
| KiCad import report | `reference/ti_drt0003a_evidence/tida_010057_import_report.txt` |
| Extracted imported footprint text | `reference/ti_drt0003a_evidence/u18_drt0003a_imported_footprint.kicad_mod.txt` |

## Keyword Search Results

TIDA-010057 CAD evidence contains:

| Evidence | Result |
|---|---|
| `USB TYPE C Interface.SchDoc` | U18 and U19 are `TPD2EUSB30DRTR`; package reference `DRT0003A`; model `DRT0003A`; model type `PCBLIB`. |
| `TIDA_010057.PcbDoc` | U18 and U19 placed as `PATTERN=DRT0003A`, value `TPD2EUSB30DRTR`, footprint description `SOTFL, 3-Leads, Body 1x1mm (inc leads), Pitch 0.35mm`. |
| KiCad Altium import | U18 and U19 imported as `Vault:DRT0003A`, value `TPD2EUSB30DRTR`, with three SMD pads and embedded `DRT0003A.stp`. |

## Coordinate Conversion

TI native Altium PcbDoc was imported with KiCad CLI 10.0.5:

```sh
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli pcb import --format altium --output /private/tmp/tida_010057_imported.kicad_pcb --report-format text --report-file /private/tmp/tida_010057_import_report.txt TIDA_010057.PcbDoc
```

KiCad importer output was then reduced to the footprint-local coordinates of U18/U19. Component placement X/Y and placement rotation were removed; the imported `Vault:DRT0003A` footprint-local pad coordinates are used directly as the local KiCad footprint authority. U18 and U19 agree on the same normalized DRT0003A footprint geometry after placement rotation removal.

## TI Official DRT0003A Pad Geometry

| Pad | TI footprint-local X | TI footprint-local Y | W | H | Shape | Rotation |
|---:|---:|---:|---:|---:|---|---:|
| 1 | -0.575 mm | +0.350 mm | 0.300 mm | 0.450 mm | rect | 0/180 equivalent |
| 2 | -0.575 mm | -0.350 mm | 0.300 mm | 0.450 mm | rect | 0/180 equivalent |
| 3 | +0.575 mm | 0.000 mm | 0.300 mm | 0.450 mm | rect | 0/180 equivalent |

Independent mechanical identity from TI CAD metadata: `DRT0003A`, SOTFL, 3 leads, body 1 x 1 mm including leads, pitch 0.35 mm. Mechanical package identity and PCB land pattern are documented separately; the PASS decision for pad W/H/X/Y uses the TI CAD/CAE PCB footprint, not only the mechanical drawing.

## Official-vs-Local Comparison

| Geometry | TI Official | Local Before | Local After | Delta | Result |
| -------- | ----------: | -----------: | ----------: | ----: | ------ |
| Pad 1 X | -0.575 mm | -0.350 mm | -0.575 mm | 0.000 mm | PASS |
| Pad 1 Y | +0.350 mm | +0.425 mm | +0.350 mm | 0.000 mm | PASS |
| Pad 1 W | 0.300 mm | 0.300 mm | 0.300 mm | 0.000 mm | PASS |
| Pad 1 H | 0.450 mm | 0.300 mm | 0.450 mm | 0.000 mm | PASS |
| Pad 2 X | -0.575 mm | +0.350 mm | -0.575 mm | 0.000 mm | PASS |
| Pad 2 Y | -0.350 mm | +0.425 mm | -0.350 mm | 0.000 mm | PASS |
| Pad 2 W | 0.300 mm | 0.300 mm | 0.300 mm | 0.000 mm | PASS |
| Pad 2 H | 0.450 mm | 0.300 mm | 0.450 mm | 0.000 mm | PASS |
| Pad 3 X | +0.575 mm | 0.000 mm | +0.575 mm | 0.000 mm | PASS |
| Pad 3 Y | 0.000 mm | -0.425 mm | 0.000 mm | 0.000 mm | PASS |
| Pad 3 W | 0.300 mm | 0.300 mm | 0.300 mm | 0.000 mm | PASS |
| Pad 3 H | 0.450 mm | 0.300 mm | 0.450 mm | 0.000 mm | PASS |
| Body X | 1.000 mm class/inc leads | 1.000 mm | 1.000 mm | 0.000 mm | PASS |
| Body Y | 1.000 mm class/inc leads | 0.800 mm | 1.000 mm | 0.000 mm | PASS |
| Pin 1 | left/upper pad in canonical local footprint | left/top in old candidate but wrong pad matrix | left/upper pad marker retained | N/A | PASS |

## U6 Footprint Modification

Modified file: `cad/FT02-LR01.pretty/TI_TPD2EUSB30DRTR_DRT-3_SOT-9X3.kicad_mod`.

Before: local candidate used pads at `(-0.35,+0.425)`, `(+0.35,+0.425)`, `(0,-0.425)` with 0.30 x 0.30 mm pads.

After: TI official DRT0003A geometry uses pads at `(-0.575,+0.350)`, `(-0.575,-0.350)`, `(+0.575,0.000)` with 0.30 x 0.45 mm pads.

U6 footprint SHA256 after B2.4: `516d38cc855eecd60bab7b9d4a8d53425036db63ddffdb7055ab569a5e3d8193`.

## KiCad CLI Validation

KiCad CLI 10.0.5 command:

```sh
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli fp export svg cad/FT02-LR01.pretty --output /private/tmp/ft02_fp_svg_b2_4 --layers F.Cu,F.Paste,F.Mask,F.SilkS,F.Fab,F.CrtYd --black-and-white
```

Result: PASS. All 9 local footprints exported to SVG and were copied to `output/footprint_svg_previews/`.

## Stop Statement

B2.4 stops here. No schematic capture, placement, routing, RF modification, board outline modification, mounting-hole modification, formal DRC, firmware build, BOM/CPL generation, or Gerber/drill export was performed.
