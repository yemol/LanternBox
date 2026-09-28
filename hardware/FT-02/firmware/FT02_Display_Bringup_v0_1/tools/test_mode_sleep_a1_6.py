#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def text(rel):
    return (ROOT / rel).read_text(encoding="utf-8")

def req(ok, msg):
    if not ok:
        raise SystemExit("FAIL: " + msg)

main = text("src/main.cpp")
mode = text("src/FT02_ModeManager.cpp")
mode_h = text("src/FT02_ModeManager.h")
build = text("src/FT02_BuildInfo.h")
sleep_diag = text("src/FT02_SleepDiagnostics.cpp")
readme = text("README.md")

req('FT02_FIRMWARE_VERSION "v2.75k91"' in build, "firmware identity is not v2.75k91")
req('FT02_FIRMWARE_FEATURE_LABEL "Mode Sleep System A1.6"' in build, "feature label is not A1.6")
req("FT02_SLEEP_LIGHT_SLICE_MS = 1000u" in main, "sleep slice is not 1000 ms")

helper = main[main.index("static void FT02_SleepIdleSlice()"):
              main.index("static bool FT02_AutoSleepBlocked()", main.index("static void FT02_SleepIdleSlice()"))]
req("esp_sleep_enable_timer_wakeup" in helper and "esp_light_sleep_start()" in helper, "timer light sleep missing")
req("Serial." not in helper, "sleep slice touches native USB/Serial")

req("FT02_EpdSignalsEnterHighImpedance" in main, "EPD sleep isolation missing")
req("FT02_EpdSignalsRestoreForWake" in main, "EPD wake restore missing")
req("digitalWrite(EPD_PWR, LOW)" in main, "EPD rail is not cut for sleep")
req("FT02_LR01HostSleepPause()" in main and "FT02_LR01HostSleepResume()" in main, "LR01 lifecycle incomplete")
req("FT02_BatteryMonitorSleepPause()" in main and "FT02_BatteryMonitorWakeResume()" in main, "battery lifecycle incomplete")
req("Serial.end()" in main and "Serial.begin(115200)" in main, "USB CDC wake recovery missing")
req("FT02_SleepDiagnosticsMarkEnter" in main and "FT02_SleepDiagnosticsMarkKeyboardWake" in main, "persistent sleep breadcrumb missing")

req("bool FT02_ModeAutoSleepAllowed();" in mode_h, "mode auto-sleep API missing")
req("return g_currentMode == FT02_MODE_INDOOR;" in mode, "outdoor mode must remain manual-sleep only")
req("if(automatic && !FT02_ModeAutoSleepAllowed()) return;" in main, "automatic sleep lacks mode guard")
req("FT02_EnterSleepMode(false);" in main, "manual sleep path missing")
req("FT02_IsPlausibleSleepWakeRaw" in main, "CardKB2 wake filtering missing")
req("FT02_SLEEP_WAKE_ARM_EMPTY_POLLS = 2u" in main, "wake release gate changed")

for forbidden in [
    "FT02_SLEEP_ISOLATION_TEST",
    "g_ft02SleepNoInitTrace",
    "SLEEP-RCA",
    "RESET-TRACE",
    "GNSS-BB",
    "GNSS-WAKE",
    "BAT-WAKE",
    "FT02_BLACKBOX_MAGIC",
]:
    req(forbidden not in main and forbidden not in sleep_diag, f"temporary diagnostic residue remains: {forbidden}")

req("ft02slpdbg" in sleep_diag and "ESP_RST" not in sleep_diag, "production sleep breadcrumb unexpectedly changed")
req("Deep power optimization" in readme, "deferred low-power boundary is undocumented")

print("PASS Mode/Sleep A1.6: stable lifecycle retained; temporary diagnostic scaffolding removed")
