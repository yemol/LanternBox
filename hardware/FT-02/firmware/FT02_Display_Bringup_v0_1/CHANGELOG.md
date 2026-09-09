# Changelog

## v2.75k67

- Fixed a production-cleanup build regression in Field Library queued-navigation coalescing after debug trace removal.
- Fixed duplicate and persistent UART warnings in System Self-Test.
- Improved Host UART protocol identification when the LR01 startup banner is missed.
- Added intentional-GNSS-OFF handling to map and location recorder flows.
- Preserved MAX17048 battery monitoring, standalone compass/calibration, GNSS software power control, Wi-Fi, Reader, Audio, Field Library, FieldManual, Map, LoRa and Input behavior.
- Cleaned production logging: normal runtime paths are quiet; hardware, storage, protocol and media failures remain diagnosable.
- Removed obsolete refresh-source trace parameters and historical debug markers from production code and regression tests.

## v2.75k66

- Integrated validated MAX17048 battery monitoring with standalone compass and LR01 GNSS software power control.
