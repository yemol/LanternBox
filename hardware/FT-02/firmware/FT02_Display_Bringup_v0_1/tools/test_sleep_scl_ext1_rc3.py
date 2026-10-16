#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
def t(rel): return (ROOT/rel).read_text(encoding='utf-8')
def req(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
main=t('src/main.cpp'); inp=t('src/FT02_InputManager.cpp'); inph=t('src/FT02_InputManager.h')
lora=t('src/FT02_LoRaLightSleepBridge.cpp')

def code_only(s):
    s=re.sub(r'/\*.*?\*/','',s,flags=re.S)
    s=re.sub(r'//.*','',s)
    return s
mc=code_only(main); ic=code_only(inp)

req('constexpr int CARDKB_SCL = 21;' in main,'CardKB SCL pin changed')
req('driver/rtc_io.h' in inp,'RTC IO API missing')
req('rtc_gpio_is_valid_gpio' in inp,'RTC capability guard missing')
req('RTC_GPIO_MODE_INPUT_OUTPUT_OD' in inp,'RTC open-drain pulse ownership missing')
req('RTC_GPIO_MODE_INPUT_ONLY' in inp,'RTC input wake ownership missing')
req('rtc_gpio_pullup_en' in inp and 'rtc_gpio_pulldown_dis' in inp,'RTC SCL bias missing')
req('rtc_gpio_hold_dis' in inp and 'rtc_gpio_deinit' in inp,'EXT1 wake cleanup missing')
req('gpio_intr_disable' in inp and 'gpio_set_intr_type(scl, GPIO_INTR_DISABLE)' in inp,'targeted SCL interrupt sanitize missing')
req('esp_sleep_enable_ext1_wakeup_io' in main and 'ESP_EXT1_WAKEUP_ANY_LOW' in main,'EXT1 ANY_LOW wake missing')
req('esp_sleep_disable_ext1_wakeup_io(FT02_CARDKB_SCL_EXT1_MASK)' in main,'targeted EXT1 disarm missing')
req('esp_sleep_get_ext1_wakeup_status' in main,'EXT1 source verification missing')
req('gpio_wakeup_enable' not in mc and 'esp_sleep_enable_gpio_wakeup' not in mc and 'GPIO_INTR_LOW_LEVEL' not in mc,'legacy CardKB digital wake remains in main')
req('gpio_config(' not in ic,'InputManager still calls gpio_config')

pulse=inp[inp.index('static bool FT02_PulseSclOutOfBandLocked'):inp.index('static bool FT02_SendCancelAndRestoreWire')]
req('rtc_gpio_set_level' in pulse,'RTC SCL pulse drive missing')
req('digitalRead(g_ft02InputSdaPin) != HIGH' in pulse,'SDA HIGH protocol guard missing')
req('gpio_config(' not in code_only(pulse),'dangerous gpio_config remains in SCL pulse')
req('leaveRtcInput' in pulse and 'rtc_gpio_set_direction(scl, RTC_GPIO_MODE_INPUT_ONLY)' in pulse,'COMMIT direct RTC handoff missing')

req('FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, false)' in inp,'REQUEST must restore digital/Wire ownership')
req('FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, true)' in inp,'COMMIT must retain RTC ownership')
req('FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_CANCEL_MS, false)' in inp,'CANCEL must restore digital/Wire ownership')
req('bool FT02_InputSleepSclRtcReady();' in inph and 'bool FT02_InputSleepSclIsLow();' in inph,'sleep RTC ownership API missing')

release=inp[inp.index('static bool FT02_ReleaseKeyboardSclToDigitalInput'):inp.index('static bool FT02_ClaimKeyboardSclRtc')]
req(release.index('rtc_gpio_hold_dis') < release.index('rtc_gpio_deinit') < release.index('FT02_SanitizeKeyboardSclDigitalInterrupt'),'wake cleanup order invalid')
resume=inp[inp.index('bool FT02_InputResumeBusAfterSleep'):]
req(resume.index('FT02_ReleaseKeyboardSclToDigitalInput') < resume.index('Wire.begin'),'Wire begins before RTC/digital cleanup')

# Other modules may legitimately use digital GPIO wake. RC3 must preserve those.
req('gpio_wakeup_enable(LORA_DIO1, GPIO_INTR_HIGH_LEVEL)' in lora,'LoRa trial GPIO wake was accidentally removed')
for path in (ROOT/'src').glob('*.cpp'):
    c=code_only(path.read_text(encoding='utf-8'))
    if path.name != 'FT02_LoRaLightSleepBridge.cpp':
        req('gpio_wakeup_enable' not in c, f'unexpected digital GPIO wake in {path.name}')

# Compile-order regression: helpers implicated by the user's failed local build
# must be defined before any caller in this translation unit.
for name in ['FT02_SanitizeKeyboardSclDigitalInterrupt','FT02_ReleaseKeyboardSclToDigitalInput','FT02_ClaimKeyboardSclRtc','FT02_ReadKeyboardSclLevel']:
    first=inp.find(name)
    req(first >= 0, f'missing helper {name}')
    definition=min([x for x in [inp.find('static void '+name), inp.find('static bool '+name), inp.find('static int '+name)] if x >= 0])
    calls=[m.start() for m in re.finditer(re.escape(name)+r'\s*\(', inp)]
    req(calls, f'no callable occurrence for {name}')
    # The first textual occurrence must be the static definition, never a caller.
    prefix=inp[max(0, first-20):first]
    req('static ' in prefix, f'{name} first use is not its definition')

print('PASS SCL2PHASE RC3 EXT1: GPIO21 RTC ownership, EXT1 wake, Wire restore, legacy digital wake removed')
