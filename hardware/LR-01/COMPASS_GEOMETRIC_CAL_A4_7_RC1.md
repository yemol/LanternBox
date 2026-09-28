# LR01 A4.7 Compass Geometric Calibration RC1

## Why A4.7 exists

Four-point tests after A4.6 showed a first-harmonic residual: opposite headings carried approximately opposite errors. That pattern points to residual hard-iron center error rather than a fixed yaw offset. A4.6 still estimated the hard-iron center from raw extrema and built its matrix from every sample, so a single extreme point and unequal dwell time during a manual sweep could bias the result.

## Changes

- Saved calibration schema version is now v4. v1/v2/v3 calibration slots are ignored automatically.
- Calibration remains horizontal 2D only. No figure-eight, pitch or roll is required.
- Up to 1600 recent X/Y samples are retained during a calibration sweep.
- A robust provisional center is formed from averaged opposite extreme groups instead of a single min/max pair.
- Samples are grouped into 36 angular sectors. Each occupied sector receives equal weight, preventing a pause at one heading from dominating the fit.
- A general 2D ellipse is fit from the sector representatives:
  `A*x^2 + B*x*y + C*y^2 + D*x + E*y = 1`.
- The fitted linear terms refine the hard-iron center.
- The fitted quadratic form produces a symmetric 2x2 soft-iron correction matrix.
- `COMPASS_CAL_STATE` appends saved `off_x/off_y/m_xx/m_xy/m_yy` diagnostic values after a successful calibration.

## Calibration motion

Keep FT-02 level and rotate it slowly through 360 degrees two or three times. A reasonably steady sweep is preferred, but A4.7 is designed not to depend on perfectly constant rotation speed.

Do not pitch, roll or perform a figure-eight. LR01 has no accelerometer, so runtime heading is horizontal 2D.

## Scope intentionally unchanged

- QMC axis handedness correction remains `atan2(-Y, X)`.
- Current 90-degree body-frame mapping remains unchanged for this experiment.
- GNSS, LoRa, Mesh/PKI, Host Protocol core fields and FT-02 firmware are unchanged.
