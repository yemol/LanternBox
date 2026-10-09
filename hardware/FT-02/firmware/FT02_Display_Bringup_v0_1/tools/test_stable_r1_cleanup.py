#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def req(c,m):
    if not c: raise SystemExit('FAIL: '+m)

audio=(ROOT/'src/FT02_AudioLog.cpp').read_text()
main=(ROOT/'src/main.cpp').read_text()
light=(ROOT/'src/FT02_LoRaLightSleepBridge.cpp').read_text()
ini=(ROOT/'platformio.ini').read_text()
storage=(ROOT/'lib/FT02Storage/src/FT02_StorageConfig.h').read_text()
project=(ROOT/'ARCHITECTURE.md').read_text()

for f in ['FT02_LR01HostRuntime.cpp','FT02_LR01HostRuntime.h','FT02_RadioLibSoftHal.cpp','FT02_RadioLibSoftHal.h','FT02_LoRaSoftSpi.cpp','FT02_LoRaSoftSpi.h','FT02_LoRaRxDiagnostic.cpp','FT02_LoRaRxDiagnostic.h']:
    req(not (ROOT/'src'/f).exists(), f+' should be removed')
req('FT02_LR01Host' not in main, 'main still references retired LR01 host')
req('FT02_LoRaRxDiagnostic' not in main, 'main still references old RX diagnostic shim')
req('FT02_MeshServiceSleepAvailable' in light and 'FT02_LoRaRxDiagnostic' not in light, 'sleep bridge is not direct to MeshService')
req('FT02_AUDIO_PA_SD_PIN' not in audio, 'audio still owns PA GPIO')
req('pinMode(4' not in audio and 'digitalWrite(4' not in audio, 'reserved GPIO4 is still driven')
req('FT02_LORA_RX_DIAG_ENABLE' not in ini, 'dead LoRa RX diagnostic build flag remains')
req('SX1262 owns HSPI; the display uses FAST software SPI' in storage, 'SD bus ownership comment stale')
req('GPIO4: **reserved / free**' in project, 'current pin guide does not reserve GPIO4')
print('PASS Stable R1 cleanup: dead transports removed, GPIO4 free, direct Mesh sleep bridge, current bus ownership')
