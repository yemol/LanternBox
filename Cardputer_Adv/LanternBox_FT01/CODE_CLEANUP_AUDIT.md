# FT-01 Code Cleanup Audit

Version: `v0.3.3-cleanup-audit`

Base: `v0.3.2a-device-compile-fix`

## Cleanup performed

- Removed unused navigation page enum: NAV_PAGE_HELP.
- Removed unused navPreviousPage after HelpManager migration.

## Module boundaries

| Module | Files | Status |
|---|---|---|
| Main / app shell | `LanternBox_FT01.ino` | Still large; owns boot, home, SD/GNSS, device page, status page, recorder bridge |
| Audio log | `AudioLogger.*`, `UiLog.*` | Good separation; treat as frozen stable subsystem |
| Navigation | `UiNavigation.*` | Mostly separated; still depends on global SD/GNSS/device state from main |
| Recorder / path | `UiRecorder.*` plus recorder state in `.ino` | Partially separated; business state still mostly in main |
| Help | `HelpManager.*` | Separated and reused by log/nav/device |

## Current file sizes

```json
{
  "LanternBox_FT01.ino": 56091,
  "UiRecorder.cpp": 5281,
  "UiRecorder.h": 223,
  "README.md": 15391,
  "UiNavigation.h": 393,
  "UiNavigation.cpp": 30784,
  "HelpManager.h": 323,
  "HelpManager.cpp": 3675,
  "UiLog.h": 345,
  "UiLog.cpp": 11537,
  "AudioLogger.h": 3584,
  "AudioLogger.cpp": 19207,
  "AUDIO_LOG_MODULE_FREEZE.md": 1592,
  "AUDIO_LOG_FREEZE_MANIFEST.json": 543,
  "NAVIGATION_UI_NOTES.md": 672
}
```

## Candidate unused functions

These are static-analysis candidates only. Do not delete blindly without compiling and device testing.

```text
baseDisplayText  (LanternBox_FT01.ino) usage=1
baseStateText  (LanternBox_FT01.ino) usage=1
countdownDisplayText  (LanternBox_FT01.ino) usage=1
displaySessionId  (LanternBox_FT01.ino) usage=1
displaySessionIdForCard  (LanternBox_FT01.ino) usage=1
drawAudioRow  (UiLog.cpp) usage=1
drawAutoIconSmall  (LanternBox_FT01.ino) usage=1
drawBatteryIcon  (LanternBox_FT01.ino) usage=1
drawClockIcon  (LanternBox_FT01.ino) usage=1
drawInfoCard  (LanternBox_FT01.ino) usage=1
drawPinIcon  (LanternBox_FT01.ino) usage=1
drawSdIconSmall  (LanternBox_FT01.ino) usage=1
gnssStateText  (LanternBox_FT01.ino) usage=1
pointsDisplayText  (LanternBox_FT01.ino) usage=1
recorderModeText  (LanternBox_FT01.ino) usage=1
satelliteDisplayText  (LanternBox_FT01.ino) usage=1
shortSessionText  (LanternBox_FT01.ino) usage=1
```

## Candidate unused variables

These are static-analysis candidates only. Some globals are intentionally reserved for future modules.

```text
NAV_BLUE  (UiNavigation.cpp) usage=1
```

## Markers found

```text
LanternBox_FT01.ino:50  // Serial debug switches.
LanternBox_FT01.ino:52  static const bool DEBUG_NMEA_RAW = false;
LanternBox_FT01.ino:53  static const bool DEBUG_GNSS_SUMMARY = true;
LanternBox_FT01.ino:174  if (!DEBUG_GNSS_SUMMARY) return;
AudioLogger.cpp:230  // Critical: do not show stale index entries.
```

## Architecture notes

1. `AudioLogger.*` and `UiLog.*` are now the cleanest modules. Keep them frozen.
2. `UiNavigation.*` is acceptable as a standalone UI/navigation module, but it still reads global GNSS and SD state from `LanternBox_FT01.ino`.
3. `LanternBox_FT01.ino` is still the biggest file and owns too many responsibilities:
   - SD init
   - GNSS parse
   - recorder/path data
   - home/status/device pages
   - app screen routing
4. Next safe refactor should not touch audio. The best next split is:
   - `DeviceStatus.*` for battery / SD / GNSS status helpers
   - `UiDevice.*` for device page drawing / key handling
   - later `StorageManager.*` for SD directory and append helpers

## Recommended next cleanup stages

### Stage 1: no-risk freeze

- Keep this version as the current cleaned baseline.
- Compile and smoke-test:
  - home
  - log
  - nav
  - device page
  - recorder

### Stage 2: low-risk file split

Move only device page UI from `.ino` to:

```text
UiDevice.h
UiDevice.cpp
```

No business logic changes.

### Stage 3: medium-risk service split

Move SD/GNSS status helpers to:

```text
DeviceStatus.h
DeviceStatus.cpp
```

Only after Stage 2 is stable.
