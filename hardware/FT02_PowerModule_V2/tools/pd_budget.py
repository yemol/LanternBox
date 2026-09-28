"""Conditional PD circuit calculations. Assumptions are not hardware qualifications."""
import json
from pathlib import Path


def parallel(a,b):
    return a*b/(a+b)


def calculate():
    rmin = parallel(47000*.99,2430*.99)
    rmax = parallel(47000*1.01,2430*1.01+10)
    imin, inom, imax = 3000/rmax,3333/parallel(47000,2430),3666/rmin
    pout, eta_rail, aux = 8,.85,.1
    psys = pout/eta_rail+aux
    # Loaded voltage and efficiency are qualification targets, not no-load register values.
    available = 8.3*imin*.90
    sw_upper = 9.45*imax/3.4
    uv_nom = .405*(1+2.87e6/402000)
    uv_min = .405*.98*(1+2.87e6*.999/(402000*1.001))-25e-9*2.87e6*1.001
    uv_max = .405*1.02*(1+2.87e6*1.001/(402000*.999))+25e-9*2.87e6*1.001
    # Board SYS undervoltage is not a vendor minimum loaded pack specification.
    sys_current = psys/uv_min
    loss_shunt_5a = 25*.005*1.01
    en_min = uv_min*(4.7e6*.99)/(470e3*1.01+4.7e6*.99) - .4e-6*(470e3*1.01)
    return {
        "status":"CONDITIONAL_ARITHMETIC_NOT_HARDWARE_PASS", "phase1_pass":False,
        "assumptions":{"rail_and_gate_efficiency_min":eta_rail,"charger_efficiency_min":.90,
            "usb_loaded_sys_floor_v":3.4,"usb_charger_input_floor_v":8.3,"usb_input_max_v":9.45,
            "nm_os_on_resistance_max_ohm":10,"sys_auxiliary_w":aux,
            "input_auxiliary_a":.020,"full_temperature_limits_qualified":False},
        "charger_input_a":{"min":imin,"nominal":inom,"max":imax},
        "fallback_estimated_a":{"min":3000/(47000*1.01),"max":3666/(47000*.99),
            "qualified":False,"note":"KILIM specified near 1.5A; sub-100mA operation/startup not established."},
        "output_envelope_w":pout,"sys_demand_w":psys,"low_corner_sys_available_w":available,
        "low_corner_margin_w":available-psys,
        "nominal_charge_headroom_a_at_4v2":max(0,(9*inom*.90-psys)/4.2),
        "sw_current_power_upper_bound_a":sw_upper,"sw_limit_a":5,
        "sw_margin_fraction":1-sw_upper/5,
        "usb_2a_contract_margin_a":2-imax-.020,
        "system_uv_v":{"nominal":uv_nom,"min":uv_min,"max":uv_max},
        "sys_current_at_uv_min_a":sys_current,"gate_enable_min_estimate_v":en_min,
        "battery_limit_a":5,"battery_path_resistance_max_ohm_assumed":.1,
        "shunt_dissipation_at_5a_w":loss_shunt_5a,
        "off_budget_ua":{"charger":3,"gauge_hibernate":12,"gate_bias_and_input":11,
            "supervisor_allocation":6,"divider_and_control_allocation":2,
            "pcm_unknown_allocation":10,"reverse_and_io_unknown_allocation":5,
            "normal_total":49,"uv_asserted_additional_max_estimate":uv_max/(470000*.99)*1e6,
            "active_gauge_extra":18,"guaranteed":False,
            "note":"Table-point bounds and allocations mixed. Gauge >50C and actual PCM leakage remain OPEN."},
        "checks":{"low_corner_8w_arithmetic":available>=psys,"steady_sw_below_5a":sw_upper<5,
            "input_below_2a_contract":imax+.020<2,"steady_sys_below_pack_5a":sys_current<5,
            "shunt_below_half_w":loss_shunt_5a<.5,"enable_above_1v3_design_guard":en_min>1.3},
        "unproven":["5V fallback current accuracy and dead-battery startup", "PD hard-reset/downgrade",
            "thermal and component temperature limits", "8W actual terminal concurrency",
            "surge clamp coordination and current trip overshoot", "inrush/ripple/short circuit below pack ceiling",
            "charge profile/TS/10000mAh timeout", "battery-only OFF at low battery and full temperature"],
    }


if __name__ == "__main__":
    result=calculate()
    (Path(__file__).resolve().parents[1]/"review/pd_budget.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2))
