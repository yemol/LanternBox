"""Mutation tests establish that key safety checks can fail on exported netlists."""
import json
import unittest
import xml.etree.ElementTree as ET
from verify_schematic import BASE, check


class NetlistTests(unittest.TestCase):
    def setUp(self):
        self.root = ET.parse(BASE/"review/reference.net.xml").getroot()
        self.erc = json.loads((BASE/"review/erc.json").read_text())
        self.profile = json.loads((BASE/"review/pd_nvm_profile.json").read_text())

    def result(self):
        return check(self.root,self.erc,self.profile)

    def move(self,ref,pin,target):
        for net in self.root.findall("nets/net"):
            for node in list(net):
                if node.get("ref") == ref and node.get("pin") == pin:
                    net.remove(node)
                    self.root.find("nets/net[@name='"+target+"']").append(node)
                    return
        self.fail("mutation target missing")

    def test_reference(self):
        self.assertEqual(self.result()["errors"],[])

    def test_shunt_bypass(self):
        self.move("C301","2","PACK_NEG")
        self.assertIn("PACK_NEG exact whitelist: no extra bypass", self.result()["errors"])

    def test_gauge_polarity(self):
        self.move("U401","5","PACK_NEG")
        self.assertIn("invariant U401.5", self.result()["errors"])

    def test_broken_main_path(self):
        self.move("U301","A1","GND")
        self.assertIn("invariant U301.A1", self.result()["errors"])

    def test_always_on_pullup(self):
        self.move("R801","1","SYS_AON")
        self.assertIn("host pullups switched", self.result()["errors"])

    def test_charge_arm_populated(self):
        comp = self.root.find("components/comp[@ref='R302']")
        for prop in list(comp.findall("property")):
            if prop.get("name") == "dnp":
                comp.remove(prop)
        self.assertIn("R302 population", self.result()["errors"])

    def test_pd_profile_wrong_voltage(self):
        self.profile["pdo"][2]["voltage_v"] = 20
        self.assertIn("9V only high power profile", self.result()["errors"])

    def test_erc_warning(self):
        self.erc["sheets"][0]["violations"].append({"severity":"warning"})
        self.assertIn("ERC zero errors and warnings", self.result()["errors"])


if __name__ == "__main__":
    unittest.main()
