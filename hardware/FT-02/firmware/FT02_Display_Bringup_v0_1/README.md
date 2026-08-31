# FT-02 v2.75k57 Production Baseline

这是 FT-02 当前可提交、可归档的生产源码基线。固件功能冻结在 **v2.75k57**；本包只做生产化清理，不引入新的运行行为。

## 冻结功能

- ESP32-S3 / 16 MB Flash / 8 MB PSRAM。
- CardKB2：SDA 47、SCL 21、0x5F。
- LR01：RX 7、TX 13，导航与无线单一权威源。
- SD：FSPI Mode3 40 MHz，SCK 42 / MOSI 2 / MISO 1 / CS 41。
- ES8311：BCLK 14 / WS 15 / DOUT 16 / DIN 17，AMIC，MCLK NC。
- NS4150B：SD/CTRL GPIO4；闲置、录音时硬关断。
- 音频播放：PSRAM clean playback、Full-TX、播放期间不刷新计时、静态总时长、46.4 ms pop-safe stop。
- 播放音量：1/10 = 0.10x；后续档位保持既有增量；用户主动调音量时采用 PA-off 安全刷新。
- 录音：PSRAM staging、900 ms EPD/SD settle、5 秒录音计时保留。
- Field Library v0.8.2：Reader clean-slate 刷新 x=0,y=86,w=800,h=394。
- SELECT/BACK/HELP：稳定释放后重新武装；方向键和文本输入保持正常重复。
- 通用 EPD partial hot-idle：350 ms；加上实机约 140 ms powerOff，尾部生命周期控制在约 0.5 秒内。

## 构建

```bash
platformio run
platformio run --target upload --upload-port /dev/cu.usbmodem2201
platformio device monitor --port /dev/cu.usbmodem2201
```

生产环境基于：

- `platform-espressif32 55.03.311`
- `framework-arduinoespressif32 3.3.11`
- `esp32-s3-devkitc1-n16r8`

## 回归检查

```bash
python3 tools/test_audio_regression.py
python3 tools/test_input_epd_regression.py
python3 tools/test_field_library_v082.py
python3 tools/test_pdf_recovery.py --builder tools/build_ft02_field_library.py
```

## SD 内容

- `SD_ROOT/`：完整运行目录。
- `SD_UPDATE_v0.8.2/`：Field Library v0.8.2 覆盖内容。
- 修改 FieldManual 运行文件时，必须同步更新 `manifest.bin` 的 CRC32，并在打包前验证 media/index/data CRC。

## 文档

主要文档在 `docs/`：

- `FT02_PROJECT_GUIDE.md`
- `LR01_HOST_PROTOCOL_A2.md`
- `FT02_LR01_HOST_INTERFACE_PIN_LOCK.md`
- `FT02_AUDIO_ES8311_PA_WIRING.md`
- Field Library / FieldManual 当前内容与质量文档

## 生产清理说明

本包已移除：阶段性 AUDIT/REPORT、旧版 release note、历史测试档案、缓存/构建产物、过期版本过程注释，以及未被当前运行路径调用的自动 quiet-window 播放计时扫描器。运行中仍有实际用途的自检、错误日志、导航记录和音频安全 guard 均保留。
