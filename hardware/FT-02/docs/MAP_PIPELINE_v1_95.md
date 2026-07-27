> v1.98 存储基线：SDMMC 1-bit 5 MHz，CLK GPIO35 / CMD GPIO2 / D0 GPIO1。地图格式与工具说明保持有效。

# FT-02 中国 OSM PBF 地图生产管线 v1.95

## 目标

v1.95 增加对中国省市级 `.osm.pbf` 文件的原生离线解析，不再要求先转换为 Shapefile。

核心链路：

```text
中国 OSM PBF
  -> 纯 Python 两遍解析
  -> 按 Web Mercator 图块窗口裁剪
  -> 道路 / 铁路 / 水系 / 中文地名渲染
  -> 256x256 单色 FTM1 图块
  -> /maps/current/
```

## 新工具

```text
tools/ft02_osm_pbf_builder.py
```

特点：

- 无需 osmium、GDAL、pyrosm 或联网服务
- 直接读取标准 `.osm.pbf`
- 默认生成 FT02 v1.93+ 可读取的 FTM1 图块
- 支持中文地名栅格化
- 输出 `preview.png`、`preview_1bit.png`、`map_manifest.json`

## 上海中心城区实机包

输入：

```text
shanghai-260726.osm.pbf
```

生成参数：

```text
中心：121.4737, 31.2304
缩放级别：15
图块范围：8 x 6
图块数量：48
细节等级：street
```

实际覆盖：

```text
西：121.4208984375
东：121.5087890625
南：31.2034049509
北：31.2597699874
```

包含：

- 主要道路和普通街道
- 铁路
- 水系
- 中文区域、车站和主要道路标签

## 生成命令

```bash
python3 tools/ft02_osm_pbf_builder.py \
  shanghai-260726.osm.pbf \
  sdcard/maps/current \
  --center-lon 121.4737 \
  --center-lat 31.2304 \
  --zoom 15 \
  --columns 8 \
  --rows 6 \
  --detail street \
  --scale 2 \
  --threshold 165 \
  --max-labels 55 \
  --name "Shanghai Central"
```

## SD 卡目录

只需将生成的 `maps` 文件夹复制到 SD 卡根目录：

```text
/maps/current/map.cfg
/maps/current/map_manifest.json
/maps/current/15/<x>/<y>.ftm
```

`preview.png` 和 `preview_1bit.png` 只用于电脑检查，不是固件运行必需文件。

## 授权

地图数据来自 OpenStreetMap，地图包须保留：

```text
© OpenStreetMap contributors
```
