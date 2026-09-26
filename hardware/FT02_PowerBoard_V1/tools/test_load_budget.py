import unittest
from load_budget import inventory, report, strict_sum


class LoadBudgetTests(unittest.TestCase):
    def test_unknown_never_becomes_zero(self):
        self.assertIsNone(strict_sum([0.34, None]))
        self.assertIsNone(strict_sum([]))

    def test_sum_and_invalid_bounds(self):
        self.assertAlmostEqual(strict_sum([0.1, 0.2]), 0.3)
        for value in (-1, float("nan"), float("inf")):
            with self.assertRaises(ValueError):
                strict_sum([value])

    def test_required_fields_and_unique_loads(self):
        rows = inventory()
        self.assertEqual(len(rows), 19)
        self.assertEqual(len({r["name"] for r in rows}), len(rows))
        for row in rows:
            for key in ("typical", "continuous_max_A", "startup_max_A", "transient_max_A",
                        "peak_duration_ms", "rail", "evidence", "concurrency"):
                self.assertIn(key, row)

    def test_reference_peaks_do_not_become_guarantees(self):
        core = inventory()[0]
        self.assertEqual(core["transient_reference_A"], 0.340)
        self.assertIsNone(core["transient_max_A"])
        self.assertIsNone(core["continuous_max_A"])

    def test_no_fabricated_whole_board_rating(self):
        data = report()
        for rail in data["rails"].values():
            self.assertIsNone(rail["continuous_worst_A"])
            self.assertIsNone(rail["transient_worst_A"])
        self.assertIsNone(data["whole_board_concurrent_continuous_W"])
        self.assertIsNone(data["whole_board_concurrent_peak_W"])
        self.assertIsNone(data["whole_board_peak_duration_ms"])


if __name__ == "__main__":
    unittest.main()
