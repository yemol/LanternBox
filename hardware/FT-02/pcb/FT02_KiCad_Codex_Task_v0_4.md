# FT-02 主控载板 v0.4 KiCad 正式设计任务书

## 一、任务目标

在现有 `FT02_MainBoard_2Layer_v0_3` 设计包基础上，建立一个全新的可编辑 KiCad 工程：

```text
FT02_MainBoard_2Layer_v0_4_KiCad/
```

完成以下工作：

1. 正式原理图。
2. 两层 PCB 布局和布线。
3. ERC 与 DRC 检查。
4. 生产输出文件。
5. BOM、引脚表和设计说明。
6. 保留完整修改记录。

不要直接覆盖 v0.3。先复制到 v0.4，再开始工作。

---

## 二、建议本地目录

项目建议放在：

```text
/Volumes/yemol_HDDisk/LanternBox/hardware/FT-02/pcb/
```

目录结构：

```text
pcb/
├── FT02_MainBoard_2Layer_v0_3/
└── FT02_MainBoard_2Layer_v0_4_KiCad/
```

若实际路径不同，只修改路径，不修改任务要求。

---

## 三、开始前必须执行

1. 检查本机 KiCad 版本：

```bash
kicad-cli --version
kicad-cli --help
```

2. 复制 v0.3：

```bash
cd /Volumes/yemol_HDDisk/LanternBox/hardware/FT-02/pcb

cp -R \
  FT02_MainBoard_2Layer_v0_3 \
  FT02_MainBoard_2Layer_v0_4_KiCad
```

3. 在 v0.4 内创建 Git 提交：

```bash
cd FT02_MainBoard_2Layer_v0_4_KiCad
git init
git add .
git commit -m "建立 FT-02 主控载板 v0.4 KiCad 设计基线"
```

4. 先阅读：

```text
README.md
FT02_MainBoard_v0_3_Design_Brief.md
FT02_MainBoard_v0_3_Connector_Pinout.csv
FT02_MainBoard_v0_3_BOM_Preliminary.csv
FT02_MainBoard_v0_3_PreSchematic_Checklist.md
```

5. 创建：

```text
DESIGN_ASSUMPTIONS.md
OPEN_ISSUES.md
PROJECT_HISTORY.md
```

所有无法从现有资料确认的信息必须写入 `OPEN_ISSUES.md`，不得偷偷猜测。

---

# 四、硬件范围

## 4.1 板载部分

必须板载：

```text
ESP32-S3-WROOM-1-N16R8
USB-C USB 2.0 接口
TPS62132 3.3V / 3A 降压电源
MicroSD 卡座
RESET 按键
BOOT 按键
电源与关键信号测试点
```

## 4.2 外接模块

通过连接器外接：

```text
4.26 英寸电子纸
GNSS 模块
LoRa / Meshtastic 子板
CardKB2
```

不在本版主板上直接焊接裸 LoRa 射频芯片。

---

# 五、冻结的 GPIO 定义

不得擅自改动以下引脚。

## 5.1 MicroSD

```text
MISO = GPIO1
MOSI = GPIO2
CS   = GPIO41
SCK  = GPIO42
```

工作基线：

```text
SPI Mode 3
40 MHz
```

## 5.2 电子纸

```text
BUSY = GPIO3
RST  = GPIO8
DC   = GPIO9
CS   = GPIO10
MOSI = GPIO11
SCK  = GPIO12
PWR  = GPIO18
```

## 5.3 GNSS

```text
ESP32 RX = GPIO39，连接 GNSS TX
ESP32 TX = GPIO38，连接 GNSS RX
PPS      = GPIO14，可选
Baud     = 38400
```

## 5.4 LoRa UART 子板

```text
ESP32 RX = GPIO4，连接 LoRa TX
ESP32 TX = GPIO5，连接 LoRa RX
AUX      = GPIO6，可选
RESET    = GPIO7，可选
```

## 5.5 CardKB2

```text
SDA = GPIO47
SCL = GPIO21
地址 = 0x5F
```

## 5.6 USB

```text
USB D- = GPIO19
USB D+ = GPIO20
```

## 5.7 预留 GPIO

```text
GPIO13
GPIO15
GPIO16
GPIO17
```

做成测试点或 1×4 扩展接口。

---

# 六、连接器定义

## 6.1 电子纸 J_EPD

使用 1×9、2.54 mm 直插接口：

| Pin | Signal |
|---:|---|
| 1 | 3V3_SYS |
| 2 | GND |
| 3 | EPD_MOSI |
| 4 | EPD_SCK |
| 5 | EPD_CS |
| 6 | EPD_DC |
| 7 | EPD_RST |
| 8 | EPD_BUSY |
| 9 | EPD_PWR |

丝印必须同时标出：

```text
3V3 GND DIN CLK CS DC RST BUSY PWR
```

## 6.2 GNSS J_GNSS

使用 1×5、2.54 mm：

| Pin | Signal |
|---:|---|
| 1 | 3V3_SYS |
| 2 | GND |
| 3 | GNSS_TX → GPIO39 |
| 4 | GNSS_RX ← GPIO38 |
| 5 | PPS → GPIO14 |

注意丝印从模块角度标：

```text
3V3 GND TX RX PPS
```

GNSS 已用 3.3V 实机验证。本版不要再加入 5V 供电选择。

## 6.3 LoRa J_LORA

使用 1×6、2.54 mm：

| Pin | Signal |
|---:|---|
| 1 | VCC_LORA |
| 2 | GND |
| 3 | LoRa TX → GPIO4 |
| 4 | LoRa RX ← GPIO5 |
| 5 | RESET → GPIO7 |
| 6 | AUX → GPIO6 |

VCC_LORA 使用互斥焊桥：

```text
SJ_LORA_3V3
SJ_LORA_5V
```

默认只允许一个焊桥闭合。PCB 丝印明确标注：

```text
DO NOT BRIDGE BOTH
```

在 `OPEN_ISSUES.md` 中保留“最终 LoRa 子板供电电压待实物确认”。

## 6.4 CardKB2 J_KB

使用 1×4、2.54 mm：

| Pin | Signal |
|---:|---|
| 1 | 3V3_SYS |
| 2 | GND |
| 3 | SDA / GPIO47 |
| 4 | SCL / GPIO21 |

预留两个 4.7 kΩ I²C 上拉电阻位置，默认 DNP，避免与 CardKB2 板载上拉并联过重。

---

# 七、ESP32-S3 原理图要求

使用：

```text
ESP32-S3-WROOM-1-N16R8
```

必须按照 Espressif 的 ESP32-S3-WROOM-1 官方参考设计完成：

1. 模组所有电源脚和地脚正确连接。
2. 每个电源区域有就近 100 nF 去耦。
3. 模组附近增加 10 µF 和 1 µF。
4. EN 使用官方推荐复位 RC 和上拉结构。
5. RESET 按键将 EN 拉低。
6. GPIO0 使用官方推荐上拉结构。
7. BOOT 按键将 GPIO0 拉低。
8. GPIO19/20 用于 USB D-/D+。
9. 不使用 GPIO33～GPIO37，因为 N16R8 模组内部 Flash/PSRAM 会占用相关资源。
10. 检查所有 strapping pin，外设不得在上电时强拉到错误状态。
11. 预留 UART0 日志测试点：

```text
U0TXD
U0RXD
GND
```

不要凭经验随意修改官方参考电路的 EN、BOOT 和电源参数。若 KiCad 官方库没有准确的 N16R8 模组符号/封装，按照 Espressif 模组数据手册创建项目专用库。

---

# 八、USB-C 要求

使用仅支持 USB 2.0 的 USB-C 母座。

必须包含：

```text
CC1 → 5.1 kΩ → GND
CC2 → 5.1 kΩ → GND
```

USB 数据：

```text
D- → 预留 22/33Ω 串联电阻 → GPIO19
D+ → 预留 22/33Ω 串联电阻 → GPIO20
```

要求：

1. D+/D- 增加低电容 ESD 保护。
2. 串联电阻靠近 ESP32-S3。
3. ESD 器件靠近 USB-C。
4. USB VBUS 先经过 PTC，再进入 5V_SYS。
5. 5V_SYS 增加 TVS 和输入储能电容。
6. USB 屏蔽外壳接地方案必须写入设计说明。
7. D+/D- 使用差分对布线。
8. 不经过过孔，除非实在无法避免。
9. 差分线下方保持连续地参考。
10. 依据实际 PCB 厂层叠参数，用 KiCad 计算器确定线宽和间距，不要伪造“精确 90Ω”结论。

本版 USB-C 是唯一正式电源输入。只预留 5V_EXT 测试焊盘，不允许把第二路电源直接并联到 5V_SYS。

---

# 九、3.3V 电源

使用：

```text
TPS62132RGTR
固定 3.3V
最大 3A
VQFN-16 RGT 3×3 mm
```

不要使用 TPS62133。

要求：

1. 完全按照 TI TPS6213x 数据手册和 TPS62130EVM-505 的 3.3V 参考布局建立电路。
2. 不自行猜测电感和电容值。
3. 确认 MODE、SS/TR、PG、DEF、FB 等引脚接法。
4. 输入与输出电容紧贴芯片。
5. 电感紧贴 SW 引脚。
6. SW 铜皮尽可能小。
7. 输入高 di/dt 环路最短。
8. PowerPAD 必须接大面积地和热过孔。
9. PG 信号可接到测试点，不强制接 ESP32。
10. 3V3_SYS 增加至少 220 µF 总线储能电容。
11. ESP32、MicroSD、电子纸、GNSS 和 LoRa 接口分别增加本地去耦。

主电源与地使用铜皮或宽走线，不要只用普通 0.25 mm 信号线承载系统电流。

---

# 十、MicroSD 要求

使用 SPI 模式，不使用 SDMMC。

连接：

```text
DAT0/DO  → GPIO1
CMD/DI   → GPIO2
CS       → GPIO41
CLK      → GPIO42
```

要求：

1. SD_CS 增加 10 kΩ 上拉。
2. SCK、MOSI、MISO、CS 均预留串联电阻封装。
3. 默认上件 0Ω。
4. 若实测需要，可更换为 22Ω 或 33Ω。
5. MicroSD 附近提供：
   - 47 µF
   - 10 µF
   - 1 µF
   - 100 nF
6. SCK 最短。
7. SPI 线不得跨越地平面分割。
8. 卡座金属外壳接地。
9. 若卡座带 Card Detect，将其接到 GPIO13；若不使用则在原理图标为 NC，并在固件说明中注明。
10. 卡座必须放在板边，保证装壳后可插拔。

---

# 十一、两层 PCB 规则

## 11.1 板框

暂定：

```text
100 mm × 60 mm
FR-4
1.6 mm
1 oz
2 层
```

四个 M3 孔：

```text
距相邻板边中心 4 mm
```

若机械设计尚未确认，保留当前尺寸，不擅自缩小。

## 11.2 顶层

顶层放置所有器件和主要信号。

布局顺序：

```text
ESP32 天线
→ USB
→ 电源
→ MicroSD
→ 外接接口
→ 低速信号
```

## 11.3 底层

底层尽量保持完整 GND 平面：

- 不放器件。
- 不做大范围电源分割。
- 只在无法避免时走少量低速线。
- 每条底层信号必须有连续地回流路径。

## 11.4 ESP32 天线

ESP32-S3 模组 PCB 天线放在板边，优先伸出主板。

天线区域要求：

```text
所有铜层无铜
无走线
无器件
无安装孔
无金属外壳遮挡
```

在 KiCad 中创建正式 Rule Area，不要只画丝印矩形。

## 11.5 电源走线

创建 Net Class：

```text
POWER_MAIN
POWER_BRANCH
SIGNAL
USB_DIFF
SD_SPI
```

最低要求：

```text
5V_SYS 主干和 3V3_SYS 主干使用铜皮或 ≥1.5 mm 走线
外设电源分支 ≥0.6 mm
普通信号按 PCB 厂能力设置
```

不得让 3A 主电流穿过细长焊盘颈部。

## 11.6 地过孔

必须在以下区域增加地过孔：

- USB ESD 附近
- Buck 电源周围
- MicroSD 卡座周围
- 所有外接接口旁
- 板边适当位置
- ESP 模组地焊盘周围

天线净空区内禁止地过孔。

---

# 十二、器件和封装规则

1. 优先使用 KiCad 官方库中经过验证的符号和封装。
2. 无准确封装时建立项目本地库：
   ```text
   lib/symbols/
   lib/footprints/
   lib/3dmodels/
   ```
3. 自建封装必须根据制造商机械图，不得凭图片估算。
4. 自建封装必须标出：
   - Pin 1
   - Courtyard
   - Fab outline
   - Silk outline
   - Assembly reference
5. 所有连接器在 PCB 丝印上标注信号名称。
6. 所有有方向器件标注 Pin 1 和极性。
7. USB-C、MicroSD 和模组封装必须进行 1:1 打印检查说明。

---

# 十三、正式原理图页面划分

将原理图划分为以下页面：

```text
00_System
01_ESP32
02_USB_Power
03_MicroSD
04_EPD
05_GNSS
06_LoRa
07_CardKB_Expansion
```

使用层次标签，不要把所有电路塞在一张巨型原理图上。

---

# 十四、ERC 与 DRC

不能通过添加“忽略所有错误”来获得零错误。

允许的 ERC 例外必须逐项写明原因。

必须执行：

```bash
kicad-cli sch erc ...
kicad-cli pcb drc ...
```

不同 KiCad 版本参数可能不同，先查看：

```bash
kicad-cli sch erc --help
kicad-cli pcb drc --help
```

验收要求：

```text
ERC：0 error
DRC：0 error
未连接网络：0 个非预期
悬空电源输入：0
封装缺失：0
网络名冲突：0
```

警告必须逐项解释，不能只说“可忽略”。

---

# 十五、必须输出的文件

最终目录至少包含：

```text
FT02_MainBoard_v0_4.kicad_pro
FT02_MainBoard_v0_4.kicad_sch
FT02_MainBoard_v0_4.kicad_pcb

lib/
├── symbols/
├── footprints/
└── 3dmodels/

docs/
├── FT02_MainBoard_v0_4_Schematic.pdf
├── FT02_MainBoard_v0_4_Top.png
├── FT02_MainBoard_v0_4_Bottom.png
├── FT02_MainBoard_v0_4_3D_Top.png
├── FT02_MainBoard_v0_4_3D_Bottom.png
├── ERC_REPORT.txt
├── DRC_REPORT.txt
├── PIN_MAPPING.md
├── POWER_TREE.md
├── DESIGN_ASSUMPTIONS.md
├── OPEN_ISSUES.md
└── MANUFACTURING_NOTES.md

manufacturing/
├── gerber/
├── drill/
├── position/
├── bom/
└── checksums.sha256

PROJECT_HISTORY.md
README.md
```

输出 Gerber 前先完成 ERC/DRC。

生产文件必须明确标注：

```text
PROTOTYPE REV_A
NOT VALIDATED ON HARDWARE
```

不要声称未经实机验证的设计已经可以直接量产。

---

# 十六、BOM 要求

BOM 至少包含：

```text
Reference
Quantity
Manufacturer
MPN
Value
Package
Description
DNP
Supplier
Supplier Part Number
Alternative Part
Note
```

不得只写“USB-C”“电容”“电阻”。

每个关键器件必须有具体 MPN：

- ESP32-S3 模组
- TPS62132
- 电感
- USB-C
- ESD
- TVS
- MicroSD 卡座
- 所有连接器
- PTC
- 大容量电容

若型号未冻结，将其写入 `OPEN_ISSUES.md`，并阻止“生产就绪”结论。

---

# 十七、Codex 工作方式要求

1. 不要重写整个设计后隐藏变化。
2. 每完成一个阶段提交一次 Git：
   ```text
   建立正式层次原理图
   完成电源与USB原理图
   完成ESP32与外设接口原理图
   ERC检查通过
   完成PCB布局
   完成PCB布线与覆铜
   DRC检查通过
   生成REV_A生产文件
   ```
3. Git 提交信息用中文。
4. Git Tag 使用英文：
   ```text
   ft02-mainboard-v0.4-revA
   ```
5. 每次提交前运行对应检查。
6. 不要修改 FT-02 固件 GPIO 定义。
7. 不要为了布线方便交换 GPIO。
8. 不要删除设计历史。
9. 不要把未确认的机械尺寸伪装成已确认。
10. 不要生成补丁式临时方案，优先从原理图和布局根因解决问题。

---

# 十八、阶段性停止条件

遇到以下情况时停止生产输出，但继续完成能完成的部分：

- USB-C 插座型号无法确认。
- MicroSD 卡座型号无法确认。
- LoRa 子板供电电压无法确认。
- GNSS 插头物理针序无法确认。
- ESP32 模组是 WROOM-1 还是 WROOM-1U 无法确认。
- 外壳和安装孔尺寸未确认。

停止时必须：

1. 保留已经完成且可打开的 KiCad 工程。
2. 在 `OPEN_ISSUES.md` 列出阻塞项。
3. 给出需要用户补充的照片、尺寸或型号。
4. 不自行猜测后继续生成“可下单”文件。

---

# 十九、最终验收报告

完成后输出一份：

```text
FT02_MainBoard_v0_4_Final_Verification.md
```

至少包含：

```text
KiCad 版本
项目文件打开情况
原理图页数
器件数量
网络数量
ERC 结果
DRC 结果
未决问题数量
板框尺寸
层数
最小线宽
最小间距
最小过孔
USB 差分对参数
天线净空尺寸
3V3 电源参数
MicroSD 走线长度
生产文件列表
SHA-256
是否允许打样
是否允许量产
```

允许的最终结论只能是以下之一：

```text
A. 不允许打样，存在阻塞问题
B. 允许工程样板打样，但未完成实机验证
C. 已完成样板验证，可进入下一轮设计
```

首次完成 PCB 设计时不得直接给出 C。

---

# 二十、直接交给 Codex 的执行命令

将下面整段交给 Codex：

```text
请读取当前目录中的 FT02_MainBoard_2Layer_v0_3 设计资料，并严格按照
FT02_KiCad_Codex_Task_v0_4.md 执行。

目标是建立 FT02_MainBoard_2Layer_v0_4_KiCad，不覆盖 v0.3。

先检查本机 KiCad 和 kicad-cli 版本，再建立 Git 基线。完成正式层次原理图、
两层 PCB、ERC、DRC、BOM、生产输出与最终验证报告。

不得修改已经冻结的 GPIO，不得用占位封装生成生产文件，不得通过批量忽略
ERC/DRC 错误来伪造通过。所有无法从现有资料确认的信息写入 OPEN_ISSUES.md。
遇到机械型号或接口物理针序缺失时，停止“生产就绪”结论，但保留完整可编辑工程。

每完成一个阶段提交一次中文 Git commit。最终给出修改文件列表、ERC/DRC 结果、
未决问题、可否工程打样的明确结论。
```
