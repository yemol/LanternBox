# LR01 A4.4 Compass Axis Fix RC2

RC1 field validation after correcting compass axis handedness:

| Reference | RC1 observed | Signed error |
|---:|---:|---:|
| 0° | 9° | +9° |
| 90° | 103° | +13° |
| 180° | 183° | +3° |
| 270° | 273° | +3° |

Circular mean signed error is approximately +7°. The installed assembly therefore aligns much more closely with a physical +90° yaw than the provisional +97° fit used by RC1.

RC2 keeps the validated axis transform:

```cpp
float h = atan2f(-y, x) * 180.0f / PI;
```

and refines the device-front installation yaw to:

```cpp
FT_COMPASS_INSTALL_YAW_OFFSET_DEG = 90.0f;
```

Using the RC1 field measurements, expected RC2 four-point headings before a fresh magnetic calibration are approximately:

- reference 0° -> 2°
- reference 90° -> 96°
- reference 180° -> 176°
- reference 270° -> 266°

This is an installation-axis correction only. QMC hard/soft-iron calibration, NVS format, Host Protocol 2, GNSS, LoRa and FT-02 Core compass UI remain unchanged.

Acceptance:
1. Keep FT-02 level and away from magnets / speakers / large steel objects.
2. Use the same north reference on both instruments (prefer magnetic north for comparison).
3. Re-check 0/90/180/270 degrees before recalibrating.
4. If four-point direction remains correct, perform one fresh compass calibration and save it.
