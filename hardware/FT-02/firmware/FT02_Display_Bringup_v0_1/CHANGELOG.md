# FT-02 v2.75k91 - Mode & Sleep System A1.6

- Closed the autonomous sleep-exit/reset issue on the current FT-02 hardware baseline.
- Preserved 1000 ms ESP32-S3 Light Sleep slices for CardKB2 I2C polling; normal background services remain excluded from the sleep loop.
- Stabilized repeated Sleep → Wake → Sleep cycles, including native USB CDC recovery.
- Added explicit LR01 host pause/resume and MAX17048 pause/resume around the sleep lifecycle.
- Preserved GNSS runtime restoration through the existing mode policy; outdoor GNSS data was verified to resume after wake.
- Removed temporary RTC blackbox, isolation-switch and GNSS/Battery wake-probe code from the production baseline.
- Retained only the lightweight NVS sleep breadcrumb used by Device Status to identify a reset that occurred during sleep.
- Deferred deep LoRa/GNSS power optimization until those functions migrate from LR01 back to Core ownership.

# FT-02 v2.75k78 - Mode & Sleep System A1.4.5 DIAG

- Added a persistent Sleep Black Box because USB-CDC serial silence during light sleep is not sufficient evidence that the Core did not wake.
- Every accepted keyboard wake is written to NVS before EPD reinitialization: raw key byte, sleep-slice count, sleep duration and cumulative wake count.
- Sleep entry is marked active in NVS. If setup() runs again before a normal wake closes the session, the black box records a reset-during-sleep event and the ESP reset reason.
- Device Status -> Startup State now appends the last persistent sleep-exit breadcrumb.
- No wake policy changes in this diagnostic build; CardKB2 filtering, 1 s light-sleep cadence and EPD reset clamp remain unchanged.

# FT-02 v2.75k77 - Mode & Sleep System A1.4.4

- Fixed a display-only false-wake condition observed specifically while charging.
- Root cause in A1.4.3: the EPD control pins were released after `SPI.end()`, including RESET; charging/source transitions could partially repower or disturb the panel while RESET floated, allowing the e-paper controller to visibly change even though the Core remained in Sleep.
- EPD RESET is now held actively LOW for the entire sleep state while CS/DC/MOSI/SCK remain high-impedance to avoid phantom power.
- Sleep service reasserts `EPD_PWR=LOW` and `EPD_RST=LOW` after every 1 s timer wake.
- Corrected the A1.4 pin-order bug that produced `IO 11/12 is not set as GPIO` errors after `SPI.end()`.
- No change to LR01, GNSS mode ownership, CardKB2 wake filtering, indoor/outdoor sleep policy, or the 1 s Core light-sleep cadence.

# FT-02 v2.75k76 - Mode & Sleep System A1.4.3

- Fixed unintended autonomous wake-ups from Sleep caused by non-user CardKB2 bytes observed on the periodic I2C poll path.
- Sleep wake now accepts only plausible CardKB2 key bytes; malformed/control values such as 0xFF are ignored instead of tearing down Sleep.
- Wake gate now requires two consecutive empty sleep polls after entry/repeat draining before it is armed, improving long-P/stale-queue immunity.
- Added wake diagnostics that report the accepted raw CardKB2 byte and ignored invalid bytes for field verification.
- Periodic timer wake source is explicitly disabled when a genuine keyboard wake transitions back to the normal UI.
- Preserved A1.4.2 behavior: Indoor may auto-sleep, Outdoor never auto-sleeps, both modes can still enter Sleep manually.

# FT-02 v2.75k75 - Mode & Sleep System A1.4.2

- Changed automatic sleep policy to **Indoor mode only**.
- Outdoor mode never enters Sleep because of the configured idle timeout.
- Manual long-P sleep remains available in both Indoor and Outdoor modes.
- Added a defensive mode check inside the automatic sleep entry path so future callers cannot accidentally auto-sleep Outdoor mode.
- System Settings now labels the option as `室内自动休眠` to match actual behavior.
- Preserved A1.4.1 GNSS/status behavior and all A1.4 Main-Core low-power changes.

# FT-02 v2.75k74 - Mode & Sleep System A1.4.1

- GNSS status bar no longer shows `室内关闭` in Indoor mode.
- Indoor mode still powers GNSS off exactly as before; the status now simply shows `已关闭`.
- No changes to GNSS power policy, sleep backend, P-key behavior, LR01 protocol, or other UI logic.

# FT-02 v2.75k73 - Mode & Sleep System A1.4

- Main Core sleep poll interval increased from 220 ms to 1000 ms, reducing timer-wake frequency to ~1 Hz while preserving CardKB2 I2C wake polling.
- Removed `Serial.flush()` from every sleep slice; debug output is flushed once at sleep entry only.
- Added EPD signal high-impedance isolation during sleep to prevent phantom back-power through CS/DC/RST/MOSI/SCK after the display rail is shut off.
- EPD signal pins and HSPI are restored before the existing full-mode wake reinitialization.
- Sleep loop still returns before all normal background services; each periodic wake only polls CardKB2 and immediately re-enters Light Sleep if no key is present.
- LR01/SX1262 behavior is intentionally unchanged because LR01 is scheduled for removal when LoRa/GNSS migrate to Main Core.

# FT-02 v2.75k72 - Mode & Sleep System A1.3

- Sleep screen is now borderless: removed the outer technical frame so the LanternBox mountain composition fills the full 800x480 panel.
- Rebalanced sleep-page layout for edge-to-edge use: masthead moved up, logo/title enlarged in presence, footer/status bands widened, and mountain / pine artwork extended to the screen edges.
- Reworked sleep backend for deeper power saving: replaced the active 10 ms delay loop with periodic timer-based light sleep slices while waiting for keyboard wake.
- Sleep still preserves keyboard wake behavior by polling CardKB2 over I2C, but the ESP32-S3 now idles in light sleep between polls instead of spinning awake all night.
- Powered down the EPD SPI bus while sleeping and reinitialized it on wake together with the existing EPD rail restore path.
- Preserved A1.2 behavior: mode system, GNSS sleep-off policy, wake quiet gate, P long-press handling, settings, communication warning fix, and all prior page logic.

# FT-02 v2.75k71 - Mode & Sleep System A1.2

- Replaced the A1.1 geometric sleep page with the user-approved LanternBox mountain composition.
- Uses the official 壳中灯 logo as a pre-rasterized 1-bit bitmap for the e-paper panel.
- Added monochrome alpine ridges, pine clusters, sun/wind-line accents, a technical frame, dynamic previous-mode text, and dynamic sleep-entry battery status.
- Preserved A1.1 behavior: GNSS wake grace, non-sticky communication warnings, event-stream P gesture handling, sleep/wake quiet gates, mode ownership, settings and low-power backend.
- No changes to CardKB2/Pinyin, LR01 protocol, map, storage, audio, battery monitoring or compass behavior.

# FT-02 v2.75k70 - Mode & Sleep System A1.1

- 修复系统设置标题缺失“系统”：标题改用完整 CJK 字库。
- 休眠页重做为 LanternBox / 壳中灯品牌延展：六边形保护壳 + 中央灯芯，替换月牙山景。
- 室内切户外后增加 12 秒 GNSS 唤醒宽限，状态栏显示“唤醒中”而不是瞬时“GNSS离线”。
- 通讯错误自检改为“本次新增才 WARN”；LR01 历史累计计数仅作为诊断信息，不再永久挂警告。
- P 键手势改为基于重复事件流识别，不再把 CardKB2 的零采样误判成松手；长按 P 不再反复切模式/立即唤醒。
- 睡眠唤醒与唤醒键吞键改为安静窗口判定，抵抗 CardKB2 按住键的重复事件。

# FT-02 v2.75k69 - Mode & Sleep System A1

- Added Indoor / Outdoor / Sleep system states with persistent last-awake mode.
- GNSS power is now owned by system mode: Indoor=OFF, Outdoor=ON, Sleep=OFF. Device Status no longer exposes a competing manual GNSS toggle.
- Added raw physical P gesture on Home only: short P toggles Indoor/Outdoor; hold P for 1 second enters Sleep. The normalized `command=p` path is intentionally ignored so Sym-layer mappings and Pinyin input do not collide.
- Added automatic idle sleep after 5 / 15 / 30 minutes or Never. Active audio, location recording, GNSS field test, compass calibration and pending map refresh block automatic sleep.
- Added System Settings for automatic sleep, boot mode and wake-return policy.
- Added a static sleep screen with retained e-paper image, previous mode and sleep-entry battery percentage.
- Sleep A1 powers off the EPD rail, shuts down Wi-Fi/audio owners, forces GNSS off, lowers ESP32-S3 CPU frequency to 80 MHz and leaves only low-rate CardKB2 polling active on the Core.
- Wake consumes the first wake key until physical release and restores the previous page or Home according to settings.
- Swapped Home positions of Field Library and Device Status and added a System Settings card.
- Updated location-recorder GNSS guidance to tell the user to switch to Outdoor mode.
- Preserved Low Power A2 scheduler, Audio On-Demand A1, CardKB2/Pinyin mappings, map, storage, compass and LR01 protocol behavior.
- Limitation: true GPIO/light-sleep keyboard wake is not yet implemented; existing LR01 protocol also has no LoRa radio-sleep command.

# FT-02 v2.75k68 - Low Power A2

- Reduced normal FT-02 scheduler cadence from 5 ms (200 Hz) to 20 ms (50 Hz); audio playback keeps the validated 5 ms cadence.
- Reduced MAX17048 background polling from 1 s to 10 s.
- Cached MAX17048 VERSION/CHIPID after first validation; live VCELL/SOC/CRATE/MODE remain sampled.
- Re-validates cached battery identity after repeated I2C failures.
- Preserved CardKB2 mappings/repeat policy, audio on-demand A1, system self-test, shared I2C, GNSS, LoRa, Wi-Fi, EPD, map and storage behavior.

# FT-02 v2.75k68 - Audio On-Demand A1

- Audio hardware is OFF after boot by default.
- Voice Journal acquires ES8311/I2S on entry and releases it on exit.
- Device Status/System Self-Test also acquires ES8311/I2S so audio diagnostics remain valid.
- NS4150B PA stays shut down outside playback; ES8311 uses full suspend on final owner release.
- I2S RX/TX channels are deleted while idle and BCLK/WS/DOUT/DIN return to high impedance.
- Shared I2C/Wire remains active for CardKB2 and other FT-02 devices.
- Recording/playback formats, controls, audio quality settings and existing UI behavior are unchanged.

# FT-02 v2.75k68 - Compass UI A3 Approved Instrument Layout

- Reworked the standalone compass page to match the approved instrument-style mockup.
- Added a rotating 0–330 degree scale, 5-degree ticks, fixed top triangle pointer, center reticle and Chinese eight-direction labels.
- Added a large bottom heading readout such as `70° 东` with `磁航向`.
- Preserved the corrected `bearing - heading` dial direction and left/right orientation.
- Removed only the compass page's old right-side information column; calibration remains available through ENTER.
- LR01 heading, GNSS, map navigation, Reader, Audio, FieldManual, Input, board and partition configuration are unchanged.

# FT-02 v2.75k68 - Compass Dial Mirror Restore A1

- Fixed the standalone compass UI rotating-dial transform so bearings use `bearing - heading`.
- Removed the left/right mirrored dial behavior introduced by the earlier compass dial direction fix.
- Kept LR01 heading data, GNSS, map arrow behavior, calibration, board/flash configuration, Reader, Audio, FieldManual and Input unchanged.

# FT-02 v2.75k68 - Shanghai 260918 Map Refresh / Residential Labels A1

- Map dataset paths moved from `shanghai-260726` to `shanghai-260918`.
- PBF cache version bumped to 7 so stale 260726 geometry caches cannot be reused.
- Named `landuse=residential`, `place=neighbourhood`, `place=quarter`, and `place=suburb` ways can now contribute labels without drawing residential polygon outlines.
- Residential/community labels are shown from Z18 upward.
- Chinese name preference is now `name:zh-Hans` -> `name:zh` -> `name` -> `name:en`.
- Map search paths moved to the 260918 dataset ID.

# Changelog

## v2.75k67

- Fixed a production-cleanup build regression in Field Library queued-navigation coalescing after debug trace removal.
- Fixed duplicate and persistent UART warnings in System Self-Test.
- Improved Host UART protocol identification when the LR01 startup banner is missed.
- Added intentional-GNSS-OFF handling to map and location recorder flows.
- Preserved MAX17048 battery monitoring, standalone compass/calibration, GNSS software power control, Wi-Fi, Reader, Audio, Field Library, FieldManual, Map, LoRa and Input behavior.
- Cleaned production logging: normal runtime paths are quiet; hardware, storage, protocol and media failures remain diagnosable.
- Removed obsolete refresh-source trace parameters and historical debug markers from production code and regression tests.

## v2.75k66

- Integrated validated MAX17048 battery monitoring with standalone compass and LR01 GNSS software power control.
