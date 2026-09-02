---
title: 低压支路、保险与极性标签
slug: energy-low-voltage-branch-protection-label-001
category: 能源
priority: P0
risk_level: high
summary: 给已知低压支路、保护件、可断开点和极性建立一致标签，使隔离、维护和备用切换不依赖个人记忆。
tags: 低压支路, 保险标签, 极性标签, 隔离点, 供电安全
guide_links: DG-0923, DG-0924
kiwix_topics: low voltage branch labels, fuse identification, polarity marking
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 低压支路、保险与极性标签

## 用途

用于标记成品低压系统中的支路用途、电压、极性、保护件和可断开点，使下一位操作人能在不猜测的情况下隔离故障。

## 适用场景

适用于 5V、USB 和已知低压 DC。市电配电箱、110V/220V、逆变器输出、高压 DC、并网和需要改造保护设计的系统不适用。

## 准备材料

耐久标签、铅笔或记号笔、路径图、已知规格资料、停用标签。

## 操作步骤

1. 先整体断开低压系统，确认无异常热、异味、火花或水汽。
2. 按路径图给每个支路、保险/保护件、连接器和断开点分配唯一编号。
3. 在标签上写已知电压、极性、目的负载和上游来源；未知值写 UNKNOWN。
4. 将保护件标签与其实际保护的支路一一对应，不写未经核实的额定能力。
5. 检查标签能否从正常操作方向看到，且不会拉扯、遮住散热或碰触导体。
6. 每次维护或路径变化后同步更新图和标签，由第二人按实物复核。

## 判断标准

- 合格：标签与图一致，支路、极性、保护和断开点可追踪。
- 待复查：字迹模糊、编号重复、保护对象不明或标签与实物不符。
- 停用：未知极性、保护件缺失/绕过、裸露导体或需要带电确认。

## 风险提示

停止条件：发现焦味、烧焦、发热、火花、潮湿、裸露导体、保护件被绕过或未知高电压时立即停止、断开并隔离。标签不能代替电气验证。

## 替代方案

无法确认时给整条支路贴 UNKNOWN / DO NOT USE，并只保留已验证关键负载路径。

## 记录建议

记录 branch_id、source_id、load_id、voltage、polarity、protection_id、disconnect_point、updated_at 和 reviewer。

## 相邻领域边界

保护件设计和维修归 Repair 或专业边界；Energy 只维护低压路径身份与系统隔离信息。

## 对应 Guide

- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：切换前确认支路与保护。
- DG-0924｜低压能源系统周期维护｜使用时机：周期复核标签和实物一致性。
