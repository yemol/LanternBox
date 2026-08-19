# QMC5883L Calibration Acceptance

Open LR01 USB Serial Monitor at 115200. This build accepts Host commands on the USB debug console as well as the real Core Host UART.

## 1. Baseline query
Send:
COMPASS_CAL_STATUS?

Expected when never calibrated:
COMPASS_CAL_STATE state=IDLE ... calibrated=0 ...

If a saved calibration already exists, calibrated=1 is correct.

## 2. Start
Send:
COMPASS_CAL_START

Expected:
COMPASS_CAL_STATE state=RUNNING progress=0 quality=0 samples=0 calibrated=<old state> ...

## 3. Move device
Slowly rotate through multiple orientations. Include full yaw rotations, pitch/roll changes, and figure-eight motion.

LR01 emits COMPASS_CAL_STATE approximately once per second while RUNNING/READY.
Verify:
- samples increases
- min/max values expand on all X/Y/Z axes
- progress increases based on coverage, not elapsed time
- quality moves 0 -> 1 -> 2/3
- GNSS, LoRa and Host PING/STATUS logs continue during calibration

## 4. Save
When state=READY and quality>=2:
COMPASS_CAL_SAVE

Expected:
COMPASS_CAL_STATE state=SAVED progress=100 quality=2|3 ... calibrated=1 ...

NAV_STATE compass_q becomes the saved quality immediately.

## 5. Reboot persistence
Reset/power-cycle LR01, then:
COMPASS_CAL_STATUS?

Expected:
calibrated=1
NAV_STATE compass_q=2|3
Heading uses saved offset/scale values.

## 6. Cancel preserves old calibration
With a saved calibration present:
COMPASS_CAL_START
Move the device briefly.
COMPASS_CAL_CANCEL

Expected:
COMPASS_CAL_STATE state=CANCELED ... calibrated=1 ...
Old saved calibration remains active before and after reboot.

## 7. Low quality save protection
COMPASS_CAL_START
Collect only a few samples, then:
COMPASS_CAL_SAVE

Expected:
LR01_ERR code=22 message="compass_cal_quality_low"
COMPASS_CAL_STATE ... quality=0|1 ...
If an old calibration existed, calibrated remains 1 and old parameters remain active.

## 8. Explicit reset
Only when intentionally testing destructive reset:
COMPASS_CAL_RESET

Expected:
COMPASS_CAL_STATE state=IDLE ... calibrated=0 ...
After reboot calibrated remains 0.
