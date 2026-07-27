> v1.98 存储基线：SDMMC 1-bit 5 MHz，CLK GPIO35 / CMD GPIO2 / D0 GPIO1。地图格式与工具说明保持有效。

# FT-02 Offline Map Foundation v1.93

## 当前目标

v1.93 只建立 FT-02 端的离线地图显示基础，不包含 GNSS 定位、路线规划和地址搜索。

本阶段验证：

- 从 SD_MMC 读取地图包
- 解析地图包配置
- 按 `z/x/y.ftm` 路径加载单色图块
- 在 800×480 墨水屏上拼接显示
- 方向键按整块地图平移
- 地图页和首页、Help 页之间正确切换
- 为以后 GNSS 经纬度转图块坐标保留接口

## 地图包目录

把工程中的测试地图：

```text
sdcard/maps/current
```

完整复制到 SD 卡根目录，最终结构必须是：

```text
/maps/current/map.cfg
/maps/current/0/0/0.ftm
/maps/current/0/0/1.ftm
...
```

不要把 `sdcard` 这一层目录复制到卡里。

## 操作

1. 等顶部 SD 状态显示正常。
2. 首页选择“地图导航”。
3. 按 Enter 进入。
4. 使用 D/Z/X/C 对应的方向键平移。
5. Enter 重新读取 `map.cfg`。
6. H 打开 Help。
7. B、Esc 或 Backspace 返回。

## FTM1 图块

每张图块：

```text
尺寸：256×256
色深：1 bit
字节序：每行 MSB first
黑色像素：1
白色像素：0
数据大小：8192 bytes
文件大小：8208 bytes
```

16 字节头部：

| 偏移 | 长度 | 内容 |
|---:|---:|---|
| 0 | 4 | ASCII `FTM1` |
| 4 | 2 | 宽度，LE，固定 256 |
| 6 | 2 | 高度，LE，固定 256 |
| 8 | 2 | 行跨度，LE，固定 32 |
| 10 | 1 | flags，固定 1 |
| 11 | 1 | 保留 |
| 12 | 4 | payload 大小，LE，固定 8192 |

## 为什么暂时不直接读取 MBTiles

MBTiles 本质上是 SQLite 数据库。它适合作为 Core/Mac 端的地图生产和交换格式，但 FT-02 当前阶段若直接读取 MBTiles，需要额外引入 SQLite、数据库页缓存和查询逻辑。

v1.93 优先验证最小闭环：

```text
SD -> 图块文件 -> 单色解码 -> 墨水屏
```

地图包稳定后，可以把大量零散文件进一步封装成单文件索引格式。

## 地图生产

`tools/ft02_map_tile_builder.py` 可以把一张已经渲染好的地图大图切成 FTM1 图块：

```bash
python3 -m pip install pillow

python3 tools/ft02_map_tile_builder.py \
  map.png \
  output/maps/current \
  --zoom 12 \
  --origin-x 3370 \
  --origin-y 1732 \
  --name "Shanghai Offline Map"
```

该脚本不会联网，也不会下载 OpenStreetMap 图块。

当前随包 demo 是人工生成的测试图，不是 OpenStreetMap 数据。

真实地图数据应在 Core/Mac 端从合法的数据源获取并自行渲染。使用 OpenStreetMap 数据时，地图包的 `attribution` 应保留 `(c) OpenStreetMap contributors`，并在随包 README 中保留 ODbL 说明。不要批量抓取 OpenStreetMap 公共瓦片服务器。

## 当前限制

- 平移单位为一张 256 像素图块
- 只有一个缩放级别
- 没有经纬度
- 没有 GNSS 定位
- 没有路径规划
- 没有 POI 搜索
- 地图刷新为整页刷新

## 下一阶段

v1.94 建议完成：

1. 确认 demo 图块在实机正常显示。
2. 生成一个真实的小区域 OSM 地图包。
3. 增加像素级平移和图块缓存。
4. 增加缩放级别切换。
5. 再接 GNSS，经纬度转换为 Web Mercator 图块坐标。
