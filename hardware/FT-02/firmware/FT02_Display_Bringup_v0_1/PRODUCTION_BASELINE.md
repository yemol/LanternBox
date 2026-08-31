# FT-02 v2.75k57 Production Baseline Freeze

Date: 2026-08-31

## Freeze rule

`v2.75k57` is the functional baseline. Production cleanup may remove documentation history, comments, unreachable development code and generated caches, but must not change runtime behavior or hardware assignments.

## Hardware/runtime invariants

- CardKB2: SDA47 / SCL21 / 0x5F
- LR01 UART: RX7 / TX13
- SD: FSPI Mode3 40 MHz, SCK42 / MOSI2 / MISO1 / CS41
- ES8311: BCLK14 / WS15 / DOUT16 / DIN17, AMIC, MCLK NC
- NS4150B SD/CTRL: GPIO4
- Audio: 44.1 kHz, 16-bit, stereo I2S slots, mono WAV payload
- EPD generic partial hot-idle: 350 ms
- Field Library Reader refresh: x=0,y=86,w=800,h=394

## Audio invariants

- PSRAM clean capture/playback retained.
- Playback Full-TX retained; no whole-block rewind retry.
- Playback elapsed timer remains disabled.
- Playback UI displays total duration only.
- Volume-safe EPD refresh only on explicit user volume adjustment.
- Stop path remains 46.4 ms fade → DAC soft mute → PA off → DAC power-down.
- Volume gain table: 0.10 / 0.18 / 0.30 / 0.46 / 0.60 / 0.75 / 0.90 / 1.10 / 1.35 / 1.60.

## Cleanup performed

- Removed superseded per-version AUDIT/REPORT files.
- Removed obsolete Field Library release notes and historical test-plan archive.
- Consolidated production documentation.
- Removed version-history comments from runtime source.
- Removed the abandoned automatic playback quiet-window scanner and its private constants/helper; it had no runtime callers. The explicit PA-off EPD guard used by volume refresh remains.
- Removed cache/build artifacts and added `.gitignore`.

## Integrity gate

Before commit/release:

1. Run audio, input/EPD, Field Library and PDF recovery regressions.
2. Verify FieldManual manifest CRC and media CRC.
3. Rebuild root SHA256/CRC32 manifests.
4. Read back both checksum manifests.
5. Verify ZIP with `unzip -t`.
