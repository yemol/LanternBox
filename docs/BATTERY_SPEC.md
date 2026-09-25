# Battery Specification Evidence

Updated2026-09-25. Canonical PowerBoard record; OI-002 is PARTIALLY RESOLVED, not closed. Source is the user-supplied transcription [FT02_BATTERY_SPEC_955565_955465.md](FT02_BATTERY_SPEC_955565_955465.md) and the user's explicit adoption instruction. The underlying product image/vendor qualification has not been independently authenticated here. The source transcription is retained unchanged.

Adopted source-record SHA-256: `409e5458b481c0d98fdc5805732ae342b2d3e72082190b7b517fd8355301663e`. This identifies the local transcription, not vendor authentication.

## Source-Backed Fields

| Field | Adopted current-pack specification | Boundary |
| --- | --- | --- |
| Model | 955565 / 955465, shared specification | No substitution with1260100 |
| Chemistry / series | Polymer Li-ion / 1S | Series count retained from FT02 requirements |
| Capacity | 5000mAh | Not a basis for inventing other C-rates |
| Nominal voltage | 3.8V,3.7V compatible | Source bottom table also says3.7V; nominal is not charge voltage |
| Dimensions / weight | 9.5 x55 x65mm /76g | Supplied pack specification, not PowerBoard height |
| Standard charge | 0.5C =2.5A | Explicit source rating, not an inferred allowance |
| Fast / maximum charge | 1C =5A / maximum5A | Ceiling, not required PowerBoard charge current |
| Maximum discharge | 1C =5A | Design ceiling; no >5A burst allowance or complete current-vs-temperature curve |
| Discharge cutoff | 2.75V | Not an exact PCM threshold or guaranteed minimum loaded pack output |
| Discharge temperature | -20C to+60C | Must not be reused as charge-temperature range |
| Pack protection present | Overcharge,overdischarge,overcurrent,short circuit,overtemperature | Presence only; thresholds/delays/recovery are unknown |
| Source termination statement | <=0.05C =<=250mA | Part of unresolved charge-method statement, not an approved charger setting |

## Charge Voltage Remains OPEN

Source states **CC to4.25V, then CV at4.2V**. Preserve both numbers; do not normalize this to a4.2V or4.25V approved profile. The old brief's4.2V full-charge statement does not resolve the new source contradiction. Do not infer a4.35V high-voltage cell from3.8V nominal. Final setpoint, tolerance and complete charge algorithm remain unapproved; charging stays inhibited.

## Still OPEN

Charge-temperature range; exact protection thresholds/tolerance/delay/recovery; NTC lead presence/R25/Beta/tolerance/wiring; connector/polarity; wire gauge/length; minimum loaded pack-output voltage and impedance; peak discharge above1C and allowed duration/repetition. Manufacturer identity and full original specification revision are not established by the model string.

Future at least10000mAh remains a separate1S compatibility target without PCB redesign, subject to a common qualified current/thermal/connector envelope and correct gauge profile. Do not double5A when capacity doubles. Current5000mAh data are not replaced by future/old pack data.

The prior source-search blocker is resolved only for the fields above. Board current rating, safe charging, UPS low-voltage margin and full battery-path verification remain OPEN.
