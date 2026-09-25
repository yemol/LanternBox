# Power Bank Compatibility - Unexecuted Matrix

No tested-compatible product list exists. HUSB238A is a candidate, not compatibility proof. USB input is5V only; detection and total input clamp must agree before increasing load. See USB_PD_POLICY and OI-008/009/020.

| Source | Cable / behavior | Admission rule |
| --- | --- | --- |
| USB-C PD5V/3A | C-to-C, both orientations | Accepted5V contract only; no9-20V request at reset/reattach |
| USB-C PD5V/2A | C-to-C | Actual accepted capability, not hardcoded2A |
| USB-C non-PD | Rp default/1.5A/3A | Detected capability; unknown/default is not2A permission |
| USB-A DCP/CDP | A-to-C, BC1.2 | Verified class/current policy, no guessed vendor protocol |
| USB-A SDP | A-to-C, no enumeration implemented | No assumed enumerated high current; explicit reduced-load/unsupported boot policy |
| Low-load auto-off | Battery full/system asleep | Keep-alive OFF baseline then bounded pulses; log energy cost |
| Weak source/cable | Added resistance/current limit | Reduce charging first; INVALID then BAT when system alone cannot be sustained |
| Both inputs | Healthy EXT, weak preferred USB | Prefer valid source, not attachment; no chatter |

Log manufacturer/model/revision, firmware if known, bank SOC, port, cable length/rating, orientation, advertised/accepted capabilities, total current limit, measured peak/RMS, boot, transfer waveform, low-load timeout, pulse settings/loss, thermal outcome and failure reason. All results NOT RUN.

## Keep-Alive

Default OFF, DNP-capable. Branch uses isolated USB power, inside accounted total source current; hardware USB-selected/valid gating and maximum on-time required. Battery/EXT cannot feed it. Reset, sleep or stuck GPIO must not produce continuous dummy load.

Illustration at5V:100mA gives50ohm/0.5W pulse;200mA gives25ohm/1W pulse. At100ms every10s, average resistor power is5mW/10mW. Not final resistor/timing selections. Use manufacturer pulse-energy curves, FET conduction/switching loss and hot-spot calculation. Pulse plus all other loads must stay inside authorized current.

Hooks: `powerbank_keepalive_enable(bool)` and `powerbank_keepalive_pulse()`. Reject with reason when DNP, USB invalid/not selected, inadequate budget or thermal limit. No PowerBoard MCU.

## Diagnostics

Record contract rejection, low-current advertisement, measured droop, current-limit event, invalid transition and source loss separately. Cable drop requires evidence. Bank shutoff and unplug can be electrically indistinguishable: report `SOURCE_LOSS_UNKNOWN` absent independent attachment evidence. Diagnostic confidence does not change autonomous UPS safety.
