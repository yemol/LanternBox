# FT-02 v2.74u Map Source Tiles A1 实机验收

## 1. 准备上海 source tiles

按 README 运行 `tools/build_ft02_map_source_tiles.py`，确认 SD 上存在：

```text
/maps/regions/regions.tsv
/maps/regions/shanghai/tiles/z11_x*_y*.osm.pbf
```

保留旧 `/maps/raw/shanghai-260726.osm.pbf` 作为 fallback。

## 2. 新区域测试

删除或避开目标位置已有 `.pbc5.*.*` regional cache，然后搜索一个此前未访问的位置。

期望日志先出现：

```text
[MAP-SOURCE-A1] manifest loaded regions=...
[MAP-SOURCE-A1] selected source=shanghai:z11/... pbf=/maps/regions/shanghai/tiles/...
```

然后：

```text
[PBF-A3.17] building new regional cache from raw PBF
[MAP-IO-A1] raw_pbf blocks=... bytes=... read=... build=...
```

记录 `blocks / bytes / read / inflate / node_pass / way_pass / build / total`。与 v2.74t 东方明珠基线比较：

```text
blocks=387
bytes=23463419
read=15961ms
build=50932ms
total=52425ms
```

首要 PASS 条件：`blocks`、`bytes` 显著下降。性能目标：首次新区域尽量 <10s。

## 3. fallback 测试

临时把目标 tile 改名，再进入相同位置。应出现：

```text
[MAP-SOURCE-A1] tile missing ... fallback monolithic
```

并继续使用旧上海大 PBF，不应崩溃。

## 4. 跨 tile 测试

搜索或移动到相邻 Z11 tile。应自动选择新的 source tile。已有 `.pbc5` 命中时仍优先使用 regional cache，不应无意义重建。
