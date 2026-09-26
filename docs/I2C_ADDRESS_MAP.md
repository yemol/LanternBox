# I2C Address Map - Incomplete

All addresses are 7-bit. GPIO47 SDA / GPIO21 SCL / 100kHz is confirmed by local pin-map v1.1 and firmware. Physical straps and pullups still require inspection; local evidence is distinguished from official datasheet verification.

| Device | Address | Evidence / status |
| --- | --- | --- |
| BQ27220 candidate | 0x55 | Datasheet section 7.3.1.1; 0xAA/0xAB are on-wire write/read bytes |
| BQ25628E candidate | 0x6A | Datasheet section 8.6 |
| HUSB238A candidate | 0x42 or 0x62 | Official datasheet table 6; mode-specific 900kohm strap, exact assembly not frozen |
| SEN0695 BMI323 | 0x69 default / 0x68 | DFRobot official module strap documentation; actual pad state unknown |
| SEN0695 BMM350 | 0x15 default / 0x14 | Same official source; actual pad state unknown |
| CardKB2 | 0x5F | Official CardKB2 manual sectionIII, printed page3:100kHz; local pin-map agrees |
| ES8311 | 0x18 / 0x19 | Manufacturer datasheet Rev7 section5:001100x where x=CE; actual installed strap still to inspect |
| Existing MAX17048 | 0x36 | Official slave address bits0110110; wire bytes0x6C/0x6D; removed in future architecture |
| Future GNSS / other sensors | UNVERIFIED | Identify actual interface use and model |
| TCA9539PWR U1101 | 0x74 | Sheet11 PW pin21 A0 and pin2 A1 tied GND; POR ports input. Source-policy integration incomplete |

The verified subset has no duplicate address. This is NOT a complete no-conflict verdict. Sources: [gauge](https://www.ti.com/lit/ds/symlink/bq27220.pdf), [charger](https://www.ti.com/lit/ds/symlink/bq25628e.pdf), [PD](https://www.hynetek.com/uploadfiles/site/219/news/24322c00-2276-4d74-98db-c74432318210.pdf), [SEN0695](https://wiki.dfrobot.com/sen0695/docs/24069).

[TCA9539 pin/address tables](https://www.ti.com/lit/ds/symlink/tca9539.pdf): P00/P01=3V3/5V PG, P02=FG_GPOUT, P03/P04/P05=charger PG/STAT/INT, P06=CHG_REQUEST, P07=MUX ST. Its INT_N alone drives PWR_INT. Gauge wake may drive P02 low then release to input, never push high. Short charger interrupt capture and host latency are unqualified; do not claim lossless event capture. P10..P17 each have100k pulldowns R1103..R1110 for defined POR input states; they are not implemented policy controls.

Only one deliberate effective pullup network per bus; calculate parallel module pullups and rise time, not arbitrary extra 10kohm resistors on every chip. Check 3.3V logic compatibility, clock stretching, partial-power-down leakage and ESD-diode phantom powering in every rail state. Do not add an I2C mux before resolving actual address/strap conflicts.

Additional evidence: [CardKB2 manual](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1225/Unit_CardKB2_User_Manual_EN.pdf), [ES8311 manufacturer datasheet hosted by Espressif](https://dl.espressif.com/dl/schematics/Audio_ES8311.pdf). CardKB2's separate firmware-version paragraph calls0xF1 a communication address; it is not a valid7-bit device address and must not be added as such. Resolve that protocol wording against the actual firmware before using the version command. It does not invalidate the documented0x5F keyboard address.

[MAX17048/49 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf), printed page16, shows the7-bit bit pattern0110110. Its prose labels0x6C/0x6D as addresses, but these include the R/W bit; do not use them as7-bit software addresses.
