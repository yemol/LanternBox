from pathlib import Path
p=Path(__file__).resolve().parents[1]/'src/FT02_MeshRadioBackend.cpp'
s=p.read_text()
def req(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
req('bool g_packetActionAttached = false;' in s,'IRQ ownership flag missing')
req('g_radio->clearPacketReceivedAction();' in s,'DIO1 detach missing')
req('g_radio->setPacketReceivedAction(onPacketReceived);' in s,'DIO1 attach missing')
enter=s[s.index('bool FT02_MeshRadioBackendSleepEnter()'):s.index('void FT02_MeshRadioBackendSleepService()')]
exit=s[s.index('void FT02_MeshRadioBackendSleepExit()'):]
req(enter.index('g_sleepSession = true;') < enter.index('detachPacketReceivedActionForSleep()'),'sleep quarantine must precede detach')
req(enter.index('detachPacketReceivedActionForSleep()') < enter.index('g_radio->sleep(true)'),'DIO1 must detach before radio sleep')
req(exit.index('startListening()') < exit.index('attachPacketReceivedActionAfterWake()'),'DIO1 must attach only after RX restart')
req('digitalRead(FT02_LoRaHardwareSpi::DIO1) == HIGH' in s,'reattach race closure missing')
req('sleep rollback failed' in s,'sleep-entry rollback missing')
print('PASS Mesh sleep IRQ lifecycle RC2')
