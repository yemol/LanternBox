"""Conditional V2 architecture arithmetic; never an ERC or hardware safety pass."""

import json
import math
from pathlib import Path


def system_power(p3, p5, efficiency=0.85, auxiliary=0.1):
    if min(p3, p5, auxiliary) < 0 or not 0 < efficiency <= 1:
        raise ValueError("Invalid power or efficiency")
    return (p3 + p5) / efficiency + auxiliary


def battery_current(power, voltage, resistance):
    if power < 0 or voltage <= 0 or resistance < 0:
        raise ValueError("Invalid battery operating point")
    if resistance == 0:
        return power / voltage
    discriminant = voltage**2 - 4 * resistance * power
    if discriminant < 0:
        raise ValueError("No steady-state solution for this path")
    # Stable lower-current root of I * (V - I*R) = P.
    return 2 * power / (voltage + math.sqrt(discriminant))


def charge_headroom(p3, p5, vin=4.75, input_limit=2.4, vsys=3.4,
                    vbat=3.8, charger_efficiency=0.90, set_current=677 / 372):
    if min(vin, vsys, vbat) <= 0 or min(input_limit, set_current) < 0:
        raise ValueError("Invalid source/current")
    if not 0 < charger_efficiency <= 1:
        raise ValueError("Invalid charger efficiency")
    power = system_power(p3, p5)
    input_margin = vin * input_limit * charger_efficiency - power
    sw_margin = 3.2 * 0.9 - power / vsys
    return {
        "source_can_sustain_system": input_margin >= 0,
        "system_fits_sw_budget": sw_margin >= 0,
        "allowed_charge_a": max(0, min(set_current, sw_margin,
                                       input_margin / vbat)),
        "input_power_margin_w": input_margin,
        "sw_current_margin_a": sw_margin,
        "implemented_current_policy": False,
    }


def shared_load_allowed(i3, i5):
    return (0 <= i3 <= 2 and 0 <= i5 <= 1.5
            and 3.3 * i3 + 5 * i5 <= 8 + 1e-12)


def report():
    power = system_power(6.6, 1.4)
    ibat = battery_current(power, 3, 0.1)
    charge_min = 639 / (372 * 1.01)
    charge_max = 715 / (372 * 0.99)
    capacity_checks = []
    for capacity in (5000, 10000):
        capacity_checks.append({
            "capacity_mah": capacity,
            "path_design_target_a": 5,
            "future_pack_rating_inferred": False,
            "bq25628_por_ideal_cc_hours": capacity / 320,
            "bq25616_ideal_cc_hours_at_min_setting": capacity / 1000 / charge_min,
            "bq25616_min_timer_remaining_hours": 8 - capacity / 1000 / charge_min,
            "real_full_charge_timer_pass": None,
        })
    return {
        "status": "ARCHITECTURE_PROPOSAL_NOT_RELEASED",
        "assumptions_not_guarantees": {
            "aggregate_output_w": 8, "rail_efficiency": 0.85,
            "usb_mode_loaded_sys_floor_v": 3.4,
            "loaded_pack_example_v": 3, "path_resistance_ohm": 0.1,
            "auxiliary_sys_w": 0.1, "sw_current_reserve_fraction": 0.1,
        },
        "individual_rail_sum_w": 3.3 * 2 + 5 * 1.5,
        "simultaneous_full_rails_admitted": shared_load_allowed(2, 1.5),
        "usb_nominal_input_w": 15,
        "usb_low_voltage_input_w": 4.75 * 3,
        "eight_w_example": {
            "system_input_w": power, "system_current_a": power / 3.4,
            "battery_current_a": ibat, "converter_input_v": 3 - ibat * 0.1,
            "below_current_pack_5a_ceiling_in_model": ibat <= 5,
            "fixed_charge_plus_system_a": power / 3.4 + 677 / 372,
            "fixed_charge_plus_system_pass": power / 3.4 + 677 / 372 <= 3.2,
            "with_3v3_2a_remaining_5v_a": (8 - 6.6) / 5,
            "with_5v_1p5a_remaining_3v3_a": (8 - 7.5) / 3.3,
            "charge_headroom": charge_headroom(6.6, 1.4),
        },
        "charge_setting_a": {"min": charge_min, "typ": 677 / 372,
                               "max": charge_max},
        "capacities": capacity_checks,
        "shunt": {"ohm": 0.005, "tolerance": 0.01,
                  "sense_at_5a_max_v": 5 * 0.005 * 1.01,
                  "dissipation_at_5a_max_w": 25 * 0.005 * 1.01,
                  "gauge_full_scale_v": 0.0512,
                  "current_lsb_a": 0.0003125, "capacity_lsb_mah": 1,
                  "netlist_bypass_proven": False},
        "standby": {
            "condition": "Battery only, physical OFF, no external IO backfeed",
            "allocations_ua": {"charger": 15, "gauge_hibernate": 12,
                               "gate": 11, "uv": 2, "pack_pcm": 10,
                               "reverse_and_io": 5, "controls": 2},
            "allocated_hibernate_ua": 57, "allocated_active_ua": 75,
            "qualified_maximum_ua": None, "requirement_pass": None,
            "annual_mah_at_50ua": 0.050 * 24 * 365,
            "annual_mah_at_100ua": 0.100 * 24 * 365,
        },
        "v2_schematic_implemented": False, "v2_erc_errors": None,
        "phase1_pass": False, "pcb_authorized": False,
    }


if __name__ == "__main__":
    output = Path(__file__).resolve().parents[1] / "review" / "power_budget.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report(), indent=2) + "\n", encoding="ascii")
    print(output)
