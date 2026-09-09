# LanternBox FT-02 LR01 Firmware

Production baseline: **LR01 A4.3 / Host Protocol 2**.

LR01 provides LoRa/Meshtastic transport, GNSS, compass and persistent GNSS fault
recording for FT-02. FT-02 communicates with LR01 through `HOST_PROTOCOL_A2`.

## Production features

- SX1262 LoRa/Meshtastic transport with channel encryption and PKI direct messaging.
- u-blox M10 GNSS via NMEA + UBX monitoring.
- Conservative GNSS receive-path recovery and receiver acquisition watchdog.
- Persistent 48-record GNSS blackbox for field diagnosis.
- Software GNSS standby/wake: `GNSS_SOFT_OFF`, `GNSS_SOFT_ON`, `GNSS_POWER?`.
- QMC compass calibration with persistent NVS data.
- 180-degree compass installation yaw correction before heading is published.
- Host Protocol 2 remains the FT-02-facing protocol.

GNSS software standby is not a physical VCC cut. While standby is active LR01 does
not poll or write the GNSS UART, preventing accidental UART-RX wakeups.

## Build

```bash
platformio run
```

Upload example:

```bash
platformio run --target upload --upload-port /dev/cu.usbserial-0001
```

The verified PlatformIO environment is `espressif32@7.0.1` with
`heltec_wifi_lora_32_V3`.

## Secrets

`include/Ft01Secrets.h` contains local channel material and is ignored by Git.
`include/Ft01Secrets.example.h` is safe to commit. If `Ft01Secrets.h` is already
tracked in an existing repository, remove it from the Git index before publishing.

## Diagnostics

Normal packet-flow traces are disabled in production. Error/recovery/startup-health
messages and explicit console diagnostic commands remain available. GNSS blackbox
commands include `GNSS_LOG_STATUS?`, `GNSS_LOG?`, `GNSS_LOG_CLEAR`, and
`GNSS_LOG_FLUSH`.

## Documentation

- `docs/HOST_PROTOCOL_A2.md`
- `docs/HARDWARE.md`
- `docs/COMPASS_CALIBRATION.md`
- `docs/ACCEPTANCE_TESTS.md`
- `docs/TROUBLESHOOTING.md`
