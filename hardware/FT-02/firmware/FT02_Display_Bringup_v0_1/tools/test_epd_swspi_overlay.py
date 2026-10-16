#!/usr/bin/env python3
from pathlib import Path
import tempfile

SCRIPT = Path(__file__).with_name('patch_gxepd2_swspi.py')
code = compile(SCRIPT.read_text(), str(SCRIPT), 'exec')

class MockEnv:
    def __init__(self, project, libdeps, pioenv='mockenv'):
        self.project = str(project); self.libdeps = str(libdeps); self.pioenv = pioenv
    def subst(self, value):
        return {'$PROJECT_DIR': self.project, '$PROJECT_LIBDEPS_DIR': self.libdeps, '$PIOENV': self.pioenv}.get(value, value)

def run_once(env):
    g = {'Import': lambda name: None, 'env': env}
    exec(code, g, g)

def make_pristine(gx):
    (gx / 'library.properties').write_text('name=GxEPD2\nversion=1.6.9\n')
    (gx / 'src/GxEPD2_EPD.h').write_text('''class GxEPD2_EPD {\npublic:\n  void enableRead(int16_t sck, int16_t mosi); // for _readData using SW SPI\nprotected:\n  uint8_t _readData();\n};\n''')
    ops = '\n'.join(['_pSPIx->beginTransaction(_spi_settings);\n_pSPIx->transfer(0xAA);\n_pSPIx->endTransaction();' for _ in range(4)])
    (gx / 'src/GxEPD2_EPD.cpp').write_text(
        '#include "GxEPD2_EPD.h"\n'
        'void GxEPD2_EPD::init()\n{\n  _pSPIx->begin(); // may steal _rst pin\n}\n'
        'void GxEPD2_EPD::end()\n{\n  _pSPIx->end();\n}\n'
        + ops + '\n'
        'void GxEPD2_EPD::_endTransfer() {}\n'
        'void GxEPD2_EPD::enableRead(int16_t sck, int16_t mosi) { _sck=sck; _mosi=mosi; }\n'
    )

with tempfile.TemporaryDirectory() as td:
    root = Path(td); project = root/'project'; libdeps=root/'libdeps'; gx=libdeps/'mockenv'/'GxEPD2'
    (gx/'src').mkdir(parents=True); project.mkdir(); make_pristine(gx)
    env=MockEnv(project,libdeps)
    run_once(env)
    h1=(gx/'src/GxEPD2_EPD.h').read_text(); c1=(gx/'src/GxEPD2_EPD.cpp').read_text()
    assert 'FT02_SW_SPI_FAST_A1' in h1 and 'enableWrite' in h1
    assert 'FT02_SW_SPI_FAST_A1' in c1 and 'GPIO_OUT_W1TS_REG' in c1
    assert 'if (_sck < 0) _pSPIx->begin();' in c1
    assert c1.find('pinMode(_sck, OUTPUT);') < c1.find('digitalWrite(_sck, LOW);', c1.find('void GxEPD2_EPD::enableWrite'))
    run_once(env)
    assert h1 == (gx/'src/GxEPD2_EPD.h').read_text()
    assert c1 == (gx/'src/GxEPD2_EPD.cpp').read_text()
print('EPD FAST SW-SPI overlay self-test: PASS')
