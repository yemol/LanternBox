"""Incorrect reference exports must be rejected."""

from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
from verify_reference import BASE, validate_netlist
from stage_contract import audit_current_boundary, candidate_operating_limits, ext_guard_limits, discharge_limits


class ReferenceTests(unittest.TestCase):
    def check_mutation(self, edit):
        tree = ET.parse(BASE / "review/reference.net.xml")
        edit(tree.getroot())
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "mutated.xml"
            tree.write(path)
            with self.assertRaises(ValueError):
                validate_netlist(path)

    def test_actual_export(self):
        report = validate_netlist(BASE / "review/reference.net.xml")
        self.assertEqual(report["pins_checked"], 418)
        self.assertEqual(report["components_checked"], 145)
        self.assertEqual(report["current_path_audit"]["status"], "PARTIAL_BOUNDARY_ONLY_OI_010_OPEN")

    def move_pin(self, root, ref, pin, source, destination):
        net = root.find(f"./nets/net[@name='{source}']")
        node = net.find(f"node[@ref='{ref}'][@pin='{pin}']")
        net.remove(node)
        root.find(f"./nets/net[@name='{destination}']").append(node)

    def test_charger_moved_before_shunt(self):
        self.check_mutation(lambda r: self.move_pin(r, "U401", "8", "BAT_PROTECTED", "BAT_GUARDED"))

    def test_guard_negative_bypass(self):
        self.check_mutation(lambda r: self.move_pin(r, "J501", "2", "BAT_MINUS", "GND"))

    def test_guard_filter_wrong_return(self):
        self.check_mutation(lambda r: self.move_pin(r, "C501", "2", "BAT_MINUS", "GND"))

    def test_guard_fet_source_reversed(self):
        self.check_mutation(lambda r: self.move_pin(r, "Q501", "1", "BAT_MINUS", "GUARD_DSG_D"))

    def test_guard_wrong_fixed_threshold_variant(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='U501']/property[@name='MPN']").set("value", "BQ29701DSER"))

    def test_guard_oc_resistor_short(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='R506']/value").__setattr__("text", "0"))

    def test_guard_low_gate_fet_substitution(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='Q502']/property[@name='MPN']").set("value", "CSD16406Q3"))

    def test_guard_bias_not_moved_after_gauge(self):
        self.check_mutation(lambda r: self.move_pin(r, "R502", "1", "BAT_PLUS", "BAT_PROTECTED"))

    def test_guard_claims_remain_bounded(self):
        limits = candidate_operating_limits()["battery_guard_candidate"]
        self.assertFalse(limits["hard_5A_peak_clamp"])
        self.assertEqual(limits["decision"], "CANDIDATE_NOT_QUALIFIED")
        self.assertEqual(limits["discharge_oc_delay_ms"], [16, 24])
        self.assertLess(limits["ideal_oc_upper_A_excluding_bias_TCR_dynamics"], 5)
        self.assertLess(limits["early_trip_example_A_at_1V8_gate_25C_max_RDS"], 1.2)
        self.assertTrue(limits["remaining"])

    def test_guard_erc_power_input_cannot_be_hidden(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='GUARD_BAT_FILTER']/node[@ref='U501'][@pin='5']").set("pintype", "passive"))

    def test_ext_recovery_is_a_separate_gate(self):
        limits = ext_guard_limits()
        self.assertLess(limits["ov_recovery_static_V"][1], 5.25)
        screen = limits["ltc4365_replacement_screen"]
        self.assertGreater(screen["ideal_divider_gain_min_for_5V25_recovery"],
                           screen["ideal_divider_gain_max_for_5V5_trip"])

    def test_ups_moved_before_shunt(self):
        self.check_mutation(lambda r: self.move_pin(r, "U701", "2", "BAT_PROTECTED", "BAT_GUARDED"))

    def test_charger_sys_powering_main(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='CHG_SYS_LOCAL']").set("name", "SYS_RAW"))

    def test_charge_arm_accidentally_populated(self):
        def edit(root):
            part = root.find("./components/comp[@ref='R407']")
            part.remove(part.find("property[@name='dnp']"))
        self.check_mutation(edit)

    def test_ts_safety_lock_removed(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='R408']/value").__setattr__("text", "100k"))

    def test_3v3_uvlo_bypass(self):
        self.check_mutation(lambda r: self.move_pin(r, "U801", "1", "RAIL_PERMIT", "SYS_RAW"))

    def test_5v_enable_bypasses_uvlo(self):
        self.check_mutation(lambda r: self.move_pin(r, "U901", "16", "REG5_ENABLE", "5V_EN"))

    def test_logic_supply_changed_to_5v(self):
        self.check_mutation(lambda r: self.move_pin(r, "U902", "5", "3V3_MAIN", "SYS_RAW"))

    def test_battery_inhibit_removed(self):
        self.check_mutation(lambda r: self.move_pin(r, "U701", "4", "MUX_EXT_SENSE", "GND"))

    def test_unreviewed_current_limit(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='R703']/value").__setattr__("text", "18.7k"))

    def test_unreviewed_stage_mpn(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='U702']/property[@name='MPN']").set("value", "TPS3808G01DRVR"))

    def test_power_output_not_hidden_as_passive(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='UPS_RAW']/node[@ref='U701'][@pin='1']").set("pintype", "passive"))

    def test_master_gate_bypassed(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='UPS_RAW']").set("name", "SYS_RAW"))

    def test_master_gate_wrong_non_reverse_blocking_variant(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='U1301']/property[@name='MPN']").set("value", "TPS22954DQCR"))

    def test_master_default_pulldown_removed(self):
        self.check_mutation(lambda r: ET.SubElement(r.find("./components/comp[@ref='R1302']"), "property", name="dnp", value=""))

    def test_system_regulator_moved_upstream(self):
        self.check_mutation(lambda r: self.move_pin(r, "U801", "10", "SYS_RAW", "UPS_RAW"))

    def test_charger_moved_downstream_of_switch(self):
        self.check_mutation(lambda r: self.move_pin(r, "U401", "18", "EXT_BUS_5V", "SYS_RAW"))

    def test_mcu_allowed_to_override_switch(self):
        self.check_mutation(lambda r: self.move_pin(r, "U1101", "10", "CHG_REQUEST", "MASTER_EN"))

    def test_master_ct_voltage_rating(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='C1303']/value").__setattr__("text", "10nF 10V"))

    def test_discharge_wrong_fet(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='Q1301']/property[@name='MPN']").set("value", "2N7002,215"))

    def test_discharge_body_diode_reversed(self):
        def edit(root):
            root.find("./nets/net[@name='DISCH_RAW_D']/node[@ref='Q1301']").set("pin", "2")
            root.find("./nets/net[@name='GND']/node[@ref='Q1301'][@pin='2']").set("pin", "3")
        self.check_mutation(edit)

    def test_discharge_limiter_shorted(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='R1309']/value").__setattr__("text", "0"))

    def test_discharge_default_pulldown_removed(self):
        self.check_mutation(lambda r: ET.SubElement(r.find("./components/comp[@ref='R1306']"), "property", name="dnp", value=""))

    def test_discharge_rail_bridge(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='DISCH_3V3_D']").set("name", "DISCH_5V_D"))

    def test_switch_common_must_have_upstream_bias(self):
        self.check_mutation(lambda r: self.move_pin(r, "J1301", "2", "PWR_SW_SUPPLY", "MASTER_EN"))

    def test_discharge_must_not_follow_on_throw(self):
        self.check_mutation(lambda r: self.move_pin(r, "Q1301", "1", "DISCHARGE_EN", "MASTER_EN"))

    def test_passive_fallback_must_remain_populated(self):
        self.check_mutation(lambda r: ET.SubElement(r.find("./components/comp[@ref='R1305']"), "property", name="dnp", value=""))

    def test_discharge_selected_corners_not_release_rating(self):
        limits = discharge_limits()
        self.assertGreater(limits["gate_min_V_selected_25C_leakage_resistor_corners"], 1.5)
        self.assertLess(limits["gate_off_upper_V_at_three_100nA_leaks"], 0.04)
        self.assertLess(limits["resistor_initial_current_A_at_5V5_min_R"], 0.038)
        self.assertLess(limits["resistor_initial_power_W_at_5V5_min_R"], 0.21)
        self.assertTrue(0.12 < limits["five_volt_example"]["zero_injection_decay_s_after_fet_on"] < 0.13)
        self.assertIn("not guaranteed", limits["all_sources_absent"])
        self.assertTrue(limits["remaining"])

    def test_ext_resistor_only_window_is_infeasible_under_stated_assumptions(self):
        limits = ext_guard_limits()["resistor_only_window_feasibility"]
        self.assertFalse(limits["feasible"])
        self.assertGreater(limits["ratio_min_to_accept_5V25"], limits["ratio_max_to_trip_by_5V5"])
        self.assertTrue(550000 < limits["minimum_top_resistor_ohm"] < 551000)

    def test_unused_io_pulldown_removed(self):
        def edit(root):
            part = root.find("./components/comp[@ref='R1103']")
            ET.SubElement(part, "property", name="dnp", value="")
        self.check_mutation(edit)

    def test_ext_protection_bypass(self):
        self.check_mutation(lambda r: self.move_pin(r, "U201", "6", "EXT_5V_SAFE", "EXT_5V"))

    def test_ext_wrong_clamping_variant(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='U201']/property[@name='MPN']").set("value", "TPS259472LRPWR"))

    def test_ext_timer_must_not_be_grounded(self):
        def edit(root):
            net = root.find("./nets/net[@name='unconnected-(U201-ITIMER-Pad10)']")
            node = net.find("node")
            net.remove(node)
            node.set("pintype", "passive")
            root.find("./nets/net[@name='GND']").append(node)
        self.check_mutation(edit)

    def test_ext_static_analysis_preserves_rejected_corner(self):
        limits = ext_guard_limits()
        low, high = limits["ov_trip_static_V"]
        self.assertTrue(5.10 < low < 5.25 < high < 5.50)
        self.assertLess(limits["control_pin_reverse_current_upper_uA"], 10)
        self.assertEqual(limits["decision"], "UNQUALIFIED_NOT_A_FROZEN_INPUT_SPEC")

    def test_no_connect_short_to_live_net(self):
        def edit(root):
            net = root.find("./nets/net[@name='unconnected-(U401-QON-Pad7)']")
            node = net.find("node")
            net.remove(node)
            root.find("./nets/net[@name='GND']").append(node)
        self.check_mutation(edit)

    def test_no_connect_marker_missing(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='unconnected-(U702-CT-Pad4)']/node").set("pintype", "passive"))

    def test_boundary_audit_independently_rejects_extra_load(self):
        actual = {}
        tree = ET.parse(BASE / "review/reference.net.xml")
        for net in tree.findall("./nets/net"):
            for node in net.findall("node"):
                actual[(node.get("ref"), node.get("pin"))] = net.get("name")
        audit_current_boundary(actual)
        actual[("R9999", "1")] = "BAT_GUARDED"
        with self.assertRaises(ValueError):
            audit_current_boundary(actual)

    def test_candidate_limits_do_not_invent_loads_or_fast_timing(self):
        limits = candidate_operating_limits()
        self.assertIsNone(limits["required_continuous_board_power_W"])
        self.assertIsNone(limits["required_peak_board_power_W"])
        self.assertIsNone(limits["required_peak_duration_ms"])
        self.assertIsNone(limits["mux_transition_guaranteed_max_us"])
        self.assertIsNone(limits["mux_ron_guaranteed_max_at_battery_input_ohm"])
        self.assertEqual(limits["mux_transition_typical_us"], 100)
        low, high = limits["system_uvlo_static_bound_V"]
        self.assertTrue(3.20 < low < 3.23 < 3.38 < high < 3.40)
        self.assertGreater(limits["stress_example_not_requirement"]["raw_current_at_static_uvlo_low_A"], 5)
        gate = limits["master_gate_candidate"]
        self.assertFalse(gate["internal_output_discharge"])
        self.assertFalse(gate["off_all_rails_zero_proven"])
        self.assertIsNone(gate["board_required_current_A"])
        self.assertLess(gate["off_EN_open_upper_V_at_100nA_bias"], gate["off_EN_falling_threshold_min_V"])

    def test_swapped_sense_inputs(self):
        def edit(root):
            root.find("./nets/net[@name='SENSE_P']/node[@ref='U601']").set("pin", "C2")
            root.find("./nets/net[@name='SENSE_N']/node[@ref='U601']").set("pin", "C1")
        self.check_mutation(edit)

    def test_shunt_bypass(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='BAT_PROTECTED']").set("name", "BAT_GUARDED"))

    def test_unapproved_shared_interrupt(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='FG_GPOUT']").set("name", "PWR_INT"))

    def test_accidental_populated_pullup(self):
        def edit(root):
            part = root.find("./components/comp[@ref='R606']")
            part.remove(part.find("property[@name='dnp']"))
        self.check_mutation(edit)

    def test_wrong_shunt(self):
        self.check_mutation(lambda r: r.find("./components/comp[@ref='R601']/property[@name='MPN']").set("value", "OTHER"))

    def test_unreviewed_footprint(self):
        def edit(root):
            ET.SubElement(root.find("./components/comp[@ref='R601']"), "footprint").text = "Generic:R_2512"
        self.check_mutation(edit)

    def test_missing_pin(self):
        def edit(root):
            net = root.find("./nets/net[@name='GND']")
            net.remove(net.find("node[@ref='U801'][@pin='3']"))
        self.check_mutation(edit)

    def test_feedback_short(self):
        self.check_mutation(lambda r: r.find("./nets/net[@name='FB_3V3']").set("name", "GND"))

    def test_duplicate_pin(self):
        def edit(root):
            ET.SubElement(root.find("./nets/net[@name='GND']"), "node", ref="U801", pin="10")
        self.check_mutation(edit)

    def test_wrong_value(self):
        def edit(root):
            root.find("./components/comp[@ref='R801']/value").text = "51.1k 1%"
        self.check_mutation(edit)

    def test_missing_sheet(self):
        def edit(root):
            design = root.find("design")
            design.remove(design.find("sheet"))
        self.check_mutation(edit)


if __name__ == "__main__":
    unittest.main()
