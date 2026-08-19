# FT-02 v2.75h Map Position Arrow A1

## 室内罗盘测试
1. 进入地图页。
2. 必须已有有效 GNSS fix 才会显示实时位置箭头。
3. 缓慢旋转设备。
4. 当 LR01 compass heading 累计变化 >= 5°，下一次地图导航检查应局部刷新箭头。
5. 串口可见：
   `[MAP-NAV-A2] heading marker refresh heading=... delta=...`

## 显示规则
- fix=1 + compass valid：黑色方向箭头。
- fix=1 + compass invalid：只显示位置圆点/圆环。
- fix=0：不显示实时位置箭头。

## 户外导航
- 位移 >= 10m：沿用原来的位置 partial refresh。
- 超出中央 Safe Box：沿用原来的地图重居中。
- 累积 partial refresh 达到 20 次：执行一次 BW full cleanup。

## 方向定义
- 0° = 地图北 / 屏幕上方
- 90° = 地图东 / 屏幕右侧
- 180° = 南
- 270° = 西
