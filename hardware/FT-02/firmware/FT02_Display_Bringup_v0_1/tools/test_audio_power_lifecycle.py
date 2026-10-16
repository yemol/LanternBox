#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
audio = (ROOT / "src/FT02_AudioLog.cpp").read_text(encoding="utf-8")
header = (ROOT / "src/FT02_AudioLog.h").read_text(encoding="utf-8")
codec = (ROOT / "src/FT02_ES8311.cpp").read_text(encoding="utf-8")
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")

def req(ok, msg):
    if not ok:
        raise SystemExit(f"FAIL: {msg}")

req("FT02_AUDIO_OWNER_JOURNAL" in header, "journal owner missing")
req("FT02_AUDIO_OWNER_SYSTEM_TEST" in header, "system-test owner missing")
req("FT02_AudioLogAcquire" in header and "FT02_AudioLogRelease" in header, "owner API missing")

begin = re.search(r"bool FT02_AudioLogBegin\(\).*?\n}\n", audio, re.S)
req(begin is not None, "AudioLogBegin missing")
req("FT02_AudioInitI2s()" not in begin.group(0), "boot still starts I2S")
req("FT02_AudioInitCodec()" not in begin.group(0), "boot still starts ES8311")
req("FT02_AudioPaInit()" in begin.group(0), "boot no longer forces PA shutdown")

start = re.search(r"static bool FT02_AudioHardwareStart\(\).*?\n}\n", audio, re.S)
req(start is not None, "hardware start helper missing")
req(start.group(0).find("FT02_AudioInitI2s()") < start.group(0).find("FT02_AudioInitCodec()"), "I2S must precede codec")
req("FT02_AudioEnsureServiceTask()" in start.group(0), "audio task not lifecycle managed")

stop = re.search(r"static bool FT02_AudioHardwareStop\(\).*?\n}\n", audio, re.S)
req(stop is not None, "hardware stop helper missing")
req(stop.group(0).find("g_ft02AudioCodec.suspend()") < stop.group(0).find("FT02_AudioDeinitI2s()"), "codec must suspend while BCLK still exists")
req("FT02_AudioPaSet(false)" in stop.group(0), "PA is not shut down")
req("FT02_AudioSuspendServiceTask()" in stop.group(0), "audio task keeps polling while off")

req("i2s_del_channel(g_ft02AudioRx)" in audio and "i2s_del_channel(g_ft02AudioTx)" in audio, "I2S channels are not deleted")
for pin in ("FT02_AUDIO_BCLK_PIN", "FT02_AUDIO_WS_PIN", "FT02_AUDIO_DOUT_PIN", "FT02_AUDIO_DIN_PIN"):
    req(f"pinMode({pin}, INPUT)" in audio, f"{pin} not returned high-Z")

for token in [
    "ES8311_DAC_REG32, 0x00",
    "ES8311_ADC_REG17, 0x00",
    "ES8311_SYSTEM_REG0E, 0xFF",
    "ES8311_SYSTEM_REG12, 0x02",
    "ES8311_SYSTEM_REG14, 0x00",
    "ES8311_SYSTEM_REG0D, 0xFC",
    "ES8311_CLK_MANAGER_REG01, 0x00",
]:
    req(token in codec, f"codec suspend step missing: {token}")

req("FT02_AudioLogAcquire(FT02_AUDIO_OWNER_JOURNAL)" in main, "journal page does not acquire audio")
req("FT02_AudioLogAcquire(FT02_AUDIO_OWNER_SYSTEM_TEST)" in main, "explicit system test does not acquire audio")
req("FT02_AudioLogPrepareForSleep" in header, "verified audio sleep preflight missing")
req("Device Status is read-only by default" in main, "device status still has side-effectful open policy")
req("FT02_AudioLogRelease(FT02_AUDIO_OWNER_JOURNAL)" in main, "journal audio is never released")
req("FT02_AudioLogRelease(FT02_AUDIO_OWNER_SYSTEM_TEST)" in main, "system-test audio is never released")

setup = re.search(r"void setup\(\).*?\n}\n\nvoid loop", main, re.S)
req(setup is not None, "setup not found")
req("FT02_AudioLogBegin();" in setup.group(0), "audio data model no longer begins at boot")

print("PASS audio-on-demand: boot=OFF journal=ON selftest=ON PA=OFF-when-idle codec=suspend I2S=deleted task=suspended pins=high-Z")
