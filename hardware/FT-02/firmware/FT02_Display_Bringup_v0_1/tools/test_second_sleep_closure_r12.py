#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT/'src/main.cpp').read_text(encoding='utf-8')
audio = (ROOT/'src/FT02_AudioLog.cpp').read_text(encoding='utf-8')
audio_h = (ROOT/'src/FT02_AudioLog.h').read_text(encoding='utf-8')
mesh = (ROOT/'src/FT02_MeshRadioBackend.cpp').read_text(encoding='utf-8')
mesh_h = (ROOT/'src/FT02_MeshService.h').read_text(encoding='utf-8')
gnss = (ROOT/'src/FT02_Gnss.cpp').read_text(encoding='utf-8')
mode = (ROOT/'src/FT02_ModeManager.cpp').read_text(encoding='utf-8')
version = (ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()

def req(ok, msg):
    if not ok:
        raise SystemExit('FAIL: '+msg)

req(version == 'v2.84-sleep-power-a5c-stable-r1.5', 'version')

open_dev = re.search(r'static void FT02_OpenDeviceStatusPage\(\).*?\n}\n', main, re.S)
req(open_dev is not None, 'device status open missing')
req('FT02_AudioLogAcquire' not in open_dev.group(0), 'device status open still powers audio')

handle_dev = re.search(r'static bool FT02_HandleDeviceStatusInput\(.*?\n}\n', main, re.S)
req(handle_dev is not None, 'device status handler missing')
req('FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST)' in handle_dev.group(0), 'explicit ENTER audio selftest missing')
req('FT02_AudioLogRelease(FT02_AUDIO_OWNER_SYSTEM_TEST)' in handle_dev.group(0), 'explicit selftest does not release audio')

wake = re.search(r'static void FT02_WakeFromSleep\(\).*?\n}\n\nstatic bool FT02_ServiceSleepMode', main, re.S)
req(wake is not None, 'wake function missing')
status_branch = re.search(r'else if\(g_ft02PageState == FT02_PAGE_DEVICE_STATUS\).*?\n    }', wake.group(0), re.S)
req(status_branch is not None, 'wake device-status branch missing')
req('FT02_AudioLogAcquire' not in status_branch.group(0), 'wake restore still powers audio on status page')

req('bool FT02_AudioLogPrepareForSleep()' in audio, 'audio verified sleep API missing')
stop = re.search(r'static bool FT02_AudioHardwareStop\(\).*?\n}\n', audio, re.S)
req(stop is not None, 'audio stop no longer returns result')
req('if(!codecSuspendOk)' in stop.group(0), 'codec suspend failure not handled')
req(stop.group(0).find('if(!codecSuspendOk)') < stop.group(0).find('FT02_AudioDeinitI2s()'), 'I2S is torn down before suspend success is known')
req('return false;' in stop.group(0), 'audio suspend failure does not fail closed')
req('FT02_AudioLogPrepareForSleep' in audio_h, 'audio preflight declaration missing')

req('bool FT02_MeshServiceSleepEnter();' in mesh_h, 'Mesh service sleep result not propagated')
mesh_enter = re.search(r'bool FT02_MeshRadioBackendSleepEnter\(\).*?\n}\n\nvoid FT02_MeshRadioBackendSleepService', mesh, re.S)
req(mesh_enter is not None, 'Mesh sleep enter result missing')
req('return false;' in mesh_enter.group(0) and 'return true;' in mesh_enter.group(0), 'Mesh sleep is not fail-closed')
req('FT02_LoRaHwProbeSetRfIdle()' in mesh_enter.group(0), 'RF front-end idle not part of closure')

set_gnss = re.search(r'bool FT02_GnssSetRuntimeEnabled\(bool enabled\).*?\n}\n', gnss, re.S)
req(set_gnss is not None, 'GNSS runtime setter missing')
req('return standbySent;' in set_gnss.group(0), 'GNSS PMREQ failure still hidden')
req('bool FT02_ModeEnterSleep()' in mode, 'mode sleep does not propagate GNSS closure')

sleep = re.search(r'static void FT02_EnterSleepMode\(bool automatic\)\n\{.*?\n}\n\nstatic void FT02_WakeFromSleep', main, re.S)
req(sleep is not None, 'sleep entry missing')
for token in [
    'FT02_AudioLogPrepareForSleep()',
    'closure failed module=audio',
    'const bool gnssSleepOk = FT02_ModeEnterSleep()',
    'closure failed module=gnss',
    'const bool compassSleepOk = FT02_CompassCoreSleepEnter()',
    'closure failed module=compass',
    'const bool meshSleepOk = FT02_MeshServiceSleepEnter()',
    'closure failed module=mesh',
]:
    req(token in sleep.group(0), 'sleep closure missing: '+token)
req(sleep.group(0).find('const bool meshSleepOk') < sleep.group(0).find('digitalWrite(EPD_PWR, LOW)'), 'CPU/peripheral sleep starts before Mesh closure')
req(sleep.group(0).find('const bool meshSleepOk') < sleep.group(0).find('FT02_StorageSleepEnter()'), 'SD handoff starts before peripheral closure')

print('PASS R1.2 second-sleep closure: status=read-only audio=verified mesh=verified gnss=propagated fail-closed=ON')
