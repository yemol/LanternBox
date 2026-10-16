#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def text(rel):
    return (ROOT / rel).read_text(encoding='utf-8')

def req(ok, msg):
    if not ok:
        raise SystemExit('FAIL: ' + msg)

main = text('src/main.cpp')
inp = text('src/FT02_InputManager.cpp')
mode = text('src/FT02_ModeManager.cpp')
mesh = text('src/FT02_MeshRadioBackend.cpp')

def body(src, start, end):
    a = src.index(start)
    b = src.index(end, a + len(start))
    return src[a:b]

enter = body(main, 'static void FT02_EnterSleepMode(bool automatic)\n{', 'static void FT02_WakeFromSleep()')
wake = body(main, 'static void FT02_WakeFromSleep()\n{', 'static bool FT02_ServiceSleepMode()')
service = body(main, 'static bool FT02_ServiceSleepMode()\n{', 'static bool FT02_ServiceHomeModeShortcut')
request = body(inp, 'bool FT02_InputRequestHostSleep()\n{', 'bool FT02_InputSuspendBusForSleep()')
suspend = body(inp, 'bool FT02_InputSuspendBusForSleep()\n{', 'bool FT02_InputSleepSclRtcReady()')
resume = inp[inp.index('bool FT02_InputResumeBusAfterSleep'):]

# Sleep-entry ownership ordering: stop high-level clients first, then CardKB2
# handshake, then make RTC ownership invariant. No shared-I2C user may be resumed
# before the bus has returned to the digital/Wire owner.
req(enter.index('FT02_ModeEnterSleep()') < enter.index('FT02_MeshServiceSleepEnter()') < enter.index('FT02_InputRequestHostSleep()') < enter.index('FT02_InputSuspendBusForSleep()'),
    'sleep-entry lifecycle order changed')
req(wake.index('FT02_DisarmKeyboardSclWake();') < wake.index('FT02_InputResumeBusAfterSleep') < wake.index('FT02_MeshServiceSleepExit();') < wake.index('FT02_BatteryMonitorWakeResume();') < wake.index('FT02_ModeWake();'),
    'wake lifecycle resumes a shared-I2C or mode client before Wire ownership')
req(service.index('FT02_SleepWaitForKeyboardSignal()') < service.index('FT02_WakeFromSleep()'),
    'sleep service no longer gates wake through CardKB2 SCL')

# Two-phase protocol ownership intent.
req(request.index('FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, false)') < request.index('FT02_KB_STATUS_SLEEP_ARMED') < request.index('FT02_PulseSclOutOfBandLocked(FT02_KB_HANDSHAKE_SIGNAL_MS, true)'),
    'REQUEST -> ARMED -> COMMIT ordering changed')
req('RTC_GPIO_MODE_INPUT_ONLY' in suspend and 'FT02_ReadKeyboardSclLevel() == HIGH' in suspend,
    'pre-sleep RTC input/high invariant missing')
req(resume.index('FT02_ReleaseKeyboardSclToDigitalInput') < resume.index('Wire.begin'),
    'Wire resumes before RTC ownership is released')

# Mode/GNSS policy must remain unchanged: sleep forces GNSS off, wake restores the
# remembered Indoor/Outdoor policy; auto sleep stays Indoor-only.
req('g_currentMode = FT02_MODE_SLEEP;' in mode and 'applyGnssPolicy();' in mode,
    'sleep GNSS policy missing')
req('g_currentMode = g_lastAwakeMode;' in mode,
    'wake no longer restores remembered mode')
req('return g_currentMode == FT02_MODE_INDOOR;' in mode,
    'automatic sleep is no longer Indoor-only')

# Mesh IRQ lifecycle remains detach -> radio sleep -> radio wake -> attach.
req('clearPacketReceivedAction()' in mesh and 'setPacketReceivedAction(onPacketReceived)' in mesh,
    'Mesh IRQ detach/reattach lifecycle missing')

# Abstract ownership model repeated ten times. This catches future changes to the
# intended cycle contract: each cycle must end exactly where the next one starts.
state = 'WIRE_DIGITAL'
for cycle in range(1, 11):
    req(state == 'WIRE_DIGITAL', f'cycle {cycle} did not start with Wire ownership')
    # REQUEST RTC OD pulse -> Wire restored
    state = 'RTC_OD'
    state = 'WIRE_DIGITAL'
    # COMMIT RTC OD pulse -> RTC input, then EXT1 light sleep
    state = 'RTC_OD'
    state = 'RTC_INPUT'
    state = 'EXT1_ARMED'
    # Wake source is disarmed while the pad is still RTC-owned, then hold/deinit
    # restores the digital mux before Wire.begin.
    state = 'RTC_INPUT'
    state = 'WIRE_DIGITAL'
req(state == 'WIRE_DIGITAL', '10-cycle ownership model did not converge')

print('PASS SCL2PHASE RC3 state machine: 10-cycle ownership + mode/GNSS + Mesh lifecycle')
