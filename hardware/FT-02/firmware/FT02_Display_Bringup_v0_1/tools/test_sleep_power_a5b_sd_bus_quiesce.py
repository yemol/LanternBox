from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
cpp=(root/'lib/FT02Storage/src/FT02_Storage.cpp').read_text()
hdr=(root/'lib/FT02Storage/src/FT02_Storage.h').read_text()
ver=(root/'VERSION.txt').read_text().strip()
assert ver=='v2.84-sleep-power-a5c-stable-r1.1'
for sym in ('FT02_StorageSleepEnter','FT02_StorageSleepExit','FT02_StorageSleepSuspended'):
    assert sym in hdr and sym in cpp
assert 'g_ft02SdSpi.end();' in cpp
assert 'pinMode(FT02StorageConfig::SD_SCK, INPUT);' in cpp
assert 'digitalWrite(FT02StorageConfig::SD_CS, HIGH);' in cpp
assert 'esp_vfs_fat_unregister_path' not in cpp[cpp.index('bool FT02_StorageSleepEnter()'):cpp.index('FT02StorageState FT02_StorageStateCurrent()')]
assert 'FT02_StorageSleepEnter()' in main
assert 'FT02_StorageSleepExit()' in main
assert 'FT02_BatterySnapshotCurrent()' in main
# A5B must not re-introduce the A5 RC1 forced battery read on sleep entry.
entry_start=main.index('static void FT02_EnterSleepMode(bool automatic)\n{')
entry_end=main.index('static void FT02_WakeFromSleep()\n{', entry_start)
entry=main[entry_start:entry_end]
assert 'FT02_BatteryMonitorRefreshNow' not in entry
# SD quiesce must occur before CardKB2 REQUEST/COMMIT begins.
assert entry.index('FT02_StorageSleepEnter()') < entry.index('FT02_InputRequestHostSleep()')
# Wake must restore shared I2C before any storage wake work, preserving RC3 lifecycle.
wake_start=main.index('static void FT02_WakeFromSleep()\n{')
wake_end=main.index('static bool FT02_ServiceSleepMode()\n{', wake_start)
wake=main[wake_start:wake_end]
assert wake.index('FT02_InputResumeBusAfterSleep') < wake.index('FT02_StorageSleepExit()')
print('Sleep Power A5B SD bus quiesce: PASS')
