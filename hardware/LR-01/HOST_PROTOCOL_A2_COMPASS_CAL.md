# HOST_PROTOCOL_A2 Compass Calibration Extension

Status: frozen additive extension to HOST_PROTOCOL_A2.
Existing NAV_STATE field order and semantics are unchanged.

## Core -> LR01

COMPASS_CAL_START
COMPASS_CAL_STATUS?
COMPASS_CAL_SAVE
COMPASS_CAL_CANCEL
COMPASS_CAL_RESET

## LR01 -> Core

COMPASS_CAL_STATE state=<STATE> progress=<0..100> quality=<0..3> samples=<n> calibrated=<0|1> min_x=<n> max_x=<n> min_y=<n> max_y=<n> min_z=<n> max_z=<n>

STATE:
- IDLE
- RUNNING
- READY
- SAVED
- CANCELED
- FAILED

Example:
COMPASS_CAL_STATE state=RUNNING progress=42 quality=1 samples=386 calibrated=1 min_x=-1240 max_x=822 min_y=-956 max_y=1105 min_z=-701 max_z=593

Ready example:
COMPASS_CAL_STATE state=READY progress=100 quality=3 samples=1450 calibrated=1 min_x=-1240 max_x=1232 min_y=-1210 max_y=1254 min_z=-1188 max_z=1191

Saved example:
COMPASS_CAL_STATE state=SAVED progress=100 quality=3 samples=1450 calibrated=1 min_x=-1240 max_x=1232 min_y=-1210 max_y=1254 min_z=-1188 max_z=1191

`calibrated` means a valid saved calibration currently exists. It can therefore remain 1 while a new RUNNING session is in progress.

## Save rules

Only session quality >=2 is saveable.
START clears only session min/max/sample/coverage state. It never changes the saved calibration.
SAVE computes hard-iron offsets and per-axis soft-iron scales, writes a new NVS slot, verifies CRC/readback, then atomically switches the active in-RAM parameters.
If SAVE fails, the prior saved calibration remains active.
CANCEL discards the active session for calibration purposes and keeps the prior saved calibration.
RESET explicitly removes both persistent calibration slots and returns to uncalibrated state.

## Calibration math

offsetX=(maxX+minX)/2
offsetY=(maxY+minY)/2
offsetZ=(maxZ+minZ)/2

rangeX=(maxX-minX)/2
rangeY=(maxY-minY)/2
rangeZ=(maxZ-minZ)/2
avgRange=(rangeX+rangeY+rangeZ)/3

scaleX=avgRange/rangeX
scaleY=avgRange/rangeY
scaleZ=avgRange/rangeZ

correctedX=(rawX-offsetX)*scaleX
correctedY=(rawY-offsetY)*scaleY
correctedZ=(rawZ-offsetZ)*scaleZ

LR01 applies all three corrected axes. With no accelerometer available, magnetic heading is atan2(correctedY, correctedX); corrected Z participates in calibration coverage/quality and is retained for future tilt compensation.

## Progress and quality

Progress is not time-based. It combines:
- valid sample count
- X/Y/Z span coverage
- minimum-axis / maximum-axis span balance
- 3D octant coverage around the running hard-iron midpoint

Quality:
0 invalid/data insufficient
1 poor
2 usable/saveable
3 good/saveable

Current thresholds:
- quality 1 requires >=100 samples and minimum axis span >=200 counts
- quality 2 requires >=400 samples, minimum axis span >=600 counts, range balance >=0.25 and >=3 octants
- quality 3 requires >=800 samples, minimum axis span >=900 counts, range balance >=0.50 and >=6 octants

READY is reached only when quality>=2. When READY, progress is reported as 100.

## Runtime heading quality

NAV_STATE remains unchanged.
`compass_q` reflects the active saved calibration quality:
- 0 sensor/heading invalid
- 1 sensor valid but no saved calibration
- 2 saved usable calibration
- 3 saved good calibration

During a new RUNNING calibration session, heading continues to use the previously saved calibration until SAVE succeeds.

## Errors

Existing LR01_ERR format is retained.

20 compass_cal_busy
21 compass_cal_not_running
22 compass_cal_quality_low
23 compass_cal_save_failed
24 compass_cal_reset_failed
25 compass_not_ready

Examples:
LR01_ERR code=20 message="compass_cal_busy"
LR01_ERR code=21 message="compass_cal_not_running"
LR01_ERR code=22 message="compass_cal_quality_low"
LR01_ERR code=23 message="compass_cal_save_failed"

## Persistence

NVS/Preferences namespace: qmc_cal
Keys: cal0, cal1
Each slot includes:
- magic
- version
- generation
- offsetX/Y/Z
- scaleX/Y/Z
- quality
- valid
- CRC32

At boot LR01 loads the newest valid generation. A corrupt or partially written new slot cannot destroy the older valid slot.

## Debug USB console

For bench acceptance only, the LR01 USB Serial console accepts the same calibration commands. Production Core communication remains Host UART 115200 8N1.
