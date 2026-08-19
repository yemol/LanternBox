# FT-02 v2.74p Map Search A5 / Regional Cache A1 验收

## 1. 搜索结果布局

搜索“人民广场”。

预期：
- 每屏最多 3 条。
- 每条两行：名称 + 类型/上下文。
- 第二行完整显示，不被下一条或反白框遮挡。
- 上下键滚动时反白框完整覆盖当前两行。

## 2. 同一区域

选择一个当前 regional cache 覆盖范围内的地点。

预期日志包含：

```text
[PBF-A3.15] RAM cache hit; index not accessed
```

## 3. 新区域第一次访问

选择一个明显离开当前区域的地点。

第一次允许出现：

```text
[PBF-A3.15] building new regional cache from raw PBF
[PBF-A3.15] cache saved path=/maps/raw/shanghai-260726.osm.pbc5.X.Y ...
```

这是该区域首次建缓存。

## 4. 返回旧区域

再搜索并跳回刚才访问过的第一个远端区域。

预期不再从 raw PBF build，而出现：

```text
[PBF-A3.15] resident cache miss; checking target SD slot
[PBF-A3.15] SD cache hit path=... index not accessed
```

## 5. 说明

- 新区域第一次生成缓存仍可能较慢。
- 已访问区域之间切换应明显加速。
- Gray4 电子纸 commit 本身约数秒，本测试不要求绕过面板物理刷新。
