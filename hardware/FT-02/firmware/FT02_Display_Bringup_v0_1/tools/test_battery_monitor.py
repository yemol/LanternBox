#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
mon = (root / 'src/FT02_BatteryMonitor.cpp').read_text(encoding='utf-8')
monh = (root / 'src/FT02_BatteryMonitor.h').read_text(encoding='utf-8')
status = (root / 'src/FT02_StatusBar.cpp').read_text(encoding='utf-8')
selftest = (root / 'src/FT02_SystemSelfTest.cpp').read_text(encoding='utf-8')
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
build = (root / 'src/FT02_BuildInfo.h').read_text(encoding='utf-8')

checks = {
    'version': 'v2.84-sleep-power-a5c-stable-r1.5' in build,
    'i2c_addr_0x36': 'MAX17048_ADDR = 0x36' in mon,
    'shared_i2c_lock': 'FT02_I2CBusLock' in mon and 'FT02_I2CBusUnlock' in mon,
    'vcell_reg': 'REG_VCELL = 0x02' in mon,
    'soc_reg': 'REG_SOC = 0x04' in mon,
    'crate_reg': 'REG_CRATE = 0x16' in mon,
    'identity_cached': 'g_identityValid' in mon and 'g_cachedVersion' in mon and 'g_cachedChipId' in mon,
    'identity_recovery': 'IDENTITY_INVALIDATE_FAILURES = 3' in mon,
    'voltage_formula': '* 78.125f / 1000000.0f' in mon,
    'soc_formula': '/ 256.0f' in mon,
    'rate_formula': '* 0.208f' in mon,
    'low_power_poll_10s': 'SAMPLE_INTERVAL_MS = 10000UL' in mon,
    'battery_power_text': '供电中' in mon,
    'charging_text': '充电中' in mon,
    'no_stable_status_text': '稳定' not in mon,
    'no_low_status_line': '电量低' not in mon and '需充电' not in mon,
    'no_battery_debug_log': '[BATTERY]' not in mon and 'status cache=' not in main,
    'no_static_86': '"86%"' not in status,
    'status_battery_partial': 'FT02_DrawStatusBarBattery' in status,
    'real_selftest': 'MAX17048 0x36' in selftest and '尚未接入检测' not in selftest,
    'boot_begin': 'FT02_BatteryMonitorBegin();' in main,
    'audio_guard': 'if(!FT02_AudioLogIsBusy())' in main and 'FT02_BatteryMonitorPoll()' in main,
    'low_threshold': 'FT02_BATTERY_LOW_PERCENT = 20' in monh,
    'critical_threshold': 'FT02_BATTERY_CRITICAL_PERCENT = 10' in monh,
    'no_quickstart': 'quickStart' not in mon and 'REG_CMD' not in mon,
}
failed = [k for k,v in checks.items() if not v]
for k,v in checks.items():
    print(f"{'PASS' if v else 'FAIL'}: {k}")
if failed:
    raise SystemExit(1)
