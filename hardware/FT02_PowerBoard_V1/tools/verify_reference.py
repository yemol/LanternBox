"""Verify the partial reference against independent datasheet expectations.

TPS63802 Rev D and BQ27220 Rev A pin/application data, not generator metadata.
This checks KiCad's exported XML and never certifies the complete board.
"""

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
from stage_contract import MPNS, expectations, audit_current_boundary, candidate_operating_limits

BASE = Path(__file__).resolve().parents[1]


def validate_netlist(path):
    root = ET.parse(path).getroot()
    actual = {}
    for net in root.findall("./nets/net"):
        for pin in net.findall("node"):
            key = (pin.get("ref"), pin.get("pin"))
            if key in actual:
                raise ValueError(f"Duplicated pin: {key}")
            if "+no_connect" in pin.get("pintype", ""):
                if len(net.findall("node")) != 1 or not net.get("name", "").startswith("unconnected-"):
                    raise ValueError(f"No-connect pin joined to a live net: {key}")
                actual[key] = None
            else:
                actual[key] = net.get("name")
    expected = {
        ("U601", "C3"): "BAT_GUARDED", ("U601", "C1"): "SENSE_P",
        ("U601", "C2"): "SENSE_N", ("U601", "B1"): "FG_BIN",
        ("U601", "B2"): "GND", ("U601", "B3"): "FG_VDD_1V8",
        ("U601", "A3"): "SCL", ("U601", "A2"): "SDA", ("U601", "A1"): "FG_GPOUT",
        ("R601", "I1"): "BAT_GUARDED", ("R601", "I2"): "BAT_PROTECTED",
        ("R601", "E1"): "SENSE_P", ("R601", "E2"): "SENSE_N",
        ("C601", "1"): "BAT_GUARDED", ("C601", "2"): "GND",
        ("C602", "1"): "FG_VDD_1V8", ("C602", "2"): "GND",
        ("R602", "1"): "FG_BIN", ("R602", "2"): "GND",
        ("R603", "1"): "3V3_MAIN", ("R603", "2"): "FG_GPOUT",
        ("R604", "1"): "SDA", ("R604", "2"): "GND",
        ("R605", "1"): "SCL", ("R605", "2"): "GND",
        ("R606", "1"): "3V3_MAIN", ("R606", "2"): "SDA",
        ("R607", "1"): "3V3_MAIN", ("R607", "2"): "SCL",
        ("U801", "1"): "RAIL_PERMIT", ("U801", "2"): "GND",
        ("U801", "3"): "GND", ("U801", "4"): "FB_3V3",
        ("U801", "5"): "PG_3V3", ("U801", "6"): "3V3_MAIN",
        ("U801", "7"): "SW3_L2", ("U801", "8"): "GND",
        ("U801", "9"): "SW3_L1", ("U801", "10"): "SYS_RAW",
        ("L801", "1"): "SW3_L1", ("L801", "2"): "SW3_L2",
        ("C801", "1"): "SYS_RAW", ("C801", "2"): "GND",
        ("C802", "1"): "3V3_MAIN", ("C802", "2"): "GND",
        ("R801", "1"): "3V3_MAIN", ("R801", "2"): "FB_3V3",
        ("R802", "1"): "FB_3V3", ("R802", "2"): "GND",
        ("R803", "1"): "3V3_MAIN", ("R803", "2"): "PG_3V3",
    }
    test_nets = ["USB_VBUS", "USB_5V", "EXT_5V", "EXT_BUS_5V", "BAT_PLUS",
                 "BAT_MINUS", "SYS_RAW", "3V3_MAIN", "5V_MAIN", "GND", "SDA",
                 "SCL", "SENSE_P", "SENSE_N", "KEEPALIVE", "PG_3V3", "PG_5V",
                 "EXT_VALID", "PWR_INT"]
    expected.update({(f"TP{1201+i}", "1"): net for i, net in enumerate(test_nets)})
    stage_pins, stage_values = expectations()
    expected.update(stage_pins)
    if actual != expected:
        diff = {str(k): [expected.get(k), actual.get(k)] for k in expected.keys() | actual.keys()
                if expected.get(k) != actual.get(k)}
        raise ValueError(f"Reference connectivity mismatch [expected, actual]: {diff}")
    parts = {c.get("ref"): c for c in root.findall("./components/comp")}
    if set(parts) != {ref for ref, _ in expected}:
        raise ValueError("Unexpected or missing component")
    values = {"U801": "TPS63802DLAR", "L801": "0.47uH", "C801": "10uF 6.3V",
              "C802": "22uF 6.3V", "R801": "511k 1%", "R802": "91k 1%", "R803": "100k 1%",
              "U601": "BQ27220YZFR", "R601": "10m 1% 1W", "C601": "1uF REF", "C602": "2.2uF REF",
              "R602": "10k 1%", "R603": "10k 1%", "R604": "1M 1%", "R605": "1M 1%",
              "R606": "5.1k 1%", "R607": "5.1k 1%"}
    values.update(stage_values)
    for ref, value in values.items():
        if parts[ref].findtext("value") != value:
            raise ValueError(f"Unexpected value: {ref}")
    dnp = {ref for ref, part in parts.items() if part.find("property[@name='dnp']") is not None}
    if dnp != {"R407", "R606", "R607"}:
        raise ValueError("Charge ARM must remain DNP; only optional I2C pullups are additionally DNP")
    for ref, mpn in MPNS.items():
        prop = parts[ref].find("property[@name='MPN']")
        if prop is None or prop.get("value") != mpn:
            raise ValueError(f"Unreviewed stage MPN: {ref}")
    for ref, pin, kind in [("U701", "1", "power_out"), ("U701", "8", "passive"),
                           ("U901", "2", "power_out"), ("U901", "4", "passive")]:
        record = root.find(f"./nets/net/node[@ref='{ref}'][@pin='{pin}']")
        if record is None or record.get("pintype") != kind:
            raise ValueError(f"Unreviewed parallel-output electrical model: {ref}:{pin}")
    if parts["R601"].find("property[@name='MPN']").get("value") != "WSK2512R0100FEA":
        raise ValueError("Unreviewed shunt MPN")
    if any(part.findtext("footprint", "") for part in parts.values()):
        raise ValueError("Footprints require a separate review; logical Kelvin terminals are not pad assignments")
    if len(root.findall("./design/sheet")) != 13:
        raise ValueError("Expected root plus twelve child sheets")
    boundary = audit_current_boundary(actual)
    return {"status": "PASS_REFERENCE_ONLY", "pins_checked": len(expected),
            "components_checked": len(parts), "sheets_parsed": 13,
            "current_path_audit": boundary}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--netlist", type=Path, default=BASE / "review/reference.net.xml")
    args = parser.parse_args()
    topology = validate_netlist(args.netlist)
    erc_path = BASE / "review/erc.json"
    erc = json.loads(erc_path.read_text())
    counts = dict(Counter(v["severity"] for s in erc["sheets"] for v in s["violations"]))
    manifest = json.loads((BASE / "review/implementation_manifest.json").read_text())
    evidence = [*sorted(BASE.glob("*.kicad_sch")), args.netlist, erc_path, BASE / "FT02_PowerBoard_V1.kicad_pro",
                BASE / "libraries/FT02_Reference.kicad_sym", BASE / "review/implementation_manifest.json",
                *sorted((BASE / "tools").glob("*.py"))]
    report = {"checked_utc": datetime.now(timezone.utc).isoformat(), "topology": topology,
        "erc": {"kicad_version": erc["kicad_version"], "counts": counts,
            "default_ignored_checks": erc.get("ignored_checks", []),
            "note": "Default ignored checks disclosed; no project-specific exclusions added."},
        "unimplemented_sheets": manifest["unimplemented_sheets"],
        "wired_reference_sheets": manifest["wired_reference_sheets"],
        "qualified_sheets": manifest["qualified_sheets"],
        "candidate_operating_limits": candidate_operating_limits(),
        "release_gate": "FAIL_INCOMPLETE_DESIGN",
        "evidence_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in evidence},
        "scope": "Pin topology and partial shunt boundary only. No complete pack path, circuit simulation, transient proof, footprints or hardware tests."}
    (BASE / "review/reference_validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"topology": topology, "erc": counts, "release_gate": report["release_gate"]}))


if __name__ == "__main__":
    main()
