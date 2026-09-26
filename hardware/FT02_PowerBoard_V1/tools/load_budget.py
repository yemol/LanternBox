"""Evidence inventory. Unknown bounds propagate; typical figures never rate a rail."""

import json
import math
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]


def entry(name, rail, typical, evidence, concurrency, startup=None, transient=None):
    return dict(name=name, rail=rail, typical=typical, continuous_max_A=None,
                startup_reference_A=startup, startup_max_A=None,
                transient_reference_A=transient, transient_max_A=None,
                peak_duration_ms=None, evidence=evidence, concurrency=concurrency)


def inventory():
    overlap = "No enforced exclusion demonstrated; simultaneous demand must be bounded"
    return [
        entry("ESP32-S3 Main Core", "3V3_MAIN", "Operating mode dependent; no board typical measured",
              "Espressif ESP32-S3 v2.2 table5-7:340mA peak at3.3V/25C,802.11b21dBm,100%TX duty; not a module/temperature maximum",
              overlap, transient=0.340),
        entry("EPD GDEQ0426T82 HAT", None, "Earlier official-source record:<35mW typical refresh, not startup bound",
              "hardware/FT-02/docs/FT02_hardware_research_new_chat_brief_v1_1.md; Waveshare4.26 HAT, rail/temperature/waveform need confirmation", overlap),
        entry("Audio Codec ES8311", None, "Manufacturer feature14mW playback+record; supply conditions not a board-current guarantee",
              "Everest ES8311 Rev7.0 via Espressif Audio_ES8311.pdf; WeAct carrier supply/losses OPEN", overlap),
        entry("Audio PA NS4150B", None, "UNKNOWN; speaker impedance, level/clipping and carrier rail missing",
              "FT02_AUDIO_ES8311_PA_WIRING.md: GPIO4 controls shutdown; no validated electrical current ceiling",
              "Current firmware playback-only policy is NOT hardware current admission; include fault/default-on case"),
        entry("GNSS MAX-M10S variant", None, "At3.0V:9.5mA VCC+2.3mA VIO tracking;11.5+2.3mA acquisition",
              "u-blox UBX-20035208 R08 table15;100mA startup supply recommendation; actual carrier/antenna excluded",
              "Alternative to F10S, not additive; other peripherals may overlap", startup=0.100),
        entry("GNSS MAX-F10S variant", None, "At3.0V:16mA VCC tracking,21mA acquisition,3mA VIO",
              "MAX-F10S UBXDOC-963802114-12732; carrier and antenna overhead OPEN",
              "Alternative to M10S, not additive; other peripherals may overlap", startup=0.100),
        entry("LoRa SX1262 TX", "3V3_MAIN", "UNKNOWN for selected RF front end,frequency and power setting",
              "Repository prototype is Heltec WiFi LoRa32V3; bare SX1262 headline is not whole carrier current",
              "Same radio TX/RX mutually exclusive; Core,SD,audio,EPD overlap not excluded"),
        entry("LoRa SX1262 RX", "3V3_MAIN", "Semtech headline4.6mA bare IC, mode dependent; not module maximum",
              "Semtech SX1262 official product page; TCXO/RF switch/carrier overhead excluded",
              "Same radio TX/RX mutually exclusive; other loads may overlap"),
        entry("Sensor BMI323/BMM350 SEN0695", "3V3_SENS", "UNKNOWN module current at selected ODR/modes",
              "DFRobot SEN0695 official schematic includes local1.8V regulation; do not add bare IC consumption twice", overlap),
        entry("CardKB2", None, "UNKNOWN; A2 POWER_TEST has no measured current",
              "Official CardKB2 manual; repo3.3V/5V supply conflict OI-021 remains OPEN", overlap),
        entry("SD card", "3V3_MAIN", "UNKNOWN; exact card manufacturer/model not frozen",
              "FT02_hardware_pin_map_v1_1.md:SDMMC1-bit5MHz; bus rate does not bound flash program pulses", overlap),
        entry("Fuel Gauge BQ27220", "BAT_GUARDED", "50uA NORMAL /9uA SLEEP typical",
              "TI BQ27220 RevA section6.5; upstream self-bias exception, not system rail consumption",
              "Always retained including HARD OFF; mode/communication duty affect current"),
        entry("Status circuits and pullups", "3V3_MAIN", "UNKNOWN complete active-state sum",
              "U1101 TCA9539,U902,and populated PG/IRQ dividers in exported netlist; bus pullups DNP; do not count IC IQ alone", overlap),
        entry("Other MainBoard peripherals", None, "UNKNOWN exact Core module,flash/PSRAM,USB/debug and indicator implementation",
              "Single-Core target differs from prototype; inventory and externally powered debug isolation OPEN", overlap),
        entry("Input/charger/protection/UPS housekeeping", None, "UNKNOWN complete sum; multiple upstream supply nodes",
              "U201/U401/U701 plus absent01/03/05; include charger battery leakage and all protection bias",
              "Retained during HARD OFF; report external/battery draw separately"),
        entry("Master switch bias/control", "UPS_RAW", "U1301 at5V:34uA enabled,5uA shutdown typical; control divider adds5/110k A when ON",
              "TPS22953 RevD sections7.5-7.8; 5A headline is NOT board current rating",
              "Upstream of gate but downstream of shunt in battery mode"),
        entry("Master raw bleeder R1303", "SYS_RAW", "5V/4.7k=1.064mA nominal at5V",
              "Actual R1303; final rail tolerances not frozen", overlap),
        entry("3V3 bleeder R1304", "3V3_MAIN", "3.3V/4.7k=0.702mA nominal",
              "Actual R1304; sensor branch discharges through ferrite", overlap),
        entry("5V bleeder R1305", "5V_MAIN", "5V/4.7k=1.064mA nominal",
              "Actual R1305; low-power tradeoff tracked, not decorative load", overlap),
    ]


def strict_sum(values):
    values = list(values)
    if not values or any(v is None for v in values):
        return None
    if any(not math.isfinite(v) or v < 0 for v in values):
        raise ValueError("Current/power bounds must be finite and nonnegative")
    return sum(values)


def report():
    rows = inventory()
    # Unknown rail assignments must prevent a falsely complete rail subtotal.
    rails = {}
    for rail in ("3V3_MAIN", "5V_MAIN"):
        selected = [r for r in rows if r["rail"] in (None, rail) or
                    (rail == "3V3_MAIN" and r["rail"] == "3V3_SENS")]
        rails[rail] = {"continuous_worst_A": strict_sum(r["continuous_max_A"] for r in selected),
                       "transient_worst_A": strict_sum(r["transient_max_A"] for r in selected),
                       "unresolved_contributors": [r["name"] for r in selected if r["continuous_max_A"] is None]}
    return dict(status="OPEN_NOT_A_BOARD_RATING", loads=rows, rails=rails,
                whole_board_concurrent_continuous_W=None, whole_board_concurrent_peak_W=None,
                whole_board_peak_duration_ms=None,
                variant_policy="Single-Core target; M10S/F10S alternatives and same-radio TX/RX are not summed together",
                deprecated_estimates="3V3 1.0A/1.5A and5V 1.2A/1.8A are not adopted bounds",
                release_gate="FAIL_LOAD_ENVELOPE_UNKNOWN")


if __name__ == "__main__":
    result = report()
    (BASE / "review/load_budget.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"status": result["status"], "loads": len(result["loads"]), "rails": result["rails"]}))
