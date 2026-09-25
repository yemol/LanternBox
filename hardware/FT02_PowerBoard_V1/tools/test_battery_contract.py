"""Source-backed battery limits must not silently become approved unknowns."""

import json
from pathlib import Path
import re
import unittest

BASE = Path(__file__).resolve().parents[1]
REPO = BASE.parents[1]


class BatteryContractTests(unittest.TestCase):
    def setUp(self):
        self.calculations = json.loads((BASE / "review/calculation_examples.json").read_text())
        self.pack = self.calculations["source_backed_pack"]

    def test_adopted_current_limits(self):
        self.assertEqual(self.pack["model"], "955565 / 955465")
        self.assertEqual(self.pack["capacity_mah"], 5000)
        self.assertEqual(self.pack["nominal_v"], 3.8)
        self.assertEqual(self.pack["standard_charge_a"], 2.5)
        self.assertEqual(self.pack["maximum_charge_a"], 5)
        self.assertEqual(self.pack["maximum_discharge_a"], 5)
        self.assertEqual(self.pack["discharge_cutoff_v"], 2.75)

    def test_unknowns_not_inferred(self):
        self.assertEqual(self.pack["charge_voltage_status"], "OPEN: source CC to4.25V then CV4.2V")
        for field in ("loaded_minimum_v", "charge_temperature_c", "above_1c_peak_a", "above_1c_duration_s"):
            self.assertIsNone(self.pack[field], field)

    def test_overlimit_examples_are_not_authorization(self):
        self.assertEqual(self.calculations["status"], "ILLUSTRATIVE_NOT_A_RATING")
        for example in self.calculations["shunt_examples"]:
            if example["assumed_a"] > self.pack["maximum_discharge_a"]:
                self.assertTrue(example["pack_boundary"].startswith("FORBIDDEN:"))
        self.assertAlmostEqual(self.calculations["shunt_reference_corner"]["drop_mv"], 50.57875)
        self.assertAlmostEqual(self.calculations["shunt_reference_corner"]["loss_w"], 0.25289375)

    def test_only_directly_supported_issue_children_close(self):
        issues = (REPO / "docs/OPEN_ISSUES.md").read_text()
        actual = re.findall(r"^\| OI-002([A-Z]) \| (CLOSED|OPEN) \|", issues, re.MULTILINE)
        expected = [(suffix, "CLOSED" if suffix in "ABCD" else "OPEN") for suffix in "ABCDEFGHIJK"]
        self.assertEqual(actual, expected)
        self.assertIn("| OI-002 PARTIAL |", issues)
        self.assertNotIn("OI-010 CLOSED", issues)


if __name__ == "__main__":
    unittest.main()
