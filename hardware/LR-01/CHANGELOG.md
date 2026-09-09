# Changelog

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
