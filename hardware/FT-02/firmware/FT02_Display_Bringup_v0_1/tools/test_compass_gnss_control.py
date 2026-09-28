#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

version=(ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()
req(version == 'v2.75k91', 'VERSION is not v2.75k91')

page=(ROOT/'src/FT02_PageState.h').read_text(encoding='utf-8')
home=(ROOT/'src/FT02_HomeCards.cpp').read_text(encoding='utf-8')
icons=(ROOT/'src/FT02_HomeCardIconData.h').read_text(encoding='utf-8')
main=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
ui=(ROOT/'src/FT02_CompassUI.cpp').read_text(encoding='utf-8')
cal=(ROOT/'src/FT02_CompassCalibrationUI.cpp').read_text(encoding='utf-8')
selftest=(ROOT/'src/FT02_SystemSelfTest.cpp').read_text(encoding='utf-8')
power=(ROOT/'src/FT02_GnssPowerControl.cpp').read_text(encoding='utf-8')
host=(ROOT/'src/FT02_LR01HostRuntime.cpp').read_text(encoding='utf-8')
host_h=(ROOT/'src/FT02_LR01HostRuntime.h').read_text(encoding='utf-8')

req('FT02_PAGE_COMPASS' in page and 'FT02_PAGE_COMPASS_CALIBRATION' in page, 'compass page states missing')
req('"指南针"' in home and '&ICON_HOME_CARD_COMPASS' in home, 'home compass card missing')
req('ICON_HOME_CARD_COMPASS' in icons, 'embedded compass icon missing')
req('g_ft02HomeSelectedCard == 8' in main and 'FT02_OpenCompassPage()' in main, 'home index 8 does not open compass')
req('FT02_CompassUIService(display, millis())' in main, 'live compass service missing')
req('FT02_OpenCompassCalibrationPage()' in main, 'compass calibration entry missing')
req(re.search(r'FT02_COMPASS_CAL_UI_EXIT\).*?g_ft02PageState\s*=\s*FT02_PAGE_COMPASS', main, re.S), 'calibration does not return to compass')
req('设备状态  罗盘校准' not in cal and '指南针  罗盘校准' in cal, 'calibration ownership not moved to compass')

for label in ('"北"','"东北"','"东"','"东南"','"南"','"西南"','"西"','"西北"'):
    req(label in ui, f'compass face missing {label}')
req('drawFixedTopPointer' in ui, 'approved fixed top pointer missing')
req('REFRESH_MIN_MS = 900u' in ui, 'compass refresh throttle changed')
req('HEADING_STEP_DEG = 3.0f' in ui, 'compass heading threshold changed')
req('setPartialWindow(BODY_X, BODY_Y, BODY_W, BODY_H)' in ui, 'compass partial update missing')
req('PARTIAL_CLEAN_LIMIT = 60u' in ui, 'compass ghost cleanup limit missing')
req('directionCn' in ui and '东北' in ui and '西北' in ui, 'Chinese direction labels missing')
# FT-02 must not own the LR01 A4.3 installation yaw offset.
req('COMPASS_INSTALL_YAW_OFFSET' not in ui, 'FT-02 duplicates LR01 installation yaw correction')

req('GNSS_SOFT_ON' in host and 'GNSS_SOFT_OFF' in host, 'LR01 GNSS compatibility commands missing')
req('GNSS_POWER_STATE' in host and 'GNSS_POWER_SET' in host, 'GNSS power response parser missing')
req('gnssPowerRevision' in host_h, 'GNSS power state revision missing')
req('PREF_KEY = "gnss_on"' in power and 'Preferences' in power, 'GNSS desired state persistence missing')
req('FT02_ModeManager.h' in main and 'FT02_ModeToggleIndoorOutdoor' in main, 'GNSS mode ownership missing')
req('FT02_GnssPowerSetEnabled' not in main, 'legacy direct GNSS toggle remains in main UI')
req('ENTER GNSS开/关' not in selftest, 'legacy GNSS toggle footer remains')
req('室内模式·GNSS关闭' in selftest, 'self-test indoor-mode GNSS state missing')
req('K罗盘校准' not in selftest and '罗盘校准' not in ''.join(line for line in selftest.splitlines() if 'BottomBar' in line), 'legacy self-test calibration shortcut remains')
req('已关闭' in main and 'FT02_GnssPowerDesiredEnabled' in main, 'status bar GNSS-off indication missing')

print('PASS k91 compass=standalone calibration=owned-by-compass GNSS=mode-policy LR01-protocol=OK FT02-yaw-offset=none')
