# FINAL DECISION LOG

No feature additions.

Exact procurement locks for this handoff:
- ESP32-S3FH4R2 / C3013940
- 40MHz crystal HD 7D040000I01 / C648973
- BMI270 / C2836813
- RV-3028-C7-32.768KHZ-1PPM-TA-QA / C3304278
- PE4259-63 / C470892
- USB4105-GF-A / C3020560

The exact JLC-assemblable HD crystal supersedes the earlier family-level Epson FA-128 discussion for fabrication closure. This is procurement closure, not a functional redesign.

GNSS1 remains DNP and user-soldered.

RF boundary / control locks approved 2026-08-15:
- E512 RF fragment source: X=153.5000..193.0011 mm, Y=76.0036..134.0036 mm.
- LR01 RF fragment target: X=26.4989..66.0000 mm, Y=0.0000..58.0000 mm.
- Rigid translation only: DX=-127.0011 mm, DY=-76.0036 mm.
- GPIO7 = ANT_SW is authoritative. ESP32 GPIO7 / ANT_SW -> R4 -> PE4259 pin6 complementary control input.
- Preserve official E512 topology: SX1268 DIO2 -> R3 -> PE4259 pin4 CTRL. Do not reroute DIO2 to ESP32 and do not convert PE4259 to single-pin mode.
- Locked LR01 radio clock: Q1 populated 32 MHz crystal path; Q2 optional/not fitted; TCXO_EN not required; FB1 NC per official E512 source.
