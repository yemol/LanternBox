#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

version=(ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()
req(version == 'v2.84-sleep-power-a5c-stable-r1.1', 'VERSION is not v2.84-sleep-power-a5c-stable-r1.1')

page=(ROOT/'src/FT02_PageState.h').read_text(encoding='utf-8')
home=(ROOT/'src/FT02_HomeCards.cpp').read_text(encoding='utf-8')
icons=(ROOT/'src/FT02_HomeCardIconData.h').read_text(encoding='utf-8')
main=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
ui=(ROOT/'src/FT02_CompassUI.cpp').read_text(encoding='utf-8')
cal=(ROOT/'src/FT02_CompassCalibrationUI.cpp').read_text(encoding='utf-8')
core=(ROOT/'src/FT02_CompassCore.cpp').read_text(encoding='utf-8')
core_h=(ROOT/'src/FT02_CompassCore.h').read_text(encoding='utf-8')
cal_store=(ROOT/'src/CompassCalibration.cpp').read_text(encoding='utf-8')
selftest=(ROOT/'src/FT02_SystemSelfTest.cpp').read_text(encoding='utf-8')
power=(ROOT/'src/FT02_GnssPowerControl.cpp').read_text(encoding='utf-8')
gnss=(ROOT/'src/FT02_Gnss.cpp').read_text(encoding='utf-8')
pio=(ROOT/'platformio.ini').read_text(encoding='utf-8')

req('FT02_PAGE_COMPASS' in page and 'FT02_PAGE_COMPASS_CALIBRATION' in page, 'compass page states missing')
req('"指南针"' in home and '&ICON_HOME_CARD_COMPASS' in home, 'home compass card missing')
req('ICON_HOME_CARD_COMPASS' in icons, 'embedded compass icon missing')
req('g_ft02HomeSelectedCard == 8' in main and 'FT02_OpenCompassPage()' in main, 'home index 8 does not open compass')
req('FT02_CompassCoreBegin()' in main and 'FT02_CompassCorePoll()' in main, 'Core compass lifecycle missing')
req('FT02_CompassUIService(display, millis())' in main, 'live compass UI service missing')
req('FT02_OpenCompassCalibrationPage()' in main, 'compass calibration entry missing')
req(re.search(r'FT02_COMPASS_CAL_UI_EXIT\).*?g_ft02PageState\s*=\s*FT02_PAGE_COMPASS', main, re.S), 'calibration does not return to compass')
req('指南针  罗盘校准' in cal and 'Core I2C' in cal, 'calibration UI not Core-owned')

for label in ('"北"','"东北"','"东"','"东南"','"南"','"西南"','"西"','"西北"'):
    req(label in ui, f'compass face missing {label}')
req('drawFixedTopPointer' in ui, 'approved fixed top pointer missing')
req('REFRESH_MIN_MS = 900u' in ui, 'compass refresh throttle changed')
req('HEADING_STEP_DEG = 3.0f' in ui, 'compass heading threshold changed')
req('setPartialWindow(BODY_X, BODY_Y, BODY_W, BODY_H)' in ui, 'compass partial update missing')
req('PARTIAL_CLEAN_LIMIT = 60u' in ui, 'compass ghost cleanup limit missing')
req('FT02_LR01HostState' not in ui and 'FT02_LR01HostCompass' not in cal, 'compass UI still depends on LR01')

req('QMC_ADDR = 0x0D' in core, 'QMC5883L address missing')
req('qmcWrite(0x0B, 0x01)' in core and 'qmcWrite(0x09, 0x1D)' in core, 'proven QMC init sequence missing')
req('atan2f(-y, x)' in core and 'INSTALL_YAW_OFFSET_DEG = 90.0f' in core, 'proven heading transform missing')
req('FT02_I2CBusLock' in core and 'FT02_I2CBusUnlock' in core, 'shared I2C guard missing')
req('prefs.begin("qmc_cal"' in cal_store, 'Core compass NVS calibration store missing')
req('FT02_CompassCoreCalStart' in core_h and 'FT02_CompassCoreCalSave' in core_h, 'Core calibration APIs missing')
req('Core I2C 0x0D' in selftest, 'self-test does not use Core compass')
req('SX1262 · HW-SPI · LongFast' in selftest, 'self-test does not report direct-Core LoRa')

req('FT02_GNSS_RX_PIN = 5' in gnss and 'FT02_GNSS_TX_PIN = 6' in gnss, 'Core GNSS UART1 pins missing')
req('FT02_GNSS_BAUD = 115200u' in gnss, 'fixed 115200 GNSS baud missing')
req('TinyGPSPlus g_gps' in gnss and 'FT02_GNSS_RX_BUFFER_BYTES = 4096u' in gnss and 'g_gps.encode' in gnss, 'Core UART LR01-style GNSS driver missing')
req('FT02_GNSS_STARTUP_GRACE_MS = 60000u' in gnss, 'conservative startup grace missing')
req('FT02_GNSS_STREAM_STALL_RECOVERY_MS = 30000u' in gnss, 'stream-stall recovery policy mismatch')
req('nmea_stall' not in gnss, 'aggressive NMEA-stall recovery remains')
req('mikalhart/TinyGPSPlus@^1.1.0' in pio, 'TinyGPSPlus dependency missing')
req('FT02_LR01HostGnssSoftwareOn' not in power and 'FT02_LR01HostGnssSoftwareOff' not in power, 'GNSS power policy still calls LR01')
req('FT02_GnssSetRuntimeEnabled' in power, 'GNSS power policy does not control Core driver')
req(not (ROOT/'src/FT02_LR01HostRuntime.cpp').exists(), 'retired LR01 runtime still exists')


map_ui=(ROOT/'src/FT02_PbfMapUI.cpp').read_text(encoding='utf-8')
req('FT02_MapHeading' in map_ui and '罗盘 %03d' in map_ui and '航迹 %03d' in map_ui, 'map navigation does not expose compass heading')
req('FT02_CompassCoreSnapshotCurrent' in map_ui, 'map navigation does not read Core compass')
req('FT02_MapNavigationHeading' in main and 'g_ft02MapLastHeadingSource' in main, 'map follow refresh still tracks GNSS course only')

req('compass.quality >= 2u' in map_ui, 'map does not gate compass heading by quality')
req('freshestGgaFixQuality' in gnss and 'SET_FIELD(fixQuality, fixQuality);' in gnss, 'GGA fix quality semantics missing')
req('const uint8_t fixType = gsaHasFix ? gsaFix : 0u;' in gnss, 'GSA-only 2D/3D fix type semantics missing')
req('(gsaHasFix || ggaHasFix)' in gnss, 'GNSS valid-fix arbitration missing')
req('FT02_LR01Host' not in main, 'main still references retired LR01 runtime')
print('PASS Stable R1 GNSS=Core-UART compass=Core-QMC LoRa=Core-Mesh')
