# Thermal Budget - Not Qualified

No enclosure ambient, copper stack, battery current limit, connector rating or measured efficiency map is frozen. No board temperature or allowable continuous power is claimed. Thermal shutdown is a fault response, not an acceptable normal operating point.

## Calculation Model

Use separate currents for charger branch, external mux, UPS and each converter. External mux carries system plus charger; UPS carries system only. Do not apply identical current to both mux losses during charging.

| Element | Loss model | Required corner evidence |
| --- | --- | --- |
| Charger | VBAT * Icharge * (1/eta_charge - 1) | Input/battery voltage, charge current, JEITA and enclosed ambient |
| External mux | Iext_rms^2 * Ron_ext + bias power | Hot maximum Ron; system plus charging |
| UPS mux | Iraw_rms^2 * Ron_ups + bias power | Low-battery Ron/current limit, thermal coupling |
| 3V3 converter | 3.3 * I3 * (1/eta3 - 1) | Buck, transition and boost; transient RMS |
| 5V converter | 5 * I5 * (1/eta5 - 1) | Lowest loaded input and maximum approved output |
| Shunt | Ibat_rms^2 * Rshunt | Pulse energy, continuous derating, TCR and Kelvin geometry |
| Protection FETs | I_rms^2 * sum(Rds_on_hot) | Gate drive at minimum battery; reverse/fault SOA |
| Inductors | I_L_rms^2 * DCR_hot + core loss | Ripple, saturation and manufacturer thermal model |
| Connectors/cables | I_rms^2 * contact_or_wire_resistance | Aging, loaded adjacent pins, crimp and insulation limits |
| Keep-alive | Vusb^2/R * duty | Resistor pulse rating, hot spot, gate-fault maximum duration |

Illustrative calculations, NOT ratings: a 56mohm typical path at 3A dissipates 0.504W; two such series paths carrying 3A dissipate 1.008W, excluding all other losses. Actual hot/low-voltage resistance is condition dependent. A 10mohm shunt at 5A dissipates 0.25W; at 8A it reaches 0.64W and the BQ27220 nominal 80mV sense boundary, with no tolerance margin.

At an assumed 90% efficiency, 3V3/2A loses 0.733W and 5V/2A loses 1.111W. These are examples, not efficiency guarantees or simultaneous-load permission. Source, mux, battery and enclosure constraints must all pass first.

## Scenario Matrix

| Scenario | Special checks |
| --- | --- |
| Charging | Charger/external mux, battery surface NTC, termination and thermal regulation |
| 3V3 full approved load | U801, inductor and SYS_RAW loss across battery range |
| 5V full approved load | LT3120 candidate, inductor, input current at low battery |
| USB 5V/2A | Cable/contact loss; residual charging within authorized total input |
| USB/EXT 5V/3A | Both muxes, connectors and enclosure coupling |
| Battery mode | Battery lead/PCM/shunt/UPS/FET path and converters |
| LoRa TX | Burst RMS heating plus droop; peak is not a DC thermal load |
| Worst allowed combination | Explicit concurrency from POWER_BUDGET, not assumed16.6W simultaneous rating |

Manufacturer theta-JA applies to its stated PCB; it does not directly predict this enclosure. Establish ambient range, measured temperatures and junction margin. Validate component ratings and battery temperature separately. Test at least one hour at each limiting corner and until temperature slope stabilizes. Abort on any qualified temperature/current limit, rail instability or repeated thermal throttling. Numeric abort limits remain blocked by OI-002/003/015/016.

Sources: [TPS2121](https://www.ti.com/lit/ds/symlink/tps2121.pdf), [TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf), [LT3120](https://www.analog.com/media/en/technical-documentation/data-sheets/lt3120.pdf), [BQ27220](https://www.ti.com/lit/ds/symlink/bq27220.pdf).
