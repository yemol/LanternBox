#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text(encoding='utf-8')
battery = (root / 'src/FT02_BatteryMonitor.cpp').read_text(encoding='utf-8')
input_cpp = (root / 'src/FT02_InputManager.cpp').read_text(encoding='utf-8')

checks = {
    'normal_scheduler_20ms': 'const uint32_t loopDelayMs = FT02_AudioLogIsPlaying() ? 5u : 20u;' in main,
    'old_fast_5ms_path_removed': '(fastInteractivePage ? 5u : 20u)' not in main,
    'playback_cadence_preserved': 'FT02_AudioLogIsPlaying() ? 5u : 20u' in main,
    'battery_background_10s': 'SAMPLE_INTERVAL_MS = 10000UL' in battery,
    'battery_identity_cached': all(x in battery for x in ('g_identityValid', 'g_cachedVersion', 'g_cachedChipId')),
    'battery_identity_revalidates': 'IDENTITY_INVALIDATE_FAILURES = 3' in battery,
    'battery_live_regs_remain': all(x in battery for x in ('REG_VCELL', 'REG_SOC', 'REG_CRATE', 'REG_MODE')),
    'cardkb_low_noise_unchanged': 'FT02_INPUT_LOW_NOISE_POLL_MS = 100' in input_cpp,
    'cardkb_repeat_unchanged': 'FT02_INPUT_SAME_KEY_REPEAT_MS = 180' in input_cpp,
}

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"{'PASS' if ok else 'FAIL'}: {name}")
if failed:
    raise SystemExit(1)
print('PASS Low Power A2: normal loop=20ms, playback=5ms, battery=10s, MAX17048 identity cached')
