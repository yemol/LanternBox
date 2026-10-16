from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
diag=(root/'src/FT02_SleepDiagnostics.cpp').read_text()
hdr=(root/'src/FT02_SleepDiagnostics.h').read_text()
selftest=(root/'src/FT02_SystemSelfTest.cpp').read_text()
ver=(root/'VERSION.txt').read_text().strip()
assert ver=='v2.84-sleep-power-a5c-stable-r1.5'
for sym in (
    'FT02_SleepTruthSessionEnter',
    'FT02_SleepTruthBeforeLightSleep',
    'FT02_SleepTruthAfterLightSleep',
    'FT02_SleepTruthSetBatteryStart',
    'FT02_SleepTruthBeginWakeBatterySettle',
    'FT02_SleepTruthFeedWakeBatterySample',
    'FT02_SleepTruthMarkKeyboardWake',
    'FT02_SleepTruthMarkUnexpectedReturn',
    'FT02_SleepTruthSnapshotCurrent',
):
    assert sym in hdr and sym in diag
    assert sym in main or sym == 'FT02_SleepTruthSnapshotCurrent'
assert 'RTC_NOINIT_ATTR FT02SleepTruthRtc g_truth;' in diag
assert 'esp_timer_get_time()' in diag
assert 'FT02_SleepTruthBeforeLightSleep();' in main
assert main.index('FT02_SleepTruthBeforeLightSleep();') < main.index('esp_light_sleep_start();')
assert 'FT02_SleepTruthAfterLightSleep(' in main
assert 'FT02_SleepTruthSessionEnter(automatic);' in main
assert 'FT02_SleepTruthMarkKeyboardWake();' in main
assert 'FT02_SleepTruthMarkUnexpectedReturn();' in main
# Truth hot-path functions must be RTC/timer bookkeeping only: no Wire or Preferences.
start=diag.index('void FT02_SleepTruthSessionEnter(bool automatic)')
truth=diag[start:]
assert 'Wire.' not in truth and 'Wire->' not in truth
assert 'Preferences ' not in truth
assert 'FT02_BatteryMonitorRefreshNow' not in truth
# Device Status must expose the result without USB serial.
assert 'void runSleepTruth()' in selftest
assert 'addItem("休眠统计"' in selftest
assert 'truth.lightSleepUs' in selftest
assert 'truth.awakeUs' in selftest
# Current battery row stays compact; completed sleep gets its own boundary sample.
assert 'SOC估算 %.0f%% %.3fV %s' in selftest
assert 'void runSleepBattery()' in selftest
assert 'addItem("休眠耗电"' in selftest
assert 'batteryStartPercent' in hdr and 'batteryWakePercent' in hdr
assert 'FT02_SleepTruthSetBatteryStart(' in main
assert 'FT02_SleepTruthBeginWakeBatterySettle();' in main
assert 'FT02_SleepTruthFeedWakeBatterySample(' in main
print('Sleep Power A5C sleep truth diagnostic: PASS')
