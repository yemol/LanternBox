# FT-02 Firmware

Current production baseline: **v2.75k91 / Mode Sleep System A1.6**.

FT-02 is the field terminal firmware for the LanternBox platform, targeting ESP32-S3 N16R8 with e-paper UI, CardKB2 input, MAX17048 battery telemetry, LR01-hosted LoRa/GNSS, offline map/field manual content, Wi-Fi sync, audio journal and local utility pages.

## Current sleep baseline

- Indoor mode supports 5 / 15 / 30 minute / Never automatic sleep.
- Outdoor mode is manual-sleep only.
- Manual long-P sleep remains available in both awake modes.
- Sleep uses 1000 ms ESP32-S3 Light Sleep slices because the current four-wire CardKB2 I2C design has no dedicated wake GPIO.
- Normal background services do not run during sleep; the periodic wake path only polls CardKB2 and re-enters Light Sleep when no valid key is present.
- E-paper power is removed while preserving the retained sleep image; display signals are isolated and restored on wake.
- Native USB CDC is recreated during wake so repeated Sleep → Wake → Sleep cycles remain usable from the serial monitor.
- LR01 host ingestion and MAX17048 background sampling are paused before sleep and resumed on wake.
- Wi-Fi is shut down before sleep and is not implicitly re-enabled after wake.
- A lightweight NVS breadcrumb is retained for Device Status so a reset that occurs during sleep can still be identified later.

The sleep stability issue that previously caused autonomous exit/reset has been closed on this baseline. Deep power optimization of LoRa/GNSS is intentionally deferred until those functions migrate from LR01 back to the Core. See `docs/FT02_SLEEP_SYSTEM_A1_6.md`.

## Build

```bash
rm -rf .pio
platformio run
```

Upload example:

```bash
platformio run --target upload --upload-port /dev/cu.usbmodemXXXX
```

Use the serial port currently assigned by the host OS.

## Validation

Run the maintained static/regression checks:

```bash
python3 tools/test_system_health_a2.py
python3 tools/test_function_surface_audit.py
python3 tools/test_battery_monitor.py
python3 tools/test_compass_gnss_control.py
python3 tools/test_audio_power_lifecycle.py
python3 tools/test_audio_regression.py
python3 tools/test_input_epd_regression.py
python3 tools/test_mode_sleep_a1_6.py
python3 tools/test_field_library_v082.py
python3 tools/test_low_power_a2.py
python3 tools/test_pdf_recovery.py --builder tools/build_ft02_field_library.py
```

## Documentation

Project and hardware documentation lives under `docs/`. Start with `docs/README.md` and `docs/FT02_PROJECT_GUIDE.md`.

## Known boundary

LoRa and GNSS are still mediated by LR01 in this baseline. LR01 deep-sleep/GNSS power-control protocol work is not part of Sleep A1.6 closeout and will be revisited when LoRa/GNSS return to Core ownership.
