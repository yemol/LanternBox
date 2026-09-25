# Bring-up Plan - All Tests NOT RUN

Future sample plan, not permission to energize the incomplete reference project. Start with current-limited supply and a battery simulator capable of absorbing charge; do not back-drive an ordinary supply. Real pack testing requires OI-002 safety/NTC/polarity closure. Fault injection needs qualified personnel, protection and rated instruments.

## Common Setup

Scope input, SYS_RAW,3V3 at Core, EXT_VALID/source status and branch currents. Log reset reason/uptime/PG. Measure USB total, charger branch and UPS battery branch independently; net gauge current cannot prove no supplement. Differential/isolated shunt probes must not bypass protection/sense through grounded leads.

Record BOM/firmware/battery profile, ambient, cable/source, concurrency, samples and raw traces. Before PASS, freeze numeric rail bounds, transient duration, temperature/current limits, measurement uncertainty and current deadbands. Unspecified acceptance is OPEN. Every transfer requires no Core brownout/reset, rails within qualified bounds, no reverse feed, and zero battery-to-system contribution whenever EXT_VALID is asserted, within specified measurement uncertainty.

| Test | Stimulus | Required observation / acceptance |
| --- | --- | --- |
| 01 Battery only boot | Qualified full-to-low range | Safe boot/UVLO, no overcurrent, stable rails |
| 02 USB only | Battery absent; each source class | Boot within authorized limit, no negotiation deadlock |
| 03 EXT_5V only | Battery absent, regulated5V | Stable operation and safe insufficient-source response |
| 04 USB + Battery | Low/mid/full battery | USB priority, allowed charging, zero UPS battery feed while valid |
| 05 EXT + Battery | Same battery points | EXT priority and charge controls |
| 06 USB removal | Approved peak load | Hardware takeover, both branches traced, no reset |
| 07 USB reinsertion | Both orientations/contact bounce | Qualify then battery exits on VALID, no overshoot/reset |
| 08 EXT removal | With/without USB | Other valid external source first, otherwise BAT |
| 09 Weak source | Current limit/resistance/load sweep | Charge reduction first, then INVALID, no rebound chatter |
| 10 No supplement | Valid source plus permitted load step | UPS BAT path off within declared measurement uncertainty |
| 11 PD negotiation | 5V2A/3A/multi-voltage/reset | 5V-only cold start/request, authorized total current |
| 12 USB-A to C | DCP/CDP/SDP | Correct classification and explicit support limits |
| 13 Low-load auto-off | Full battery, lowest system load | Timeout, diagnostic evidence and UPS capture |
| 14 Keep-alive | Pulse sweep and stuck-control fault | Qualified-bank retention, OFF/gating and maximum pulse bound |
| 15 Battery100% | Verified charge termination | Correct100% without offset; no charge cycling fault |
| 16 Low SOC | Approach frozen cutoff | Safe policy, battery path within limits |
| 17 Remaining mAh | Controlled reference coulomb cycle | Qualified profile/learning; error and temperature logged |
| 18 Charge current | Approved sweep | Sign/magnitude versus reference and charge ceiling |
| 19 Discharge current | Approved load sweep | Signed current, shunt drop/temperature |
| 20 Charging state | Start/stop/NTC/termination | Measured current plus flags; stale invalid |
| 21 Discharging state | BAT with varying load | Current direction and source consistent |
| 22 Idle/relax | No intentional battery transfer | Qualified deadband/timing, not voltage guessing |
| 23 3V3 load step | Defined edge at input corners | Core droop/overshoot/settling within frozen bounds |
| 24 5V load step | Step, enable/disable | Regulation/inrush/discharge, no3V3 disturbance |
| 25 Both rails | Allowed concurrent loads | Envelope enforced, charging shed; no16.6W on10/15W source |
| 26 Wi-Fi peak | Actual TX/settings | Rail/PG/reset and source current traces |
| 27 LoRa22dBm | Approved RF setup/duty | Rail and GNSS effects; compliant RF test conditions |
| 28 SD write | Supported card variants | Sustained/burst, no errors/reset |
| 29 EPD refresh | Actual panel/cold waveform | Refresh spike and stability |
| 30 Audio playback | Actual PA/speaker/approved level | Crest/startup, ripple/noise/temperature |
| 31 Audio recording | PA off, codec + SD | No supply noise/dropped writes |
| 32 GNSS | M10S and F10S startup/acquisition | Supply margin,C/N0/fix vs baseline |
| 33 Combined FT02 | Worst credible concurrency | Envelope and no-supplement invariant |
| 34 Thermal soak | >=1hour and stable temperature | Battery/component/connector qualified limits |
| 35 Battery idle | Rails off,3V3 only, system sleep | Whole-pack current incl. housekeeping |
| 36 External + full | Sleep/awake,5V off/on | Total current and no repeated charge cycles |
| 37 5V disabled | Otherwise identical workload | Measured delta, leakage/reverse path/discharge |
| 38 Bank matrix | Source/cable/orientation/SOC combinations | Completed log retaining failures |

Additional safety cases: output shorts; NTC open/short/hot/cold; battery absence; charger reset/watchdog; stuck/off-state I2C; input OVP with rated equipment; reverse polarity strategy before injection; USB/EXT backfeed; sensor-filter ringing; GPIO stuck/reset; protection-variant assembly error. Never short or overcharge an unqualified real lithium cell for these tests.
