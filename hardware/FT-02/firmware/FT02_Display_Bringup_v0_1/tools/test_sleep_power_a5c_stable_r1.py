#!/usr/bin/env python3
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def req(cond,msg):
    if not cond: raise SystemExit('FAIL: '+msg)

version=(ROOT/'VERSION.txt').read_text().strip()
build=(ROOT/'src/FT02_BuildInfo.h').read_text()
main=(ROOT/'src/main.cpp').read_text()
diag=(ROOT/'src/FT02_SleepDiagnostics.cpp').read_text()
hdr=(ROOT/'src/FT02_SleepDiagnostics.h').read_text()
selftest=(ROOT/'src/FT02_SystemSelfTest.cpp').read_text()
settings=(ROOT/'src/FT02_SystemSettingsUI.cpp').read_text()
compass=(ROOT/'src/CompassCalibration.cpp').read_text()

req(version=='v2.84-sleep-power-a5c-stable-r1.5','version')
req('SLEEP POWER A5C STABLE R1' in build,'stable feature label')
req('[SLEEP-POWER]' not in main,'serial sleep-power experiment log remains')
req('FT02_SleepTruthSetBatteryStart' in main and 'FT02_SleepTruthBeginWakeBatterySettle' in main and 'FT02_SleepTruthFeedWakeBatterySample' in main,'sleep battery boundary hooks')
req('resultBatteryStartValid' in diag and 'resultBatteryWakeValid' in diag,'RTC battery result storage')
req('batterySampleValid' in hdr,'battery snapshot surface')
req('addItem("休眠耗电"' in selftest,'sleep drain device-status row')
req('addItem("休眠统计"' in selftest,'sleep health device-status row')
req('%+.1f%%/h' not in selftest,'instant CRATE removed from device-status battery row')
req('if (!prefs.isKey(key)) return false;' in compass,'missing calibration key must not emit Preferences error')
req('FT02_LOGD("[EPD] backend=' in main,'EPD routine startup log not converged')
req('FT02_LOGD("[FT02] reset reason=' in main,'reset routine log not converged')
req('FT02_FIRMWARE_UI_VERSION' in build and 'v2.84 A5C R1.5' in build,'compact UI firmware identity missing')
req('FT02_FIRMWARE_UI_VERSION' in settings,'settings UI must use compact firmware identity')
req('FT02_FIRMWARE_VERSION)' not in settings,'settings UI must not render full firmware identity')
print('PASS A5C Stable R1 cleanup: sleep health retained, battery boundary telemetry retained, routine logs converged')
