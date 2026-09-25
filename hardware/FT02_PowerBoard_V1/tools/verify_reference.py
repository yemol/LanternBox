"""Verify the partial reference against independent datasheet expectations.

TPS63802 Rev D pin table and application circuit, not generator metadata.
This checks KiCad's exported XML and never certifies the complete board.
"""

import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parents[1]


def validate_netlist(path):
    root = ET.parse(path).getroot()
    actual = {}
    for net in root.findall("./nets/net"):
        for pin in net.findall("node"):
            key = (pin.get("ref"), pin.get("pin"))
            if key in actual:
                raise ValueError(f"Duplicated pin: {key}")
            actual[key] = net.get("name")
    expected = {
        ("U801", "1"): "SYS_RAW", ("U801", "2"): "GND",
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
    if actual != expected:
        diff = {str(k): [expected.get(k), actual.get(k)] for k in expected.keys() | actual.keys()
                if expected.get(k) != actual.get(k)}
        raise ValueError(f"Reference connectivity mismatch [expected, actual]: {diff}")
    parts = {c.get("ref"): c for c in root.findall("./components/comp")}
    if set(parts) != {ref for ref, _ in expected}:
        raise ValueError("Unexpected or missing component")
    values = {"U801": "TPS63802DLAR", "L801": "0.47uH", "C801": "10uF 6.3V",
              "C802": "22uF 6.3V", "R801": "511k 1%", "R802": "91k 1%", "R803": "100k 1%"}
    for ref, value in values.items():
        if parts[ref].findtext("value") != value:
            raise ValueError(f"Unexpected value: {ref}")
    if len(root.findall("./design/sheet")) != 13:
        raise ValueError("Expected root plus twelve child sheets")
    return {"status": "PASS_REFERENCE_ONLY", "pins_checked": len(expected),
            "components_checked": len(parts), "sheets_parsed": 13}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--netlist", type=Path, default=BASE / "review/reference.net.xml")
    args = parser.parse_args()
    topology = validate_netlist(args.netlist)
    erc_path = BASE / "review/erc.json"
    erc = json.loads(erc_path.read_text())
    counts = dict(Counter(v["severity"] for s in erc["sheets"] for v in s["violations"]))
    manifest = json.loads((BASE / "review/implementation_manifest.json").read_text())
    evidence = [*sorted(BASE.glob("*.kicad_sch")), args.netlist, erc_path, BASE / "FT02_PowerBoard_V1.kicad_pro"]
    report = {"checked_utc": datetime.now(timezone.utc).isoformat(), "topology": topology,
        "erc": {"kicad_version": erc["kicad_version"], "counts": counts,
            "default_ignored_checks": erc.get("ignored_checks", []),
            "note": "Default ignored checks disclosed; no project-specific exclusions added."},
        "unimplemented_sheets": manifest["unimplemented_sheets"],
        "release_gate": "FAIL_INCOMPLETE_DESIGN",
        "evidence_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in evidence},
        "scope": "Topology only. No circuit simulation, transient proof, shunt audit, footprints or hardware tests."}
    (BASE / "review/reference_validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"topology": topology, "erc": counts, "release_gate": report["release_gate"]}))


if __name__ == "__main__":
    main()
