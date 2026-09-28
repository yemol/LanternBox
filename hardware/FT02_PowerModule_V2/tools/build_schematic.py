"""Generate editable KiCad review sheets from the explicit V2 circuit inventory."""

import csv
import json
from pathlib import Path
from uuid import NAMESPACE_URL, uuid5

from design import PARTS, PINS, PROFILE, PROJECT, SHEETS

BASE = Path(__file__).resolve().parents[1]


class Atom(str):
    pass


def n(name, *args):
    return [Atom(name), *args]


def emit(value):
    if isinstance(value, list):
        return "(" + " ".join(map(emit, value)) + ")"
    if isinstance(value, Atom):
        return str(value)
    if isinstance(value, str):
        return json.dumps(value)
    return str(value)


def uid(key):
    return str(uuid5(NAMESPACE_URL, PROJECT + "/" + key))


ROOT = uid("root")

SOURCES = {k:"https://www.ti.com/lit/ds/symlink/" + device + ".pdf" for k,device in
           {"BQ25638":"bq25638", "TPS259474":"tps25947", "TPS22953":"tps22953",
            "TPS3808":"tps3808", "TPS63802":"tps63802", "TPS61022":"tps61022"}.items()}
SOURCES.update(STUSB4500="https://www.st.com/resource/en/datasheet/stusb4500.pdf",
               MAX17055="https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17055.pdf")


def fx(size=1.27, *extra):
    return n("effects", n("font", n("size", size, size)), *extra)


def prop(key, value, x=0, y=0, hidden=False):
    return n("property", key, value, n("at", x, y, 0),
             fx(1.27, *([n("hide", Atom("yes"))] if hidden else [])))


def height(kind):
    return max(abs(p[4]) for p in PINS[kind]) + 2.54


def symbol(kind, embedded=False):
    small = kind in {"R", "C", "L", "FUSE", "TVS", "PWR_FLAG"}
    width, h = (2.54, 2.54) if small else (20.32, height(kind))
    graphic = n("rectangle", n("start", -width, h), n("end", width, -h),
                n("stroke", n("width", 0), n("type", Atom("default"))),
                n("fill", n("type", Atom("background"))))
    graphics = [graphic]
    if kind == "C":
        paths = [[(0,2.54),(0,.762)],[(-2.54,.762),(2.54,.762)],
                 [(-2.54,-.762),(2.54,-.762)],[(0,-.762),(0,-2.54)]]
        graphics = [n("polyline", n("pts",*[n("xy",x,y) for x,y in path]),
                      n("stroke",n("width",.254),n("type",Atom("default"))),
                      n("fill",n("type",Atom("none")))) for path in paths]
    pins = [n("pin", Atom(t), Atom("line"), n("at", x, y, angle), n("length", 2.54 if small else 5.08),
              n("name", name, fx()), n("number", number, fx())) for number,name,t,x,y,angle in PINS[kind]]
    return n("symbol", "FT02_V2:" + kind if embedded else kind,
        n("pin_names", n("offset", 1.016)), n("in_bom", Atom("yes")), n("on_board", Atom("yes")),
        prop("Reference", "U", 0, h+3), prop("Value", kind, 0, h+6), prop("Footprint", "", hidden=True),
        prop("Datasheet", SOURCES.get(kind,""), hidden=True), n("symbol",kind+"_0_1",*graphics), n("symbol",kind+"_1_1",*pins))


def text(value, x, y, key, size=1.27):
    return n("text", value, n("at",x,y,0), fx(size,n("justify",Atom("left"))), n("uuid",uid(key)))


def component(part, x, y):
    kind,ref,sheet = part["kind"],part["ref"],part["sheet"]
    small = kind in {"R","C","L","TVS","FUSE","PWR_FLAG"}
    rx,ry = (x+13,y-2.54) if small else (x,y+height(kind)+5.08)
    vx,vy = (x+13,y+2.54) if small else (x,y+height(kind)+10.16)
    obj = n("symbol",n("lib_id","FT02_V2:"+kind),n("at",x,y,0),n("unit",1),
        n("in_bom",Atom("no" if kind=="PWR_FLAG" else "yes")), n("on_board",Atom("no" if kind=="PWR_FLAG" else "yes")),
        n("dnp",Atom("yes" if part["dnp"] else "no")), n("uuid",uid(ref)),
        prop("Reference",ref,rx,ry),prop("Value",part["value"],vx,vy), prop("Footprint","",hidden=True),
        prop("Datasheet",SOURCES.get(kind,""),hidden=True),prop("MPN",part["mpn"],hidden=True),prop("ReviewNote",part["note"],hidden=True),
        *[n("pin",p[0],n("uuid",uid(ref+"/pin/"+p[0]))) for p in PINS[kind]],
        n("instances",n("project",PROJECT,n("path","/"+ROOT+"/"+uid("sheet/"+sheet),n("reference",ref),n("unit",1)))))
    result = [obj]
    for number,_,_,dx,dy,angle in PINS[kind]:
        px,py = round(x+dx,4),round(y-dy,4)
        net = part["nets"][number]
        if net is None:
            result.append(n("no_connect",n("at",px,py),n("uuid",uid(ref+"/nc/"+number))))
            continue
        ox,oy = {0:(-5.08,0),180:(5.08,0),270:(0,-5.08),90:(0,5.08)}[angle]
        ex,ey = round(px+ox,4),round(py+oy,4)
        result.append(n("wire",n("pts",n("xy",px,py),n("xy",ex,ey)),
            n("stroke",n("width",0),n("type",Atom("default"))),n("uuid",uid(ref+"/wire/"+number))))
        result.append(n("global_label",net,n("shape",Atom("bidirectional")),n("at",ex,ey,180 if angle==0 else 0),
            fx(1.27,n("justify",Atom("right" if angle==0 else "left"))),n("uuid",uid(ref+"/label/"+number)),
            prop("Intersheetrefs","${INTERSHEET_REFS}",ex,ey,True)))
    return result


def document(name, items, kinds=()):
    return n("kicad_sch",n("version",20250114),n("generator",Atom("eeschema")),
        n("uuid",ROOT if name==PROJECT else uid("file/"+name)),n("paper","A2"),
        n("title_block",n("title",name),n("rev","V2-PD-R0"),n("comment",1,"REVIEW DRAFT - CHARGE INHIBITED - NO PCB RELEASE")),
        n("lib_symbols",*[symbol(k,True) for k in sorted(kinds)]),*items)


def main():
    (BASE/"libraries").mkdir(exist_ok=True)
    (BASE/"review").mkdir(exist_ok=True)
    lib=n("kicad_symbol_lib",n("version",20231120),n("generator",Atom("kicad_symbol_editor")),*[symbol(k) for k in PINS])
    (BASE/"libraries/FT02_V2.kicad_sym").write_text(emit(lib)+"\n")
    table=n("sym_lib_table",n("version",7),n("lib",n("name","FT02_V2"),n("type","KiCad"),
        n("uri","${KIPRJMOD}/libraries/FT02_V2.kicad_sym"),n("options",""),n("descr","Datasheet-pinned V2 review symbols; physical footprints pending")))
    (BASE/"sym-lib-table").write_text(emit(table)+"\n")
    root=[text("FT-02 POWER MODULE V2 / SCHEMATIC REVIEW",20,20,"root/title",4),
          text("9V PD preferred / limited 5V fallback / one NVDC / physical system OFF",20,32,"root/subtitle",2),
          text("CHARGE INHIBITED. Thermal, PD fallback, NVM and battery gates remain OPEN. NO PCB RELEASE.",20,42,"root/warning",1.5)]
    summaries=[
        ("USB-C + STUSB4500", "9V/3A preferred, 9V/2A alternate; factory NVM must be replaced.", "5V low-power fallback remains unqualified."),
        ("ONE INPUT PROTECTION STAGE", "Fuse + ESD + TPS259474L; PD-controlled enable.", "OV cutoff approximately 10.4V; no source MUX."),
        ("BQ25638 SINGLE-CELL NVDC", "USB to SYS and battery; battery to SYS when USB is removed.", "CE arm DNP + TS forced fault: charging disabled."),
        ("MAX17055 + KELVIN SHUNT", "SOC, voltage and signed current; dedicated 5mohm shunt.", "Gauge self-supply exception documented; no extra main return."),
        ("HARD OFF + SYSTEM UNDERVOLTAGE", "Physical switch grounds TPS22953 EN; charger/gauge stay alive.", "TPS3808 guards SYS; OFF leakage budget is not yet qualified."),
        ("3V3 BUCK-BOOST + SENSOR FILTER", "TPS63802: 2A individual output target.", "3V3_SENS is included in the 3V3 current and shared power budget."),
        ("5V BOOST", "TPS61022: 1.5A individual output target.", "All terminal rails share a proposed 8W qualification envelope."),
        ("TERMINAL INTERFACE", "I2C, gauge alert, charger status and switched rail test points.", "All host pullups switched; no external I/O drive while OFF."),
    ]
    for i,(sheet,note) in enumerate(SHEETS.items()):
        x,y=25.4+(i%2)*270,55.88+(i//2)*65
        root.append(n("sheet",n("at",x,y),n("size",240,43.18),n("fields_autoplaced"),
            n("stroke",n("width",0.1524),n("type",Atom("default"))),n("fill",n("color",0,0,0,0)),n("uuid",uid("sheet/"+sheet)),
            prop("Sheetname",sheet,x+120,y-3),prop("Sheetfile",sheet+".kicad_sch",x+120,y+46),
            n("instances",n("project",PROJECT,n("path","/"+ROOT,n("page",str(i+2)))))))
        for j,line in enumerate(summaries[i]):
            root.append(text(line,x+8,y+10+j*11,sheet+"/summary/"+str(j),2.54 if j==0 else 1.5))
        parts=[p for p in PARTS if p["sheet"]==sheet]
        big=[p for p in parts if p["kind"] not in {"R","C","L","FUSE","TVS","PWR_FLAG"}]
        small=[p for p in parts if p not in big]
        items=[text(note,15,17,sheet+"/title",2),
               text("Local symbols use datasheet pin identities. Footprints/connector pin-to-pad maps are NOT released.",15,27,sheet+"/status")]
        for j,p in enumerate(big):
            items += component(p,101.6+(j%3)*177.8,91.44+(j//3)*100)
        start=210.82 if len(big)>3 else 182.88
        for j,p in enumerate(small):
            items += component(p,30.48+(j%6)*91.44,start+(j//6)*43.18)
        items.append(text("Use review/SCHEMATIC_REVIEW.md for source, charge lock, PWR_FLAG and netlist proof boundaries.",15,382,sheet+"/footer"))
        (BASE/(sheet+".kicad_sch")).write_text(emit(document(sheet,items,{p["kind"] for p in parts}))+"\n")
    root += [text("MAIN: USB-C -> protection -> NVDC SYS -> HARD OFF -> 3V3 buck-boost / 5V boost",25,325,"root/path",2),
             text("BATTERY: protected pack <-> NVDC BAT; pack return -> 5mohm shunt -> board GND",25,338,"root/battery",2),
             text("Independent source/UPS MUX and EXT input removed. Charger and gauge are upstream of HARD OFF.",25,351,"root/mux",1.5),
             text("Review limits, sources, warnings and OPEN gates: review/SCHEMATIC_REVIEW.md",25,364,"root/review",1.5)]
    (BASE/(PROJECT+".kicad_sch")).write_text(emit(document(PROJECT,root))+"\n")
    (BASE/(PROJECT+".kicad_pro")).write_text(json.dumps({"meta":{"version":1},"text_variables":{"DESIGN_STATUS":"REVIEW_ONLY_CHARGE_INHIBITED"}},indent=2)+"\n")
    (BASE/"review/circuit_manifest.json").write_text(json.dumps({"project":PROJECT,"sheets":SHEETS,"parts":PARTS},indent=2)+"\n")
    (BASE/"review/pd_nvm_profile.json").write_text(json.dumps(PROFILE,indent=2)+"\n")
    with (BASE/"review/review_bom.csv").open("w",newline="") as stream:
        writer=csv.writer(stream)
        writer.writerow(["Reference","Value","MPN","DNP","Footprint","ReviewNote"])
        for p in PARTS:
            if p["kind"]!="PWR_FLAG":
                writer.writerow([p["ref"],p["value"],p["mpn"],p["dnp"],"NOT_RELEASED",p["note"]])
    print(f"Generated {len(SHEETS)+1} pages, {len(PARTS)} symbols, charge arm DNP")


if __name__=="__main__":
    main()
