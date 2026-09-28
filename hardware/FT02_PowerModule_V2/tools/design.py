"""V2-PD review circuit. Physical charge arm is DNP; no manufacturing release."""

PROJECT = "FT02_PowerModule_V2"
PINS = {}


def pins(kind, left, right):
    result = []
    for side, rows in ((-1, left), (1, right)):
        for index, (number, name, electrical) in enumerate(rows):
            result.append((str(number), name, electrical, side * 25.4,
                           ((len(rows) - 1) / 2 - index) * 5.08,
                           0 if side < 0 else 180))
    PINS[kind] = result


def rows(numbers, names, types):
    return list(zip(numbers.split(), names.split(), types.split()))


pins("STUSB4500", rows("24 22 1 2 4 5 6 12 13 10 25 18",
    "VDD VSYS CC1DB CC1 CC2 CC2DB RESET ADDR0 ADDR1 GND EP VBUS_VS_DISCH",
    "power_in power_in bidirectional bidirectional bidirectional bidirectional input input input power_in power_in input"),
    rows("23 21 16 20 14 9 7 8 19 11 15 17 3",
    "VREG_2V7 VREG_1V2 VBUS_EN_SNK POWER_OK2 POWER_OK3 DISCH SCL SDA ALERT ATTACH GPIO A_B_SIDE NC",
    "power_out power_out open_collector open_collector open_collector open_collector input bidirectional open_collector open_collector open_collector open_collector passive"))
pins("BQ25638", rows("A1 B1 A2 B2 A5 F3 F2 F4 E4 C5 D4 A4 B4 C4 F1",
    "VBUS VBUS PMID PMID REGN ILIM CE_N TS_BIAS TS ADCIN QON PGND PGND PGND BATP",
    "power_in power_in power_out passive power_out passive input power_out input input input power_in power_in power_in input"),
    rows("A3 B3 C3 B5 D1 D2 D3 E1 E2 E3 C1 C2 F5 D5 E5",
    "SW SW SW BTST SYS SYS SYS BAT BAT BAT STAT_N PG_N INT_N SCL SDA",
    "power_out passive passive passive power_out passive passive passive passive passive open_collector open_collector open_collector input bidirectional"))
pins("MAX17055", rows("10 6 5 2 11", "BATT CSP CSN AIN EP", "power_in power_in input input power_in"),
    rows("7 3 4 8 9 1", "REG SCL SDA ALRT THRM NC", "power_out input bidirectional open_collector output passive"))
pins("TPS259474", rows("5 1 2 9 8", "IN EN_UVLO OVLO ILM GND", "power_in input input passive power_in"),
    rows("6 3 4 7 10", "OUT PG PGTH DVDT ITIMER", "power_out open_collector input passive passive"))
pins("TPS22953", rows("1 2 3 4 5 11", "IN IN BIAS EN GND EP", "power_in power_in power_in input power_in power_in"),
    rows("9 10 8 7 6", "OUT OUT SNS PG CT", "power_out passive input open_collector passive"))
pins("TPS3808", rows("6 5 3", "VDD SENSE MR_N", "power_in input input"),
    rows("1 4 2", "RESET_N CT GND", "open_collector passive power_in"))
pins("TPS63802", rows("10 1 2 3 8", "VIN EN MODE AGND GND", "power_in input input power_in power_in"),
    rows("9 7 6 4 5", "L1 L2 VOUT FB PG", "power_out passive power_out input open_collector"))
pins("TPS61022", rows("7 5 6 1", "VIN EN MODE GND", "power_in input input power_in"),
    rows("2 3 4", "SW VOUT FB", "power_out power_out input"))
pins("PMOS", rows("1", "G", "input"), rows("2 3", "S D", "passive passive"))
pins("NMOS", rows("1", "G", "input"), rows("2 3", "S D", "passive passive"))
pins("PNP", rows("1", "B", "passive"), rows("2 3", "E C", "passive passive"))
pins("KELVIN", rows("1 3", "I_PACK S_PACK", "passive passive"), rows("2 4", "I_GND S_GND", "passive passive"))
pins("SW_SPDT", rows("1", "COM", "passive"), rows("2 3", "ON OFF", "passive passive"))
usb = ["A1", "A12", "B1", "B12", "A4", "A9", "B4", "B9", "A5", "B5", "A6", "B6", "A7", "B7", "A8", "B8", "SH"]
pins("USB_C", [(n, n, "passive") for n in usb[:9]], [(n, n, "passive") for n in usb[9:]])
for count in (2, 4, 6, 12):
    pins("CONN" + str(count), [(str(n), str(n), "passive") for n in range(1, count + 1)], [])
for kind in ("R", "C", "L", "FUSE", "TVS"):
    PINS[kind] = [("1", "K" if kind == "TVS" else "~", "passive", 0, 5.08, 270),
                  ("2", "A" if kind == "TVS" else "~", "passive", 0, -5.08, 90)]
PINS["PWR_FLAG"] = [("1", "PWR_FLAG", "power_out", 0, -5.08, 90)]

PARTS = []
SHEETS = {
    "01_USB_PD": "STUSB4500: program and VERIFY PDO1=5V/0.1A, PDO2=9V/2A, PDO3=9V/3A before use.",
    "02_INPUT_PROTECTION": "One input eFuse, not a source MUX. Independent OV cutoff also blocks factory-default 15V/20V PDOs.",
    "03_CHARGER_NVDC": "BQ25638 SYS is the main source. CE arm DNP + TS forced fault: CHARGING IS PHYSICALLY INHIBITED.",
    "04_BATTERY_GAUGE": "Protected pack only. PACK_NEG is isolated from board GND except the four-terminal shunt.",
    "05_HARD_OFF": "Physical switch OFF grounds gate EN. Charger and gauge are upstream. UV restart/thermal tests remain OPEN.",
    "06_3V3_RAIL": "TPS63802 3V3/2A individual target; aggregate output budget 8W, not both rails at maximum.",
    "07_5V_RAIL": "TPS61022 BOOST, not buck; 5V/1.5A individual target. No rail derives directly from USB VBUS.",
    "08_HOST_INTERFACE": "All host pullups use switched 3V3. Private PD service bus must NOT join the host bus.",
}


def add(sheet, kind, ref, value, nets, mpn="", dnp=False, note=""):
    assert set(nets) == {p[0] for p in PINS[kind]}, (ref, nets)
    PARTS.append(dict(sheet=sheet, kind=kind, ref=ref, value=value, nets=nets,
                      mpn=mpn, dnp=dnp, note=note))


def passive(sheet, kind, ref, value, a, b, dnp=False, note=""):
    add(sheet, kind, ref, value, {"1": a, "2": b}, dnp=dnp, note=note)


s = "01_USB_PD"
n = {p: None for p in usb}
for p in ("A1", "A12", "B1", "B12", "SH"):
    n[p] = "GND"
for p in ("A4", "A9", "B4", "B9"):
    n[p] = "USB_VBUS_RAW"
n.update(A5="CC1", B5="CC2")
add(s, "USB_C", "J101", "USB-C POWER ONLY", n, note="Connector MPN/pad map pending; D+/D-/SBU unused.")
add(s, "STUSB4500", "U101", "STUSB4500QTR", {
    "24":"USB_FUSED", "22":"GND", "1":"CC1", "2":"CC1", "4":"CC2", "5":"CC2",
    "6":"GND", "12":"GND", "13":"GND", "10":"GND", "25":"GND",
    "18":"PD_VBUS_SENSE", "23":"PD_2V7", "21":"PD_1V2", "16":"PD_SINK_N",
    "20":"PD_9V_N", "14":"PD_9V_N", "9":"PD_DISCH", "7":"PD_SCL", "8":"PD_SDA",
    "19":None, "11":None, "15":None, "17":None, "3":None}, "STUSB4500QTR")
for kind, ref, value, a, b in [
    ("FUSE","F101","2A 32V FAST / REVIEW","USB_VBUS_RAW","USB_FUSED"),
    ("TVS","D101","VBUS TVS 12V / REVIEW","USB_FUSED","GND"),
    ("TVS","D102","CC ESD 5V / REVIEW","CC1","GND"),
    ("TVS","D103","CC ESD 5V / REVIEW","CC2","GND"),
    ("C","C101","1uF 35V","USB_FUSED","GND"),
    ("C","C102","1uF 10V","PD_2V7","GND"),
    ("C","C103","1uF 10V","PD_1V2","GND"),
    ("R","R101","1k 1%","USB_FUSED","PD_VBUS_SENSE"),
    ("R","R102","1k 0.5W pulse","USB_SAFE","PD_DISCH"),
]: passive(s,kind,ref,value,a,b)
add(s,"CONN4","J102","PD NVM SERVICE", {"1":"PD_SCL","2":"PD_SDA","3":"GND","4":"USB_FUSED"},
    note="Pin4 is USB 5/9V, NOT logic supply. Fixture provides separate 3.3V pullups; USB must be present.")
for ref, net, why in [("#FLG101","USB_VBUS_RAW","External USB source"),
                       ("#FLG102","USB_FUSED","USB source through populated F101"),
                       ("#FLG103","GND","External return connected to USB and shunt")]:
    add(s,"PWR_FLAG",ref,net,{"1":net},note=why)

s = "02_INPUT_PROTECTION"
add(s,"TPS259474","U201","TPS259474LRPWR", {"5":"USB_FUSED","1":"USB_EN","2":"USB_OV",
    "9":"USB_ILM","8":"GND","6":"USB_SAFE","3":None,"4":"GND","7":"USB_DVDT","10":None},"TPS259474LRPWR")
add(s,"PMOS","Q201","BSS84 signal only", {"1":"USB_EN_GATE","2":"USB_FUSED","3":"USB_ENABLE_TOP"},"BSS84")
for kind,ref,value,a,b in [
    ("R","R201","100k 1%","USB_FUSED","USB_EN_GATE"),
    ("R","R202","100k 1%","USB_EN_GATE","PD_SINK_N"),
    ("R","R203","562k 0.1%","USB_ENABLE_TOP","USB_EN"),
    ("R","R204","205k 0.1%","USB_EN","GND"),
    ("R","R205","768k 0.1%","USB_FUSED","USB_OV"),
    ("R","R206","100k 0.1%","USB_OV","GND"),
    ("R","R207","1.65k 1%","USB_ILM","GND"),
    ("C","C201","4.7nF 50V","USB_DVDT","GND"),
    ("C","C202","10uF 25V","USB_SAFE","GND"),
]: passive(s,kind,ref,value,a,b)

s = "03_CHARGER_NVDC"
add(s,"BQ25638","U301","BQ25638YBGR", {
    "A1":"USB_SAFE","B1":"USB_SAFE","A2":"CHG_PMID","B2":"CHG_PMID","A5":"CHG_REGN",
    "F3":"CHG_ILIM","F2":"CHG_CE_N","F4":"CHG_TS_BIAS","E4":"CHG_TS","C5":"GND","D4":None,
    "A4":"GND","B4":"GND","C4":"GND","F1":"BAT_SENSE",
    "A3":"CHG_SW","B3":"CHG_SW","C3":"CHG_SW","B5":"CHG_BTST",
    "D1":"SYS_AON","D2":"SYS_AON","D3":"SYS_AON","E1":"PACK_POS","E2":"PACK_POS","E3":"PACK_POS",
    "C1":"CHG_STAT_N","C2":"CHG_PG_N","F5":"CHG_INT_N","D5":"HOST_SCL","E5":"HOST_SDA"},"BQ25638YBGR")
add(s,"PNP","Q301","MMBT3906", {"1":"PD_ENABLE_B","2":"CHG_REGN","3":"ILIM_HIGH_EN"},"MMBT3906")
add(s,"NMOS","Q302","2N7002", {"1":"ILIM_HIGH_EN","2":"GND","3":"ILIM_HIGH_RETURN"},"2N7002")
for kind,ref,value,a,b in [
    ("C","C301","1uF 25V","USB_SAFE","GND"),
    ("C","C302","10uF 25V","CHG_PMID","GND"),
    ("C","C303","100nF 25V","CHG_PMID","GND"),
    ("C","C304","47nF 16V","CHG_BTST","CHG_SW"),
    ("C","C305","4.7uF 10V","CHG_REGN","GND"),
    ("L","L301","1uH Isat>=8A","CHG_SW","SYS_AON"),
    ("C","C306","22uF 10V","SYS_AON","GND"),
    ("C","C307","22uF 10V","SYS_AON","GND"),
    ("C","C308","10uF 10V","PACK_POS","GND"),
    ("R","R301","100k 1%","CHG_REGN","CHG_CE_N"),
    ("R","R303","0R TS SAFETY LOCK","CHG_TS","GND"),
    ("R","R304","100R 1%","PACK_POS","BAT_SENSE"),
    ("R","R305","47k 1%","CHG_ILIM","GND"),
    ("R","R306","2.43k 1%","CHG_ILIM","ILIM_HIGH_RETURN"),
    ("R","R307","100k 1%","CHG_REGN","PD_ENABLE_B"),
    ("R","R308","100k 1%","PD_ENABLE_B","PD_9V_N"),
    ("R","R309","100k 1%","ILIM_HIGH_EN","GND"),
    ("R","R310","1.2k 1%","CHG_ILIM","ILIM_FILTER"),
    ("C","C309","330nF 10V","ILIM_FILTER","GND"),
    ("C","C310","100nF 10V","CHG_TS_BIAS","GND"),
]: passive(s,kind,ref,value,a,b)
passive(s,"R","R302","0R CHARGE ARM DNP","CHG_CE_N","GND",True,
        "Do not populate until cell voltage/temperature/NTC/timer approved and R303 replaced by real TS network.")

s = "04_BATTERY_GAUGE"
add(s,"CONN2","J401","PROTECTED 1S PACK", {"1":"PACK_POS","2":"PACK_NEG"},note="Actual connector/wire polarity and current rating OPEN.")
add(s,"KELVIN","R401","5m 1% >=0.5W 4T", {"1":"PACK_NEG","3":"PACK_NEG","2":"GND","4":"GND"},
    note="Logical Kelvin pads; physical part/pad mapping not released. Exactly one main return shunt.")
add(s,"MAX17055","U401","MAX17055ETB+T", {"10":"PACK_POS","6":"PACK_NEG","5":"GND","2":"PACK_NEG",
    "11":"PACK_NEG","7":"FG_REG","3":"HOST_SCL","4":"HOST_SDA","8":"FG_ALRT_N","9":None,"1":None},"MAX17055ETB+T")
passive(s,"C","C401","100nF 10V","PACK_POS","PACK_NEG")
passive(s,"C","C402","470nF 10V","FG_REG","PACK_NEG")
add(s,"PWR_FLAG","#FLG401","PACK_POS",{"1":"PACK_POS"},note="External protected pack positive; USB charger also drives it.")
add(s,"PWR_FLAG","#FLG402","PACK_NEG",{"1":"PACK_NEG"},note="External protected pack return; no bypass conductor added by flag.")

s = "05_HARD_OFF"
add(s,"TPS22953","U501","TPS22953DQCR", {"1":"SYS_AON","2":"SYS_AON","3":"SYS_AON","4":"SYSTEM_EN",
    "5":"GND","11":"GND","9":"SYS_SW","10":"SYS_SW","8":"SYS_SW","7":"GATE_PG_N","6":"GATE_CT"},"TPS22953DQCR")
add(s,"TPS3808","U502","TPS3808G01DBVR", {"6":"SYS_AON","5":"UV_SENSE","3":"SYS_AON","1":"UV_PERMIT","4":None,"2":"GND"},"TPS3808G01DBVR")
add(s,"SW_SPDT","SW501","SYSTEM ON / OFF", {"1":"SYSTEM_EN","2":"UV_PERMIT","3":"GND"},note="OFF physically grounds EN. Switch carries control current only.")
for kind,ref,value,a,b in [
    ("R","R501","2.87M 0.1%","SYS_AON","UV_SENSE"),
    ("R","R502","402k 0.1%","UV_SENSE","GND"),
    ("R","R503","470k 1%","SYS_AON","UV_PERMIT"),
    ("R","R504","4.7M 1%","SYSTEM_EN","GND"),
    ("C","C501","1uF 10V","SYS_AON","GND"),
    ("C","C502","100nF 10V","SYS_AON","GND"),
    ("C","C503","10nF 50V","GATE_CT","GND"),
    ("C","C504","22uF 10V","SYS_SW","GND"),
    ("R","R505","10k bleed","SYS_SW","GND"),
    ("R","R506","100k PG probe","SYS_SW","GATE_PG_N"),
]: passive(s,kind,ref,value,a,b)

s = "06_3V3_RAIL"
add(s,"TPS63802","U601","TPS63802DLAR", {"10":"SYS_SW","1":"SYS_SW","2":"GND","3":"GND","8":"GND",
    "9":"SW3_A","7":"SW3_B","6":"3V3_MAIN","4":"FB3","5":"PG3_N"},"TPS63802DLAR")
for kind,ref,value,a,b in [
    ("L","L601","0.47uH Isat>=5A","SW3_A","SW3_B"),
    ("C","C601","10uF 10V","SYS_SW","GND"),
    ("C","C602","22uF 10V","3V3_MAIN","GND"),
    ("C","C603","22uF 10V","3V3_MAIN","GND"),
    ("R","R601","511k 0.1%","3V3_MAIN","FB3"),
    ("R","R602","91k 0.1%","FB3","GND"),
    ("R","R603","100k 1%","3V3_MAIN","PG3_N"),
    ("L","FB601","600R@100MHz / >=0.5A","3V3_MAIN","3V3_SENS"),
    ("C","C604","4.7uF 10V","3V3_SENS","GND"),
    ("C","C605","100nF 10V","3V3_SENS","GND"),
    ("R","R604","10k bleed","3V3_MAIN","GND"),
]: passive(s,kind,ref,value,a,b)

s = "07_5V_RAIL"
add(s,"TPS61022","U701","TPS61022RWUR", {"7":"SYS_SW","5":"SYS_SW","6":"GND","1":"GND",
    "2":"SW5","3":"5V_MAIN","4":"FB5"},"TPS61022RWUR")
for kind,ref,value,a,b in [
    ("L","L701","1uH Isat>=10A","SYS_SW","SW5"),
    ("C","C701","10uF 10V","SYS_SW","GND"),
    ("C","C702","22uF 10V","5V_MAIN","GND"),
    ("C","C703","22uF 10V","5V_MAIN","GND"),
    ("C","C704","22uF 10V","5V_MAIN","GND"),
    ("R","R701","732k 0.1%","5V_MAIN","FB5"),
    ("R","R702","100k 0.1%","FB5","GND"),
    ("C","C705","110pF C0G","5V_MAIN","FB5"),
    ("R","R703","10k bleed","5V_MAIN","GND"),
]: passive(s,kind,ref,value,a,b)

s = "08_HOST_INTERFACE"
add(s,"CONN12","J801","TERMINAL / LOGICAL", {
    "1":"3V3_MAIN","2":"GND","3":"5V_MAIN","4":"GND","5":"3V3_SENS","6":"GND",
    "7":"HOST_SCL","8":"HOST_SDA","9":"FG_ALRT_N","10":"CHG_PG_N","11":"CHG_STAT_N","12":"CHG_INT_N"},
    note="No external drive when OFF; connector/current ratings and mainboard GPIO assignment OPEN.")
for index,net in enumerate(("HOST_SCL","HOST_SDA","FG_ALRT_N","CHG_PG_N","CHG_STAT_N","CHG_INT_N"),801):
    passive(s,"R","R"+str(index),"10k 1%","3V3_MAIN",net)
add(s,"CONN4","J802","RAIL TEST / LOGICAL", {"1":"3V3_MAIN","2":"5V_MAIN","3":"SYS_SW","4":"GND"})

PROFILE = {
    "status":"LOGICAL_NVM_SPEC_NOT_A_PROGRAMMED_DEVICE",
    "device":"STUSB4500QTR", "POWER_OK_CFG":2, "POWER_ONLY_ABOVE_5V":0,
    "REQ_SRC_CURRENT":0, "PDO_NUMBER":3,
    "pdo":[{"index":1,"voltage_v":5,"current_a":0.1},
           {"index":2,"voltage_v":9,"current_a":2},
           {"index":3,"voltage_v":9,"current_a":3}],
    "required_before_power_test":["Program NVM with official tooling", "Read back and compare all fields",
        "Cold start with battery absent and main switch OFF", "Confirm no 12V/15V/20V request",
        "Confirm POWER_OK2/3 only follows matching PDO after PS_RDY", "Test detach, hard-reset and downgrade"],
}
