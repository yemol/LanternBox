"""Check KiCad's exported connectivity, not just the generator's intended circuit."""

import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from design import PARTS, SHEETS, PROFILE

BASE = Path(__file__).resolve().parents[1]


def check(root, erc, profile):
    errors = []
    checks = []

    def require(name, condition):
        checks.append({"name": name, "pass": bool(condition)})
        if not condition:
            errors.append(name)

    nodes = {}
    nets = {}
    for net in root.findall("nets/net"):
        name = net.get("name")
        nets[name] = {(n.get("ref"), n.get("pin")) for n in net.findall("node")}
        for node in nets[name]:
            require("unique pin " + repr(node), node not in nodes)
            nodes[node] = name
    comps = {c.get("ref"): c for c in root.findall("components/comp")}
    expected = {p["ref"]: p for p in PARTS if p["kind"] != "PWR_FLAG"}
    require("component inventory", set(comps) == set(expected))
    for ref, part in expected.items():
        if ref not in comps:
            continue
        require(ref + " value", comps[ref].findtext("value") == part["value"])
        dnp = any(p.get("name") == "dnp" for p in comps[ref].findall("property"))
        require(ref + " population", dnp == part["dnp"])
        for pin, net in part["nets"].items():
            actual = nodes.get((ref, pin))
            require(ref + "." + pin + " connection",
                    actual == net if net else actual is None or actual.startswith("unconnected-"))
    sheets = root.findall("design/sheet")
    require("root plus eight populated pages", len(sheets) == 9 and
            all(any(c.find("sheetpath").get("names") == "/" + s + "/" for c in comps.values()) for s in SHEETS))
    # Independent invariants must still fail if the generator inventory is changed.
    for ref, pin, net in [
        ("J101","A4","USB_VBUS_RAW"), ("F101","1","USB_VBUS_RAW"),
        ("F101","2","USB_FUSED"), ("U201","5","USB_FUSED"), ("U201","6","USB_SAFE"),
        ("U301","A1","USB_SAFE"), ("U301","D1","SYS_AON"),
        ("U301","E1","PACK_POS"), ("U501","1","SYS_AON"), ("U501","9","SYS_SW"),
        ("U601","10","SYS_SW"), ("U701","7","SYS_SW"),
        ("SW501","1","SYSTEM_EN"), ("SW501","3","GND"), ("U501","4","SYSTEM_EN"),
        ("U401","6","PACK_NEG"), ("U401","5","GND"),
        ("U101","20","PD_9V_N"), ("U101","14","PD_9V_N"),
        ("U101","16","PD_SINK_N"), ("R308","2","PD_9V_N"),
        ("R303","1","CHG_TS"), ("R303","2","GND"),
    ]:
        require("invariant " + ref + "." + pin, nodes.get((ref,pin)) == net)
    allowed_pack_return = {("J401","2"), ("R401","1"), ("R401","3"),
        ("U401","6"), ("U401","2"), ("U401","11"), ("C401","2"), ("C402","2")}
    # KiCad omits power symbols from its component/net export; flags are audited in the source manifest.
    require("PACK_NEG exact whitelist: no extra bypass", nets.get("PACK_NEG") == allowed_pack_return)
    bridges = []
    for ref in comps:
        connected = {n for (r,p),n in nodes.items() if r == ref}
        if {"PACK_NEG","GND"} <= connected:
            bridges.append(ref)
    require("only shunt and gauge reference span return domains", set(bridges) == {"R401","U401"})
    require("shunt force and sense mapping", all(nodes.get(("R401",p)) == n for p,n in
            {"1":"PACK_NEG","3":"PACK_NEG","2":"GND","4":"GND"}.items()))
    for ref in ("J801","J802"):
        require(ref + " no always-on power", not any(r == ref and n in
                {"SYS_AON","PACK_POS","USB_SAFE","USB_FUSED","CHG_REGN"} for (r,p),n in nodes.items()))
    require("host pullups switched", all(nodes.get(("R"+str(i),"1")) == "3V3_MAIN" for i in range(801,807)))
    require("private PD bus isolation", nodes.get(("U101","7")) != nodes.get(("U401","3")) and
            nodes.get(("U101","8")) != nodes.get(("U401","4")))
    require("9V only high power profile", profile.get("POWER_OK_CFG") == 2 and
            [(p["voltage_v"],p["current_a"]) for p in profile["pdo"]] == [(5,0.1),(9,2),(9,3)])
    require("no legacy source mux or EXT net", not any("MUX" in c.findtext("value","") for c in comps.values()) and
            not any("EXT_" in n for n in nets))
    violations = [v for s in erc["sheets"] for v in s["violations"]]
    require("ERC zero errors and warnings", not violations)
    require("ERC includes errors and warnings", set(erc["included_severities"]) == {"error","warning"})
    flags = [p for p in PARTS if p["kind"] == "PWR_FLAG"]
    require("five external-source flags only", {p["nets"]["1"] for p in flags} ==
            {"USB_VBUS_RAW","USB_FUSED","GND","PACK_POS","PACK_NEG"} and len(flags) == 5 and all(p["note"] for p in flags))
    return {"static_connectivity": "PASS" if not errors else "FAIL", "errors": errors,
            "check_count": len(checks), "checks": checks, "sheets": len(sheets), "components": len(comps),
            "erc_errors": sum(v["severity"] == "error" for v in violations),
            "erc_warnings": sum(v["severity"] == "warning" for v in violations),
            "phase1_release": "FAIL_OPEN_GATES", "pcb_authorized": False,
            "scope": "Logical connectivity only. No transient, firmware, footprint, thermal or hardware certification.",
            "gauge_self_supply": "MAX17055 BATT-to-CSP bias and local decoupling bypass shunt intentionally; pack PCM internal current unknown."}


def main():
    netfile = BASE / "review/reference.net.xml"
    result = check(ET.parse(netfile).getroot(), json.loads((BASE/"review/erc.json").read_text()),
                   json.loads((BASE/"review/pd_nvm_profile.json").read_text()))
    result["netlist_sha256"] = hashlib.sha256(netfile.read_bytes()).hexdigest()
    (BASE/"review/schematic_checks.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k != "checks"},indent=2))
    raise SystemExit(bool(result["errors"]))


if __name__ == "__main__":
    main()
