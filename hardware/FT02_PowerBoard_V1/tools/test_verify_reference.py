"""Incorrect reference exports must be rejected."""

from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
from verify_reference import BASE, validate_netlist


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
        self.assertEqual(validate_netlist(BASE / "review/reference.net.xml")["pins_checked"], 41)

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
