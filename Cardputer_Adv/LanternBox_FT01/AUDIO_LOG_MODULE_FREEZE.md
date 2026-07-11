# FT-01 Audio Log Module Freeze

Version: `v0.3.0-audio-log-final`

This is the confirmed stable baseline for the FT-01 audio log module.

## Confirmed functions

- Record audio to WAV.
- Save recording.
- Playback selected recording.
- Stop playback with `P`.
- Playback gain control with `U / D`.
- Audio list shows date / time / duration.
- Refresh list with `A`.
- Delete selected recording with `B`.
- Delete updates both WAV file state and `/lanternbox/audio/index.jsonl`.
- Stale index entries are filtered when the list is rebuilt.
- Log Help page includes all current controls.

## Module boundary

### `AudioLogger.h / AudioLogger.cpp`

Owns:

- microphone recording pipeline
- WAV writing
- playback pipeline
- audio gain
- audio list loading
- index maintenance
- deletion and stale index cleanup

Does not own:

- screen layout
- button help text
- page navigation

### `UiLog.h / UiLog.cpp`

Owns:

- log page drawing
- log page key handling
- delete confirmation UI
- help entry
- loading / refresh screen

Does not own:

- WAV internals
- SD index format details beyond AudioLogger API

## Stable data paths

```text
/lanternbox/audio/audio_###.wav
/lanternbox/audio/index.jsonl
```

## Controls

```text
P/Enter  播放
R 录音 | S 保存 | A 刷新
B 删除 | U/D 音量
```

## Do not regress

- Do not reintroduce full-audio RAM buffering.
- Do not scan all `audio_001.wav` to `audio_999.wav` on boot.
- Do not show stale index records if the WAV is missing.
- Do not move audio implementation into UI code.
- Do not add aggressive global SD guards around every SD call.
