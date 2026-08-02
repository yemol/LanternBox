# FT-02 主控载板 v0.3

本包把 v0.2 的概念方案推进到“电气冻结草案 + KiCad 布局起步文件”。

## 当前状态

- 2 层板，暂定 100 mm × 60 mm。
- 板载 ESP32-S3-WROOM-1-N16R8、MicroSD、USB-C、3.3V/3A 电源。
- 外接电子纸、GNSS、LoRa UART 子板和 CardKB2。
- LoRa 已精简为 1×6 UART 接口。
- 已生成连接器针脚表、网络表、预选 BOM、布局坐标和可编辑的 KiCad PCB 起步文件。
- 当前仍不是可直接下单的生产版 Gerber。

## 重要修正

v0.1/v0.2 中写过 TPS62133 作为 3.3V 电源，这是错误的：
TPS62133 是固定 5V 输出版本。v0.3 改为 TPS62132 固定 3.3V/3A，
也可改用 TPS62130 可调版本并按官方参考设计设为 3.3V。

## 打开顺序

1. 先阅读 `FT02_MainBoard_v0_3_Design_Brief.md`
2. 查看 `FT02_MainBoard_v0_3_Schematic_Block.svg`
3. 查看 `FT02_MainBoard_v0_3_Placement.svg`
4. 在 KiCad 中打开 `FT02_MainBoard_v0_3.kicad_pcb`

KiCad 文件目前用于板框、安装孔、接口位置和功能区规划。
正式原理图与布线将在机械尺寸和具体连接器型号锁定后继续。
