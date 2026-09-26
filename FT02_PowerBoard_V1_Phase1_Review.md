# FT02 PowerBoard V1 Phase 1 Review

日期：2026-09-25。结论：**FAIL / OPEN ISSUE，Phase 1 尚未完成，禁止进入 PCB 正式布线或打样。**

这是可复核的阶段检查点，不是完整正式原理图交付。已采用新增电池来源支持的参数；仍缺少部分安全/接口参数，输入控制、UPS、PD 和多页电路本身也未闭环，不能把所有未完成项归因于电池资料。

## PASS：已完成且范围明确

- 已读取实际项目 `/Volumes/yemol_HDDisk/LanternBox` 的规则、硬件记录与相关固件；原有改动基线提交 `808b603`。
- 已记录需求冻结边界、候选器件依据、参数化功率/热预算、电池与 Gauge 策略、USB/充电宝政策、接口、EMI/机械约束及38项实机测试计划。文档存在不等于其中参数已冻结。
- KiCad10.0.5成功解析1张顶层+12张子页，导出SVG和XML网表。
- 已同步采用955565/955465 Polymer Li-ion、5000mAh、3.8V nominal（3.7V compatible）、9.5 x55 x65mm、76g；标准充电2.5A，快充/最大充电5A，最大放电5A，放电截止2.75V，放电温度-20C至+60C，以及包级五项保护存在性。仅关闭OI-002A/B/C/D来源字段。
- 相对393d5e8新增04充电器、07候选UPS及系统欠压、09的5V转换器、10传感器电源和11接口接线；08的3V3使能接入欠压控制。独立网表检查：107个符号、305个引脚通过；29项网表/边界/候选参数检查通过，包括充电误使能、采样极性、分流旁路、欠压旁路、错误器件、NC误接、未用输入下拉和输出类型负向测试。
- 另有4项电池规格回归检查通过：来源电流限制、未知参数保持空值、超5A算例禁用、仅关闭OI-002A/B/C/D。来源SHA-256与原理图/网表/ERC证据哈希核对通过；这些检查不替代硬件安全验证。
- 已检查本轮新增页面的完整比例渲染，修正5V页标签重叠；保留早先电量计/3V3/测试点检查。没有添加ERC排除项或PWR_FLAG。

## FAIL：未达到的阶段门槛

- **完整正式原理图未完成**：01 USB-C/PD、02 EXT输入、03外部源MUX、05电池板级保护四页仍为 `NOT IMPLEMENTED`。其余为已接线参考页，不是已放行模块；主链路未闭环，本轮完成目标尚未达到。
- **ERC未通过**：3个错误、7个警告。缺少EXT_BUS_5V、GND和BAT_GUARDED上游供电定义，7条孤立测试网络仍无电路。没有用PWR_FLAG伪造供电；逐项判断见下表。
- **充电电压OPEN**：来源CC至4.25V，随后CV为4.2V，矛盾未解决。充电温度、精确保护阈值、NTC、连接器、线径、最低带载包电压、超过1C的峰值及持续时间仍OPEN；默认禁止充电。不能把放电温度当成充电温度，也不能把2.75V截止当成PCM阈值或最低带载电压。
- **UPS仍UNFROZEN**：U701已按候选接线，不能仅凭包最大5A否定或批准。整板并发持续负载、峰值和持续时间未确认。29.8k限流、名义3.30V SYS_RAW欠压控制是待审查值；CP2接地不能采用5us快切换数据，220uF不能证明无掉电，低电压热态压降也未得到保证。
- **全部电流计量未通过**：U401 BAT与U701 IN2已接到R601负载侧，精确节点清单检查通过；U601 BAT供电、C601动态电流及Kelvin输入偏置作为上游例外显式保留。缺少板级guard和完整内部反向/故障状态审计，OI-010不关闭。
- 没有实机测试、完整瞬态仿真、无掉电证明、已核准整板额定功率、完整生产BOM或合格封装集。

## OPEN ISSUE：首要阻塞

| 项目 | 仍需关闭的内容 |
| --- | --- |
| 电池 OI-002/004/010/011 | 充电电压歧义、充电温度、精确保护/NTC/连接器线径、最低带载电压、>1C峰值持续时间、UPS与辅助电流计量 |
| 输入控制 OI-005/006/020 | 总输入限流、硬件充电退让、弱源失效/恢复防振荡、无电池低能力源启动 |
| PD OI-008/009 | HUSB238A确切订货/熔丝/寄存器配置，冷启动只取5V和下游过压保护 |
| Charger OI-007 | BQ25628E本地SYS网络、charger-only状态/反向/复位/看门狗验证 |
| 负载/热 OI-003/019/021 | 真实并发负载、保持时间、CardKB2实际供电接点、热裕量 |
| 生产前置 OI-015/016/017/018 | 原理图闭环、封装、装配、采购、USB-C与外壳机械冻结 |

## ERC逐项审查

| 项目 | 工程判断 |
| --- | --- |
| Error：U401 VBUS / EXT_BUS_5V | 03尚未提供外部源输出，真实未完成；不能排除 |
| Error：U401 GND | 输入/电池连接器及回路尚未实现，真实未完成；不能虚设电源驱动 |
| Error：U601 BAT / BAT_GUARDED | 05保护链缺失；主充放电局部连接不等于已有安全电池源 |
| Warning：USB_VBUS / TP1201 | 仅测试点，01输入缺失；未接受为发布豁免 |
| Warning：USB_5V / TP1202 | 01受保护输出缺失；未接受为发布豁免 |
| Warning：EXT_5V / TP1203 | 02输入缺失；未接受为发布豁免 |
| Warning：BAT_PLUS / TP1205 | 05电池连接缺失；未接受为发布豁免 |
| Warning：BAT_MINUS / TP1206 | 05回路缺失；未接受为发布豁免 |
| Warning：KEEPALIVE / TP1215 | 限脉冲/默认关断电路未实现；未接受为发布豁免 |
| Warning：EXT_VALID / TP1218 | 未有负载资格验证状态机；U701分压不能冒充此信号 |

本轮另修复两处符号建模冲突：U701 OUT1/8与U901 PVOUT2/4分别是同一内部功率输出的并联端子，仅一个端子建模为power_out，另一个为passive。网表强制两脚同网，未更改物理接线、未排除检查。未把其他独立驱动端改成passive。

全部21条顶层问题中，OI-001已关闭，OI-002部分解决（4个来源子项关闭、7个子项开放），其余19条开放。没有通过推断关闭架构或安全项。详细30问逐项回答见 [DESIGN_REVIEW](docs/DESIGN_REVIEW.md)。

## 审查入口

- [需求冻结](FT02_PowerBoard_V1_Design_Freeze.md)
- [开放问题](docs/OPEN_ISSUES.md) / [已采纳电池规格与未决项](docs/BATTERY_SPEC.md)
- [KiCad工程](hardware/FT02_PowerBoard_V1/FT02_PowerBoard_V1.kicad_pro)
- [ERC原始结果](hardware/FT02_PowerBoard_V1/review/erc.json) / [独立网表核验](hardware/FT02_PowerBoard_V1/review/reference_validation.json)
- [参数算例](hardware/FT02_PowerBoard_V1/review/calculation_examples.json) / [3V3参考预览](hardware/FT02_PowerBoard_V1/review/svg/FT02_PowerBoard_V1-08_3V3_BUCKBOOST.svg)
- [电量计参考预览](hardware/FT02_PowerBoard_V1/review/svg/FT02_PowerBoard_V1-06_FUEL_GAUGE.svg) / [参考电路与旁路边界](docs/FUEL_GAUGE_DESIGN.md)
- [38项Bring-up计划，全部未执行](docs/BRINGUP_TEST.md)

## 工作区与下一步

本任务仅提交新PowerBoard工程与相关根目录/docs/bom文件，不纳入既有固件或LR01改动。工作期间观察到其他固件文件继续变化；未回退或覆盖，也不声称整个工作区哈希保持不变。

当前电池来源记录已找到并保留不改，不再要求重复提供已知规格。后续需要供应商澄清4.25V/4.2V和其余明确OPEN参数；独立推进输入限流/硬件充电退让、EXT_VALID状态控制、PD冷启动及其余原理图。没有创建PCB、Gerber、钻孔、贴片生产包或RevA标签；无工程样板/量产授权结论。
