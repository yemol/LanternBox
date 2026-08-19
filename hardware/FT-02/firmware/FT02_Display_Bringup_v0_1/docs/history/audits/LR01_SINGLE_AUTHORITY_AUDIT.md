# FT02 v2.75a LR01 Single Authority Audit

Architecture lock:

- Core GPIO7/13 UART is owned only by `FT02_LR01HostRuntime`.
- Core has no GNSS UART, NMEA parser, GNSS baud probing, or satellite-sentence parser.
- Core has no Compass I2C/QMC driver. Heading/quality come only from LR01 `NAV_STATE`.
- Core has no SX126x control, Meshtastic protobuf transport, want_config/full-sync, or radio reset GPIO control.
- GPIO6 LR01 reset is not driven by Core production code.
- Radio state, nodes, RX/TX lifecycle and delivery ACK enter through LR01 ASCII Host Protocol A2 only.

Compatibility names such as `FT02_GnssSnapshot` and `FT02_LoRaCommunicationRuntime` remain at the application/UI layer to avoid destabilizing maps, recorders and inbox/outbox. They are state/cache APIs only and contain no direct peripheral ownership.
