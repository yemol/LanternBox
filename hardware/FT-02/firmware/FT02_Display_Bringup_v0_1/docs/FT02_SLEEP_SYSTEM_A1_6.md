# FT-02 Mode & Sleep System A1.6

## Status

A1.6 is the frozen functional sleep baseline for FT-02 v2.75k91. The previous autonomous sleep exit/reset issue is closed for the current architecture.

## Mode policy

- **Indoor**: automatic sleep may be configured to 5 / 15 / 30 minutes / Never.
- **Outdoor**: no automatic sleep; manual long-P sleep remains available.
- **Sleep**: preserves the previous awake mode so wake can restore the correct runtime policy.

## Enter-sleep lifecycle

1. Save the return page and clear the Home P-gesture state.
2. Block sleep while active audio, location recording, GNSS field test, compass calibration or pending map refresh requires the device to stay awake.
3. Release idle audio owners and shut down active Wi-Fi ownership.
4. Apply sleep mode policy through `FT02_ModeEnterSleep()`.
5. Draw the static e-paper sleep frame, cut the EPD power rail and isolate display signals.
6. Reduce the Core clock to 80 MHz.
7. Put input diagnostics into low-noise mode.
8. Pause LR01 host ingestion and MAX17048 background sampling.
9. Mark the sleep session in the persistent health breadcrumb.
10. Enter periodic 1000 ms Light Sleep slices.

## Sleep service

The current CardKB2 connection is four-wire I2C and has no dedicated wake GPIO. The Core therefore wakes once per second, polls CardKB2 once, and immediately returns to Light Sleep if no plausible key event exists. Normal GNSS/LoRa/map/storage/audio/battery background work does not run inside this loop.

A wake gate requires stable release after the long-P entry gesture, which prevents immediate bounce-back into the UI. Invalid or unrelated CardKB2 bytes do not tear down Sleep.

## Wake lifecycle

1. Disable the periodic timer wake source.
2. Restore the pre-sleep CPU frequency.
3. Recreate native USB CDC.
4. Restore normal input mode.
5. Resume LR01 host ingestion and MAX17048 sampling.
6. Restore EPD signals, panel power and SPI, then reinitialize full display mode.
7. Call `FT02_ModeWake()` and refresh GNSS/battery status.
8. Restore the configured return page or Home.
9. Reacquire page-owned audio only when the destination page needs it.
10. Redraw the page and return to normal runtime.

## Retained production diagnostics

`FT02_SleepDiagnostics` stores only a small NVS breadcrumb used by Device Status:

- last normal keyboard wake,
- sleep session count,
- keyboard wake count,
- reset-during-sleep count,
- last reset reason if setup starts while a sleep session was still marked active.

Temporary RTC blackboxes, isolation switches, GNSS wake probes and per-phase RCA traces used during development are not part of the production baseline.

## Verified behavior

Field testing established the following on the current hardware baseline:

- long-duration sleep remains stable without autonomous return to Home,
- manual CardKB2 wake exits sleep normally,
- repeated Sleep → Wake → Sleep cycles remain stable,
- native USB serial reconnects after wake,
- LR01 host handling does not reintroduce the previous sleep reset,
- battery telemetry remains available after wake,
- outdoor GNSS data resumes after wake, observed through changing satellite/navigation data,
- audio remains usable after wake and continues to follow on-demand ownership.

## Deferred work

This closeout intentionally does **not** optimize total sleep current. LoRa and GNSS are still controlled through LR01, and current LR01 firmware does not provide the final Core-owned low-power model planned for the product. Deep power work, including CardKB2 event wake and direct LoRa/GNSS power ownership, should be reopened only after those functions migrate back to Core.
