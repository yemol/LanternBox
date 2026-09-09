#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

version=(ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()
req(version.startswith('FT-02 v2.75k67'), 'VERSION is not k67')

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

for label in ('"N"','"E"','"S"','"W"','"NE"','"SE"','"SW"','"NW"'):
    req(label in ui, f'compass face missing {label}')
req('REFRESH_MIN_MS = 900u' in ui, 'compass refresh throttle changed')
req('HEADING_STEP_DEG = 3.0f' in ui, 'compass heading threshold changed')
req('setPartialWindow(BODY_X, BODY_Y, BODY_W, BODY_H)' in ui, 'compass partial update missing')
req('PARTIAL_CLEAN_LIMIT = 60u' in ui, 'compass ghost cleanup limit missing')
req('directionCn' in ui and '东北' in ui and '西北' in ui, 'Chinese direction labels missing')
# FT-02 must not own the LR01 A4.3 installation yaw offset.
req('COMPASS_INSTALL_YAW_OFFSET' not in ui, 'FT-02 duplicates LR01 installation yaw correction')

req('GNSS_SOFT_ON' in host and 'GNSS_SOFT_OFF' in host and 'GNSS_POWER?' in host, 'LR01 A4.3 GNSS power commands missing')
req('GNSS_POWER_STATE' in host and 'GNSS_POWER_SET' in host, 'GNSS power response parser missing')
req('gnssPowerRevision' in host_h, 'GNSS power state revision missing')
req('PREF_KEY = "gnss_on"' in power and 'Preferences' in power, 'GNSS desired state persistence missing')
req('FT02_GnssPowerSetEnabled' in main, 'system self-test does not control GNSS')
req('FT02_SystemSelfTestSetPage(1u);' in main, 'GNSS toggle does not keep self-test on page 2')
req('ENTER GNSS开/关' in selftest, 'system self-test GNSS toggle footer missing')
req('用户已关闭' in selftest and '软件待机' in selftest, 'self-test intentional-off state missing')
req('K罗盘校准' not in selftest and '罗盘校准' not in ''.join(line for line in selftest.splitlines() if 'BottomBar' in line), 'legacy self-test calibration shortcut remains')
req('已关闭' in main and 'FT02_GnssPowerDesiredEnabled' in main, 'status bar GNSS-off indication missing')

print('PASS k67 compass=standalone-standard-face calibration=owned-by-compass GNSS-soft-switch=NVS-persistent LR01-A4.3-protocol=OK FT02-yaw-offset=none')
