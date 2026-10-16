#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def t(p): return (ROOT/p).read_text(encoding='utf-8')
def req(c,m):
    if not c: raise SystemExit('FAIL: '+m)
ver=t('VERSION.txt').strip()
g=t('src/FT02_Gnss.cpp')
main=t('src/main.cpp')
inp=t('src/FT02_InputManager.cpp')
mesh=t('src/FT02_MeshRadioBackend.cpp')
batt=t('src/FT02_BatteryMonitor.cpp')
req(ver=='v2.84-sleep-power-a5c-stable-r1.5','version mismatch')
req('FT02_GNSS_UBX_PMREQ_STANDBY' in g,'PMREQ frame missing')
for tok in ['0x02, 0x41, 0x10, 0x00','0x04, 0x00, 0x00, 0x00','0x08, 0x00, 0x00, 0x00','0x5F, 0x5B']:
    req(tok in g,'PMREQ token missing '+tok)
req('requestSoftwareStandby()' in g and 'closeGnssSerialForStandby()' in g,'standby lifecycle missing')
req('digitalWrite(FT02_GNSS_TX_PIN, HIGH)' in g,'UART TX idle-high clamp missing')
req('sendReceiverWakeEdge()' in g and 'wakeByte = 0xFFu' in g,'UART-RX wake edge missing')
req('g_bootWakeProbePending' in g,'reset-safe standby wake recovery missing')
req('[SLEEP-ENERGY]' not in main,'battery energy audit leaked into A5A')
req('FT02_BatteryMonitorRefreshNow();' not in main[main.find('static void FT02_EnterSleepMode'):main.find('static void FT02_WakeFromSleep')], 'new pre-sleep battery I2C read leaked in')
req('esp_sleep_enable_ext1_wakeup_io' in main and 'ESP_EXT1_WAKEUP_ANY_LOW' in main,'RC3 EXT1 wake missing')
req('FT02_InputRequestHostSleep()' in main and 'FT02_InputSuspendBusForSleep()' in main,'RC3 CardKB sleep handshake missing')
req('FT02_RecoverSharedI2CBus' in inp,'RC3 InputManager baseline missing')
req('g_radio->sleep(true)' in mesh,'Mesh sleep baseline changed')
req('MAX17048' in batt,'Battery baseline missing')
print('PASS A5A GNSS-only: PMREQ standby + UART wake, no Battery/Input/Mesh sleep changes')
