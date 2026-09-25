# FT02 PowerBoard V1 Design Freeze

状态：需求已记录；电气实现未冻结；不可打样。

## 项目基线

路径：`/Volumes/yemol_HDDisk/LanternBox`。原 HEAD：`9b2b18d5c1fc886b769d72a9dbecfce2d816d4c9`，分支 main。本任务基线提交：`808b603`。55 条原有工作区状态与资料哈希见 [基线记录](hardware/FT02_PowerBoard_V1/review/git_baseline.json)，不纳入电源板提交。

已读根目录 `AGENTS.md `（名称末尾有空格）及 `CODEX_NOTES.md`。本任务不继承旧 LR01 PCB 的电源架构和器件冻结。

## 已核实的项目资料

| 资料 | 事实与适用边界 |
| --- | --- |
| `hardware/FT-02/docs/FT02_hardware_pin_map_v1_1.md` | SDA47/SCL21，100kHz；CardKB2 0x5F；较新的 SDMMC 1-bit 基线 |
| `hardware/FT-02/docs/FT02_hardware_research_new_chat_brief_v1_1.md` | Waveshare 4.26 英寸黑白 HAT / GDEQ0426T82；原型使用独立外设稳压；其中旧 SD SPI 描述不覆盖较新引脚表 |
| `hardware/FT-02/firmware/FT02_Display_Bringup_v0_1/docs/FT02_MAX17048_BATTERY_MONITOR.md` | 1S 3.7V/4.2V，MAX17048 0x36，共享 I2C；不是保护/断电模块；没有电池电流能力 |
| 当前 `src/FT02_AudioLog.cpp` | WeAct ES8311 + NS4150B，GPIO4 已用于 PA shutdown；不得按历史“空闲”分配 |
| CardKB2 A2 `POWER_TEST.md` | 尚未测量功耗；建议测 5V 支路，与旧说明的 3.3V 供电存在待核实差异 |

资料优先级：本次任务书 > 当前冻结记录/实装事实 > 当前固件 > 旧说明。冲突单独记录，不静默更改现有系统。

## 电池冻结边界

当前：1S Li-ion/LiPo，3.7V nominal，4.2V full，5000mAh，来自用户任务书。

在硬件说明、相关固件文档、备份文本及电池相关 Git 历史中未找到当前电芯的最大充电、持续/峰值放电、温度、PCM 阈值、NTC、连接器、线径。知识库的通用电池文章不能作为设备规格。见 [BATTERY_SPEC](docs/BATTERY_SPEC.md)，OI-002 阻塞。

禁止由容量推算电流，禁止用旧 10000mAh/1260100 代替。未来至少 10000mAh，仍为 1S 同电压体系，不改 PCB；仍须符合共同电流、保护、连接器及热包络，并更新真实电池 profile。

## 冻结需求与未决参数

| 项目 | 已冻结需求 | 实现未决 |
| --- | --- | --- |
| 外部输入 | USB-C 5V、稳压 EXT_5V；2A 普通目标、最高约3A且符合源能力 | 限流、保护、线损、连接器 |
| Solar | 外置 DC/DC 将6-24V转为5V；板上禁止原始太阳能输入 | 实际 DC/DC 能力 |
| PD/Legacy | 只请求5V；正确CC；BC1.2明确范围；不强拉过流 | HUSB238A I2C复位配置、寄存器、总输入电流控制 |
| 自动选择 | USB优先，可硬件改优先级；弱USB不应阻止健康EXT接管 | 仲裁与恢复判定 |
| UPS | EXT_VALID时电池不供System；无效后硬件切电池；恢复有效立即硬件切回 | 阈值、滞回、资格验证、保持电容 |
| Charger | 独立支路，SYS不接FT02系统，系统优先于充电 | 电池限值、charger-only验证、硬件退让 |
| 3V3 | Buck-boost，3.3V，转换器能力目标>=2A | 整板连续额定及阶跃/热验证 |
| 5V | Buck-boost，两种输入模式均稳定，可关闭；转换器能力目标约2A | 最低输入时能力、补偿、整板包络 |
| 3V3_SENS | 从3V3派生，磁珠/去耦/可选开关，不另设DC/DC | 磁珠及负载冻结 |
| Gauge | signed current、remaining/full/design mAh、SOC、温度与状态；全部主充放电同一shunt | profile、分流器、微电流旁路审计 |
| SOC UI | 保留0-100%，不人为偏移或伪造满电 | 不改已有固件 |
| Core | 单ESP32-S3，未来移除LR01 ESP32；PowerBoard不加MCU | I2C状态/控制扩展电路 |
| GPIO | 共享SDA47/SCL21、100kHz；目标另用PWR_INT与5V_EN | 新GPIO不指定；INT合并须验证 |
| Keep-alive | DNP、默认OFF、USB选中且有效才允许脉冲 | 实机确定电流、脉宽、周期 |
| 低功耗 | LED默认DNP，无常开dummy load，分别报告六种状态 | 总开销未收敛 |

## 机械与阶段

USB-C在PCB外缘，位置由机壳决定。X/Y/Z、开孔、板框、安装孔、连接器、磁力计/GNSS/扬声器间距均未冻结。4层、<=45x30mm、争取40x28mm、<=6mm均为目标，不是已实现尺寸。

本轮只执行Phase 1；完整正式原理图、ERC和架构审查闭环前，不进入PCB正式布线。参考电路和可打开的层次文件不等于整板电气通过。
