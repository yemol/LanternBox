# Battery Specification Evidence

OI-002 BLOCKING: current frozen electrical limits were not found in inspected project records.

| Field | Current pack | Evidence |
| --- | --- | --- |
| Chemistry / series | Li-ion or LiPo / 1S | User task brief |
| Nominal / full voltage | 3.7V / 4.2V | Brief and existing monitor docs |
| Capacity | 5000mAh | Brief |
| Manufacturer / model / revision | UNKNOWN | Frozen record required |
| Charge current and regulation tolerance | UNKNOWN | No C-rate inference |
| Continuous discharge | UNKNOWN | Blocks battery power rating |
| Peak discharge / duration / repetition | UNKNOWN | Blocks shunt/FET/mux qualification |
| Charge / discharge temperature limits | UNKNOWN | Blocks NTC/JEITA limits |
| Loaded minimum voltage / impedance | UNKNOWN | Blocks UPS operating margin |
| PCM OVP/UVP/OCP/SCP/temperature | UNKNOWN | Need thresholds, tolerance, delay, recovery |
| NTC R25 / Beta / tolerance / wiring | UNKNOWN | External pack-surface interface planned |
| Connector / polarity / wire gauge / length | UNKNOWN | Blocks drop/fuse/contact rating |

Future: 1S 3.7V, at least 10000mAh; same PCB capacity goal, but only within a qualified common current/thermal/connector envelope. Old 10000mAh/1260100 electrical limits are excluded.

Search: hardware and hardware documentation, related FT02 firmware notes, text/JSON/CSV backup material, and battery-monitor commit 30e4cc7. Software SOC warning thresholds are not cell protection thresholds.
