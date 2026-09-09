# FT-02 v2.75k67

FT-02 production firmware for ESP32-S3. This baseline integrates the validated battery, compass, GNSS, Wi-Fi, reader, audio, map, Field Library, FieldManual and LR01 host functions.

## Current hardware/runtime functions

- MAX17048 battery SOC / voltage / charge-rate monitoring
- LR01 Host Protocol A2
- GNSS software standby / wake control with persistent user preference
- Standalone compass UI and calibration
- LoRa communication and node status
- Wi-Fi module with explicit lifecycle ownership
- Offline map, map search, location recording and location logs
- Reader / PDF / EPUB flow
- ES8311 recording and playback
- Field Library v0.8.2 and FieldManual runtime
- CardKB2 input and offline Pinyin IME

## v2.75k67 fixes

- System Self-Test no longer duplicates a single historical UART error into two persistent warnings.
- Host UART health is based on current link/protocol state; communication warnings use new error deltas and high cumulative thresholds.
- LR01 A2 can be identified from A2-only responses even when the startup banner was missed.
- GNSS OFF is treated as an intentional user state in Self-Test, map and location recorder flows.
- Low and critical battery warnings remain at 20% and 10%.

## Build

```bash
rm -rf .pio
platformio run
```

Upload example:

```bash
platformio run --target upload --upload-port /dev/cu.usbmodem2201
```

Use the serial port currently assigned by the host OS.

## Regression checks

```bash
python3 tools/test_system_health_a2.py
python3 tools/test_function_surface_audit.py
python3 tools/test_battery_monitor.py
python3 tools/test_compass_gnss_control.py
python3 tools/test_audio_regression.py
python3 tools/test_input_epd_regression.py
python3 tools/test_field_library_v082.py
python3 tools/test_pdf_recovery.py --builder tools/build_ft02_field_library.py
```

## LR01 compatibility

GNSS software power control and the 180-degree compass installation correction require the matching LR01 A4.3 firmware.

Hardware pin and protocol details are documented under `docs/`.
