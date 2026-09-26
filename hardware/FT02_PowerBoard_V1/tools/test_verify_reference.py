"""Incorrect reference exports must be rejected."""

from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
from verify_reference import BASE, validate_netlist
from stage_contract import audit_current_boundary, candidate_operating_limits, ext_guard_limits


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
        self.assertEqual(report["pins_checked"], 363)
        self.assertEqual(report["components_checked"], 127)
        self.assertEqual(report["current_path_audit"]["status"], "PARTIAL_BOUNDARY_ONLY_OI_010_OPEN")

    def move_pin(self, root, ref, pin, source, destination):
        net = root.find(f"./nets/net[@name='{source}']")
        node = net.find(f"node[@ref='{ref}'][@pin='{pin}']")
        net.remove(node)
        root.find(f"./nets/net[@name='{destination}']").append(node)

    def test_charger_moved_before_shunt(self):
        self.check_mutation(lambda r: self.move_pin(r, "U401", "8", "BAT_PROTECTED", "BAT_GUARDED"))

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
