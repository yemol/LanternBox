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
gnss_driver=t('src/FT02_Gnss.cpp')
compass=t('src/FT02_CompassUI.cpp')
cal=t('src/FT02_CompassCalibrationUI.cpp')
wifi=t('src/wifi/FT02_WifiManager.cpp')
wireless=t('src/FT02_WirelessManager.cpp')
mesh=t('src/FT02_MeshService.cpp')

# Home surface and handlers.
for label in ['知识库','地图导航','日志记录','定位记录','内部通讯','随身书库','设备状态','WiFi网络','指南针','系统设置']:
    req(f'"{label}"' in home, 'home card missing '+label)
for idx in range(10):
    req(f'g_ft02HomeSelectedCard == {idx}' in main, f'home handler missing index {idx}')
for state in ['FT02_PAGE_COMPASS','FT02_PAGE_COMPASS_CALIBRATION','FT02_PAGE_WIRELESS_WIFI','FT02_PAGE_DEVICE_STATUS','FT02_PAGE_SYSTEM_SETTINGS']:
    req(state in page and state in main, 'page state missing '+state)

# Battery real monitor, no static fake percentage.
for token in ['0x36','VCELL','SOC','CRATE']:
    req(token in batt, 'battery monitor missing '+token)
req('86%' not in status, 'static battery 86% regression')
req('FT02_BatteryMonitorRefreshNow();' in selftest, 'battery self-test not live')

# Compass module/calibration ownership.
req('drawCompassFace' in compass and 'drawFixedTopPointer' in compass, 'approved fixed-pointer compass face missing')
req('ENTER 校准' in compass, 'compass calibration entry missing')
for cmd in ['FT02_CompassCoreCalStart','FT02_CompassCoreCalSave','FT02_CompassCoreCalCancel','FT02_CompassCoreCalReset']:
    req(cmd in cal, 'Core compass calibration command missing '+cmd)
core_compass=t('src/FT02_CompassCore.cpp')
req('QMC_ADDR = 0x0D' in core_compass and 'INSTALL_YAW_OFFSET_DEG = 90.0f' in core_compass, 'Core QMC heading transform missing')
req('FT02_LR01HostState' not in compass, 'Compass UI still reads LR01 state')

# GNSS is owned directly by Core over UART1. Legacy LR01 GNSS
# parser/commands may remain for compatibility, but production policy must not call them.
req('FT02_GNSS_RX_PIN = 5' in gnss_driver and 'FT02_GNSS_TX_PIN = 6' in gnss_driver, 'Core GNSS UART1 pins missing')
req('TinyGPSPlus g_gps' in gnss_driver and 'FT02_GNSS_RX_BUFFER_BYTES = 4096u' in gnss_driver and 'g_gps.encode' in gnss_driver, 'Core UART LR01-style GNSS driver missing')
req('FT02_LR01HostGnssSoftwareOn' not in gnss and 'FT02_LR01HostGnssSoftwareOff' not in gnss, 'GNSS policy still controls LR01')
req('FT02_GnssSetRuntimeEnabled' in gnss, 'GNSS mode policy does not control Core driver')
req('Preferences' in gnss and 'gnss_on' in gnss, 'GNSS NVS persistence missing')
req('FT02_GnssPowerControlPoll();' in main, 'GNSS power policy poll missing')
req('GPS' in main and '已关闭' in main, 'GNSS status-bar OFF state missing')

# WiFi lifecycle: off by default and on only under an owner.
req('WiFi.mode(WIFI_OFF);' in wifi, 'WiFi default/shutdown OFF missing')
req('FT02_WIFI_OWNER_WIFI_UI' in wifi and 'FT02_WIFI_OWNER_SYNC' in wifi, 'WiFi ownership missing')
req('FT02_WirelessManagerCloseWiFi();' in main, 'WiFi close on page exit missing')
req('FT02_WifiManagerShutdown(FT02_WIFI_OWNER_WIFI_UI);' in wireless, 'WiFi UI shutdown missing')

# LoRa/Meshtastic is owned directly by Core; retired LR01 transport is absent.
req(not (ROOT/'src/FT02_LR01HostRuntime.cpp').exists(), 'retired LR01 runtime still present')
req('FT02_MeshServiceBegin' in mesh and 'FT02_MeshServiceSendPrivateText' in mesh, 'Core MeshService surface missing')
req('FT02_MeshServicePoll();' in main, 'Core MeshService poll missing')

# Self-test remains exactly 3 x 7 visible slots.
req('FT02_SELFTEST_PAGE_COUNT = 3' in selftest, 'self-test page count')
req('FT02_SELFTEST_ITEMS_PER_PAGE = 7' in selftest, 'self-test items/page')
req('MAX_ITEMS = 21' in t('src/FT02_SystemSelfTest.h'), 'self-test report capacity')

print('PASS Stable R1 surface home=10 battery=MAX17048 compass=Core-QMC GNSS=Core-UART Mesh=Core-SX1262 WiFi=owner-gated selftest=3x7')
