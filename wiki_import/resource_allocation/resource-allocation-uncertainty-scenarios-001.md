---
title: Best / Expected / Worst 离线手算卡
slug: resource-allocation-uncertainty-scenarios-001
category: 风险决策
priority: P1
risk_level: caution
summary: 用 best、expected、worst 三种简单情景表达库存误差、补给延迟、消耗变化和设备效率下降，并按最差可信结果保护安全线。
tags: 资源分配, 不确定性, 三情景, 保守估计, 离线手算
guide_links: DG-0926, DG-0927, DG-0930
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# Best / Expected / Worst 离线手算卡

## 用途

在无法获得精确数字时保留不确定性，不用单一猜测掩盖库存误差、补给延误、需求波动和设备退化。

## 适用场景

适用于数量估计、天气输入、农业收获、设备实际容量、外部补给日期或消耗变化存在明显区间时。

## 准备材料

当前可分配量、最低保障线、消耗基线、补给来源、历史偏差、铅笔和三列表。

## 操作步骤

1. 只选择影响决定的少数变量，例如 usable quantity、daily consumption、replenishment date。
2. BEST 使用有证据支持的较好值，不写理想愿望。
3. EXPECTED 使用最近可比记录或中间可信值，作为当前普通预算。
4. WORST 使用最差可信值，包括较低库存、较高消耗和补给失约。
5. 对三栏分别估算何时触及 safety minimum 与 reserve。
6. 若 expected 可行但 worst 不可行，提前设置 degraded trigger，不立即释放 reserve。
7. 每得到新实物或记录证据，只更新相关变量并保留旧假设。

## 判断标准

- 每个情景的差异来自明确变量。
- expected 不是 best 的另一种写法。
- worst case 能检查 safety minimum。
- 计算可以用纸笔复现，不依赖联网预测或优化器。

## 风险提示

三情景不是概率保证。未知来源、危险设备、污染物资和临床判断不能用区间估计替代专业边界。

## 停止条件

worst case 已低于关键最低线、输入没有单位或来源、情景被用来合理化危险操作时，停止普通 allocation 并先盘点或转专业域。

## 替代方案

变量过多时只保留“当前可用量、日消耗、下一补给”三项；不能确认的值用 UNKNOWN，并按更保守方案行动。

## 记录建议

记录变量、三种取值、来源、触线日期、预设动作、新 evidence 和下次更新。

## 领域边界

专业域提供输入范围，Records 保存假设版本；Resource Allocation 用轻量情景选择可回退计划，不建立复杂数学优化器。

## 对应 Guide

- DG-0926｜有限资源快速盘点与可用量确认
- DG-0927｜未来 7 天跨资源预算
- DG-0930｜长期短缺资源重分配与周期复查
