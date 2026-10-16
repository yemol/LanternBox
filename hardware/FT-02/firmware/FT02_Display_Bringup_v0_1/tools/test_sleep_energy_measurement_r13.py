#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT/'src/main.cpp').read_text(encoding='utf-8')
diag = (ROOT/'src/FT02_SleepDiagnostics.cpp').read_text(encoding='utf-8')
hdr = (ROOT/'src/FT02_SleepDiagnostics.h').read_text(encoding='utf-8')
selftest = (ROOT/'src/FT02_SystemSelfTest.cpp').read_text(encoding='utf-8')
version = (ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()

def req(ok, msg):
    if not ok:
        raise SystemExit('FAIL: '+msg)

req(version == 'v2.84-sleep-power-a5c-stable-r1.5', 'version')
req('batteryBoundaryAgeMs <= 15000u' in main, 'sleep-start cache freshness guard missing')
req('FT02_SleepTruthSetBatteryWake(' not in main, 'immediate wake sample still booked as endpoint')
req('FT02_SleepTruthBeginWakeBatterySettle();' in main, 'wake settle collector not started')
req('FT02_SleepTruthFeedWakeBatterySample(' in main, 'background battery poll does not feed collector')
req('FT02_WAKE_BATTERY_SAMPLE_TARGET = 3u' in diag, 'three-sample endpoint target missing')
req('median3(' in diag, 'median endpoint filter missing')
req('batteryWakeSettling' in hdr and 'batteryWakeSampleCount' in hdr, 'settle status not surfaced')
req('hours < 2.0' in selftest and '短时参考' in selftest, 'short-window rate suppression missing')
req('Δ%+ldmV' in selftest, 'voltage delta missing')
block = re.search(r'void runSleepBattery\(\).*?\n}\n\nvoid runSleepTruth', selftest, re.S)
req(block is not None, 'sleep battery UI block missing')
req('FT02_SELFTEST_WARN' not in block.group(0) and 'FT02_SELFTEST_FAIL' not in block.group(0), 'SOC drain still affects system health verdict')
print('PASS R1.3 sleep energy measurement: fresh start + delayed 3-sample median endpoint + short-window guard')
