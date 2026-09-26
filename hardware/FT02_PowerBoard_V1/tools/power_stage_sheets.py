"""Pin-level power stages. These circuits do not qualify the upstream architecture."""


def ic_pins(left, right):
    pins = []
    for side, rows in ((-1, left), (1, right)):
        for i, (number, label, electrical) in enumerate(rows):
            pins.append((str(number), label, electrical, side * 25.4,
                         round(((len(rows) - 1) / 2 - i) * 5.08, 4), 0 if side < 0 else 180))
    return pins


PINS = {
    "TPS259474": ic_pins([
        (5, "IN", "power_in"), (1, "EN_UVLO", "input"), (2, "OVLO", "input"),
        (9, "ILM", "passive"), (8, "GND", "power_in"),
    ], [(6, "OUT", "power_out"), (3, "PG", "open_collector"), (4, "PGTH", "input"),
        (7, "DVDT", "passive"), (10, "ITIMER", "passive")]),
    "TPS2121": ic_pins([
        (7, "IN1", "power_in"), (2, "IN2", "power_in"), (6, "PR1", "input"),
        (5, "OV1", "input"), (4, "OV2", "input"), (3, "CP2", "input"),
    ], [(1, "OUT", "power_out"), (8, "OUT", "passive"), (9, "ST", "open_collector"),
        (10, "ILIM", "passive"), (11, "SS", "passive"), (12, "GND", "power_in")]),
    "TPS3808G01DBV": ic_pins([
        (6, "VDD", "power_in"), (5, "SENSE", "input"), (3, "MR_N", "input"),
    ], [(1, "RESET_N", "open_collector"), (4, "CT", "passive"), (2, "GND", "power_in")]),
    "SN74LVC1G08DBV": ic_pins([
        (1, "A", "input"), (2, "B", "input"), (5, "VCC", "power_in"),
    ], [(4, "Y", "output"), (3, "GND", "power_in")]),
    "BQ25628E": ic_pins([
        (18, "VBUS", "power_in"), (17, "PMID", "power_out"), (2, "REGN", "power_out"),
        (4, "ILIM", "passive"), (5, "TS_BIAS", "power_out"), (6, "TS", "input"),
        (7, "QON", "input"), (14, "CE_N", "input"), (15, "GND", "power_in"),
    ], [
        (1, "BTST", "passive"), (16, "SW", "power_out"), (9, "SYS", "power_out"),
        (8, "BAT", "power_out"), (3, "PG_N", "open_collector"), (10, "STAT_N", "open_collector"),
        (11, "INT_N", "open_collector"), (12, "SDA", "bidirectional"), (13, "SCL", "input"),
    ]),
    "LT3120": ic_pins([
        (17, "VIN", "power_in"), (19, "PVIN", "power_in"), (20, "PVIN", "power_in"),
        (21, "PVIN", "power_in"), (16, "EN_UVLO", "input"), (15, "SYNC_MODE", "input"),
        (13, "VCC", "power_out"), (12, "MPPC", "input"), (8, "PROG", "passive"),
        (14, "RT", "passive"), (23, "BST1", "passive"),
        (24, "SW1", "bidirectional"), (25, "SW1", "bidirectional"),
        (3, "GND", "power_in"), (11, "GND", "power_in"),
    ], [
        (2, "PVOUT", "power_out"), (4, "PVOUT", "passive"),
        (5, "ISP", "input"), (6, "ISN", "input"), (1, "BST3", "passive"),
        (7, "PGOOD", "open_collector"), (9, "FB", "input"), (10, "VC", "passive"),
        (28, "BST2", "passive"), (26, "SW2", "bidirectional"), (27, "SW2", "bidirectional"),
        (18, "GND", "power_in"), (22, "GND", "power_in"), (29, "EP_GND", "power_in"),
    ]),
    "TCA9539PW": ic_pins([
        (24, "VCC", "power_in"), (12, "GND", "power_in"), (23, "SDA", "bidirectional"),
        (22, "SCL", "input"), (1, "INT_N", "open_collector"), (3, "RESET_N", "input"),
        (21, "A0", "input"), (2, "A1", "input"),
        (4, "P00", "bidirectional"), (5, "P01", "bidirectional"),
        (6, "P02", "bidirectional"), (7, "P03", "bidirectional"),
    ], [(pin, "P" + port, "bidirectional") for pin, port in
        [(8, "04"), (9, "05"), (10, "06"), (11, "07"), (13, "10"), (14, "11"),
         (15, "12"), (16, "13"), (17, "14"), (18, "15"), (19, "16"), (20, "17")]]),
    "CONN12": ic_pins([(i, str(i), "passive") for i in range(1, 13)], []),
    "CONN2": ic_pins([(1, "1", "passive"), (2, "2", "passive")], []),
    "2N7002": ic_pins([(1, "G", "input")], [(3, "D", "passive"), (2, "S", "passive")]),
}

NOTES = {
    "02_EXT_INPUT": "CANDIDATE WIRED: regulated EXT_5V ONLY; TPS259474L UV/OV/circuit breaker. Transient/source window OPEN.",
    "04_BATTERY_CHARGER": "WIRED / CHARGE LOCKED OFF: local SYS only. R407 DNP and TS forced fault; source-priority controls OPEN.",
    "07_UPS_POWER_MUX": "CANDIDATE WIRED / UNFROZEN: TPS2121 and rail UVLO. Source qualification and UPS timing NOT proven.",
    "09_5V_BUCKBOOST": "WIRED / UNQUALIFIED: LT3120 5V 500kHz application; default OFF, gated by rail UVLO. Load margins OPEN.",
    "10_SENSOR_POWER": "WIRED: 3V3_MAIN ferrite branch, decoupling and RC damping provision. EMI/load qualification OPEN.",
    "11_MAINBOARD_INTERFACE": "PARTIAL: logical connector and TCA9539 0x74 status I/O. No new MCU. Source-policy integration OPEN.",
}


def build(name, component, text_item):
    items = []
    kinds = set()

    def part(kind, ref, value, x, y, nets, mpn="", dnp=False, datasheet=""):
        kinds.add(kind)
        items.extend(component(name, kind, ref, value, x, y, nets, mpn, dnp, datasheet))

    def passive(kind, ref, value, x, y, hi, lo, mpn="", dnp=False):
        part(kind, ref, value, x, y, {"1": hi, "2": lo}, mpn, dnp)

    def notes(lines):
        for i, text in enumerate(lines):
            items.append(text_item(text, 12.7, 246.38 + 6.35*i, name + "/boundary/" + str(i), 1.016))

    if name == "02_EXT_INPUT":
        part("CONN2", "J201", "REGULATED EXT_5V", 96.52, 71.12, {"1": "EXT_5V", "2": "GND"})
        part("TPS259474", "U201", "TPS259474LRPWR CANDIDATE", 109.22, 144.78, {
            "5": "EXT_5V", "1": "EXT_UV_SENSE", "2": "EXT_OV_SENSE", "9": "EXT_ILM",
            "8": "GND", "6": "EXT_5V_SAFE", "3": None, "4": "GND", "7": "EXT_DVDT", "10": None,
        }, "TPS259474LRPWR", datasheet="https://www.ti.com/lit/ds/symlink/tps25947.pdf")
        rows = [
            ("C", "C201", "1uF 50V", "EXT_5V", "GND"),
            ("C", "C202", "22uF 10V", "EXT_5V_SAFE", "GND"),
            ("C", "C203", "4.7nF 50V REF", "EXT_DVDT", "GND"),
            ("R", "R201", "562k 0.1%", "EXT_5V", "EXT_UV_SENSE"),
            ("R", "R202", "205k 0.1%", "EXT_UV_SENSE", "GND"),
            ("R", "R203", "562k 0.1%", "EXT_5V", "EXT_OV_SENSE"),
            ("R", "R204", "165k 0.1%", "EXT_OV_SENSE", "GND"),
            ("R", "R205", "1.65k 1% REF", "EXT_ILM", "GND"),
        ]
        for i, (kind, ref, value, hi, lo) in enumerate(rows):
            passive(kind, ref, value, 210.82+(i%3)*66.04, 50.8+(i//3)*50.8, hi, lo)
        notes([
            "IN=regulated external5V; OUT=EXT_5V_SAFE for future03 mux. No raw solar. Connector is logical, not released.",
            "L variant latches overcurrent; UV recovery remains automatic. ITIMER NC selects fastest response, NOT a hard peak cap.",
            "PG unused/NC,PGTH grounded. R201/R203 limit control-pin reverse current below10uA for a -5.5V test assumption.",
            "Static OVP does NOT qualify5V+/-5%, hotplug overshoot or downstream5.5V safety. TVS/negative-clamp/thermal gates OPEN.",
        ])
    elif name == "04_BATTERY_CHARGER":
        part("BQ25628E", "U401", "BQ25628ERYKR", 109.22, 93.98, {
            "18": "EXT_BUS_5V", "17": "CHG_PMID", "2": "CHG_REGN", "4": "CHG_ILIM",
            "5": "CHG_TS_BIAS", "6": "CHG_TS", "7": None, "14": "CHG_CE_N", "15": "GND",
            "1": "CHG_BTST", "16": "CHG_SW", "9": "CHG_SYS_LOCAL", "8": "BAT_PROTECTED",
            "3": "CHG_PG_N", "10": "CHG_STAT_N", "11": "CHG_INT_N", "12": "SDA", "13": "SCL",
        }, "BQ25628ERYKR", datasheet="https://www.ti.com/lit/ds/symlink/bq25628e.pdf")
        rows = [
            ("C", "C401", "1uF 10V", "EXT_BUS_5V", "GND"),
            ("C", "C402", "10uF 10V", "CHG_PMID", "GND"),
            ("C", "C403", "100nF 10V", "CHG_PMID", "GND"),
            ("C", "C404", "47nF 10V", "CHG_BTST", "CHG_SW"),
            ("C", "C405", "4.7uF 10V", "CHG_REGN", "GND"),
            ("L", "L401", "1uH REF", "CHG_SW", "CHG_SYS_LOCAL"),
            ("C", "C406", "22uF 10V", "CHG_SYS_LOCAL", "GND"),
            ("C", "C407", "10uF 10V", "BAT_PROTECTED", "GND"),
            ("R", "R401", "2.80k 1%", "CHG_ILIM", "GND"),
            ("R", "R402", "100k 1%", "EXT_BUS_5V", "CHG_CE_N"),
            ("R", "R403", "10k 1%", "3V3_MAIN", "CHG_PG_N"),
            ("R", "R404", "10k 1%", "3V3_MAIN", "CHG_STAT_N"),
            ("R", "R405", "10k 1%", "3V3_MAIN", "CHG_INT_N"),
            ("R", "R406", "100k 1%", "CHG_REQUEST", "GND"),
            ("R", "R407", "0 ARM DNP", "CHG_CE_N", "CHG_CE_DRAIN"),
            ("R", "R408", "0 TS LOCK", "CHG_TS", "GND"),
            ("C", "C408", "100nF 10V", "CHG_TS_BIAS", "GND"),
        ]
        for i, (kind, ref, value, hi, lo) in enumerate(rows):
            if i < 12:
                x, y = 210.82 + (i % 3)*66.04, 50.8 + (i//3)*43.18
            else:
                x, y = 30.48 + (i-12)*71.12, 218.44
            passive(kind, ref, value, x, y, hi, lo, dnp=ref == "R407")
        part("2N7002", "Q401", "2N7002", 96.52, 175.26,
             {"1": "CHG_REQUEST", "2": "GND", "3": "CHG_CE_DRAIN"}, "2N7002,215",
             datasheet="https://assets.nexperia.com/documents/data-sheet/2N7002.pdf")
        notes([
            "QON intentionally NC: internal pullup, no ship/reset button. SYS_LOCAL has NO FT02 system or debug loads.",
            "CE pullup uses protected external input; R407 MUST remain DNP. TS=GND forces fault; this is NOT an NTC substitute.",
            "R401 sets ~0.893A nominal branch input limit, NOT whole-board current. No charge profile/temperature approval.",
            "Charger input leakage/reverse paths and hardware charge shedding remain review gates before enabling R407.",
        ])
    elif name == "07_UPS_POWER_MUX":
        part("TPS2121", "U701", "TPS2121RUXR UNFROZEN", 109.22, 71.12, {
            "7": "EXT_BUS_5V", "2": "BAT_PROTECTED", "6": "MUX_EXT_SENSE",
            "5": "GND", "4": "MUX_EXT_SENSE", "3": "GND", "1": "SYS_RAW",
            "8": "SYS_RAW", "9": "MUX_BAT_STATUS", "10": "MUX_ILIM", "11": "MUX_SS", "12": "GND",
        }, "TPS2121RUXR", datasheet="https://www.ti.com/lit/ds/symlink/tps2121.pdf")
        part("TPS3808G01DBV", "U702", "TPS3808G01DBVR", 109.22, 144.78, {
            "6": "SYS_RAW", "5": "SYS_UV_SENSE", "3": "SYS_RAW", "1": "RAIL_PERMIT",
            "4": None, "2": "GND",
        }, "TPS3808G01DBVR", datasheet="https://www.ti.com/lit/ds/symlink/tps3808.pdf")
        rows = [
            ("C", "C701", "1uF 10V", "EXT_BUS_5V", "GND"),
            ("C", "C702", "1uF 10V", "BAT_PROTECTED", "GND"),
            ("C", "C703", "220uF 10V REF", "SYS_RAW", "GND"),
            ("R", "R701", "30.9k 0.1%", "EXT_BUS_5V", "MUX_EXT_SENSE"),
            ("R", "R702", "10k 0.1%", "MUX_EXT_SENSE", "GND"),
            ("R", "R703", "29.8k 1% REF", "MUX_ILIM", "GND"),
            ("C", "C704", "1nF REF", "MUX_SS", "GND"),
            ("R", "R704", "100k 1%", "3V3_MAIN", "MUX_BAT_STATUS"),
            ("C", "C705", "100nF 10V", "SYS_RAW", "GND"),
            ("R", "R705", "715k 0.1%", "SYS_RAW", "SYS_UV_SENSE"),
            ("R", "R706", "100k 0.1%", "SYS_UV_SENSE", "GND"),
            ("R", "R707", "10k 1%", "SYS_RAW", "RAIL_PERMIT"),
        ]
        for i, (kind, ref, value, hi, lo) in enumerate(rows):
            passive(kind, ref, value, 210.82+(i%3)*66.04, 50.8+(i//3)*43.18, hi, lo)
        notes([
            "IN1=external,IN2=post-shunt battery. PR1/OV2 shared divider disables battery above external threshold (~4.34V).",
            "CP2=GND:100us TYP transition, NOT5us and no guaranteed maximum. Voltage-only sense is NOT qualified EXT_VALID.",
            "UVLO nominal SYS_RAW=3.30075V; CT NC=20ms typical recovery. This is NOT the pack cutoff or loaded-voltage spec.",
            "ILIM reference=3/3.5/4A at29.8k (resistor tolerance additional). Load, thermal, rebound and hold-up gates OPEN.",
        ])
    elif name == "09_5V_BUCKBOOST":
        nets = {str(pin): "GND" for pin in (3, 11, 18, 22, 29)}
        nets.update({str(pin): "SYS_RAW" for pin in (17, 19, 20, 21)})
        nets.update({str(pin): "5V_MAIN" for pin in (2, 4, 5, 6)})
        nets.update({str(pin): "REG5_VCC" for pin in (8, 12, 13)})
        nets.update({str(pin): "REG5_SW1" for pin in (24, 25)})
        nets.update({str(pin): "REG5_SW2" for pin in (26, 27)})
        nets.update({"1": "REG5_BST3", "7": "PG_5V", "9": "FB_5V", "10": "REG5_VC",
                     "14": "REG5_RT", "15": "GND", "16": "REG5_ENABLE", "23": "REG5_BST1", "28": "REG5_BST2"})
        part("LT3120", "U901", "LT3120IUFDM", 109.22, 93.98, nets, "LT3120IUFDM#PBF",
             datasheet="https://www.analog.com/media/en/technical-documentation/data-sheets/lt3120.pdf")
        rows = [
            ("L", "L901", "4.7uH REF", "REG5_SW1", "REG5_SW2"),
            ("C", "C901", "10uF 10V", "SYS_RAW", "GND"),
            ("C", "C902", "220uF 10V", "5V_MAIN", "GND"),
            ("C", "C903", "220nF 10V", "REG5_BST1", "REG5_SW1"),
            ("C", "C904", "220nF 10V", "REG5_BST2", "REG5_SW2"),
            ("C", "C905", "22nF 10V", "REG5_BST3", "5V_MAIN"),
            ("C", "C906", "4.7uF 10V", "REG5_VCC", "GND"),
            ("R", "R901", "536k 1%", "5V_MAIN", "FB_5V"),
            ("R", "R902", "102k 1%", "FB_5V", "GND"),
            ("R", "R903", "158k 1%", "REG5_RT", "GND"),
            ("R", "R904", "46.4k 1%", "REG5_VC", "REG5_COMP"),
            ("C", "C907", "1800pF C0G", "REG5_COMP", "GND"),
            ("R", "R905", "100k 1%", "5V_EN", "GND"),
            ("R", "R906", "100k 1%", "3V3_MAIN", "PG_5V"),
            ("R", "R907", "10k 1%", "REG5_ENABLE", "GND"),
            ("C", "C908", "100nF 10V", "3V3_MAIN", "GND"),
        ]
        for i, (kind, ref, value, hi, lo) in enumerate(rows):
            x, y = (210.82+(i%3)*66.04, 50.8+(i//3)*43.18) if i < 12 else (45.72+(i-12)*81.28, 218.44)
            passive(kind, ref, value, x, y, hi, lo)
        part("SN74LVC1G08DBV", "U902", "SN74LVC1G08DBVR", 109.22, 167.64,
             {"1": "RAIL_PERMIT", "2": "5V_EN", "5": "3V3_MAIN", "4": "REG5_ENABLE", "3": "GND"},
             "SN74LVC1G08DBVR", datasheet="https://www.ti.com/lit/ds/symlink/sn74lvc1g08.pdf")
        notes([
            "ADI page28 TA07 topology,500kHz/Burst. U902 ANDs Core5V_EN with RAIL_PERMIT; output pulldown holds OFF when unpowered.",
            "ISP/ISN tied to PVOUT; PROG/MPPC tied to VCC. No programmed output current limit is claimed.",
            "Reference example at VIN=2.5V supplies1.25A. This is NOT a qualified FT02 5V/2A or simultaneous rail rating.",
            "U902 supply=3V3_MAIN; inputs tolerate5.5V. L/C selection, loop stability, UVLO timing and thermal qualification OPEN.",
        ])
    elif name == "10_SENSOR_POWER":
        passive("R", "FB1001", "220R at100MHz", 60.96, 58.42, "3V3_MAIN", "3V3_SENS", "BLM21PG221SN1D")
        passive("C", "C1001", "100nF 10V", 152.4, 58.42, "3V3_SENS", "GND")
        passive("C", "C1002", "1uF 10V", 233.68, 58.42, "3V3_SENS", "GND")
        passive("R", "R1001", "1R 1%", 60.96, 116.84, "3V3_SENS", "SENS_DAMP")
        passive("C", "C1003", "10uF 10V", 152.4, 116.84, "SENS_DAMP", "GND")
        part("CONN2", "J1001", "SENSOR POWER", 109.22, 185.42,
             {"1": "3V3_SENS", "2": "GND"})
        notes([
            "Always-on filtered derivative of3V3_MAIN; no additional DC/DC. Optional load switch NOT fitted in this variant.",
            "Ferrite source: Murata BLM21PG221SN1D,2A nominal rating,DCR<=45mohm; not a branch current limiter.",
            "Series1R+10uF shunt branch is a damping provision. Actual bias-dependent impedance/load/EMI must be tested.",
            "J1001 is a logical interface, not a released connector choice. SEN0695 module retains its own local1.8V regulator.",
        ])
    elif name == "11_MAINBOARD_INTERFACE":
        nets = {"24": "3V3_MAIN", "12": "GND", "23": "SDA", "22": "SCL", "1": "PWR_INT",
                "3": "IO_RESET_N", "21": "GND", "2": "GND", "4": "PG_3V3", "5": "PG_5V",
                "6": "FG_GPOUT", "7": "CHG_PG_N", "8": "CHG_STAT_N", "9": "CHG_INT_N",
                "10": "CHG_REQUEST", "11": "MUX_BAT_STATUS"}
        nets.update({str(pin): "IO_UNUSED" + str(pin-3) for pin in range(13, 21)})
        part("TCA9539PW", "U1101", "TCA9539PWR", 109.22, 93.98, nets, "TCA9539PWR",
             datasheet="https://www.ti.com/lit/ds/symlink/tca9539.pdf")
        main = ["GND", "GND", "3V3_MAIN", "3V3_MAIN", "5V_MAIN", "5V_MAIN", "SDA", "SCL", "PWR_INT", "5V_EN", "3V3_SENS", "GND"]
        part("CONN12", "J1101", "MAINBOARD LOGICAL", 312.42, 93.98, {str(i+1): net for i, net in enumerate(main)})
        for i, (ref, value, hi, lo) in enumerate([
            ("R1101", "10k 1%", "3V3_MAIN", "PWR_INT"),
            ("R1102", "10k 1%", "3V3_MAIN", "IO_RESET_N"),
        ]):
            passive("R", ref, value, 45.72+i*81.28, 160.02, hi, lo)
        passive("C", "C1101", "100nF 10V", 208.28, 160.02, "3V3_MAIN", "GND")
        passive("C", "C1102", "1uF 10V", 289.56, 160.02, "3V3_MAIN", "GND")
        for i in range(8):
            passive("R", "R"+str(1103+i), "100k 1%", 45.72+(i%4)*81.28,
                    193.04+(i//4)*33.02, "IO_UNUSED"+str(10+i), "GND")
        notes([
            "A1=A0=0:0x74. At POR all ports input. CHG_REQUEST external pulldown and R407 DNP prevent unintended charge.",
            "P00/P01=rail PG;P02=gauge;P03..P05=charger status;P06=charge request;P07=MUX ST. P10..P17 have100k pulldowns.",
            "PWR_INT is expander interrupt only; gauge wake needs P02 drive-low then input release, NEVER drive-high.",
            "Connector/pullups/off-state behavior and source-policy I/O remain OPEN. No GPIO4 allocation; Core uses SDA47/SCL21.",
        ])
    else:
        raise ValueError(name)
    return items, sorted(kinds)
