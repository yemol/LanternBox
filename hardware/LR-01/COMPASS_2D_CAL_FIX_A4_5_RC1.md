# LR01 A4.5 Compass 2D Calibration Fix RC1

## Root cause

The runtime heading is a 2D magnetic azimuth because LR01 has no accelerometer for tilt compensation. The old calibration nevertheless required 3D X/Y/Z coverage and computed X/Y scale using an average of X, Y and Z ranges. Users were therefore instructed to pitch/roll and perform figure-eight motion.

That mixes tilted magnetic-field projections into a heading model that later uses only X/Y. A calibration could pass quality=2/3 yet make horizontal heading substantially worse.

Field evidence before this fix:

- Before recalibration with the axis fix: 0->9, 90->103, 180->183, 270->273
- After old 3D-style recalibration: 0->13, 90->77, 180->133, 270->327

The second set is strongly direction-dependent, not a constant yaw offset.

## Fix

- Keep the A4.4 body-frame correction: `atan2(-correctedY, correctedX)`.
- Keep device-front yaw at +90 degrees.
- Compass calibration is now horizontal 2D only.
- Calibration quality uses X/Y span, X/Y balance and four-quadrant coverage.
- Z remains visible in calibration status for diagnostics but does not affect quality or X/Y scale.
- Candidate calibration uses `avgRange=(rangeX+rangeY)/2`.
- `offsetZ=0`, `scaleZ=1` because Z is not used for heading and there is no tilt compensation.
- Calibration NVS version is bumped from 1 to 2, so old 3D-derived saved calibrations are automatically ignored after flashing.

## New calibration motion

Keep the complete FT-02 level and horizontal. Do not tilt it and do not perform a figure-eight.

1. Start calibration.
2. Place FT-02 flat, away from magnets / speakers / large metal objects.
3. Rotate slowly through 360 degrees while keeping it level.
4. Repeat 2 to 3 full horizontal turns until quality reaches 2 or 3.
5. Save.
6. Reboot LR01 and verify 0 / 90 / 180 / 270 degrees.
