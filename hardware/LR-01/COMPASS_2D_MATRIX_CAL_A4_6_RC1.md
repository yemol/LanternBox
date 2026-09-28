# Compass 2D Matrix Calibration A4.6 RC1

## Why A4.5 is not enough

A4.5 corrected the major design error of mixing Z/tilt samples into a 2D heading model. Bench data then showed:

- before A4.5 save: 0->5, 90->87, 180->151, 270->270
- after A4.5 save: 0->348, 90->100, 180->173, 270->256

The post-calibration result reduces the worst single-direction error but leaves direction-dependent residual error. Independent `scaleX/scaleY` can correct only an ellipse whose principal axes are aligned with sensor X/Y. It cannot correct an ellipse rotated by local soft-iron / cross-axis coupling.

## A4.6 model

A4.6 keeps horizontal-only calibration and computes:

1. hard-iron center from X/Y extrema
2. X/Y covariance including the cross term
3. symmetric inverse-square-root matrix of the covariance
4. corrected vector using a full 2x2 matrix

```text
[ Xc ]   [ m00  m01 ] [ X - offsetX ]
[ Yc ] = [ m01  m11 ] [ Y - offsetY ]
```

This preserves the horizontal 2D model while correcting rotated-ellipse distortion.

## User calibration motion

Keep FT-02 level and rotate slowly through 360 degrees for 2 to 3 complete turns. Do not do figure-eight, pitch, or roll motion.

## Persistence

Calibration blob version is now 3. Existing v1/v2 calibration blobs are ignored automatically.
