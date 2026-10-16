from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
selftest = (ROOT / "src/FT02_SystemSelfTest.cpp").read_text()
header = (ROOT / "src/FT02_SystemSelfTest.h").read_text()
version = (ROOT / "VERSION.txt").read_text().strip()

def req(cond, msg):
    if not cond:
        raise SystemExit("FAIL: " + msg)

req(version == "v2.84-sleep-power-a5c-stable-r1.5", "version")
req("FT02_SELFTEST_PAGE_COUNT = 4" in selftest, "four-page device status")
req("FT02_SELFTEST_POWER_PAGE = 3" in selftest, "dedicated power page index")
req("FT02_SELFTEST_PAGE_STARTS[FT02_SELFTEST_PAGE_COUNT] = {0, 6, 12, 19}" in selftest, "page starts")
req("FT02_SELFTEST_PAGE_LENGTHS[FT02_SELFTEST_PAGE_COUNT] = {6, 6, 7, 3}" in selftest, "page lengths")
req("MAX_ITEMS = 24" in header, "report capacity")
req("设备状态  电源与休眠" in selftest, "power page title")
req("当前电池" in selftest and "上次休眠耗电" in selftest and "休眠统计" in selftest, "power page sections")
req("电压 %.3f → %.3fV" in selftest, "voltage split line")
req("SOC估算 %.0f → %.0f%%" in selftest, "soc split line")
req("窗口 %.2fh    实睡 %.2fh    占比 %.1f%%" in selftest, "sleep truth line 1")
req("清醒 %.1fmin    会话%lu    LS%lu    异常%lu" in selftest, "sleep truth line 2")
req("drawPowerSleepPage(display)" in selftest, "specialized renderer")
req("runBatteryCached();\n    runSleepBattery();\n    runSleepTruth();" in selftest, "passive battery group ordering")
req("runBattery();\n    runSleepBattery();\n    runSleepTruth();" in selftest, "active battery group ordering")
print("PASS R1.5 dedicated power/sleep Device Status page")
