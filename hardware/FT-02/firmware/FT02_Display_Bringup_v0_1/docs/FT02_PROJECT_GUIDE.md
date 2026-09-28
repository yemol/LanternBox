# LanternBox FT-02 Project Guide

**Current firmware baseline:** FT-02 v2.75k91 / Mode Sleep System A1.6  
**Target:** ESP32-S3 N16R8

This file is the main maintenance entry point for the FT-02 repository. Historical per-build patch notes and temporary diagnostic reports are intentionally not kept as current documentation; version history belongs in the root `CHANGELOG.md`.

## 1. Architecture

FT-02 is the LanternBox field terminal. The current hardware remains a two-controller design:

### Core ESP32-S3

Owns:

- 800×480 e-paper UI
- CardKB2 input / local Pinyin IME
- SD storage
- FieldManual / Field Library
- offline PBF map and search
- location and audio journals
- Wi-Fi sync / Core portal
- system settings, self-test and sleep lifecycle

### LR01 side

Currently supplies the Core with LoRa/GNSS/navigation/compass related host telemetry over UART. LoRa and GNSS are planned to return to direct Core ownership in a later hardware/software phase. Do not redesign deep-sleep power ownership around the current LR01 protocol before that migration.

Current host UART runtime:

```text
Core GPIO13 TX -> LR01 RX
Core GPIO7  RX <- LR01 TX
115200 8N1, 3.3 V logic
```

See `LR01_HOST_PROTOCOL_A2.md` and `FT02_LR01_HOST_INTERFACE_PIN_LOCK.md`.

## 2. Main hardware interfaces

### CardKB2 / shared I2C

```text
SDA = GPIO47
SCL = GPIO21
CardKB2 = 0x5F
MAX17048 = 0x36
```

The current CardKB2 installation remains four-wire I2C. Sleep wake therefore uses periodic Light Sleep slices and I2C polling rather than a dedicated keyboard wake GPIO.

### E-paper

```text
PWR  = GPIO18
BUSY = GPIO3
RST  = GPIO8
DC   = GPIO9
CS   = GPIO10
MOSI = GPIO11
SCK  = GPIO12
SPI  = 4 MHz
```

### Audio

```text
BCLK = GPIO14
WS   = GPIO15
DOUT = GPIO16
DIN  = GPIO17
PA SD/CTRL = GPIO4
```

Audio follows on-demand ownership: codec/I2S/PA are not kept fully active when no feature owns them.

## 3. Operating modes

- **Indoor**: GNSS desired OFF; automatic sleep can be 5 / 15 / 30 minutes / Never.
- **Outdoor**: GNSS desired ON; no automatic sleep.
- **Sleep**: preserves the previous awake mode and restores it on wake.

Home P gesture:

- short P: Indoor ↔ Outdoor
- long P: enter Sleep

See `FT02_SLEEP_SYSTEM_A1_6.md` for the frozen lifecycle.

## 4. Build and upload

```bash
rm -rf .pio
platformio run
```

Upload:

```bash
platformio run --target upload --upload-port /dev/cu.usbmodemXXXX
```

Monitor:

```bash
platformio device monitor --port /dev/cu.usbmodemXXXX
```

Always use the device path currently assigned by the host OS. Native USB CDC disconnect/re-enumeration during FT-02 Light Sleep is expected.

## 5. Repository layout

```text
src/                 firmware source
boards/              PlatformIO board definitions
tools/               maintained content/build/regression tools
docs/                long-lived hardware/protocol/project documents
SD_ROOT/              full SD layout baseline
SD_UPDATE_v0.8.2/     Field Library update payload
platformio.ini        build entry
VERSION.txt           repository firmware version
CHANGELOG.md          version history
```

## 6. Validation before commit

Run at least:

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

Then run `platformio run` on the development machine before flashing or tagging a release.

## 7. Current frozen boundaries

### Sleep A1.6

The prior autonomous sleep exit/reset issue is closed. Repeated Sleep → Wake → Sleep, CardKB2 wake, USB recovery, LR01 host resume, battery resume and GNSS runtime recovery have been field-verified on the current architecture.

### Deep power optimization

Not part of this baseline. Current sleep current is not considered final. Deep LoRa/GNSS power control and a future event-driven CardKB2 wake path are deferred.

### LR01 GNSS power compatibility

The host retains compatibility commands/state parsing, but current deployed LR01 behavior does not provide a reliable proof of GNSS standby. Treat mode-level GNSS power control as provisional until LoRa/GNSS return to Core ownership.

## 8. Documentation rule

Do not add one Markdown file for every temporary experiment.

- permanent behavior/design → update the relevant document in `docs/`
- version change → update `CHANGELOG.md`
- temporary investigation → keep it out of the production repository once the issue is closed
