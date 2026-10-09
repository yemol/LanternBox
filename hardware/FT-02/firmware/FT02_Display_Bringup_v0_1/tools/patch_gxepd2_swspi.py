# FT02 EPD FAST SW-SPI A1 build overlay for GxEPD2 1.6.9.
# Keeps upstream GxEPD2 API/features and changes only the low-level write path.
# The FT02 display remains on the same pins but uses direct ESP32-S3 GPIO
# set/clear registers instead of Arduino digitalWrite() for every SPI edge.

Import("env")

from pathlib import Path
import re
import subprocess
import sys

MARKER_A1 = "FT02_SW_SPI_PATCH_A1"
MARKER_FAST = "FT02_SW_SPI_FAST_A1"
PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
PIOENV = env.subst("$PIOENV")
LIBDEPS_ROOT = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / PIOENV


def find_gxepd2():
    if not LIBDEPS_ROOT.exists():
        return None
    direct = LIBDEPS_ROOT / "GxEPD2"
    if (direct / "src" / "GxEPD2_EPD.cpp").exists():
        return direct
    for props in LIBDEPS_ROOT.rglob("library.properties"):
        try:
            text = props.read_text(errors="ignore")
        except Exception:
            continue
        if re.search(r"(?mi)^name\s*=\s*GxEPD2\s*$", text):
            root = props.parent
            if (root / "src" / "GxEPD2_EPD.cpp").exists():
                return root
    return None


def ensure_gxepd2():
    root = find_gxepd2()
    if root:
        return root
    print("[FT02-EPD-FAST] GxEPD2 not present yet; installing project dependencies...")
    cmd = [sys.executable, "-m", "platformio", "pkg", "install", "-e", PIOENV]
    subprocess.check_call(cmd, cwd=str(PROJECT_DIR))
    root = find_gxepd2()
    if not root:
        raise RuntimeError("GxEPD2 dependency could not be located after package install")
    return root


def patch_header(path: Path):
    s = path.read_text()
    changed = False

    if "void enableWrite(int16_t sck, int16_t mosi);" not in s:
        needle = "void enableRead(int16_t sck, int16_t mosi); // for _readData using SW SPI"
        if needle not in s:
            raise RuntimeError("Unsupported GxEPD2_EPD.h: enableRead signature not found; expected GxEPD2 1.6.9")
        s = s.replace(
            needle,
            needle + "\n  void enableWrite(int16_t sck, int16_t mosi); // " + MARKER_FAST + ": fast SW SPI write path",
            1,
        )
        changed = True

    if "void _ft02BeginTransaction(const SPISettings& settings);" not in s:
        needle2 = "uint8_t _readData();"
        if needle2 not in s:
            raise RuntimeError("Unsupported GxEPD2_EPD.h: _readData declaration not found")
        helpers = (
            "void _ft02BeginTransaction(const SPISettings& settings); // " + MARKER_FAST + "\n"
            "  void _ft02Transfer(uint8_t data);\n"
            "  void _ft02EndTransaction();\n  "
        )
        s = s.replace(needle2, helpers + needle2, 1)
        changed = True

    # Upgrade marker/comment if this dependency was already patched by A1.
    if MARKER_A1 in s and MARKER_FAST not in s:
        s = s.replace(MARKER_A1, MARKER_FAST)
        changed = True

    if changed:
        path.write_text(s)
    return changed


def ensure_cpp_routing(s: str):
    """Apply the original A1 routing only when dependency is still pristine."""
    changed = False
    if "_ft02BeginTransaction(_spi_settings);" not in s:
        begin_re = re.compile(r"(?m)^(\s*)_pSPIx->begin\(\);(\s*//[^\n]*)?$")
        s, begin_count = begin_re.subn(
            lambda m: f"{m.group(1)}if (_sck < 0) _pSPIx->begin();{m.group(2) or ''}", s
        )
        if begin_count < 1:
            raise RuntimeError("Unsupported GxEPD2_EPD.cpp: hardware SPI begin call not found")

        end_re = re.compile(r"(?m)^(\s*)_pSPIx->end\(\);$")
        s, end_count = end_re.subn(lambda m: f"{m.group(1)}if (_sck < 0) _pSPIx->end();", s)
        if end_count < 1:
            raise RuntimeError("Unsupported GxEPD2_EPD.cpp: hardware SPI end call not found")

        bt_count = s.count("_pSPIx->beginTransaction(_spi_settings);")
        tr_count = s.count("_pSPIx->transfer(")
        et_count = s.count("_pSPIx->endTransaction();")
        if bt_count < 4 or tr_count < 4 or et_count < 4:
            raise RuntimeError(
                f"Unsupported GxEPD2_EPD.cpp transaction layout bt={bt_count} tr={tr_count} et={et_count}"
            )
        s = s.replace("_pSPIx->beginTransaction(_spi_settings);", "_ft02BeginTransaction(_spi_settings);")
        s = s.replace("_pSPIx->transfer(", "_ft02Transfer(")
        s = s.replace("_pSPIx->endTransaction();", "_ft02EndTransaction();")
        changed = True
    return s, changed


def patch_cpp(path: Path):
    s = path.read_text()
    original = s

    s, _ = ensure_cpp_routing(s)

    # Direct GPIO register access is used only on ESP32 and only when SCK/MOSI
    # are GPIO0..31. FT02 uses GPIO12/11. Other targets/pins keep the safe
    # digitalWrite fallback.
    include_block = '''\n#if defined(ARDUINO_ARCH_ESP32)\n#include "soc/soc.h"\n#include "soc/gpio_reg.h"\n#endif\n'''
    if '"soc/gpio_reg.h"' not in s:
        # Insert after the source's own include block, before code begins.
        first_include_end = 0
        for m in re.finditer(r"(?m)^#include[^\n]*\n", s):
            first_include_end = m.end()
        if first_include_end == 0:
            raise RuntimeError("Unsupported GxEPD2_EPD.cpp: include block not found")
        s = s[:first_include_end] + include_block + s[first_include_end:]

    anchor = "void GxEPD2_EPD::enableRead(int16_t sck, int16_t mosi)"
    if anchor not in s:
        raise RuntimeError("Unsupported GxEPD2_EPD.cpp: enableRead implementation not found")

    fast_impl = r'''// FT02_SW_SPI_FAST_A1: fast software write-side SPI for ESP32-S3.
// No hardware SPI controller is touched. The direct W1TS/W1TC registers only
// affect the selected output bit, so unrelated GPIOs are not read-modify-written.
void GxEPD2_EPD::_ft02BeginTransaction(const SPISettings& settings)
{
  if (_sck < 0) _pSPIx->beginTransaction(settings);
}

void GxEPD2_EPD::_ft02Transfer(uint8_t data)
{
  if (_sck < 0)
  {
    _pSPIx->transfer(data);
    return;
  }

#if defined(ARDUINO_ARCH_ESP32) && (!defined(FT02_EPD_FAST_SW_SPI) || FT02_EPD_FAST_SW_SPI)
  if ((_sck >= 0) && (_sck < 32) && (_mosi >= 0) && (_mosi < 32))
  {
    const uint32_t sckMask = (1UL << _sck);
    const uint32_t mosiMask = (1UL << _mosi);
    for (uint8_t bit = 0x80; bit != 0; bit >>= 1)
    {
      if (data & bit) REG_WRITE(GPIO_OUT_W1TS_REG, mosiMask);
      else            REG_WRITE(GPIO_OUT_W1TC_REG, mosiMask);
      REG_WRITE(GPIO_OUT_W1TS_REG, sckMask);
      // A tiny guard keeps the clock pulse conservative while remaining far
      // faster than digitalWrite()-based bit banging on the ESP32-S3.
      __asm__ __volatile__("nop; nop; nop; nop;");
      REG_WRITE(GPIO_OUT_W1TC_REG, sckMask);
    }
    return;
  }
#endif

  for (int i = 0; i < 8; i++)
  {
    digitalWrite(_mosi, (data & 0x80) ? HIGH : LOW);
    data <<= 1;
    digitalWrite(_sck, HIGH);
    digitalWrite(_sck, LOW);
  }
}

void GxEPD2_EPD::_ft02EndTransaction()
{
  if (_sck < 0) _pSPIx->endTransaction();
}

void GxEPD2_EPD::enableWrite(int16_t sck, int16_t mosi)
{
  _sck = sck;
  _mosi = mosi;
  if ((_sck >= 0) && (_mosi >= 0))
  {
    // Configure direction before touching the level. This also fixes the A1
    // boot warning: "IO xx is not set as GPIO".
    pinMode(_sck, OUTPUT);
    pinMode(_mosi, OUTPUT);
    digitalWrite(_sck, LOW);
    digitalWrite(_mosi, LOW);
  }
}

'''

    # Replace any prior FT02 helper block (A1 or FAST) up to enableRead().
    helper_re = re.compile(
        r"// FT02_SW_SPI_(?:PATCH_A1|FAST_A1):.*?(?=void GxEPD2_EPD::enableRead\(int16_t sck, int16_t mosi\))",
        re.S,
    )
    if helper_re.search(s):
        s = helper_re.sub(fast_impl, s, count=1)
    else:
        s = s.replace(anchor, fast_impl + anchor, 1)

    if s != original:
        path.write_text(s)
        return True
    return False


root = ensure_gxepd2()
h = root / "src" / "GxEPD2_EPD.h"
c = root / "src" / "GxEPD2_EPD.cpp"
changed_h = patch_header(h)
changed_c = patch_cpp(c)
print(
    f"[FT02-EPD-FAST] GxEPD2 overlay ready root={root} "
    f"header={'patched' if changed_h else 'ok'} cpp={'patched' if changed_c else 'ok'} "
    "backend=FAST-GPIO-SW-SPI"
)
