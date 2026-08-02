# FT-02 主控载板 2 层 PCB 设计方案 v0.3

> 状态：电气冻结草案 / KiCad 布局起步  
> 板框：100 × 60 mm 暂定  
> 层数：2 层  
> 目标：板载 ESP32-S3、MicroSD、USB-C 和 3.3V 电源；外接电子纸、GNSS、LoRa 和 CardKB2。

## 1. 架构

```text
USB-C 5V
  │
  ├─ ESD / TVS / PTC
  │
  ├─ USB D- → GPIO19
  ├─ USB D+ → GPIO20
  │
  └─ 5V_SYS
       │
       └─ TPS62132 3.3V / 3A
            │
            └─ 3V3_SYS
                 ├─ ESP32-S3-WROOM-1-N16R8
                 ├─ MicroSD
                 ├─ EPD 接口
                 ├─ GNSS 接口
                 ├─ LoRa 接口
                 └─ CardKB2
```

## 2. 电源修正

上一版暂定的 TPS62133 是固定 5V 输出型号，不能产生 3.3V。
本版改为：

```text
首选：TPS62132，固定 3.3V / 3A
备选：TPS62130，可调版，按官方 EVM 设为 3.3V
```

不在没有复核 TI 参考设计的情况下自行更改电感、电容和反馈网络。

## 3. 引脚基线

| 功能 | GPIO |
|---|---:|
| USB D- / D+ | 19 / 20 |
| SD MISO / MOSI / CS / SCK | 1 / 2 / 41 / 42 |
| EPD BUSY / RST / DC / CS | 3 / 8 / 9 / 10 |
| EPD MOSI / SCK / PWR | 11 / 12 / 18 |
| GNSS RX / TX / PPS | 39 / 38 / 14 |
| LoRa RX / TX / AUX / RESET | 4 / 5 / 6 / 7 |
| CardKB2 SDA / SCL | 47 / 21 |
| 扩展测试点 | 13 / 15 / 16 / 17 |

GNSS 固件基线：

```text
RX = GPIO39
TX = GPIO38
Baud = 38400
```

## 4. 连接器

- `J_EPD`：1×9，2.54mm
- `J_GNSS`：1×5，2.54mm
- `J_LORA`：1×6，2.54mm
- `J_KB`：1×4，2.54mm

LoRa 最小连接仅使用 VCC、GND、TX、RX；RESET 与 AUX 保留但可不接。

## 5. PCB 层叠与布局规则

### 顶层
- 所有器件和主要信号。
- ESP32-S3 天线放在板边，优先让天线部分伸出主板。
- USB、SD、Buck 开关节点不得靠近天线。
- SD SCK 最短，连续参考地，不跨地缝。
- Buck 的输入环路和 SW 环路必须紧凑。

### 底层
- 不放器件。
- 尽量保持完整 GND 面。
- 仅在无法避免时走少量低速信号。
- 天线净空区内所有层禁止铺铜和走线。

## 6. 暂定机械

- 板框：100 × 60 mm。
- 四角 M3 安装孔，中心距板边 4 mm。
- ESP32 天线朝左。
- USB-C 位于左下。
- MicroSD 位于右侧。
- EPD 位于上边。
- GNSS 位于右上。
- LoRa 位于右侧下部。
- CardKB2 位于下边。

## 7. 尚未冻结

- USB-C 具体连接器型号与外壳开孔高度。
- MicroSD 卡座具体型号与插卡方向。
- LoRa 子板最终供电电压和插头方向。
- GNSS 模块插头顺序。
- 主板最终尺寸与安装孔位置。
- 是否增加外部 5V 输入端子。
- ESP32 使用 PCB 天线版还是 WROOM-1U 外接天线版。

## 8. 下一轮

1. 锁定连接器和模块照片/尺寸。
2. 完成正式 KiCad 原理图。
3. 执行 ERC。
4. 替换占位封装为制造商封装。
5. 完成布局、走线、覆铜和 DRC。
6. 输出 Gerber、钻孔、BOM 和坐标文件。
