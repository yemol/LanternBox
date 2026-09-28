import unittest
from pd_budget import calculate


class PDBudgetTests(unittest.TestCase):
    def test_conditional_checks(self):
        self.assertTrue(all(calculate()["checks"].values()))

    def test_no_false_release(self):
        result=calculate()
        self.assertFalse(result["phase1_pass"])
        self.assertFalse(result["off_budget_ua"]["guaranteed"])
        self.assertFalse(result["fallback_estimated_a"]["qualified"])

    def test_shared_output_not_simultaneous(self):
        self.assertLess(calculate()["output_envelope_w"],3.3*2+5*1.5)

    def test_uv_tolerances_not_nominal_only(self):
        uv=calculate()["system_uv_v"]
        self.assertLess(uv["min"],uv["nominal"])
        self.assertGreater(uv["max"],uv["nominal"])

    def test_low_battery_off_budget(self):
        b=calculate()["off_budget_ua"]
        self.assertLess(b["uv_asserted_additional_max_estimate"],8)
        self.assertLess(b["normal_total"]+b["active_gauge_extra"]+b["uv_asserted_additional_max_estimate"],100)


if __name__ == "__main__":
    unittest.main()
