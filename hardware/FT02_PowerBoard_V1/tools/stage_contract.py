"""Independent physical-pin contracts; deliberately does not import the generator.

Sources: BQ25628E Rev C pin table/application, LT3120 TA07/pin functions,
TPS2121 Rev F pin table, TPS3808 Rev N DBV pin table, SN74LVC1G08 DBV,
TPS25947 Rev C RPW and TCA9539 Rev C PW pin tables. Values are WIP review choices.
"""

IC_NETS = {
    "U201": {5: "EXT_5V", 1: "EXT_UV_SENSE", 2: "EXT_OV_SENSE", 9: "EXT_ILM",
             8: "GND", 6: "EXT_5V_SAFE", 3: None, 4: "GND", 7: "EXT_DVDT", 10: None},
    "J201": {1: "EXT_5V", 2: "GND"},
    "U401": {18: "EXT_BUS_5V", 17: "CHG_PMID", 2: "CHG_REGN", 4: "CHG_ILIM",
             5: "CHG_TS_BIAS", 6: "CHG_TS", 7: None, 14: "CHG_CE_N", 15: "GND",
             1: "CHG_BTST", 16: "CHG_SW", 9: "CHG_SYS_LOCAL", 8: "BAT_PROTECTED",
             3: "CHG_PG_N", 10: "CHG_STAT_N", 11: "CHG_INT_N", 12: "SDA", 13: "SCL"},
    "Q401": {1: "CHG_REQUEST", 2: "GND", 3: "CHG_CE_DRAIN"},
    "U701": {7: "EXT_BUS_5V", 2: "BAT_PROTECTED", 6: "MUX_EXT_SENSE", 5: "GND",
             4: "MUX_EXT_SENSE", 3: "GND", 1: "UPS_RAW", 8: "UPS_RAW",
             9: "MUX_BAT_STATUS", 10: "MUX_ILIM", 11: "MUX_SS", 12: "GND"},
    "U702": {6: "SYS_RAW", 5: "SYS_UV_SENSE", 3: "SYS_RAW", 1: "RAIL_PERMIT", 4: None, 2: "GND"},
    "U901": {1: "REG5_BST3", 2: "5V_MAIN", 3: "GND", 4: "5V_MAIN", 5: "5V_MAIN",
             6: "5V_MAIN", 7: "PG_5V", 8: "REG5_VCC", 9: "FB_5V", 10: "REG5_VC",
             11: "GND", 12: "REG5_VCC", 13: "REG5_VCC", 14: "REG5_RT", 15: "GND",
             16: "REG5_ENABLE", 17: "SYS_RAW", 18: "GND", 19: "SYS_RAW", 20: "SYS_RAW",
             21: "SYS_RAW", 22: "GND", 23: "REG5_BST1", 24: "REG5_SW1", 25: "REG5_SW1",
             26: "REG5_SW2", 27: "REG5_SW2", 28: "REG5_BST2", 29: "GND"},
    "U902": {1: "RAIL_PERMIT", 2: "5V_EN", 3: "GND", 4: "REG5_ENABLE", 5: "3V3_MAIN"},
    "U1101": {1: "PWR_INT", 2: "GND", 3: "IO_RESET_N", 4: "PG_3V3", 5: "PG_5V",
              6: "FG_GPOUT", 7: "CHG_PG_N", 8: "CHG_STAT_N", 9: "CHG_INT_N",
              10: "CHG_REQUEST", 11: "MUX_BAT_STATUS", 12: "GND", 13: "IO_UNUSED10", 14: "IO_UNUSED11",
              15: "IO_UNUSED12", 16: "IO_UNUSED13", 17: "IO_UNUSED14", 18: "IO_UNUSED15",
              19: "IO_UNUSED16", 20: "IO_UNUSED17", 21: "GND",
              22: "SCL", 23: "SDA", 24: "3V3_MAIN"},
    "J1101": {1: "GND", 2: "GND", 3: "3V3_MAIN", 4: "3V3_MAIN", 5: "5V_MAIN", 6: "5V_MAIN",
              7: "SDA", 8: "SCL", 9: "PWR_INT", 10: "5V_EN", 11: "3V3_SENS", 12: "GND"},
    "J1001": {1: "3V3_SENS", 2: "GND"},
    "U1301": {1: "UPS_RAW", 2: "UPS_RAW", 3: "UPS_RAW", 4: "MASTER_EN", 5: "GND",
              6: "MASTER_CT", 7: "GND", 8: "SYS_RAW", 9: "SYS_RAW", 10: "SYS_RAW", 11: "GND"},
    "J1301": {1: "PWR_SW_ON", 2: "MASTER_EN", 3: "GND"},
}

TWO_TERMINAL = [
    ("C201", "1uF 50V", "EXT_5V", "GND"),
    ("C202", "22uF 10V", "EXT_5V_SAFE", "GND"),
    ("C203", "4.7nF 50V REF", "EXT_DVDT", "GND"),
    ("R201", "562k 0.1%", "EXT_5V", "EXT_UV_SENSE"),
    ("R202", "205k 0.1%", "EXT_UV_SENSE", "GND"),
    ("R203", "562k 0.1%", "EXT_5V", "EXT_OV_SENSE"),
    ("R204", "165k 0.1%", "EXT_OV_SENSE", "GND"),
    ("R205", "1.65k 1% REF", "EXT_ILM", "GND"),
    ("C401", "1uF 10V", "EXT_BUS_5V", "GND"),
    ("C402", "10uF 10V", "CHG_PMID", "GND"),
    ("C403", "100nF 10V", "CHG_PMID", "GND"),
    ("C404", "47nF 10V", "CHG_BTST", "CHG_SW"),
    ("C405", "4.7uF 10V", "CHG_REGN", "GND"),
    ("L401", "1uH REF", "CHG_SW", "CHG_SYS_LOCAL"),
    ("C406", "22uF 10V", "CHG_SYS_LOCAL", "GND"),
    ("C407", "10uF 10V", "BAT_PROTECTED", "GND"),
    ("R401", "2.80k 1%", "CHG_ILIM", "GND"),
    ("R402", "100k 1%", "EXT_BUS_5V", "CHG_CE_N"),
    ("R403", "10k 1%", "3V3_MAIN", "CHG_PG_N"),
    ("R404", "10k 1%", "3V3_MAIN", "CHG_STAT_N"),
    ("R405", "10k 1%", "3V3_MAIN", "CHG_INT_N"),
    ("R406", "100k 1%", "CHG_REQUEST", "GND"),
    ("R407", "0 ARM DNP", "CHG_CE_N", "CHG_CE_DRAIN"),
    ("R408", "0 TS LOCK", "CHG_TS", "GND"),
    ("C408", "100nF 10V", "CHG_TS_BIAS", "GND"),
    ("C701", "1uF 10V", "EXT_BUS_5V", "GND"),
    ("C702", "1uF 10V", "BAT_PROTECTED", "GND"),
    ("C703", "220uF 10V REF", "UPS_RAW", "GND"),
    ("R701", "30.9k 0.1%", "EXT_BUS_5V", "MUX_EXT_SENSE"),
    ("R702", "10k 0.1%", "MUX_EXT_SENSE", "GND"),
    ("R703", "29.8k 1% REF", "MUX_ILIM", "GND"),
    ("C704", "1nF REF", "MUX_SS", "GND"),
    ("R704", "100k 1%", "3V3_MAIN", "MUX_BAT_STATUS"),
    ("C705", "100nF 10V", "SYS_RAW", "GND"),
    ("R705", "715k 0.1%", "SYS_RAW", "SYS_UV_SENSE"),
    ("R706", "100k 0.1%", "SYS_UV_SENSE", "GND"),
    ("R707", "10k 1%", "SYS_RAW", "RAIL_PERMIT"),
    ("L901", "4.7uH REF", "REG5_SW1", "REG5_SW2"),
    ("C901", "10uF 10V", "SYS_RAW", "GND"),
    ("C902", "220uF 10V", "5V_MAIN", "GND"),
    ("C903", "220nF 10V", "REG5_BST1", "REG5_SW1"),
    ("C904", "220nF 10V", "REG5_BST2", "REG5_SW2"),
    ("C905", "22nF 10V", "REG5_BST3", "5V_MAIN"),
    ("C906", "4.7uF 10V", "REG5_VCC", "GND"),
    ("R901", "536k 1%", "5V_MAIN", "FB_5V"),
    ("R902", "102k 1%", "FB_5V", "GND"),
    ("R903", "158k 1%", "REG5_RT", "GND"),
    ("R904", "46.4k 1%", "REG5_VC", "REG5_COMP"),
    ("C907", "1800pF C0G", "REG5_COMP", "GND"),
    ("R905", "100k 1%", "5V_EN", "GND"),
    ("R906", "100k 1%", "3V3_MAIN", "PG_5V"),
    ("R907", "10k 1%", "REG5_ENABLE", "GND"),
    ("C908", "100nF 10V", "3V3_MAIN", "GND"),
    ("FB1001", "220R at100MHz", "3V3_MAIN", "3V3_SENS"),
    ("C1001", "100nF 10V", "3V3_SENS", "GND"),
    ("C1002", "1uF 10V", "3V3_SENS", "GND"),
    ("R1001", "1R 1%", "3V3_SENS", "SENS_DAMP"),
    ("C1003", "10uF 10V", "SENS_DAMP", "GND"),
    ("R1101", "10k 1%", "3V3_MAIN", "PWR_INT"),
    ("R1102", "10k 1%", "3V3_MAIN", "IO_RESET_N"),
    ("C1101", "100nF 10V", "3V3_MAIN", "GND"),
    ("C1102", "1uF 10V", "3V3_MAIN", "GND"),
    ("R1103", "100k 1%", "IO_UNUSED10", "GND"),
    ("R1104", "100k 1%", "IO_UNUSED11", "GND"),
    ("R1105", "100k 1%", "IO_UNUSED12", "GND"),
    ("R1106", "100k 1%", "IO_UNUSED13", "GND"),
    ("R1107", "100k 1%", "IO_UNUSED14", "GND"),
    ("R1108", "100k 1%", "IO_UNUSED15", "GND"),
    ("R1109", "100k 1%", "IO_UNUSED16", "GND"),
    ("R1110", "100k 1%", "IO_UNUSED17", "GND"),
    ("R1301", "10k 1%", "UPS_RAW", "PWR_SW_ON"),
    ("R1302", "100k 1%", "MASTER_EN", "GND"),
    ("C1301", "1uF 10V", "UPS_RAW", "GND"),
    ("C1302", "1uF 10V", "SYS_RAW", "GND"),
    ("C1303", "10nF 50V REF", "MASTER_CT", "GND"),
    ("R1303", "4.7k 1%", "SYS_RAW", "GND"),
    ("R1304", "4.7k 1%", "3V3_MAIN", "GND"),
    ("R1305", "4.7k 1%", "5V_MAIN", "GND"),
]

MPNS = {"U201": "TPS259474LRPWR", "U401": "BQ25628ERYKR", "Q401": "2N7002,215", "U701": "TPS2121RUXR",
        "U702": "TPS3808G01DBVR", "U901": "LT3120IUFDM#PBF", "U902": "SN74LVC1G08DBVR",
        "U1101": "TCA9539PWR", "FB1001": "BLM21PG221SN1D", "U1301": "TPS22953DQCR"}


def audit_master_gate(actual):
    """Exact live-terminal boundaries, not an analog/off-state certification."""
    boundaries = {
        "UPS_RAW": {("U701", "1"), ("U701", "8"), ("C703", "1"),
                    ("U1301", "1"), ("U1301", "2"), ("U1301", "3"),
                    ("C1301", "1"), ("R1301", "1")},
        "MASTER_EN": {("U1301", "4"), ("J1301", "2"), ("R1302", "1")},
        "PWR_SW_ON": {("R1301", "2"), ("J1301", "1")},
        "SYS_RAW": {("U1301", "8"), ("U1301", "9"), ("U1301", "10"), ("C1302", "1"),
                    ("R1303", "1"), ("U702", "6"), ("U702", "3"), ("C705", "1"),
                    ("R705", "1"), ("R707", "1"), ("U801", "10"), ("C801", "1"),
                    ("U901", "17"), ("U901", "19"), ("U901", "20"), ("U901", "21"),
                    ("C901", "1"), ("TP1207", "1")},
    }
    for net, expected in boundaries.items():
        found = {pin for pin, name in actual.items() if name == net}
        if found != expected:
            raise ValueError(f"Unreviewed master-gate boundary at {net}: {found ^ expected}")
    return {
        "status": "WIRED_BOUNDARY_ONLY_HARD_OFF_NOT_QUALIFIED",
        "series_path": "U701 -> UPS_RAW -> U1301 -> SYS_RAW -> U801/U901 -> system rails",
        "switch": "maintained SPDT: J1301 common2 to1 ON, to3 OFF; open defaults OFF",
        "boundary_nodes": {n: sorted(':'.join(p) for p in pins) for n, pins in boundaries.items()},
        "retained": ["charger input EXT_BUS_5V", "charger BAT_PROTECTED", "gauge BAT_GUARDED"],
        "charging": "DISABLED by R407 DNP and TS lock; future OFF-state charging not qualified",
        "not_proven": ["all analog off-state leakage and discharge times",
                       "MainBoard/debug/interface injection and on-state reverse current",
                       "switch contact bounce/cable ESD, startup and final load/thermal margins"],
    }


def candidate_operating_limits():
    # TPS3808G01 Rev N: +/-2% threshold, +/-25nA sense bias; R705/R706 +/-0.1%.
    top, bottom, tol = 715000, 100000, 0.001
    low = 0.405 * 0.98 * (1 + top*(1-tol)/(bottom*(1+tol))) - 25e-9*top*(1+tol)
    high = 0.405 * 1.02 * (1 + top*(1+tol)/(bottom*(1-tol))) + 25e-9*top*(1+tol)
    return {
        "decision": "TPS2121_UNFROZEN_NOT_A_BOARD_RATING",
        "ext_guard_candidate": ext_guard_limits(),
        "master_gate_candidate": master_gate_limits(),
        "required_continuous_board_power_W": None,
        "required_peak_board_power_W": None,
        "required_peak_duration_ms": None,
        "system_uvlo_nominal_V": 0.405*(1+top/bottom),
        "system_uvlo_static_bound_V": [low, high],
        "system_uvlo_boundary": "SYS_RAW only; not loaded pack floor; excludes noise/propagation/rebound",
        "mux_operating_minimum_input_V": 2.8,
        "mux_current_limit_table_A": [3, 3.5, 4],
        "mux_current_limit_boundary": "At exact29.8k; resistor tolerance and transient response additional",
        "mux_transition_typical_us": 100,
        "mux_transition_guaranteed_max_us": None,
        "mux_ron_guaranteed_max_at_battery_input_ohm": None,
        "stress_example_not_requirement": {
            "rails_W": 16.6, "assumed_efficiency": 0.9,
            "raw_current_at_static_uvlo_low_A": 16.6 / 0.9 / low,
            "conclusion": "Exceeds pack5A and chosen limiter minimum; simultaneous2A+2A NOT approved",
        },
        "hold_up_example_not_guarantee": {
            "assumed_current_A": 3, "nominal_C_uF": 220, "assumed_time_us": 100,
            "capacitive_droop_V": 3*100/220,
            "conclusion": "1.36V droop before ESR/derating; nominal capacitor does NOT prove no brownout",
        },
    }


def ext_guard_limits():
    def bounds(bottom, threshold_min, threshold_max):
        top, tol, bias = 562000, 0.001, 0.1e-6
        return [threshold_min*(1+top*(1-tol)/(bottom*(1+tol)))-bias*top*(1+tol),
                threshold_max*(1+top*(1+tol)/(bottom*(1-tol)))+bias*top*(1+tol)]
    return {
        "decision": "UNQUALIFIED_NOT_A_FROZEN_INPUT_SPEC",
        "ov_trip_static_V": bounds(165000, 1.183, 1.223),
        "uv_start_static_V": bounds(205000, 1.183, 1.223),
        "uv_stop_static_V": bounds(205000, 1.076, 1.116),
        "assumed_reverse_test_V": -5.5,
        "control_pin_reverse_current_upper_uA": 5.5/(562000*0.999)*1e6,
        "breaker_table_A_at_exact_1k65": [1.8, 2.028, 2.2],
        "not_proven": ["full5V+/-5% source window; low OV corner may reject5.25V",
                       "hotplug overshoot/inductive negative excursion/TVS energy",
                       "load-current headroom, inrush and thermal limits",
                       "weak-source rebound inhibit and source-wide charge shedding"],
    }


def master_gate_limits():
    # Selected datasheet corners, NOT interpolation or a full-board leakage bound.
    return {
        "decision": "UNFROZEN_LOAD_AND_OFF_STATE_QUALIFICATION_OPEN",
        "bias_operating_V": [2.5, 5.7],
        "input_operating_V": "0.7 <= VIN <= VBIAS; IN and BIAS tied UPS_RAW",
        "datasheet_continuous_switch_ceiling_A_at_70C": 5,
        "board_required_current_A": None,
        "ron_max_ohm_at_2V5_bias_input_105C": 0.027,
        "ron_max_ohm_at_3V3_bias_input_105C": 0.025,
        "ron_max_ohm_at_5V_bias_input_105C": 0.024,
        "thermal_example_not_rating": {
            "assumed_current_A": 3, "selected_ron_ohm": 0.027,
            "loss_W": 3**2*0.027, "drop_V": 3*0.027,
            "datasheet_DQC_theta_JA_C_per_W": 65.2,
            "illustrative_rise_C": 3**2*0.027*65.2,
            "warning": "2.5V table corner/example only; not enclosure thermal proof or an interpolated guarantee",
        },
        "off_EN_open_upper_V_at_100nA_bias": 100e-9*100000*1.01,
        "off_EN_falling_threshold_min_V": 0.560,
        "reverse_blocking": "Disabled only;10us typical response, no maximum;11uA max at105C for OUT5V/IN0V/EN0V",
        "internal_output_discharge": False,
        "bleeder_nominal_power_W_at_raw5_3V3_5V": (5**2+3.3**2+5**2)/4700,
        "five_volt_discharge_example_not_guarantee": {
            "assumed_C_uF": 220, "R_ohm": 4700, "start_V": 5, "end_V": 0.1,
            "zero_injection_time_s": 4700*220e-6*3.912023005428146,
            "warning": "About4s, not instant OFF; excludes tolerance, extra capacitance and all injection",
        },
        "off_all_rails_zero_proven": False,
        "remaining": ["full-interval leakage, I2C/debug injection and rail decay specification",
                      "mechanical contacts/cable ESD and bounce",
                      "source-startup, CT ramp, effective ceramic capacitance and current headroom"],
    }


def expectations():
    pins = {(ref, str(pin)): net for ref, mapping in IC_NETS.items() for pin, net in mapping.items()}
    values = {}
    for ref, value, a, b in TWO_TERMINAL:
        pins[(ref, "1")], pins[(ref, "2")] = a, b
        values[ref] = value
    return pins, values


def audit_current_boundary(actual):
    """Prove only the installed sheet-06 boundary, not the absent pack/guard chain."""
    boundaries = {
        "BAT_GUARDED": {("R601", "I1"), ("U601", "C3"), ("C601", "1")},
        "BAT_PROTECTED": {("R601", "I2"), ("U401", "8"), ("U701", "2"), ("C407", "1"), ("C702", "1")},
        "SENSE_P": {("R601", "E1"), ("U601", "C1"), ("TP1213", "1")},
        "SENSE_N": {("R601", "E2"), ("U601", "C2"), ("TP1214", "1")},
        "CHG_SYS_LOCAL": {("U401", "9"), ("L401", "2"), ("C406", "1")},
    }
    for net, expected in boundaries.items():
        found = {pin for pin, name in actual.items() if name == net}
        if found != expected:
            raise ValueError(f"Unreviewed current boundary at {net}: {found ^ expected}")
    return {
        "status": "PARTIAL_BOUNDARY_ONLY_OI_010_OPEN",
        "boundary_nodes": {net: sorted(':'.join(p) for p in pins) for net, pins in boundaries.items()},
        "charge_terminal": "U401:8 -> BAT_PROTECTED -> R601:I2/I1 -> BAT_GUARDED",
        "discharge_terminal": "BAT_GUARDED -> R601:I1/I2 -> BAT_PROTECTED -> U701:2",
        "upstream_exceptions": ["U601:C3 gauge supply", "C601:1 decoupling charging/discharging current",
                                "U601:C1/C2 sense-input bias and test-probe leakage via Kelvin terminals"],
        "not_proven": ["BAT_PLUS/BAT_MINUS to board guard (sheet05 absent)",
                       "final auxiliary paths in absent sheets01/03/05 and candidate sheet02 fault states",
                       "IC internal leakage/reverse paths in all operating and fault states",
                       "physical four-terminal footprint and trace connections"],
    }
