# FT02 PowerBoard V1 Phase 1 Review

日期：2026-09-25。结论：**FAIL / OPEN ISSUE，Phase 1 尚未完成，禁止进入 PCB 正式布线或打样。**

这是可复核的阶段检查点，不是完整正式原理图交付。已采用新增电池来源支持的参数；仍缺少部分安全/接口参数，输入控制、UPS、PD 和多页电路本身也未闭环，不能把所有未完成项归因于电池资料。

## PASS：已完成且范围明确

- 已读取实际项目 `/Volumes/yemol_HDDisk/LanternBox` 的规则、硬件记录与相关固件；原有改动基线提交 `808b603`。
- 已记录需求冻结边界、候选器件依据、参数化功率/热预算、电池与 Gauge 策略、USB/充电宝政策、接口、EMI/机械约束及38项实机测试计划。文档存在不等于其中参数已冻结。
- KiCad10.0.5成功解析1张顶层+12张子页，导出SVG和XML网表。
- 已同步采用955565/955465 Polymer Li-ion、5000mAh、3.8V nominal（3.7V compatible）、9.5 x55 x65mm、76g；标准充电2.5A，快充/最大充电5A，最大放电5A，放电截止2.75V，放电温度-20C至+60C，以及包级五项保护存在性。仅关闭OI-002A/B/C/D来源字段。
- 新增BQ27220及10mohm四端分流器参考页，保留原3V3参考电路与19个逻辑测试点。独立网表检查：36个符号、70个引脚连接通过；检查器12项测试通过，包括采样极性、旁路、DNP误装和未审查封装的负向测试。
- 另有4项电池规格回归检查通过：来源电流限制、未知参数保持空值、超5A算例禁用、仅关闭OI-002A/B/C/D。来源SHA-256与原理图/网表/ERC证据哈希核对通过；这些检查不替代硬件安全验证。
- 已检查新增电量计页与更新顶层的完整比例渲染；前一检查点已检查3V3及测试点页。没有添加ERC排除项掩盖问题。

## FAIL：未达到的阶段门槛

- **完整正式原理图未完成**：9张子页仍明确标记 `NOT IMPLEMENTED`。电量计及3V3页均为局部参考电路，不是已放行模块。
- **ERC未通过**：3个错误、12个警告。BAT_GUARDED/GND/SYS_RAW缺少上游供电定义，BAT_PROTECTED及多条测试点网络尚无实现；保留原始报告，没有用PWR_FLAG伪造上游供电。
- **充电电压OPEN**：来源CC至4.25V，随后CV为4.2V，矛盾未解决。充电温度、精确保护阈值、NTC、连接器、线径、最低带载包电压、超过1C的峰值及持续时间仍OPEN；默认禁止充电。不能把放电温度当成充电温度，也不能把2.75V截止当成PCM阈值或最低带载电压。
- **UPS兼容性未通过**：TPS2121推荐输入下限2.8V高于来源2.75V截止，且限流上限4.5A小于包最大5A；必须审定更低整板功率/更早截止或替代方案，当前未冻结其中任何一项。
- **全部电流计量未通过**：主充放电支路计划接分流器负载侧，但尚未实现；参考页明确保留U601 BAT供电的分流器上游旁路，OI-010不关闭。
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
