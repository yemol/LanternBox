# PowerBoard Protocol - Core-side Contract Draft

No PowerBoard MCU or synthetic I2C register map is introduced. Core reads real peripheral registers and assembles a status structure. Every field carries validity, sample timestamp and source; stale/absent data must not silently become zero, FULL or BATTERY.

| Field | Intended evidence source | Outstanding issue |
| --- | --- | --- |
| ext_present, usb_present | Input detection and PD attach/VBUS observations | Define presence separately for both inputs |
| ext_valid | Hardware qualification output | Concrete validity circuit missing |
| usb_pd_active | HUSB238A contract status | Correct register map needed |
| usb_voltage_mv | HUSB238A VBUS measurement | Accuracy/update/availability validation |
| usb_current_capability_ma | PD contract or verified Type-C/legacy classification | Do not confuse requested with authorized current |
| system_source | UPS selected-channel output via qualified readout | EXT/BATTERY plus validity=false when unknown |
| charger_present | Qualified I2C communication/device identity | A bus ACK alone is not charge permission |
| charge_state, charge_fault | Charger status/fault register and charging enable | IDLE/CHARGING/FULL/FAULT; invalid flag separate |
| battery_soc_percent | BQ27220 SOC | No UI offset/clamp workaround |
| battery_remaining_mah, battery_full_charge_capacity_mah | BQ27220 capacities | Profile/learning validity |
| battery_design_capacity_mah | Qualified profile / gauge configuration | Must match installed pack |
| battery_voltage_mv, battery_current_ma, battery_avg_current_ma | BQ27220 measured telemetry | Verify sign with instrument; Core convention positive=charging |
| battery_temperature_c | Actual pack thermistor path reported through qualified source | Do not substitute IC junction temperature |
| battery_time_to_empty_min, battery_time_to_full_min | BQ27220 estimates | Invalid/not-applicable sentinel converted to validity=false |
| battery_soh, battery_cycle_count | BQ27220 | Meaning and persisted profile verified before UI use |
| battery_state | Measured current plus gauge and charger flags | CHARGING/DISCHARGING/RELAX/FULL/FAULT; uncertainty preserved |
| rail_3v3_good, rail_5v_good | Converter PG outputs via verified readout | Cannot be inferred merely from EN |

System-source selection and battery-current direction are different facts. Net charging current does not prove that an unintended battery-to-system branch is absent.

PWR_INT wired-OR is only a candidate. BQ27220 GPOUT is open drain but also participates in wake from shutdown; BQ25628E INT is an open-drain pulse. Prove voltage compatibility, reset polarity, pullups, wake behavior, event retention and independent interrupt clearing before joining them. Shared interrupts need polling/latching that cannot lose short events.

GPIO target is SDA/SCL (existing), PWR_INT, 5V_EN. Keep-alive, mux status, rail PG and charge permits need a concrete I2C-accessible control/monitor path without a new MCU. They do not magically become readable because the charger and gauge have I2C. Exact connector pins and extra IC choice remain open.

Keep-alive requests return accepted/inhibited with a reason. Pulse width and period are bounded by policy; hardware source gating and maximum on-time override firmware. Charge enable/current commands are clamped to the qualified pack, source and thermal limits.
