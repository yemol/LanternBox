"""Independent physical-pin contracts; deliberately does not import the generator.

Sources: BQ25628E Rev C pin table/application, LT3120 TA07/pin functions,
TPS2121 Rev F pin table, TPS3808 Rev N DBV pin table, SN74LVC1G08 DBV
pin table and TCA9539 Rev C PW pin table. Values are WIP review choices.
"""

IC_NETS = {
    "U401": {18: "EXT_BUS_5V", 17: "CHG_PMID", 2: "CHG_REGN", 4: "CHG_ILIM",
             5: "CHG_TS_BIAS", 6: "CHG_TS", 7: None, 14: "CHG_CE_N", 15: "GND",
             1: "CHG_BTST", 16: "CHG_SW", 9: "CHG_SYS_LOCAL", 8: "BAT_PROTECTED",
             3: "CHG_PG_N", 10: "CHG_STAT_N", 11: "CHG_INT_N", 12: "SDA", 13: "SCL"},
    "Q401": {1: "CHG_REQUEST", 2: "GND", 3: "CHG_CE_DRAIN"},
    "U701": {7: "EXT_BUS_5V", 2: "BAT_PROTECTED", 6: "MUX_EXT_SENSE", 5: "GND",
             4: "MUX_EXT_SENSE", 3: "GND", 1: "SYS_RAW", 8: "SYS_RAW",
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
}

TWO_TERMINAL = [
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
    ("C703", "220uF 10V REF", "SYS_RAW", "GND"),
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
]

MPNS = {"U401": "BQ25628ERYKR", "Q401": "2N7002,215", "U701": "TPS2121RUXR",
        "U702": "TPS3808G01DBVR", "U901": "LT3120IUFDM#PBF", "U902": "SN74LVC1G08DBVR",
        "U1101": "TCA9539PWR", "FB1001": "BLM21PG221SN1D"}


def candidate_operating_limits():
    # TPS3808G01 Rev N: +/-2% threshold, +/-25nA sense bias; R705/R706 +/-0.1%.
    top, bottom, tol = 715000, 100000, 0.001
    low = 0.405 * 0.98 * (1 + top*(1-tol)/(bottom*(1+tol))) - 25e-9*top*(1+tol)
    high = 0.405 * 1.02 * (1 + top*(1+tol)/(bottom*(1-tol))) + 25e-9*top*(1+tol)
    return {
        "decision": "TPS2121_UNFROZEN_NOT_A_BOARD_RATING",
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
                       "final auxiliary paths in absent sheets01/02/03/05",
                       "IC internal leakage/reverse paths in all operating and fault states",
                       "physical four-terminal footprint and trace connections"],
    }
