#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


audio = (ROOT / "src" / "FT02_AudioLog.cpp").read_text(encoding="utf-8")
codec = (ROOT / "src" / "FT02_ES8311.cpp").read_text(encoding="utf-8")
self_test = (ROOT / "src" / "FT02_SystemSelfTest.cpp").read_text(encoding="utf-8")
platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
build_info = (ROOT / "src" / "FT02_BuildInfo.h").read_text(encoding="utf-8")

for pin, gpio in (("BCLK", 14), ("WS", 15), ("DOUT", 16), ("DIN", 17)):
    require(
        re.search(rf"FT02_AUDIO_{pin}_PIN\s*=\s*{gpio}\s*;", audio) is not None,
        f"{pin} is not on GPIO{gpio}",
    )

require("FT02_AUDIO_SAMPLE_RATE = 44100" in audio, "sample rate changed")
require("I2S_DATA_BIT_WIDTH_16BIT" in audio, "I2S is not 16-bit")
require("I2S_SLOT_MODE_STEREO" in audio, "I2S does not use two slots")
require("config.gpio_cfg.mclk = I2S_GPIO_UNUSED" in audio, "MCLK GPIO was allocated")
require("channelConfig.auto_clear_after_cb = true" in audio, "idle TX does not guarantee zero data")
require("FT02_AUDIO_ES8311_DIGITAL_MIC = false" in audio, "analog microphone path is not selected")
require("FT02_AUDIO_PA_SD_PIN = 4" in audio, "NS4150B SD/CTRL is not on GPIO4")
require("FT02_AUDIO_PLAYBACK_MAX_GAIN = 2.0f" in audio, "playback normalization is allowed above 2x")
require(re.search(r"FT02_ES8311_DAC_VOLUME\s*=\s*0xB8\s*;", codec) is not None, "DAC baseline is not -3.5dB / ~2/3 amplitude")

require("config.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT" in audio, "ESP32-S3 compatible default I2S clock is not selected")
require("I2S_CLK_SRC_APLL" not in audio, "unsupported ESP32-S3 APLL enum is still referenced")
require("I2C noise immunity 1" in codec and "I2C noise immunity 2" in codec, "ES8311 I2C immunity prewrites missing")
require("FT02_I2CBusLock" in codec, "codec I2C transactions are not guarded")
require("dacReg31_" in codec, "runtime DAC mute cache missing")
main_cpp = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
input_cpp = (ROOT / "src" / "FT02_InputManager.cpp").read_text(encoding="utf-8")
ui_cpp = (ROOT / "src" / "FT02_AudioLogUI.cpp").read_text(encoding="utf-8")
i2c_guard = (ROOT / "src" / "FT02_I2CBus.cpp").read_text(encoding="utf-8")
require("FT02_I2CBusLock" in input_cpp and "xSemaphoreCreateMutexStatic" in i2c_guard, "shared CardKB2/ES8311 I2C mutex missing")
handler = main_cpp[main_cpp.index("if(event.key == FT02_KEY_CHAR && event.command == 'r')"):main_cpp.index("if((event.key == FT02_KEY_SELECT)", main_cpp.index("if(event.key == FT02_KEY_CHAR && event.command == 'r')"))]
require(handler.find("FT02_AudioLogStartRecording") >= 0, "record start call missing")
require("FT02_DrawAudioLogRecordingBodyPartial" in handler, "recording cue-before-capture was not restored")
require(handler.find("FT02_DrawAudioLogRecordingBodyPartial") < handler.find("FT02_AudioLogStartRecording"), "recording starts before cue commit")
record_refresh = main_cpp[main_cpp.index("if(audioState == FT02_AUDIO_LOG_RECORDING"):main_cpp.index("else if(audioState == FT02_AUDIO_LOG_PLAYING", main_cpp.index("if(audioState == FT02_AUDIO_LOG_RECORDING"))]
require("FT02_DrawAudioLogRecordingTimerPartial" in record_refresh, "recording timer partial refresh is missing")

expected_clock_registers = {
    "FT02_ES8311_CLOCK_REG01": "0xBF",
    "FT02_ES8311_CLOCK_REG02": "0x18",
    "FT02_ES8311_CLOCK_REG03": "0x10",
    "FT02_ES8311_CLOCK_REG04": "0x10",
    "FT02_ES8311_CLOCK_REG05": "0x00",
    "FT02_ES8311_CLOCK_REG06": "0x03",
    "FT02_ES8311_CLOCK_REG07": "0x00",
    "FT02_ES8311_CLOCK_REG08": "0xFF",
}
for name, value in expected_clock_registers.items():
    require(
        re.search(rf"{name}\s*=\s*{value}\s*;", codec) is not None,
        f"unexpected 1.4112MHz/44.1kHz coefficient: {name}",
    )

require("{0x18, 0x19}" in codec, "both legal ES8311 I2C addresses are not probed")
require("digitalMic ? 0x5A : 0x1A" in codec, "ES8311 microphone selector table changed")
require(codec.count("0x0C") >= 3, "16-bit Philips I2S register value is missing")
require("if(readRegister(reg, actual) && actual == value) return true;" in codec, "codec register readback verification missing")

hardware_start_match = re.search(r"static bool FT02_AudioHardwareStart\(\).*?\n}\n", audio, re.DOTALL)
require(hardware_start_match is not None, "FT02_AudioHardwareStart not found")
hardware_start = hardware_start_match.group(0)
require(
    hardware_start.find("FT02_AudioInitI2s()") < hardware_start.find("FT02_AudioInitCodec()"),
    "ES8311 is configured before SCLK is started",
)

require("WM8960" not in audio, "WM8960 remains in audio runtime")
require("WM8960" not in platformio, "WM8960 dependency remains")
require("ES8311与I2S正常" in self_test, "self-test success text was not migrated")
require("ES8311无响应" in self_test, "self-test codec failure text was not migrated")
version_text = (ROOT / "VERSION.txt").read_text(encoding="utf-8").strip()
require(version_text == "v2.75k91", "firmware version mismatch")
require('FT02_FIRMWARE_VERSION "v2.75k91"' in build_info, "BuildInfo/version mismatch")

require("DAC idle mute" in codec, "codec does not finish initialization muted")
require("DAC idle power down" in codec, "codec does not power DAC down while idle")
require("FT02_AudioPaSet(false)" in audio and "FT02_AudioPaSet(true)" in audio, "PA runtime gate missing")
require("setDacMuted(bool muted)" in codec, "runtime DAC mute API missing")
require("FT02_AUDIO_PLAYBACK_PRIME_BLOCKS = 2" in audio, "playback zero-prime missing")
require("g_ft02AudioCodec.setDacMuted(false)" in audio, "playback has no explicit DAC unmute")
require(audio.count("g_ft02AudioCodec.setDacMuted(true)") >= 2, "idle/record mute guards missing")
require("prepareDacForPaShutdown" in codec and "powerDownDac" in codec, "split pop-safe DAC shutdown API missing")

bclk = 44100 * 16 * 2
require(bclk == 1_411_200, "BCLK calculation failed")


# Clean capture regression gates
require('FT02_AUDIO_PSRAM_STAGE_TARGET_BYTES = 4u * 1024u * 1024u' in audio, "4 MiB PSRAM staging missing")
require('FT02_InputSetLowNoiseMode' in input_cpp, "low-noise CardKB mode missing")
require('FT02_INPUT_LOW_NOISE_POLL_MS = 100' in input_cpp, "CardKB low-noise interval mismatch")
require('FT02_InputSetLowNoiseMode(FT02_AudioLogIsCapturing())' in main_cpp, "audio capture does not activate low-noise input mode")
require('FT02_AUDIO_RECORD_START_SETTLE_MS = 900' in audio, "post-EPD capture settle missing")
require('FT02_AUDIO_BURST_TRIGGER = 30000' in audio, "saturated burst threshold missing")
require('FT02_AUDIO_BURST_MAX_RUN = 128' in audio, "saturated burst window missing")
require('secondBurstRepairSamples' in audio, "saturated burst diagnostics missing")
require('config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512' in audio, "S3 512x internal MCLK resolution missing")
require('g_ft02AudioRecordRestartGuardUntilMs' in main_cpp, "post-record R guard missing")
require('physicalKeyStillDown' in main_cpp, "queued-command release handling missing")
require('2500u' in audio, "AudioLog post-record restart guard missing")

# Clean playback and pop-safe stop regression gates
require('FT02_AUDIO_PLAYBACK_PSRAM_MAX_BYTES = 4u * 1024u * 1024u' in audio, "playback PSRAM staging missing")
require('FT02_AudioWriteAll' in audio and 'portMAX_DELAY' in audio, "full blocking TX writer missing")
require('Rewind this block and retry' not in audio, "unsafe full-block rewind remains")
require('i2s_channel_disable(g_ft02AudioRx)' in audio and 'g_ft02AudioPlayback.rxSuspended = true' in audio, "RX suspend clean playback missing")
play_refresh = main_cpp[main_cpp.index('else if(audioState == FT02_AUDIO_LOG_PLAYING)'):main_cpp.index('    }\n    if(g_ft02PageState == FT02_PAGE_AUDIO_LOG_LIST', main_cpp.index('else if(audioState == FT02_AUDIO_LOG_PLAYING)'))]
require('FT02_DrawAudioLogPlayingStatusPartial' not in play_refresh, "playback EPD timer refresh must remain disabled")
require('playbackElapsedMs /' not in play_refresh, "playback elapsed bucket calculation must remain disabled")
require('FT02_AUDIO_PLAYBACK_FADE_BLOCKS = 4' in audio, "manual-stop fade length missing")
require('FT02_AudioWriteFadeToZero()' in audio, "manual-stop PCM fade path missing")
require('static const uint8_t kRamp[]' in codec and 'prepareDacForPaShutdown' in codec, "DAC shutdown gain ramp missing")
require(audio.find('prepareDacForPaShutdown()') < audio.find('FT02_AudioPaSet(false)', audio.find('prepareDacForPaShutdown()')), "PA is shut down before DAC soft-mute preparation")


# Static total-duration and user-volume safe-refresh gates
require('总时长' in ui_cpp, "static total-duration label missing")
playing_status = ui_cpp[ui_cpp.index('static void FT02_DrawAudioPlayingStatusContent'):ui_cpp.index('static void FT02_DrawAudioPlayingContent')]
require('playbackElapsedMs' not in playing_status, "elapsed playback time is still formatted in the UI")
require('playbackDurationMs' in playing_status, "total playback duration is missing")
require('FT02_DrawAudioLogPlayingStatusPartial' not in ui_cpp, "playback status partial renderer still exists")
require('[AUDIO-EPD] playback timer pending' not in main_cpp, "playback timer scheduler still exists")
require('[AUDIO-EPD] quiet-window refresh' not in main_cpp, "quiet-window playback refresh path still exists")
require('FT02_AudioLogFindPlaybackEpdQuietWindow' not in main_cpp, "quiet-window finder is still reachable from UI")
require('[AUDIO-EPD] playback timer pending' not in main_cpp, "automatic playback timer refresh returned")
require('FT02_AudioLogFindPlaybackEpdQuietWindow' not in main_cpp, "automatic quiet-window scheduler returned")
require('FT02_AudioLogFindPlaybackEpdQuietWindow' not in audio, "abandoned quiet-window scanner remains in audio runtime")
require('FT02_AudioLogSetPlaybackEpdGuard(true)' in main_cpp, "user volume EPD guard missing")
require('FT02_DrawAudioLogPlayingVolumePartial' in main_cpp, "user volume partial refresh missing")
require('FT02_AUDIO_VOLUME_REFRESH_COALESCE_MS = 220u' in main_cpp, "volume refresh coalescing missing")
require('FT02_EpdPowerOffAfterCommit(display)' in ui_cpp, "playback cue does not return to EPD-off clean-audio policy")
require('FT02_DrawAudioLogPlayingVolumePartial' in ui_cpp, "volume-only partial renderer missing")
volume_fn = ui_cpp[ui_cpp.index('void FT02_DrawAudioLogPlayingVolumePartial'):]
require('FT02_EpdInteractivePowerOffNow(display)' in volume_fn, "safe volume EPD power-off path missing")
require('FT02_DrawAudioLogPlayingStatusPartial' not in ui_cpp, "elapsed/status playback partial renderer returned")

print(
    "PASS ES8311 I2C=47/21 I2S=14/15/16/17 PA_SD=GPIO4 MCLK=NC "
    "I2S=44100Hz/16-bit/2-slot BCLK=1411200Hz AMIC=ON "
    "DAC=IDLE_POWERDOWN PA=IDLE_SHUTDOWN S3-CLOCK=DEFAULT/512x I2C-GUARD=ON "
    "EPD-REC-TIMER=ON PSRAM-REC=4MiB(~47.5s) PSRAM-PLAY=4MiB FULL-TX=ON STOP-FADE=46.4ms POP-SAFE=ON RX-SUSPEND=ON EPD-PLAY-TIMER=OFF STATIC-TOTAL-DURATION=ON VOLUME-SAFE-REFRESH=ON CARDKB=10Hz EPD+SD-SETTLE=900ms BURSTFIX<=2.9ms playback-zero-prime=2 normalize<=2x DAC_BASE=-3.5dB(~0.668x)"
)
