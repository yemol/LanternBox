# PowerBoard Interface - Logical Draft

No connector pin numbers are frozen. Assigning them without current/mating/enclosure data would create a false contract.

| Net | Direction relative to PowerBoard | Rule |
| --- | --- | --- |
| 3V3_MAIN | Power out | Future MainBoard regulated supply, not DevKit5V input |
| 5V_MAIN | Power out | Switchable peripheral rail; current envelope applies |
| GND | Common reference | Contacts/copper based on total current; no sense bypass |
| SDA | Bidirectional open drain | Existing Core GPIO47,3.3V domain,100kHz |
| SCL | Core clock, slave stretching possible | Existing GPIO21,100kHz; BQ27220 stretching support |
| PWR_INT | Open-drain candidate output | Core GPIO TBD; wired-OR not approved yet |
| 5V_EN | Core input | Core GPIO TBD; reset-safe default OFF required |
| EXT_VALID / SOURCE_STATUS | Optional monitored signal | Prefer proven I2C readout; no silent GPIO addition |

Target: reuse two I2C pins plus PWR_INT/5V_EN, four total. This is a budget, not an implemented interface. GPIO4 is occupied by the current audio PA. No new MCU. Concrete I2C expander/monitor and hardware gating remain OI-013.

Evaluate pullups in parallel across actual boards, rise time with total capacitance, sink current, powered-off leakage and clock stretching. Avoid back-power via SDA/SCL/INT/EN when boards/rails are off. USB/EXT/BAT are not3.3V logic supplies.

External connectors: USB-C5V sink only; EXT regulated5V only; BAT1S polarity/NTC contacts TBD. Do not infer wire gauge or pinout from capacity. Debug leads must not bypass shunt/protection. Do not parallel outputs with the existing DevKit regulator/USB source; future MainBoard and present bring-up adapter need separately reviewed power-entry isolation.
