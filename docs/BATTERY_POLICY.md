# Battery Policy

Current 1S/3.7V/4.2V/5000mAh is confirmed by the user brief only. Electrical safety limits and pack construction are UNKNOWN until the existing frozen specification can be accessed.

Charging remains inhibited by default. A nominal 1A proposal is not an allowed setting until checked against the real pack. Firmware reset, watchdog expiry, I2C failure and no temperature reading must not enable an unqualified charging profile. Regulation setpoints must account for IC accuracy so worst-case battery voltage/current stay within the pack limit.

Battery temperature protection must be autonomous in the charger. Use the pack NTC if suitable; otherwise a pack-surface external NTC with a qualified R25/Beta/tolerance. Open/short NTC faults must inhibit charge. Do not parallel charger TS and gauge thermistor bias networks without electrical proof. The gauge may use a validated host-reported pack temperature path where supported, while charger safety stays independent.

Protected-pack assembly retains the pack PCM/BMS. Board fuse/reverse protection and connector protection still need coordination. Optional bare-cell assembly requires the correct BQ297xx suffix and qualified back-to-back FETs, with mutually exclusive stuffing/bypass variants. Bare-cell plus protection-bypass is a forbidden assembly combination. Pack protection thresholds must not be copied from a different battery.

Gauge profile and protection configuration must identify the installed battery. Future 10000mAh compatibility requires capacity/profile update plus an approved common voltage/current/thermal/connector envelope. Larger capacity alone does not authorize larger current. PCM recovery, charging a deeply discharged pack, disconnected battery and reverse insertion require explicit test cases.
