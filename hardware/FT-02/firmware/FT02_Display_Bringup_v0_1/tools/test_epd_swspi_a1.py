#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / 'src/main.cpp').read_text()
pio = (ROOT / 'platformio.ini').read_text()
patch = (ROOT / 'tools/patch_gxepd2_swspi.py').read_text()

checks = []
def ck(name, cond): checks.append((name, bool(cond)))

ck('main has no EPD HSPI object', 'g_ft02EpdSpi' not in main)
ck('setup announces FAST SW SPI', '[EPD] backend=FAST-GPIO-SW-SPI' in main)
ck('enableWrite used for EPD', main.count('display.epd2.enableWrite(EPD_SCK, EPD_MOSI);') >= 3)
ck('sleep releases EPD pins', 'pinMode(EPD_MOSI, INPUT);' in main and 'pinMode(EPD_SCK, INPUT);' in main)
ck('GxEPD2 pinned 1.6.9', 'zinggjm/GxEPD2@1.6.9' in pio)
ck('build overlay enabled', 'pre:tools/patch_gxepd2_swspi.py' in pio)
ck('fast direct gpio path', 'GPIO_OUT_W1TS_REG' in patch and 'GPIO_OUT_W1TC_REG' in patch)
ck('safe fallback retained', 'digitalWrite(_mosi' in patch)
enable = patch[patch.find('void GxEPD2_EPD::enableWrite'): ]
ck('pinMode before initial digitalWrite', enable.find('pinMode(_sck, OUTPUT);') < enable.find('digitalWrite(_sck, LOW);'))
ck('hardware SPI still skipped for EPD', 'if (_sck < 0) _pSPIx->begin();' in patch)

failed = [n for n, ok in checks if not ok]
for n, ok in checks: print(f"{'PASS' if ok else 'FAIL'} {n}")
if failed: raise SystemExit('EPD FAST SW-SPI audit failed: ' + ', '.join(failed))
print(f'EPD FAST SW-SPI audit: {len(checks)}/{len(checks)} PASS')
