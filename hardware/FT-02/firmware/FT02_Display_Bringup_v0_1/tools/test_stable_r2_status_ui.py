from pathlib import Path

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

root = Path(__file__).resolve().parents[1]
version = (root/'VERSION.txt').read_text().strip()
build = (root/'src/FT02_BuildInfo.h').read_text()
status = (root/'src/FT02_StatusBar.cpp').read_text()
selftest = (root/'src/FT02_SystemSelfTest.cpp').read_text()

req(version == 'v2.84-sleep-power-a5c-stable-r1.5', 'version')
req('FT02_FIRMWARE_VERSION "v2.84-sleep-power-a5c-stable-r1.5"' in build, 'build identity')
req('snprintf(out, outSize, "Mesh")' in status, 'top status bar must say Mesh')
req('snprintf(out, outSize, "LoRa")' not in status, 'old LoRa top label still active')
req('addItem("Mesh无线"' in selftest, 'device status Mesh无线 missing')
req('addItem("Mesh网络"' in selftest, 'device status Mesh网络 missing')
req('SX1262 · HW-SPI · LongFast' in selftest, 'new radio mode detail missing')
req('Meshtastic · 节点%u · PKI%u' in selftest, 'Meshtastic network detail missing')
req('addItem("LoRa通讯"' not in selftest, 'old LoRa通讯 status remains')
req('addItem("LoRa网络"' not in selftest, 'old LoRa网络 status remains')
print('PASS stable-r2 Mesh status UI')
