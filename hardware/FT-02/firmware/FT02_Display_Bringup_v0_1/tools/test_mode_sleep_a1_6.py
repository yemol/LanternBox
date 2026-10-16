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

req('FT02_FIRMWARE_VERSION "v2.84-sleep-power-a5c-stable-r1.5"' in build, "firmware identity mismatch")
req('FT02_FIRMWARE_FEATURE_LABEL "SLEEP POWER A5C STABLE R1.5 / SLEEP HEALTH / SD QUIESCE / GNSS STANDBY / SCL2PHASE RC3 EXT1"' in build, "feature label mismatch")

helper = main[main.index("static bool FT02_SleepWaitForKeyboardSignal()"):
              main.index("static bool FT02_SleepBlocked(", main.index("static bool FT02_SleepWaitForKeyboardSignal()"))]
req("Serial." not in helper, "sleep wait touches native USB/Serial")

req("FT02_EpdSignalsEnterHighImpedance" in main, "EPD sleep isolation missing")
req("FT02_EpdSignalsRestoreForWake" in main, "EPD wake restore missing")
req("digitalWrite(EPD_PWR, LOW)" in main, "EPD rail is not cut for sleep")
req("FT02_LR01Host" not in main, "retired LR01 lifecycle hook remains")
req("FT02_BatteryMonitorSleepPause()" in main and "FT02_BatteryMonitorWakeResume()" in main, "battery lifecycle incomplete")
req("Serial.end()" in main and "Serial.begin(115200)" in main, "USB CDC wake recovery missing")
req("FT02_SleepDiagnosticsMarkEnter" in main and "FT02_SleepDiagnosticsMarkKeyboardWake" in main, "persistent sleep breadcrumb missing")

req("bool FT02_ModeAutoSleepAllowed();" in mode_h, "mode auto-sleep API missing")
req("return g_currentMode == FT02_MODE_INDOOR;" in mode, "outdoor mode must remain manual-sleep only")
req("if(automatic && !FT02_ModeAutoSleepAllowed()) return;" in main, "automatic sleep lacks mode guard")
req("FT02_EnterSleepMode(false);" in main, "manual sleep path missing")
req("FT02_InputRequestHostSleep()" in main, "CardKB2 host-sleep command missing")
req("FT02_SleepWaitForKeyboardSignal" in main, "CardKB2 SCL wake wait missing")

# k92 deliberately retains the RTC_NOINIT sleep RCA trace because it was
# present in the last user-validated stable k89 runtime. Only later GNSS/Battery
# blackbox experiments must be absent.
req("g_ft02SleepNoInitTrace" in main and "SLEEP-RCA" in main and "RESET-TRACE" in main,
    "validated sleep RCA trace was removed")
for forbidden in [
    "GNSS-BB",
    "GNSS-WAKE",
    "BAT-WAKE",
]:
    req(forbidden not in main and forbidden not in sleep_diag, f"temporary wake diagnostic residue remains: {forbidden}")

req("ft02slpdbg" in sleep_diag and "ESP_RST_POWERON" in sleep_diag, "A5C sleep truth reset boundary missing")
req("Sleep Power A4" in readme, "Sleep Power A4 is undocumented")

req("FT02_InputRequestHostSleep()" in main, "CardKB2 host-sleep command missing")
req("esp_sleep_enable_ext1_wakeup_io" in main and "ESP_EXT1_WAKEUP_ANY_LOW" in main, "SCL EXT1 wake missing")
req("gpio_wakeup_enable" not in main and "GPIO_INTR_LOW_LEVEL" not in main, "legacy digital SCL wake remains in main")
req("FT02_InputSuspendBusForSleep()" in main and "FT02_InputResumeBusAfterSleep" in main, "I2C sleep bridge missing")
req("esp_sleep_enable_timer_wakeup" not in main, "periodic timer wake must be removed")
print("PASS Mode/Sleep A1.6 + Sleep Power A4 keyboard-wake lifecycle")
