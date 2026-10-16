from pathlib import Path
import re, sys
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
diag=(root/'src/FT02_SleepDiagnostics.cpp').read_text()
selftest=(root/'src/FT02_SystemSelfTest.cpp').read_text()
header=(root/'src/FT02_SystemSelfTest.h').read_text()
version=(root/'VERSION.txt').read_text().strip()

def req(c,m):
    if not c:
        print('FAIL:',m); sys.exit(1)

req(version=='v2.84-sleep-power-a5c-stable-r1.5','version')
# Every physical sleep starts fresh, regardless of automatic/manual.
block=re.search(r'void FT02_SleepTruthSessionEnter\(bool automatic\).*?\n}\n',diag,re.S)
req(block is not None,'session enter block')
b=block.group(0)
req('(void)automatic;' in b and 'resetTruth(true, now);' in b,'session is not unconditionally reset')
req('g_truth.sessionCount = 1u;' in b,'session count not isolated')
# Accepted wake closes the measurement window.
kw=re.search(r'void FT02_SleepTruthMarkKeyboardWake\(\).*?\n}\n',diag,re.S)
req(kw and 'g_truth.windowActive = 0u;' in kw.group(0),'keyboard wake does not close session')
# Device Status normal opening and wake restore must use passive refresh.
defs=list(re.finditer(r'static void FT02_OpenDeviceStatusPage\(\)\n\{.*?\n}\n',main,re.S))
open_block=defs[-1] if defs else None
req(open_block and 'FT02_SystemSelfTestRefreshPassive();' in open_block.group(0),'device status open is not passive')
wake_idx=main.rfind('else if(g_ft02PageState == FT02_PAGE_DEVICE_STATUS)')
wake_end=main.find('else if(g_ft02PageState == FT02_PAGE_WIRELESS_WIFI)', wake_idx)
req(wake_idx>=0 and wake_end>wake_idx and 'FT02_SystemSelfTestRefreshPassive();' in main[wake_idx:wake_end], 'wake return is not passive')
req('void FT02_SystemSelfTestRefreshPassive();' in header,'passive API missing')
pb=re.search(r'void FT02_SystemSelfTestRefreshPassive\(\).*?\n}\n\nvoid FT02_SystemSelfTestRun',selftest,re.S)
req(pb is not None,'passive implementation missing')
p=pb.group(0)
for forbidden in ['storageWriteReadProbe(', 'FT02_BatteryMonitorRefreshNow(', 'runMap();', 'saveReportLog();']:
    req(forbidden not in p, f'passive status still performs side effect: {forbidden}')
req('runStoragePassive();' in p and 'runBatteryCached();' in p and 'runMapPassive();' in p,'passive cached rows missing')
# Full active self-test remains available only through explicit ENTER path.
sel=re.search(r'if\(event.key == FT02_KEY_SELECT\).*?return true;\n    }',main,re.S)
req(sel and 'FT02_SystemSelfTestRun();' in sel.group(0),'ENTER no longer runs active self-test')
req('SOC估算' in selftest,'SOC semantics not labeled')
print('PASS R1.4 session isolation + truly read-only Device Status + SOC semantics')
