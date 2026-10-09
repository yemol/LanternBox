#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def req(c,m):
    if not c: raise SystemExit('FAIL: '+m)
fl=(ROOT/'src/FT02_FieldLibrary.cpp').read_text()
inp=(ROOT/'src/FT02_InputManager.cpp').read_text()
epd=(ROOT/'src/FT02_EpdInteractiveRuntime.cpp').read_text()
build=(ROOT/'src/FT02_BuildInfo.h').read_text()
version=(ROOT/'VERSION.txt').read_text().strip()
req(version == 'v2.84-sleep-power-a5c-stable-r1.1','version')
req('"v2.84-sleep-power-a5c-stable-r1.1"' in build,'BuildInfo/version')
req('const int px = FT02_LIBRARY_READING_PARTIAL_X;' in fl,'reader x constant')
req('const int py = FT02_LIBRARY_READING_PARTIAL_Y;' in fl,'reader y constant')
req('const int pw = FT02_LIBRARY_READING_PARTIAL_W;' in fl,'reader w constant')
req('const int ph = FT02_LIBRARY_READING_PARTIAL_H;' in fl,'reader h constant')
req('FT02_INPUT_DISCRETE_FAST_REPRESS_MS = 270' in inp,'adaptive fast re-press')
req('FT02_INPUT_REPEAT_START_MIN_MS = 270' in inp and 'FT02_INPUT_REPEAT_START_MAX_MS = 340' in inp,'adaptive repeat start window')
req('FT02_DrainQueuedSameDiscreteRaw' in inp,'repeat backlog drain')
req('FT02_INPUT_ACTIVITY_QUIET_MS = 220' in inp,'stable active raw window')
req('key == FT02_KEY_UP || key == FT02_KEY_DOWN' in inp,'direction keys are discrete')
req('static_cast<uint8_t>(raw) == 0xFFu' in inp,'invalid 0xFF filter')
# The deprecated EPD dispatch-owner path must remain absent.
req('FT02_EpdInteractiveBeginInputDispatch' not in epd,'old EPD input-owner path returned')
ui=(ROOT/'src/FT02_AudioLogUI.cpp').read_text()
main=(ROOT/'src/main.cpp').read_text()
audio_h=(ROOT/'src/FT02_AudioLog.h').read_text()
audio=(ROOT/'src/FT02_AudioLog.cpp').read_text()
req('FT02_DrawAudioLogPlayingVolumePartial' in ui,'volume partial renderer')
req('FT02_AUDIO_PLAY_VOLUME_Y = 316' in ui,'volume partial y')
req('FT02_EpdInteractivePowerOffNow(display)' in ui,'EPD off while guarded')
req('FT02_AUDIO_VOLUME_REFRESH_COALESCE_MS = 220u' in main,'volume coalesce')
req('FT02_AudioLogSetPlaybackEpdGuard(true)' in main and 'FT02_AudioLogSetPlaybackEpdGuard(false)' in main,'volume EPD guard path')
req('bool FT02_AudioLogSetPlaybackEpdGuard(bool enabled);' in audio_h,'guard public declaration')
# Volume curve: level 1 is half of the previous baseline; later step deltas stay unchanged.
expected_volume_table = '0.0f, 0.10f, 0.18f, 0.30f, 0.46f, 0.60f,\n    0.75f, 0.90f, 1.10f, 1.35f, 1.60f'
req(expected_volume_table in audio, 'volume gain table')
old = [0.20,0.28,0.40,0.56,0.70,0.85,1.00,1.20,1.45,1.70]
new = [0.10,0.18,0.30,0.46,0.60,0.75,0.90,1.10,1.35,1.60]
req(abs(new[0] - old[0] * 0.5) < 1e-9, 'level 1 is not half of k55')
req(all(abs((new[i]-new[i-1])-(old[i]-old[i-1])) < 1e-9 for i in range(1,10)), 'step increments changed')

req('PARTIAL_HOT_IDLE_MS = 350u' in epd,'hot-idle is not 350 ms')
req('PARTIAL_HOT_IDLE_MS = 2500u' not in epd,'old 2500 ms hot-idle returned')
print('PASS keyboard-adaptive-a2 post-refresh=350ms input=adaptive300/50 activity220 text-repeat180 audio-time=static')
