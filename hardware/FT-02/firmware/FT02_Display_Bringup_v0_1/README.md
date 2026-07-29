# FT-02 v2.30 / PBF Map UI A3.13 / SPI40

本版本基于 v2.29，仅调整地图底部状态栏的字体与排版。

## 地图底栏

底栏使用专用 20px 常规黑体点阵，不再将 16x16 字模放大到 32x32。

显示内容：

```text
速度 -- | 方向 -- | 海拔 -- | 距离 -- | Z18 100米
```

特点：

- 中文字形 20x20，字符步进 22px
- 数字、字母和符号为紧凑 12px 步进
- 五格独立居中
- 完整包含“距”字
- 保留完整窗口刷新、Z16～Z20、缩放中心保持和缓存快速启动

## 存储基线

```text
FSPI / Mode 3 / 40MHz
SCK42 MOSI2 MISO1 CS41
CMD18 / ACMD23 + CMD25
16KiB 多块传输
```

## 编译

```bash
cd FT02_v2_30_PBF_MapUI_A313_SPI40_BalancedFooter20_Full
"$HOME/.platformio/penv/bin/pio" run -e ft02-pbf-a1-n16r8-spi40 -t clean
"$HOME/.platformio/penv/bin/pio" run -e ft02-pbf-a1-n16r8-spi40
"$HOME/.platformio/penv/bin/pio" run -e ft02-pbf-a1-n16r8-spi40 -t upload
```
