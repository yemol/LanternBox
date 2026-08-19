# FT-02 编译与刷机

## 1. 工程要求

- PlatformIO
- ESP32 Arduino framework
- FT-02 自定义板定义必须保留：

```text
boards/ft02-esp32-s3-n16r8.json
```

当前 PlatformIO 环境：

```text
ft02-pbf-a1-n16r8-spi40
```

## 2. 编译

在工程根目录：

```bash
pio run
```

成功后再进行上传。

## 3. 上传

根据本机实际串口执行：

```bash
pio run -t upload
```

如果工程配置未指定串口，可在 `platformio.ini` 或命令行中指定实际端口。

## 4. 版本确认

固件版本唯一权威源：

```text
src/FT02_BuildInfo.h
```

打包/提交前运行：

```bash
python3 tools/check_firmware_version.py
```

必须得到：

```text
PASS: firmware version ... is centralized in FT02_BuildInfo.h
```

## 5. 不要做的事

- 不要删除 `boards/`
- 不要把 Core 的 GNSS / QMC5883L / SX126x 旧驱动重新接回生产路径
- 不要恢复 Meshtastic protobuf full-sync
- 不要在 Core 保存罗盘 offset / scale
- 不要通过复制旧工程覆盖当前 `src/`

