# P2.2 Production Passive BOM Audit

Status: PASS for passive BOM content; P2.2 overall remains BLOCKED by remaining pin authority conflict.
Date: 2026-08-15

`PRODUCTION_PASSIVE_BOM = PASS`

The production passive BOM has been expanded with formal reference designators, values, tolerances, ratings, packages, manufacturers, exact MPNs, assembly status, and authority rationale. This did not perform schematic capture, PCB placement, routing, footprint geometry modification, or RF geometry modification.

## Authority Sources Consulted

- Espressif ESP32-S3 Hardware Design Guidelines / schematic checklist: `https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/`
- Espressif official ESP32-S3 pin/package data: `https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf`
- Bosch Sensortec BMI270 datasheet BST-BMI270-DS000 Rev. 1.6: `https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf`
- Micro Crystal RV-3028-C7 datasheet/application manual: `https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf` and `https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf`
- TI TPD2EUSB30 datasheet and TIDM594A CAD evidence retained from B2.4.
- Semtech SX1268 / E512 RF reference evidence retained from frozen RF audit.
- Y1 7D040000I01 evidence currently available in-package only as B2 footprint geometry closure plus LCSC/C648973 distributor data; Tier A terminal/case-pad authority remains missing.


## Added / Frozen Passive Counts

| Category | Count | Notes |
|---|---:|---|
| New populated resistors | 15 | R101-R115 |
| New populated capacitors | 12 | C101, C104-C114 |
| New inductors / beads | 1 | L101 |
| New DNP positions | 2 | C102/C103 USB D+/D- shunt tuning pads |
| Existing crystal-network passives now exact-MPN frozen | 3 refs / 4 pcs | CXT1/CXT2 and LXT1 |

## Added Production Passives

| Ref | Function | Value | Package | Exact MPN | Assembly | Authority |
|---|---|---|---|---|---|---|
| R101 | ESP32 EN pull-up | 10 kOhm | 0402 1005 | `RC0402FR-0710KL` | TOP SMT | Espressif CHIP_PU support; locked 10k pull-up |
| C101 | ESP32 EN RC delay capacitor | 1 uF | 0402 1005 | `GRM155R61A105KE15D` | TOP SMT | Espressif CHIP_PU support; locked 1uF to GND |
| R102 | ESP32 GPIO0 BOOT pull-up | 10 kOhm | 0402 1005 | `RC0402FR-0710KL` | TOP SMT | Espressif strapping; locked BOOT pull-up |
| R103 | USB_D+ series resistor | 22 Ohm | 0402 1005 | `RC0402FR-0722RL` | TOP SMT | Espressif native USB guideline; symmetric series resistor |
| R104 | USB_D- series resistor | 22 Ohm | 0402 1005 | `RC0402FR-0722RL` | TOP SMT | Espressif native USB guideline; symmetric series resistor |
| C102 | USB_D+ optional shunt tuning pad | DNP / tuning placeholder | 0402 1005 | `N/A / DNP` | DNP | Espressif USB tuning placeholder only; not populated |
| C103 | USB_D- optional shunt tuning pad | DNP / tuning placeholder | 0402 1005 | `N/A / DNP` | DNP | Espressif USB tuning placeholder only; not populated |
| R105 | USB-C CC1 Rd UFP | 5.1 kOhm | 0402 1005 | `RC0402FR-075K1L` | TOP SMT | USB Type-C UFP Rd requirement |
| R106 | USB-C CC2 Rd UFP | 5.1 kOhm | 0402 1005 | `RC0402FR-075K1L` | TOP SMT | USB Type-C UFP Rd requirement |
| R107 | USB VBUS sense divider high | 100 kOhm | 0402 1005 | `RC0402FR-07100KL` | TOP SMT | Locked USB_VBUS_SENSE 100k/100k divider; no USB power path |
| R108 | USB VBUS sense divider low | 100 kOhm | 0402 1005 | `RC0402FR-07100KL` | TOP SMT | Locked USB_VBUS_SENSE 100k/100k divider; no USB power path |
| R109 | I2C_SDA pull-up to 3V3_AON | 4.7 kOhm | 0402 1005 | `RC0402FR-074K7L` | TOP SMT | Locked GPIO contract; shared BMI270/RV3028 I2C |
| R110 | I2C_SCL pull-up to 3V3_AON | 4.7 kOhm | 0402 1005 | `RC0402FR-074K7L` | TOP SMT | Locked GPIO contract; shared BMI270/RV3028 I2C |
| L101 | ESP32 analog/RF 3V3 supply isolation bead | Ferrite bead 120 Ohm @100MHz | 0402 1005 | `BLM15AG121SN1D` | TOP SMT | Espressif ESP32-S3 power-filter guidance for analog/RF 3V3 feed |
| C104 | ESP32 VDD3P3_RTC bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Espressif local decoupling |
| C105 | ESP32 VDD3P3_CPU bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Espressif local decoupling |
| C106 | ESP32 VDD_SPI high-frequency bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Espressif VDD_SPI decoupling |
| C107 | ESP32 VDD_SPI bulk bypass | 1 uF | 0402 1005 | `GRM155R61A105KE15D` | TOP SMT | Espressif VDD_SPI decoupling |
| C108 | ESP32 VDD3P3 main bulk near pins 2/3 | 10 uF | 0603 1608 | `GRM188R61A106KE69D` | TOP SMT | Espressif 3V3 bulk decoupling; 0603 chosen for DC-bias margin |
| C109 | ESP32 VDD3P3 secondary bulk/local | 10 uF | 0603 1608 | `GRM188R61A106KE69D` | TOP SMT | Espressif 3V3 bulk decoupling; 0603 chosen for DC-bias margin |
| C110 | ESP32 filtered analog/RF 3V3 bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Espressif analog/RF 3V3 local bypass after L101 |
| C111 | Board 3V3_AON local bulk for digital section | 10 uF | 0603 1608 | `GRM188R61A106KE69D` | TOP SMT | Board-level local 3V3_AON bulk for ESP32/USB/sensor cluster |
| C112 | BMI270 VDD bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Bosch BMI270 VDD decoupling |
| C113 | BMI270 VDDIO bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Bosch BMI270 VDDIO decoupling |
| R111 | BMI270 SDO I2C address strap to GND | 0 Ohm | 0402 1005 | `CRCW04020000Z0ED` | TOP SMT | Bosch BMI270 I2C SDO/address handling; address fixed low |
| R112 | BMI270 CSB I2C mode strap to VDDIO | 0 Ohm | 0402 1005 | `CRCW04020000Z0ED` | TOP SMT | Bosch BMI270 I2C mode requires CSB high |
| C114 | RV-3028 VDD bypass | 100 nF | 0402 1005 | `GRM155R71C104KA88D` | TOP SMT | Micro Crystal RV-3028-C7 VDD decoupling |
| R113 | RV-3028 VBACKUP no-backup tie-down | 10 kOhm | 0402 1005 | `RC0402FR-0710KL` | TOP SMT | RV-3028 no-backup configuration; VBACKUP not floating |
| R114 | RV-3028 EVI unused tie-down | 10 kOhm | 0402 1005 | `RC0402FR-0710KL` | TOP SMT | RV-3028 EVI unused handling; not floating |
| R115 | RV-3028 INT open-drain pull-up | 10 kOhm | 0402 1005 | `RC0402FR-0710KL` | TOP SMT | RTC_INT to ESP32 GPIO1; open-drain pull-up |
| CXT1,CXT2 | ESP32 40MHz crystal load capacitors | 15 pF | 0402 1005 | `GRM1555C1H150JA01D` | TOP SMT | Locked 40MHz crystal network; exact passive closure |
| LXT1 | ESP32 XTAL_P series inductor | 24 nH | 0402 1005 | `LQG15HN24NJ02D` | TOP SMT | Locked 40MHz crystal network; exact passive closure |

## Package Compatibility

| Package | Intended KiCad footprint family | Result |
|---|---|---|
| 0402 1005 resistor | `Resistor_SMD:R_0402_1005Metric` | PACKAGE COMPATIBILITY = PASS |
| 0402 1005 capacitor | `Capacitor_SMD:C_0402_1005Metric` | PACKAGE COMPATIBILITY = PASS |
| 0603 1608 10uF capacitor | `Capacitor_SMD:C_0603_1608Metric` | PACKAGE COMPATIBILITY = PASS; chosen for DC-bias margin |
| 0402 1005 ferrite/inductor | `Inductor_SMD:L_0402_1005Metric` or manufacturer-compatible bead footprint | PACKAGE COMPATIBILITY = PASS pending schematic footprint assignment |

All populated added passives have exact MPNs. DNP tuning positions intentionally use `N/A / DNP`.

Formal BOM: `cad/FT02-LR01_v1.1_PRODUCTION_BOM.csv`. `spec/LOCKED_BOM.csv` was updated to the same detailed production-BOM structure, with the prior BOM retained at `spec/LOCKED_BOM_PRE_P2_2.csv`.
