"""Explicit calculation examples, not FT02 ratings or converter simulation."""

import json
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]


def main():
    rt, rb = 511000, 91000
    low = 0.5*0.99*(1+rt*0.99/(rb*1.01))
    high = 0.5*1.01*(1+rt*1.01/(rb*0.99))
    data = {
        "status": "ILLUSTRATIVE_NOT_A_RATING",
        "feedback": {
            "source": "TPS63802 Rev D sections8.5/10.2.2.5 and Table10-6",
            "nominal_v": 0.5*(1+rt/rb),
            "pwm_reference_and_resistor_only_min_v": low,
            "pwm_reference_and_resistor_only_max_v": high,
            "additional_100na_bias_times_max_rtop_v": 100e-9*rt*1.01,
            "divider_current_nominal_ua": 0.5/rb*1e6,
            "caveat": "Not full rail bounds. PWM reference accuracy does not qualify PFM ripple; bias, temperature drift, regulation, PCB drop and transients remain.",
        },
        "dual_rail_example": {"output_w": 3.3*2+5*2,
            "input_a_at_assumed_3v_90pct": (3.3*2+5*2)/(3*0.9)},
        "mux_example": {"ron_typ_ohm": 0.056, "assumed_a": 3,
            "one_path_w": 3**2*0.056, "two_same_current_paths_w": 2*3**2*0.056},
        "hold_up_example": {"assumed_w": 5, "assumed_gap_us": 100,
            "assumed_start_v": 4.5, "assumed_end_v": 3.0,
            "required_effective_uf": 2*5*100e-6/(4.5**2-3.0**2)*1e6,
            "caveat": "Assumed gap/voltages; not guaranteed timing, battery limits or selected capacitance."},
        "shunt_examples": [{"assumed_a": a, "candidate_ohm": 0.01,
            "drop_mv": a*10, "loss_w": a*a*0.01,
            "pack_boundary": "At source maximum, not a board rating" if a == 5 else "FORBIDDEN: exceeds source5A; no pulse allowance"}
            for a in [5, 8]],
        "source_backed_pack": {"model": "955565 / 955465", "nominal_v": 3.8,
            "capacity_mah": 5000, "standard_charge_a": 2.5, "maximum_charge_a": 5,
            "maximum_discharge_a": 5, "discharge_cutoff_v": 2.75,
            "charge_voltage_status": "OPEN: source CC to4.25V then CV4.2V",
            "loaded_minimum_v": None, "charge_temperature_c": None,
            "above_1c_peak_a": None, "above_1c_duration_s": None},
        "shunt_reference_corner": {"mpn": "WSK2512R0100FEA", "nominal_ohm": 0.01,
            "tolerance_fraction": 0.01, "full_resistor_tcr_ppm_per_c": 35,
            "assumed_resistor_delta_c": 45, "pack_ceiling_a": 5,
            "additive_upper_ohm": 0.01*(1+0.01+35e-6*45),
            "drop_mv": 5*0.01*(1+0.01+35e-6*45)*1000,
            "loss_w": 25*0.01*(1+0.01+35e-6*45),
            "caveat": "45C element delta is an example, not bounded by pack ambient. No offset/ageing/fault/pulse/PCB thermal qualification."},
        "keepalive_examples": [{"assumed_ma": ma, "r_ohm": 5000/ma,
            "pulse_w": 5*ma/1000, "average_mw_at_100ms_per_10s": 5*ma*0.1/10}
            for ma in [100, 200]],
    }
    (BASE / "review/calculation_examples.json").write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps(data, indent=2))


if __name__ == "__main__":
    main()
