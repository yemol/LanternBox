#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def t(rel): return (ROOT / rel).read_text(encoding='utf-8')
def req(c,m):
    if not c: raise SystemExit('FAIL: '+m)

home=t('src/FT02_HomeCards.cpp')
main=t('src/main.cpp')
page=t('src/FT02_PageState.h')
batt=t('src/FT02_BatteryMonitor.cpp')
status=t('src/FT02_StatusBar.cpp')
selftest=t('src/FT02_SystemSelfTest.cpp')
gnss=t('src/FT02_GnssPowerControl.cpp')
compass=t('src/FT02_CompassUI.cpp')
cal=t('src/FT02_CompassCalibrationUI.cpp')
wifi=t('src/wifi/FT02_WifiManager.cpp')
wireless=t('src/FT02_WirelessManager.cpp')
lr01=t('src/FT02_LR01HostRuntime.cpp')

# Home surface and handlers.
for label in ['知识库','地图导航','日志记录','定位记录','内部通讯','设备状态','随身书库','WiFi网络','指南针']:
    req(f'"{label}"' in home, 'home card missing '+label)
for idx in range(9):
    req(f'g_ft02HomeSelectedCard == {idx}' in main, f'home handler missing index {idx}')
for state in ['FT02_PAGE_COMPASS','FT02_PAGE_COMPASS_CALIBRATION','FT02_PAGE_WIRELESS_WIFI','FT02_PAGE_DEVICE_STATUS']:
    req(state in page and state in main, 'page state missing '+state)

# Battery real monitor, no static fake percentage.
for token in ['0x36','VCELL','SOC','CRATE']:
    req(token in batt, 'battery monitor missing '+token)
req('86%' not in status, 'static battery 86% regression')
req('FT02_BatteryMonitorRefreshNow();' in selftest, 'battery self-test not live')

# Compass module/calibration ownership.
req('drawCompassFace' in compass and 'drawHeadingPointer' in compass, 'standard compass face missing')
req('ENTER 校准' in compass, 'compass calibration entry missing')
for cmd in ['FT02_LR01HostCompassCalStart','FT02_LR01HostCompassCalSave','FT02_LR01HostCompassCalCancel','FT02_LR01HostCompassCalReset']:
    req(cmd in cal, 'compass calibration command missing '+cmd)
req('COMPASS_INSTALL_YAW_OFFSET' not in compass and 'YAW_OFFSET' not in compass, 'FT02 must not duplicate LR01 +180 yaw correction')

# GNSS soft switch and persistence.
for token in ['GNSS_SOFT_ON','GNSS_SOFT_OFF','GNSS_POWER?']:
    req(token in lr01, 'LR01 host command missing '+token)
req('Preferences' in gnss and 'gnss_on' in gnss, 'GNSS NVS persistence missing')
req('FT02_GnssPowerControlPoll();' in main, 'GNSS power reconciliation poll missing')
req('GPS' in main and '已关闭' in main, 'GNSS status-bar OFF state missing')

# WiFi lifecycle: off by default and on only under an owner.
req('WiFi.mode(WIFI_OFF);' in wifi, 'WiFi default/shutdown OFF missing')
req('FT02_WIFI_OWNER_WIFI_UI' in wifi and 'FT02_WIFI_OWNER_SYNC' in wifi, 'WiFi ownership missing')
req('FT02_WirelessManagerCloseWiFi();' in main, 'WiFi close on page exit missing')
req('FT02_WifiManagerShutdown(FT02_WIFI_OWNER_WIFI_UI);' in wireless, 'WiFi UI shutdown missing')

# LR01 host link fundamentals.
req('FT02_LR01_RX_BUFFER_BYTES = 4096' in lr01, 'LR01 RX buffer regression')
req('FT02_LR01_BAUD = 115200' in lr01, 'LR01 baud regression')
req('GNSS_POWER_STATE ' in lr01, 'GNSS power response parser missing')

# Self-test remains exactly 3 x 7 visible slots.
req('FT02_SELFTEST_PAGE_COUNT = 3' in selftest, 'self-test page count')
req('FT02_SELFTEST_ITEMS_PER_PAGE = 7' in selftest, 'self-test items/page')
req('MAX_ITEMS = 21' in t('src/FT02_SystemSelfTest.h'), 'self-test report capacity')

print('PASS k67 function-surface home=9 battery=MAX17048 compass=standalone GNSS=soft-switch WiFi=owner-gated LR01=A2 selftest=3x7')
