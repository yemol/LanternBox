# Manufacturer Evidence Register

These are official sources inspected during preflight. Reading relevant sections is not completion of pin-by-pin design verification. HUSB238A has multiple published PDF URLs; latest revision and exact ordering option still need confirmation. No distributor availability claims are made.

| ID | Source | Evidence used |
| --- | --- | --- |
| DS-01 | [TPS2121 Rev. F](https://www.ti.com/lit/ds/symlink/tps2121.pdf) | Sections 7.3/7.5, operating range, current, resistance, switching modes; section 10 examples |
| DS-02 | [BQ25628E Rev. C](https://www.ti.com/lit/ds/symlink/bq25628e.pdf) | Pin table, recommended passives, electrical data, internal power path, 0x6A, charging controls |
| DS-03 | [BQ27220 Rev. A](https://www.ti.com/lit/ds/symlink/bq27220.pdf) | Pin table, sense range, high-side/system application, current modes, address and timing |
| DS-04 | [BQ27220 TRM Rev. A](https://www.ti.com/lit/ug/sluubd4a/sluubd4a.pdf) | Capacity/profile fields; standard telemetry commands; RAM/OTP configuration |
| DS-05 | [TPS63802 Rev. D](https://www.ti.com/lit/ds/symlink/tps63802.pdf) | Input range, specified 3.3V capability, PG, no-load current, true shutdown |
| DS-06 | [LT3120 Rev. 0](https://www.analog.com/media/en/technical-documentation/data-sheets/lt3120.pdf) | Features and 5V application examples; low-input derating; compensation and layout remain to design |
| DS-07 | [HUSB238A official 20-page PDF](https://www.hynetek.com/uploadfiles/site/219/news/24322c00-2276-4d74-98db-c74432318210.pdf) | Tables 6-9; mode/address pins; shared analog/I2C pins; legacy detection; fuse limits |
| DS-08 | [BQ297xx Rev. I](https://www.ti.com/lit/ds/symlink/bq2970.pdf) | Protection functions and factory-defined thresholds; exact suffix not selected |
| DS-09 | [Vishay WSK2512,11-Dec-2023](https://www.vishay.com/docs/30108/wsk2512.pdf) | Four-terminal10mohm1% candidate WSK2512R0100FEA;1W at70C;35ppm/C full-resistor TCR; I1/I2/E1/E2 logical terminals, physical pad assignment pending |
| PS-01 | [BQ27220 product page](https://www.ti.com/product/BQ27220) | Active; capacity range includes 10000mAh, published maximum 32000mAh |
| PS-02 | [BQ24157 product page](https://www.ti.com/product/BQ24157) | Alternative screening: 1.55A, I2C; incomplete battery NTC/JEITA fit |
| PS-03 | [BQ25306 product page](https://www.ti.com/product/BQ25306) | Alternative screening: 3A standalone, no I2C |
| PS-04 | [BQ24160 ordering page](https://www.ti.com/product/BQ24160/part-details/BQ24160RGET) | Alternative screening: 2.5A, I2C, JEITA, power path, Active |
| IF-01 | [SEN0695 official address configuration](https://wiki.dfrobot.com/sen0695/docs/24069) | BMI323/BMM350 address straps on the module |
| IF-02 | [SEN0695 official schematic](https://dfimg.dfrobot.com/wiki/23756/SEN0695_bmi323-bmm350-9-dof-imu-sensor_schematics_V1.0.pdf) | Module has local 1.8V regulation; a bare BMM350 is not a 3.3V VDD load |
| IF-03 | [CardKB2 official manual](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1225/Unit_CardKB2_User_Manual_EN.pdf) | SectionIII confirms0x5F/100kHz; version-command wording unresolved |
| IF-04 | [ES8311 Rev7 manufacturer document hosted by Espressif](https://dl.espressif.com/dl/schematics/Audio_ES8311.pdf) | Section5 confirms001100x, CE selects0x18/0x19 |
| IF-05 | [MAX17048/49](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf) | Page16 bit pattern0110110;0x36, not8-bit0x6C/6D |

Before schematic release, archive exact datasheets/reference layouts and checksum them, record page/revision beside each calculated component, and verify all package pads against manufacturer drawings. Do not substitute HUSB238 register definitions for HUSB238A.

Additional check 2026-09-25: TPS63802 Rev D pin table7-1, Figure10-1 and Tables10-5/10-6 support the implemented reference. PG pullup is deliberately tied to3V3_MAIN rather than SYS_RAW for Core voltage compatibility. Footprints and qualified passive ordering suffixes are not released.

Battery update/reference extension2026-09-25: DS-03 page1 system-side high-side diagram, pin table and sections8.2/10 support sheet06. BAT1uF,VDD2.2uF minimum decoupling, fixed BIN10k for embedded packs, GPOUT10k,1M bus pulldowns and optional DNP bus pullups are reference values, not a complete FT02 qualification. Gauge VDD must not power external devices or I2C pullups. DS-09 temperature coefficient is35ppm/C for the complete10mohm resistor, not the element-only headline figure. Battery evidence is separately maintained in BATTERY_SPEC.md; no IC datasheet resolves its4.25V/4.2V charging ambiguity.

[TI SLVAFJ1](https://www.ti.com/lit/pdf/SLVAFJ1), February2023, sections1.1/2.1/2.2: automatic switchover example uses100mA/320uF and VCOMP mode. VCOMP does not support5us fast mode; example dip calculation uses100us. This does not establish FT02 heavy-load hold-up, fixed external-priority qualification or worst-case switching time.
