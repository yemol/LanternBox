# LOCKED DESIGN SPEC

- Board: 66.00 x 58.00 mm, 2-layer FR4, 1.00 mm, 35um baseline.
- Assembly: all SMT Top. Bottom SMT forbidden. GNSS is Bottom TH DNP/user solder.
- MCU: ESP32-S3FH4R2 / C3013940 / 4MB QSPI Flash / 2MB QSPI PSRAM.
- Main clock: HD 7D040000I01 / C648973 / 40MHz / CL10pF / +/-10ppm / SMD2016-4P.
- Clock network: CXT1=CXT2=15pF C0G/NP0 0402; LXT1=24nH 0402 at XTAL_P.
- LoRa: SX1268IMLTRT, official E512V01A 490MHz RF geometry is authoritative.
- Radio clock: Q1 populated 32 MHz crystal path; Q2 optional/not fitted; TCXO_EN not required; FB1 NC per official E512 source.
- RF switch: PE4259-63 / C470892.
- RF switch control: GPIO7 = ANT_SW -> R4 -> PE4259 pin6 complementary control input; SX1268 DIO2 -> R3 -> PE4259 pin4 CTRL. Do not reroute SX1268 DIO2 to ESP32 and do not convert PE4259 to single-pin mode.
- IMU: BMI270 / C2836813.
- RTC: RV-3028-C7-32.768KHZ-1PPM-TA-QA / C3304278.
- USB: USB4105-GF-A / C3020560, data/debug only, no USB system power.
- Host: FH34D-10S-0.5SH(50), Top, 10P/0.5mm, cable exits bottom edge, Pin1 left.
- GNSS: MAX-M10S carrier, DNP, 5P2.54 Bottom TH; VCC GND TX RX PPS.
- Power: FT-02 3V3_AON, 3.3V nominal, host capability >=1A peak.
- Mounting: 4x M2, 2.2mm NPTH, nominal 6.0mm copper/track keepout.
- No new features or hardware.
