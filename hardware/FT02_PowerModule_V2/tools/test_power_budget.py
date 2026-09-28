import unittest

from power_budget import battery_current, charge_headroom, report, shared_load_allowed


class PowerBudgetTests(unittest.TestCase):
    def test_shared_power_rejects_simultaneous_maxima(self):
        self.assertAlmostEqual(report()["individual_rail_sum_w"], 14.1)
        self.assertFalse(shared_load_allowed(2, 1.5))

    def test_individual_rail_capabilities_fit_proposed_envelope(self):
        self.assertTrue(shared_load_allowed(2, 0))
        self.assertTrue(shared_load_allowed(0, 1.5))
        self.assertTrue(shared_load_allowed(2, 0.28))
        self.assertFalse(shared_load_allowed(2.01, 0))
        self.assertFalse(shared_load_allowed(0, 1.51))
        self.assertFalse(shared_load_allowed(-1, 1))

    def test_joint_charger_current_failure_is_exposed(self):
        case = report()["eight_w_example"]
        self.assertGreater(case["fixed_charge_plus_system_a"], 3.2)
        self.assertFalse(case["fixed_charge_plus_system_pass"])
        self.assertLess(case["charge_headroom"]["allowed_charge_a"], 0.083)
        self.assertFalse(case["charge_headroom"]["implemented_current_policy"])

    def test_weak_source_is_not_silently_supplemented(self):
        case = charge_headroom(6.6, 1.4, input_limit=0.5)
        self.assertFalse(case["source_can_sustain_system"])
        self.assertEqual(case["allowed_charge_a"], 0)

    def test_current_budget_cannot_be_negative_charging(self):
        case = charge_headroom(6.6, 7.5)
        self.assertFalse(case["system_fits_sw_budget"])
        self.assertEqual(case["allowed_charge_a"], 0)

    def test_off_load_has_charge_headroom_conditionally(self):
        self.assertAlmostEqual(charge_headroom(0, 0)["allowed_charge_a"], 677 / 372)

    def test_battery_solution_includes_path_loss(self):
        case = report()["eight_w_example"]
        current = case["battery_current_a"]
        self.assertGreater(current, case["system_input_w"] / 3)
        self.assertLess(current, 5)
        self.assertAlmostEqual(current * (3 - current * 0.1), case["system_input_w"])
        self.assertLess(case["converter_input_v"], 3)

    def test_battery_impossible_and_invalid_operating_points(self):
        self.assertEqual(battery_current(6, 3, 0), 2)
        self.assertEqual(battery_current(0, 3, 0.1), 0)
        for args in ((100, 3, 0.1), (1, 0, 0.1), (1, 3, -1)):
            with self.assertRaises(ValueError):
                battery_current(*args)

    def test_capacity_does_not_change_path_rating(self):
        cases = report()["capacities"]
        self.assertEqual([c["path_design_target_a"] for c in cases], [5, 5])
        self.assertTrue(all(not c["future_pack_rating_inferred"] for c in cases))

    def test_por_charging_does_not_prove_10000mah_compatibility(self):
        case = report()["capacities"][1]
        self.assertEqual(case["bq25628_por_ideal_cc_hours"], 31.25)
        self.assertGreater(case["bq25628_por_ideal_cc_hours"], 28)
        self.assertGreater(case["bq25616_min_timer_remaining_hours"], 2)
        self.assertIsNone(case["real_full_charge_timer_pass"])

    def test_proposed_charge_setting_below_standard_pack_rate(self):
        self.assertLess(report()["charge_setting_a"]["max"], 2.5)

    def test_shunt_headroom_is_not_a_bypass_proof(self):
        shunt = report()["shunt"]
        self.assertLess(shunt["sense_at_5a_max_v"], shunt["gauge_full_scale_v"])
        self.assertAlmostEqual(shunt["dissipation_at_5a_max_w"], 0.12625)
        self.assertFalse(shunt["netlist_bypass_proven"])
        self.assertLess(10000 / shunt["capacity_lsb_mah"], 65535)

    def test_standby_allocations_are_not_a_qualified_maximum(self):
        standby = report()["standby"]
        self.assertEqual(sum(standby["allocations_ua"].values()), 57)
        self.assertEqual(standby["allocated_active_ua"], 57 - 12 + 30)
        self.assertIsNone(standby["qualified_maximum_ua"])
        self.assertIsNone(standby["requirement_pass"])
        self.assertAlmostEqual(standby["annual_mah_at_50ua"], 438)

    def test_no_schematic_or_release_claim_from_arithmetic(self):
        data = report()
        self.assertFalse(data["v2_schematic_implemented"])
        self.assertIsNone(data["v2_erc_errors"])
        self.assertFalse(data["phase1_pass"])
        self.assertFalse(data["pcb_authorized"])


if __name__ == "__main__":
    unittest.main()
