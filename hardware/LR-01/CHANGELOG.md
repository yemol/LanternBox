## A4.7 Compass Geometric Calibration RC1

- Replaced extrema-center + sample-covariance calibration with a geometric 2D ellipse fit.
- Added robust grouped-extrema provisional center and equal-angle sample weighting.
- General conic linear terms now refine hard-iron X/Y center.
- Calibration persistence schema bumped to v4, automatically invalidating A4.6/v3 slots.
- Added saved calibration coefficients to `COMPASS_CAL_STATE` for bench diagnostics.
- QMC axis handedness and current body-frame mapping are unchanged.

## A4.6 Compass 2D Matrix Calibration RC1

- Replaces independent X/Y min-max scaling with a full symmetric 2x2 correction matrix.
- Adds X-Y covariance/cross-axis correction for rotated soft-iron ellipses.
- Keeps hard-iron center from X/Y extrema.
- Keeps the validated `atan2(-Y, X)` body-frame transform and +90 degree device-front alignment unchanged.
- Calibration persistence version raised to v3, so A4.5 v2 calibration slots are intentionally ignored after flashing.
- Host protocol and calibration commands remain unchanged.

# Changelog

## 2026-09-24 - A4.5 Compass 2D Calibration Fix RC1

- Root-fixes the calibration regression observed after A4.4: old 3D-style calibration changed a near-correct four-point set (9/103/183/273) into a heavily warped set (13/77/133/327).
- Replaces 3D X/Y/Z quality and scaling with horizontal X/Y-only calibration, matching the actual 2D heading model.
- Calibration quality now uses X/Y spans, X/Y balance and four-quadrant coverage; Z remains diagnostic only.
- Candidate scale now uses `(rangeX + rangeY) / 2`; Z no longer changes X/Y scale.
- Bumps calibration NVS format version from 1 to 2 so previously saved 3D-derived calibration is automatically ignored.
- Retains `atan2(-correctedY, correctedX)`, +90 degree body yaw, Host Protocol 2, GNSS and LoRa behavior.
- New calibration procedure: keep FT-02 level and rotate horizontally 2-3 full turns; no figure-eight and no pitch/roll motion.

## 2026-09-24 - A4.4 Compass Axis Fix RC2

- Field-validated RC1 axis handedness fix: measured 0->9, 90->103, 180->183, 270->273 degrees.
- Refines device-front installation yaw from provisional +97 degrees to physical +90 degrees.
- Keeps `atan2(-correctedY, correctedX)` axis transform unchanged.
- Keeps QMC calibration/NVS, Host Protocol 2, GNSS, LoRa and FT-02 Core behavior unchanged.
- Expected pre-recalibration four-point result from the measured RC1 data: approximately 2, 96, 176, 266 degrees.

## 2026-09-24 - A4.4 Compass Axis Fix RC1

- Corrects opposite compass yaw handedness observed on the installed FT-02 assembly.
- Changes heading from `atan2(correctedY, correctedX)` to `atan2(-correctedY, correctedX)`.
- Replaces the old assumed +180 degree installation yaw with a measured +97 degree device-front alignment.
- Keeps QMC hard/soft-iron calibration, NVS layout, Host Protocol 2 and GNSS/LoRa behavior unchanged.
- Fit basis: 0->278, 90->178, 180->99, 270->12. Predicted corrected headings are approximately 359, 99, 178, 265 degrees.

## 2026-09-09 - A4.3 Submission Clean

- No functional protocol change from the tested A4.3 build.
- Retains 180-degree compass installation correction and GNSS software standby.
- Disables routine LoRa packet-flow traces by default; errors and recovery events remain.
- Removes intermediate verification/package artifacts from the repository root.
- Adds Git ignore rules and a safe secrets template.
- Simplifies production comments and version banners.

## 2026-09-08 - A4.3 Compass / GNSS control

- Added 180-degree compass installation yaw correction after calibration.
- Retained `GNSS_SOFT_OFF`, `GNSS_SOFT_ON`, and `GNSS_POWER?`.
- GNSS latitude/longitude/fix behavior and Host Protocol 2 field names remain unchanged.

## 2026-09-08 - GNSS software standby

- Added u-blox M10 `UBX-RXM-PMREQ` software standby with UART-RX wake.
- Suppresses GNSS UART polling/recovery/periodic blackbox sampling while standby is active.
- Reapplies the RAM acquisition baseline after wake.
- Records `GNSS_SOFT_OFF` and `GNSS_SOFT_ON` blackbox events.

## 2026-09-08 - GNSS production recovery

- Added UBX receiver monitoring, RAM-only acquisition configuration and UBX-PVT fallback.
- Added persistent GNSS blackbox diagnostics.
- Receiver watchdog performs a deliberate cold restart only after sustained strong-sky
  evidence with no ephemeris/used-SV progress, then allows an undisturbed reacquisition window.
- GNSS UART RX buffer remains 4096 bytes.

## 2026-08-19 - Host Protocol 2 / compass calibration

- Added persistent QMC compass calibration.
- Retained NAV/RADIO/SYSTEM state, messaging, node list and delivery status interfaces.
