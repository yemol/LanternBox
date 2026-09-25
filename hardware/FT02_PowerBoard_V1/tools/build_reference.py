"""Build the partial Phase 1 hierarchy. Does not build a release or PCB.

Only the TPS63802 reference circuit and test-point inventory are instantiated.
Unresolved sheets explicitly say NOT IMPLEMENTED; ERC is never suppressed.
"""

import csv
import json
from pathlib import Path
from uuid import uuid5, NAMESPACE_URL

BASE = Path(__file__).resolve().parents[1]
PROJECT = "FT02_PowerBoard_V1"


class Atom(str):
    pass


def node(name, *args):
    return [Atom(name), *args]


def emit(value):
    if isinstance(value, list):
        return "(" + " ".join(emit(v) for v in value) + ")"
    if isinstance(value, Atom):
        return str(value)
    if isinstance(value, str):
        return json.dumps(value, ensure_ascii=False)
    return str(value)


def uid(name):
    return str(uuid5(NAMESPACE_URL, PROJECT + "/" + name))


def effects(size=1.27, *options):
    return node("effects", node("font", node("size", size, size)), *options)


def prop(name, value, x=0, y=0, hidden=False):
    return node("property", name, value, node("at", x, y, 0),
                effects(1.27, *([node("hide", Atom("yes"))] if hidden else [])))


def line(x1, y1, x2, y2):
    return node("polyline", node("pts", node("xy", x1, y1), node("xy", x2, y2)),
                node("stroke", node("width", 0), node("type", Atom("default"))),
                node("fill", node("type", Atom("none"))))


PINS = {
    "TPS63802": [
        ("1", "EN", "input", -15.24, 2.54, 0),
        ("2", "MODE", "input", -15.24, -2.54, 0),
        ("3", "AGND", "power_in", -15.24, -12.7, 0),
        ("4", "FB", "input", 15.24, -2.54, 180),
        ("5", "PG", "open_collector", 15.24, -7.62, 180),
        ("6", "VOUT", "power_out", 15.24, 7.62, 180),
        ("7", "L2", "bidirectional", 5.08, 20.32, 270),
        ("8", "GND", "power_in", -15.24, -7.62, 0),
        ("9", "L1", "bidirectional", -5.08, 20.32, 270),
        ("10", "VIN", "power_in", -15.24, 7.62, 0),
    ],
    "R": [("1", "~", "passive", 0, 5.08, 270), ("2", "~", "passive", 0, -5.08, 90)],
    "C": [("1", "~", "passive", 0, 5.08, 270), ("2", "~", "passive", 0, -5.08, 90)],
    "L": [("1", "~", "passive", 0, 5.08, 270), ("2", "~", "passive", 0, -5.08, 90)],
    "TP": [("1", "~", "passive", 0, -5.08, 90)],
}


def library_symbol(kind, embedded=False):
    name = "FT02_Reference:" + kind if embedded else kind
    graphic = []
    if kind == "TPS63802":
        graphic.append(node("rectangle", node("start", -10.16, 15.24), node("end", 10.16, -15.24),
                            node("stroke", node("width", 0), node("type", Atom("default"))),
                            node("fill", node("type", Atom("background")))))
    elif kind == "C":
        graphic += [line(-2.54, 0.635, 2.54, 0.635), line(-2.54, -0.635, 2.54, -0.635)]
    elif kind == "TP":
        graphic.append(node("circle", node("center", 0, 0), node("radius", 1.27),
                            node("stroke", node("width", 0), node("type", Atom("default"))),
                            node("fill", node("type", Atom("none")))))
    else:
        graphic.append(node("rectangle", node("start", -1.27, 2.54), node("end", 1.27, -2.54),
                            node("stroke", node("width", 0), node("type", Atom("default"))),
                            node("fill", node("type", Atom("none")))))
    pins = []
    for number, label, electrical, x, y, angle in PINS[kind]:
        length = 5.08 if kind == "TPS63802" else (4.445 if kind == "C" else 2.54)
        if kind == "TP":
            length = 3.81
        pins.append(node("pin", Atom(electrical), Atom("line"), node("at", x, y, angle),
                         node("length", length), node("name", label, effects()), node("number", number, effects())))
    prefix = "U" if kind == "TPS63802" else kind
    return node("symbol", name, node("pin_names", node("offset", 1.016)),
                node("in_bom", Atom("yes")), node("on_board", Atom("yes")),
                prop("Reference", prefix, 0, 18), prop("Value", kind, 0, 16),
                prop("Footprint", "", hidden=True), prop("Datasheet", "", hidden=True),
                node("symbol", kind + "_0_1", *graphic), node("symbol", kind + "_1_1", *pins))


SHEETS = [
    ("01_USB_C_PD", "NOT IMPLEMENTED: HUSB238A cold-start 5V-only ordering/register proof; OI-008/009/020."),
    ("02_EXT_INPUT", "NOT IMPLEMENTED: regulated EXT_5V ONLY. NO RAW SOLAR INPUT. Protection MPNs pending."),
    ("03_EXTERNAL_POWER_MUX", "NOT IMPLEMENTED: TPS2121 candidate; total input-current control and weak-USB fallback unresolved."),
    ("04_BATTERY_CHARGER", "NOT IMPLEMENTED: charger-only qualification, NTC, input charge-shedding and safe defaults pending."),
    ("05_BATTERY_PROTECTION", "NOT IMPLEMENTED: current 5000mAh pack electrical/PCM/NTC/connector limits missing. OI-002."),
    ("06_FUEL_GAUGE", "NOT IMPLEMENTED: BQ27220 candidate; common shunt, auxiliary current and profile not closed."),
    ("07_UPS_POWER_MUX", "NOT IMPLEMENTED: valid-source qualification, low-battery margin and hold-up not closed."),
    ("08_3V3_BUCKBOOST", "REFERENCE CIRCUIT ONLY: TPS63802 Fig10-1/Table10-6. NOT a qualified FT02 rail."),
    ("09_5V_BUCKBOOST", "NOT IMPLEMENTED: LT3120 candidate; minimum-input capability and compensation pending."),
    ("10_SENSOR_POWER", "NOT IMPLEMENTED: load switch, ferrite and damping selection await actual module loads."),
    ("11_MAINBOARD_INTERFACE", "NOT IMPLEMENTED: connector and I2C control/readout unresolved. Do not allocate GPIO4 (audio PA)."),
    ("12_TEST_POINTS", "LOGICAL TEST-POINT INVENTORY: most nets have no implemented source. Footprints NOT assigned."),
]

ROOT_ID = uid("root")
BOM = []
MANIFEST = {"status": "INCOMPLETE_NOT_FOR_FABRICATION", "implemented_sheets": ["08_3V3_BUCKBOOST", "12_TEST_POINTS"],
            "unimplemented_sheets": [name for name, _ in SHEETS if not name.startswith(("08_", "12_"))],
            "components": {}}


def text_item(value, x, y, key, size=1.27):
    return node("text", value, node("at", x, y, 0), effects(size, node("justify", Atom("left"))), node("uuid", uid(key)))


def component(sheet, kind, ref, value, x, y, nets, mpn=""):
    ident = uid(ref)
    ref_x, ref_y = (x, y + 20.32) if kind == "TPS63802" else (x + 10.16, y - 2.54)
    val_x, val_y = (x, y + 25.4) if kind == "TPS63802" else (x + 10.16, y + 2.54)
    part = node("symbol", node("lib_id", "FT02_Reference:" + kind), node("at", x, y, 0), node("unit", 1),
                node("in_bom", Atom("yes")), node("on_board", Atom("yes")), node("dnp", Atom("no")),
                node("uuid", ident), prop("Reference", ref, ref_x, ref_y),
                prop("Value", value, val_x, val_y), prop("Footprint", "", hidden=True),
                prop("Datasheet", "https://www.ti.com/lit/ds/symlink/tps63802.pdf" if kind != "TP" else "", hidden=True),
                prop("MPN", mpn, hidden=True), prop("DesignStatus", "REFERENCE_NOT_RELEASED", hidden=True),
                *[node("pin", p[0], node("uuid", uid(ref + "/pin/" + p[0]))) for p in PINS[kind]],
                node("instances", node("project", PROJECT,
                    node("path", "/" + ROOT_ID + "/" + uid("sheet/" + sheet), node("reference", ref), node("unit", 1)))))
    sheet_nodes = [part]
    for number, _, _, dx, dy, angle in PINS[kind]:
        px, py = round(x + dx, 4), round(y - dy, 4)
        vx, vy = {0: (-5.08, 0), 180: (5.08, 0), 270: (0, -5.08), 90: (0, 5.08)}[angle]
        ex, ey = round(px + vx, 4), round(py + vy, 4)
        sheet_nodes.append(node("wire", node("pts", node("xy", px, py), node("xy", ex, ey)),
            node("stroke", node("width", 0), node("type", Atom("default"))), node("uuid", uid(ref + "/wire/" + number))))
        sheet_nodes.append(node("global_label", nets[number], node("shape", Atom("bidirectional")),
            node("at", ex, ey, 180 if angle == 0 else 0),
            effects(1.016, node("justify", Atom("right" if angle == 0 else "left"))),
            node("uuid", uid(ref + "/label/" + number)), prop("Intersheetrefs", "${INTERSHEET_REFS}", ex, ey, True)))
    MANIFEST["components"][ref] = {"kind": kind, "nets": nets, "value": value, "mpn": mpn, "sheet": sheet}
    BOM.append([ref, value, mpn, "", "REFERENCE_NOT_RELEASED", "OI-015: footprint/procurement/derating pending"])
    return sheet_nodes


def document(name, items, kinds=()):
    return node("kicad_sch", node("version", 20250114), node("generator", Atom("eeschema")),
                node("uuid", ROOT_ID if name == PROJECT else uid("file/" + name)), node("paper", "A4"),
                node("title_block", node("title", name), node("rev", "WIP-01"),
                     node("comment", 1, "INCOMPLETE - NOT FOR FABRICATION")),
                node("lib_symbols", *[library_symbol(k, True) for k in kinds]), *items)


def main():
    (BASE / "libraries").mkdir(exist_ok=True)
    (BASE / "review").mkdir(exist_ok=True)
    lib = node("kicad_symbol_lib", node("version", 20231120), node("generator", Atom("kicad_symbol_editor")),
               *[library_symbol(k) for k in PINS])
    (BASE / "libraries/FT02_Reference.kicad_sym").write_text(emit(lib) + "\n")
    table = node("sym_lib_table", node("version", 7), node("lib", node("name", "FT02_Reference"),
        node("type", "KiCad"), node("uri", "${KIPRJMOD}/libraries/FT02_Reference.kicad_sym"),
        node("options", ""), node("descr", "Local reference symbols; no production footprints")))
    (BASE / "sym-lib-table").write_text(emit(table) + "\n")
    top = [text_item("PHASE 1 WIP - 10 sheets unimplemented. Battery specification and architecture blockers remain.", 15.24, 15.24, "rootnote")]
    for i, (name, note) in enumerate(SHEETS):
        x, y = 20.32 + (i % 3) * 86.36, 35.56 + (i // 3) * 30.48
        top.append(node("sheet", node("at", x, y), node("size", 76.2, 20.32), node("fields_autoplaced"),
            node("stroke", node("width", 0.1524), node("type", Atom("default"))),
            node("fill", node("color", 0, 0, 0, 0)), node("uuid", uid("sheet/" + name)),
            prop("Sheetname", name, x + 38.1, y - 2.54), prop("Sheetfile", name + ".kicad_sch", x + 38.1, y + 22.86),
            node("instances", node("project", PROJECT, node("path", "/" + ROOT_ID, node("page", str(i + 2)))))))
        items = [text_item(note, 12.7, 15.24, name + "/note"),
                 text_item("No PWR_FLAG, no ERC exclusions, no production footprint claims.", 12.7, 22.86, name + "/status")]
        kinds = []
        if name.startswith("08_"):
            kinds = ["TPS63802", "R", "C", "L"]
            items += component(name, "TPS63802", "U801", "TPS63802DLAR", 100.33, 77.47,
                {"1": "SYS_RAW", "2": "GND", "3": "GND", "4": "FB_3V3", "5": "PG_3V3",
                 "6": "3V3_MAIN", "7": "SW3_L2", "8": "GND", "9": "SW3_L1", "10": "SYS_RAW"}, "TPS63802DLAR")
            items += component(name, "L", "L801", "0.47uH", 180.34, 50.8, {"1": "SW3_L1", "2": "SW3_L2"}, "XFL4015-471ME")
            items += component(name, "C", "C801", "10uF 6.3V", 35.56, 58.42, {"1": "SYS_RAW", "2": "GND"}, "GRM188R60J106ME84")
            items += component(name, "C", "C802", "22uF 6.3V", 35.56, 104.14, {"1": "3V3_MAIN", "2": "GND"}, "GRM188R60J226MEA0")
            items += component(name, "R", "R801", "511k 1%", 180.34, 96.52, {"1": "3V3_MAIN", "2": "FB_3V3"})
            items += component(name, "R", "R802", "91k 1%", 241.3, 96.52, {"1": "FB_3V3", "2": "GND"})
            items += component(name, "R", "R803", "100k 1%", 241.3, 50.8, {"1": "3V3_MAIN", "2": "PG_3V3"})
            items += [text_item("EN tied to SYS_RAW: default-on reference only. Rail-disable and system UVLO not yet implemented.", 12.7, 144.78, name + "/enable"),
                      text_item("PG pulled to 3V3_MAIN (not raw input) for Core voltage compatibility. Input protection remains mandatory.", 12.7, 152.4, name + "/pg"),
                      text_item("Reference passives are not a released BOM. No transient simulation or full-temperature qualification performed.", 12.7, 160.02, name + "/limits")]
        elif name.startswith("12_"):
            kinds = ["TP"]
            nets = ["USB_VBUS", "USB_5V", "EXT_5V", "EXT_BUS_5V", "BAT_PLUS", "BAT_MINUS", "SYS_RAW", "3V3_MAIN", "5V_MAIN", "GND",
                    "SDA", "SCL", "SENSE_P", "SENSE_N", "KEEPALIVE", "PG_3V3", "PG_5V", "EXT_VALID", "PWR_INT"]
            for j, net in enumerate(nets):
                items += component(name, "TP", f"TP{1201+j}", "TP_" + net, 25.4 + (j % 5) * 50.8,
                                   45.72 + (j // 5) * 33.02, {"1": net})
        else:
            items += [text_item("Intent is documented in docs/POWER_ARCHITECTURE.md and docs/OPEN_ISSUES.md.", 12.7, 43.18, name + "/intent"),
                      text_item("This page has no electrical implementation. It must not be counted as a completed design.", 12.7, 53.34, name + "/empty")]
        (BASE / (name + ".kicad_sch")).write_text(emit(document(name, items, kinds)) + "\n")
    (BASE / (PROJECT + ".kicad_sch")).write_text(emit(document(PROJECT, top)) + "\n")
    (BASE / (PROJECT + ".kicad_pro")).write_text(json.dumps({"meta": {"version": 1}, "text_variables": {"DESIGN_STATUS": "INCOMPLETE_NOT_FOR_FABRICATION"}}, indent=2) + "\n")
    (BASE / "review/implementation_manifest.json").write_text(json.dumps(MANIFEST, indent=2) + "\n")
    bom = BASE.parents[1] / "bom/FT02_PowerBoard_V1_BOM.csv"
    bom.parent.mkdir(exist_ok=True)
    with bom.open("w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(["Reference", "Value", "Candidate_MPN", "Footprint", "Status", "Open_Item"])
        writer.writerows(BOM)
    print("Generated WIP hierarchy: one regulator reference, logical test points, ten explicit unimplemented sheets")


if __name__ == "__main__":
    main()
