# P2.2 Power / Decoupling Closure

Status: PASS for defined power/decoupling requirements; P2.2 overall remains BLOCKED by pin authority conflict.
Date: 2026-08-15

`POWER_DECOUPLING_REQUIREMENTS = PASS`

## Authority Sources Consulted

- Espressif ESP32-S3 Hardware Design Guidelines / schematic checklist: `https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/`
- Espressif official ESP32-S3 pin/package data: `https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf`
- Bosch Sensortec BMI270 datasheet BST-BMI270-DS000 Rev. 1.6: `https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf`
- Micro Crystal RV-3028-C7 datasheet/application manual: `https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf` and `https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf`
- TI TPD2EUSB30 datasheet and TIDM594A CAD evidence retained from B2.4.
- Semtech SX1268 / E512 RF reference evidence retained from frozen RF audit.
- Y1 7D040000I01 evidence currently available in-package only as B2 footprint geometry closure plus LCSC/C648973 distributor data; Tier A terminal/case-pad authority remains missing.


| Device | Power / Support Node | Requirement | Production Ref | Result |
|---|---|---|---|---|
| ESP32-S3FH4R2 | CHIP_PU / LR01_EN | 10k pull-up + 1uF to GND | R101 + C101 | SATISFIED |
| ESP32-S3FH4R2 | GPIO0 / BOOT | 10k pull-up; SW1 pulls low | R102 + SW1 | SATISFIED |
| ESP32-S3FH4R2 | VDD3P3_RTC | Local 100nF bypass | C104 | SATISFIED |
| ESP32-S3FH4R2 | VDD3P3_CPU | Local 100nF bypass | C105 | SATISFIED |
| ESP32-S3FH4R2 | VDD_SPI | 100nF + 1uF local bypass | C106 + C107 | SATISFIED |
| ESP32-S3FH4R2 | Main 3V3 pins / digital section | 10uF local bulk, DC-bias margin | C108 + C109 + C111 | SATISFIED |
| ESP32-S3FH4R2 | analog/RF 3V3 feed | filtered feed + 100nF bypass | L101 + C110 | SATISFIED |
| USB D+/D- | series impedance damping | 22R symmetric series resistors | R103 + R104 | SATISFIED |
| USB D+/D- | optional shunt tuning | DNP only, no default capacitance | C102 + C103 | SATISFIED / DNP |
| USB-C CC1/CC2 | UFP Rd to GND | independent 5.1k each | R105 + R106 | SATISFIED |
| USB VBUS sense | no system power path; 100k/100k divider | locked divider, 5V -> 2.5V sense | R107 + R108 | SATISFIED; no conflict found |
| BMI270 | VDD | independent 100nF local bypass | C112 | SATISFIED |
| BMI270 | VDDIO | independent 100nF local bypass | C113 | SATISFIED |
| BMI270 | I2C mode/address | CSB high, SDO low by populated straps | R112 + R111 | SATISFIED |
| I2C bus | SDA/SCL pull-ups | 4.7k to 3V3_AON | R109 + R110 | SATISFIED |
| RV-3028-C7 | VDD | 100nF local bypass | C114 | SATISFIED |
| RV-3028-C7 | VBACKUP no-backup | not floating; no backup source added | R113 to GND | SATISFIED pending final schematic capture |
| RV-3028-C7 | EVI unused | not floating | R114 to GND | SATISFIED pending final schematic capture |
| RV-3028-C7 | INT open-drain to ESP32 GPIO1 | pull-up to 3V3_AON | R115 | SATISFIED |
| Y1 40MHz | load capacitors | 15pF C0G/NP0 each | CXT1/CXT2 | BOM SATISFIED; pin authority still BLOCKED |
| Y1 40MHz | XTAL_P series element | 24nH | LXT1 | BOM SATISFIED; pin authority still BLOCKED |
| SX1268 | RF supply/DC-DC/crystal passives | Existing E512 local network | RF_C1/RF_C16/RF_C17/RF_C18/RF_L7 and frozen RF refs | SATISFIED BY FROZEN RF |
| PE4259 | control filtering/series | E512 R3/R4 + C14/C15 | RF_R3/RF_R4/RF_C14/RF_C15 | SATISFIED BY FROZEN RF |
| U6 TPD2EUSB30DRTR | support passives | TI evidence does not require extra external passives | none | NOT REQUIRED |
| GNSS 5-pin carrier | module decoupling | carrier/module-side responsibility; LR01 only exposes locked 5-pin interface | none added | NOT REQUIRED unless future module integration changes |

No USB-to-3V3 main power path was created. No backup battery, supercapacitor, backup connector, new sensor, or new function was added.
