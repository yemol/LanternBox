# FT02 Load Budget

Status: **OPEN, not a board rating**. The exact per-load records, units, provenance and unknown-bound propagation are generated in [load_budget.json](../hardware/FT02_PowerBoard_V1/review/load_budget.json) by `tools/load_budget.py`. Every record has typical, continuous maximum, startup peak, transient peak, duration, rail, source and concurrency fields. A reference observation is separate from a guaranteed maximum.

`?` below means UNKNOWN, never zero. Current figures are mA unless stated otherwise. All maximum and duration fields remain OPEN where the final module/configuration is not bounded.

| Load | Typical / source observation | Continuous max | Startup max | Transient max / duration | Rail | Concurrency / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| ESP32-S3 Core | Mode dependent;340mA specified RF peak reference | ? | ? | ? / ? | 3V3 | No exclusion; [Espressif v2.2 table5-7](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf),3.3V/25C/21dBm, not whole module maximum |
| EPD GDEQ0426T82 HAT | Prior source record <35mW refresh | ? | ? | ? / ? | ? | No exclusion; [local hardware identity](../hardware/FT-02/docs/FT02_hardware_research_new_chat_brief_v1_1.md); [Waveshare manual](https://www.waveshare.com/wiki/4.26inch_e-Paper_HAT_Manual), new retrieval failed; do not promote old typical value to peak |
| ES8311 codec | 14mW feature figure, not carrier current | ? | ? | ? / ? | Carrier input ? | No exclusion; [Everest Rev7.0](https://dl.espressif.com/dl/schematics/Audio_ES8311.pdf) |
| NS4150B PA | ? speaker/load/gain/rail | ? | ? | ? / ? | ? | Playback-only firmware is not a hardware cap; [actual PA control](../hardware/FT-02/firmware/FT02_Display_Bringup_v0_1/docs/FT02_AUDIO_ES8311_PA_WIRING.md) |
| MAX-M10S | At3V:tracking9.5+2.3;acquisition11.5+2.3 | ? | ?;100mA source recommendation | ? / ? | Carrier input ? | M10S OR F10S; [R08 table15](https://content.u-blox.com/sites/default/files/MAX-M10S_DataSheet_UBX-20035208.pdf); carrier/active antenna additional |
| MAX-F10S | At3V:tracking16+3;acquisition21+3 | ? | ?;100mA source recommendation | ? / ? | Carrier input ? | Alternative, not second simultaneous GNSS; [manufacturer](https://content.u-blox.com/sites/default/files/documents/MAX-F10S_DataSheet_UBXDOC-963802114-12732.pdf) |
| SX1262 TX | ? selected front end/frequency/power | ? | ? | ? / ? | 3V3 | Same-radio TX/RX exclusive; other load overlap OPEN |
| SX1262 RX | 4.6 bare-chip headline, mode dependent | ? | ? | ? / ? | 3V3 | [Semtech](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262); not whole Heltec carrier consumption |
| SEN0695 IMU/magnetometer | ? selected ODR/modes | ? | ? | ? / ? | 3V3_SENS | No exclusion; [DFRobot](https://wiki.dfrobot.com/sen0695/docs/24069); includes local1.8V regulator, avoid double counting |
| CardKB2 | ? not measured | ? | ? | ? / ? | 3V3/5V conflict | OI-021; [official manual](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1225/Unit_CardKB2_User_Manual_EN.pdf); actual wiring still must be reconciled |
| SD | ? exact card absent | ? | ? | ? / ? | 3V3 | SDMMC1-bit5MHz is not a current ceiling; include flash-program pulses |
| BQ27220 | 0.050 NORMAL /0.009 SLEEP | ? final duty | ? | ? / ? | BAT_GUARDED | Always retained; [TI RevA](https://www.ti.com/lit/ds/symlink/bq27220.pdf); upstream shunt exception |
| Status / I2C / PG | ? total; resistor draw depends on logic levels | ? | ? | ? / ? | 3V3 | Actual U1101/U902 and pullup nets; DNP bus pullups excluded |
| Other MainBoard loads | ? module/flash/PSRAM/debug | ? | ? | ? / ? | ? | Single-Core target, not silently adding/removing prototype LR01 controller |
| Input/charger/protection/UPS overhead | ? incomplete input/protection sheets | ? | ? | ? / ? | Multiple | Retained HARD OFF; separate external and battery current |
| U501 battery guard | 0.004 typical/0.0055 maximum at3.8V, VM=0; not complete stage draw | ? all states | ? filter/gates | ? / ? | BAT_PLUS | Separate from preceding row; add gate pulls and fault paths. [Guard review](BATTERY_PROTECTION_REVIEW.md); FET/R506 series loss is not an extra parallel current |
| U1301/control | At5V:0.034 enabled /0.005 shutdown bias; selected ON or OFF divider0.0455 plus leakage | ? full input/temperature | ? | ? / ? | UPS_RAW | [TPS22953](https://www.ti.com/lit/gpn/TPS22953); post-shunt in battery mode |
| R1303 raw bleeder | 1.064 at5V nominal | ? voltage envelope | None separately | ? / ? | SYS_RAW | Required discharge provision, not keep-alive |
| R1304 3V3 bleeder | 0.702 nominal | ? voltage envelope | None separately | ? / ? | 3V3 | Includes sensor discharge through ferrite |
| R1305 5V bleeder | 1.064 nominal | ? voltage envelope | None separately | ? / ? | 5V | Counts while5V enabled |
| Q1301/2/3 active discharge | OFF discharge; ON leakage not zero | ? hot leakage / sustained injection | ? overlap | <=37.04 each at assumed5.5V/minimum148.5ohm; duration ? | Three system rails | Mechanical OFF; all three can overlap. [Conditional calculations](SYSTEM_HARD_OFF_REVIEW.md); not an approved rail envelope |

## Aggregate Decision

| Required result | Current defensible result |
| --- | --- |
| 3V3 continuous / transient worst case | UNKNOWN / UNKNOWN; includes3V3_SENS |
| 5V continuous / transient worst case | UNKNOWN / UNKNOWN |
| Whole-board concurrent continuous / peak | UNKNOWN / UNKNOWN |
| Peak duration | UNKNOWN |

No enforced exclusion between Core RF, SD, EPD, GNSS, sensors and audio has been demonstrated. The safe review assumption is potential overlap, not a fabricated reduced duty cycle. Radio TX/RX are alternative states of one radio; M10S/F10S are alternative assemblies, never summed together. Unknown rail assignments block both candidate rail budgets. No total is produced from incomplete typical-only figures.

Earlier3V3 1.0A/1.5A and5V 1.2A/1.8A numbers remain deprecated estimates. TPS2121,TPS22953,TPS63802 and LT3120 cannot be frozen from this inventory. Needed evidence: final module/rail identities, PA/speaker and SD choice, current envelopes across temperature/supply, boot capacitance, pulse durations and demonstrable concurrency limits. No firmware was modified.
