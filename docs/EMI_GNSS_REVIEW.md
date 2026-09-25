# EMI / GNSS Review - Open

Layout and validation constraints only; no EMC or GNSS performance pass. No PCB or RF test has been performed.

| Aggressor | Evidence / decision | Verification required |
| --- | --- | --- |
| BQ25628E | Nominal1.5MHz candidate | Reference hot-loop layout; charging disabled/enabled/load sweep |
| TPS63802 | Variable switching behavior, power-save versus PWM | Spectrum in buck/boost/transition/light load, not a single fixed tone |
| LT3120 | Programmable400kHz-2MHz | Frequency not selected; trade inductance, loss and receiver interference |
| Inductor/SW nodes | Three switching stages may operate together | Minimize hot loops/SW area, reference layouts and return paths |
| 3V3_SENS | Filter from3V3_MAIN | Ferrite DC bias/DCR, bulk capacitance, damping and load-step ringing |
| Keep-alive | Optional pulsed input current | Source/cable ringing and rail/GNSS disturbance |

Evaluate L1 and future L5 in the actual antenna/enclosure arrangement. A switching fundamental far below GNSS bands does not eliminate harmonic/broadband coupling. Burst spectra and edge slew matter; no arbitrary frequency-notch or compliance claim.

Maintain uninterrupted ground return reference. Do not split ground into islands that divert sensitive returns. Put GNSS filtering near the module. Keep inductors, SW nodes, high-current loops and charger away from antenna/feed and BMM350. Separation distances await mechanical testing, not an invented number.

Test M10S and F10S separately: log C/N0 per band/satellite, satellites used, fix continuity and acquisition/reacquisition against a controlled quiet baseline. Repeat battery/external/charging, both rails, power-save/PWM, Wi-Fi, LoRa TX, SD, audio and keep-alive. Record antenna, sky/simulator conditions and configuration. Acceptance delta must come from navigation requirements; none is currently frozen.

Sources: [BQ25628E](https://www.ti.com/lit/ds/symlink/bq25628e.pdf), [TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf), [LT3120](https://www.analog.com/media/en/technical-documentation/data-sheets/lt3120.pdf), [MAX-F10S](https://content.u-blox.com/sites/default/files/documents/MAX-F10S_DataSheet_UBXDOC-963802114-12732.pdf).
