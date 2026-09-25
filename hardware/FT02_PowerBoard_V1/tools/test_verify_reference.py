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
        self.assertEqual(validate_netlist(BASE / "review/reference.net.xml")["pins_checked"], 70)

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
