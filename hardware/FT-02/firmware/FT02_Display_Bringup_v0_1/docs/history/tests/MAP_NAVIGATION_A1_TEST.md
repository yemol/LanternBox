# FT-02 v2.74t Map Navigation A1 实机验收

## 1. 黑白地图

进入地图，确认不再出现 `[GRAY4-MAP] production commit begin`，应出现：

```text
[MAP-BW-A1] production BW commit; Gray4 disabled for navigation
```

确认道路层级仍可通过线宽区分，建筑物为黑色轮廓、内部白色。

## 2. 手动缩放/移动

连续方向键、Q/E 缩放，停手后应先出现对应加载提示，再只生成一次最终黑白地图。验证缩放结束后中心不跳回默认点。

## 3. 10 秒导航检查

保持 GNSS Follow 开启并真实移动。每约 10 秒应出现：

```text
[MAP-NAV-A1] 10s check moved=...
```

位移小于 10m 时不刷新。

## 4. 安全框内局部刷新

移动超过 10m 且当前位置仍在中央安全框内，应出现：

```text
[MAP-NAV-A1] marker partial ...
[MAP-NAV-A1] partial commit rect=... total=...ms
```

此时不应出现 `[PBF-A3.15] regional cache map build begin`，整张地图也不应闪刷。

## 5. 安全框越界重新居中

继续移动直到位置标记接近地图边缘，应出现：

```text
[MAP-NAV-A1] safe-box exit ...
```

屏幕先显示“正在更新导航地图”，再重新居中并读取/构建对应 regional cache。

## 6. 局刷清残影

累计 20 次有效位置局刷后，应出现：

```text
[MAP-NAV-A1] partial cleanup threshold reached (20); BW full refresh without PBF rebuild
```

只做黑白全刷，不重新扫描 PBF。

## 7. R 安全

无 GNSS Fix 时按 R：中心和 Zoom 必须保持不变，日志：

```text
[MAP-NAV-A1] R ignored: no valid GNSS fix; center/zoom preserved
```

## 8. CardKB2 稳定释放

阻塞刷新结束后，必须看到连续释放门限日志：

```text
[MAP-INPUT-A5] navigation re-armed after stable release 150ms
```

不得再出现刷新结束后无操作却触发 R/ENTER/方向键的情况。
