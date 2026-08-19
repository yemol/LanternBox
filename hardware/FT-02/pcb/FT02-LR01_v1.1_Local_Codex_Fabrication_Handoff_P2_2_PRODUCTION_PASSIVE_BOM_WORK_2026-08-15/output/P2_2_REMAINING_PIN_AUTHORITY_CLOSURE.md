# P2.2 Remaining Pin Authority Closure

Status: BLOCKED
Date: 2026-08-15

`REMAINING_PIN_AUTHORITY = BLOCKED`

## Authority Sources Consulted

- Espressif ESP32-S3 Hardware Design Guidelines / schematic checklist: `https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/`
- Espressif official ESP32-S3 pin/package data: `https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf`
- Bosch Sensortec BMI270 datasheet BST-BMI270-DS000 Rev. 1.6: `https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf`
- Micro Crystal RV-3028-C7 datasheet/application manual: `https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf` and `https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf`
- TI TPD2EUSB30 datasheet and TIDM594A CAD evidence retained from B2.4.
- Semtech SX1268 / E512 RF reference evidence retained from frozen RF audit.
- Y1 7D040000I01 evidence currently available in-package only as B2 footprint geometry closure plus LCSC/C648973 distributor data; Tier A terminal/case-pad authority remains missing.


## Closed Items

| Item | Result | Evidence |
|---|---|---|
| BMI270 pin authority | PASS | Bosch BST-BMI270-DS000 Rev. 1.6 pin table; local symbol pin numbers/pad numbers 1-14 retained |
| BMI270 I2C mode | PASS | CSB tied high through R112; SDO tied low through R111 for address selection; INT1 used, INT2 unused |
| RV-3028 no-backup support | PASS for BOM/network definition | VDD bypass C114, VBACKUP tie-down R113, EVI tie-down R114, INT pull-up R115 |
| USB UFP support | PASS | CC1/CC2 independent 5.1k Rd; USB data series resistors; no USB power path |

## BMI270 Pin Mapping

| Physical Pin | Manufacturer Function | Symbol Pin | Footprint Pad | LR01 Handling | Result |
|---:|---|---:|---:|---|---|
| 1 | SDO / I2C address | 1 | 1 | R111 0R to GND | PASS |
| 2 | ASDx / secondary interface | 2 | 2 | unused per primary I2C configuration | PASS |
| 3 | ASCx / secondary interface | 3 | 3 | unused per primary I2C configuration | PASS |
| 4 | INT1 | 4 | 4 | `BMI_INT1` to ESP32 GPIO6 | PASS |
| 5 | VDDIO | 5 | 5 | 3V3_AON + C113 | PASS |
| 6 | GNDIO | 6 | 6 | GND | PASS |
| 7 | GND | 7 | 7 | GND | PASS |
| 8 | VDD | 8 | 8 | 3V3_AON + C112 | PASS |
| 9 | INT2 | 9 | 9 | unused | PASS |
| 10 | OCSB / secondary CS | 10 | 10 | unused | PASS |
| 11 | OSDO / secondary data | 11 | 11 | unused | PASS |
| 12 | CSB | 12 | 12 | R112 0R to VDDIO for I2C mode | PASS |
| 13 | SCx / SCL | 13 | 13 | `I2C_SCL` | PASS |
| 14 | SDx / SDA | 14 | 14 | `I2C_SDA` | PASS |

## Y1 Pin Authority

`Y1 PIN AUTHORITY = BLOCKED`

B2 closed land-pattern geometry for `7D040000I01 / C648973`, but P2.2 requires manufacturer-authoritative terminal/case/NC pad function. The current package does not contain a Tier A manufacturer pin-function table for `7D040000I01`; the available evidence is footprint geometry plus distributor/product data, which P2.2 explicitly forbids using alone for exact pin mapping PASS.

| Pad | Manufacturer Function | Symbol Pin | Footprint Pad | Result |
|---:|---|---:|---:|---|
| 1 | NOT TIER-A CONFIRMED | 1 | 1 | BLOCKED |
| 2 | NOT TIER-A CONFIRMED | 2 | 2 | BLOCKED |
| 3 | NOT TIER-A CONFIRMED | 3 | 3 | BLOCKED |
| 4 | NOT TIER-A CONFIRMED | 4 | 4 | BLOCKED |

## New Manufacturer-vs-Locked Conflict

`DESIGN DECISION REQUIRED`

| Device | Net | Current locked value | Manufacturer/package authority | Conflict | Possible options |
|---|---|---|---|---|---|
| U1 ESP32-S3FH4R2 QFN56 | `GNSS_PPS` | `GPIO47` in `LOCKED_GPIO_CONTRACT.csv` | Espressif ESP32-S3 QFN56 pin table / KiCad symbol: physical pin 47 is `MTDI`; QFN56 does not expose a `GPIO47` pad as a numbered package pin | Locked GPIO assignment cannot be mapped to physical pin -> symbol pin -> footprint pad | Engineering must choose an exposed ESP32-S3FH4R2 QFN56 GPIO for PPS, change exact MCU/package, or approve removing PPS. Codex did not alter the contract. |

Because this conflict is real and affects a locked GPIO contract, `EXACT_SYMBOL_PIN_MAPPING` cannot be PASS in P2.2.
