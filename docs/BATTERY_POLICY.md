# Battery Policy

Adopt current955565/955465 protected Polymer Li-ion1S/5000mAh,3.8V nominal(3.7V compatible), from [BATTERY_SPEC](BATTERY_SPEC.md). Source standard charge2.5A, fast/maximum charge5A, maximum discharge5A, cutoff2.75V and discharge temperature-20C to+60C are now known. Pack protection presence is confirmed; precise thresholds remain unknown.

Charging remains inhibited by default. A1A initial proposal and1.5-2A board-capability options are below the source charge ceiling but are NOT approved charging profiles. BQ25628E's2A capability need not match the pack's2.5A standard or5A fast rate. Actual permitted charge current is the minimum of5A pack ceiling, source residual power, charger/thermal limits and approved configuration, accounting for tolerances.

**Charge voltage OPEN:** source CC-to4.25V then CV-at4.2V is inconsistent. Do not implement4.25V, silently select4.2V, or infer4.35V from3.8V nominal. Charge temperature is also OPEN: -20C to+60C is discharge only. Reset, watchdog expiry, I2C failure or missing temperature reading must not enable charging. Regulation tolerances must be checked after the source voltage is clarified.

Battery temperature protection must be autonomous in the charger. Use the pack NTC if suitable; otherwise a pack-surface external NTC with a qualified R25/Beta/tolerance. Open/short NTC faults must inhibit charge. Do not parallel charger TS and gauge thermistor bias networks without electrical proof. The gauge may use a validated host-reported pack temperature path where supported, while charger safety stays independent.

Protected-pack assembly retains the pack PCM/BMS. Board fuse/reverse protection and connector protection still need coordination. Optional bare-cell assembly requires the correct BQ297xx suffix and qualified back-to-back FETs, with mutually exclusive stuffing/bypass variants. Bare-cell plus protection-bypass is a forbidden assembly combination. Pack protection thresholds must not be copied from a different battery.

The source confirms all five pack protection functions, not their performance or independence. A2.75V discharge cutoff must not be programmed as an assumed PCM trip or used as a guaranteed loaded voltage. Treat5A as the maximum battery discharge design ceiling; permit no above5A pulse without new evidence. A qualified board limit may be lower because of mux, connector, thermal and loaded-voltage constraints. No full5A board rating is frozen.

Gauge profile and protection configuration must identify the installed battery. Future 10000mAh compatibility requires capacity/profile update plus an approved common voltage/current/thermal/connector envelope. Larger capacity alone does not authorize larger current. PCM recovery, charging a deeply discharged pack, disconnected battery and reverse insertion require explicit test cases.
