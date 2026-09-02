---
title: 分配时距与补给置信度
slug: resource-allocation-horizon-replenishment-001
category: 风险决策
priority: P1
risk_level: caution
summary: 在今天、三至七天、数周和数月四个时距中分开记录确定、预计、不确定和无补给，防止把预测当作已经到手的资源。
tags: 资源分配, 分配时距, 补给置信度, 补给中断, 长期韧性
guide_links: DG-0927, DG-0929
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# 分配时距与补给置信度

## 用途

让短期预算看见长期后果，并让补给预测保留不确定性。今天能完成的任务，不一定值得消耗未来数周的恢复能力。

## 适用场景

适用于补给有日期、只有大致窗口、依赖天气或收获、已经延误，或明确没有补给的资源计划。

## 准备材料

当前可分配量、消耗基线、补给来源记录、历史到达偏差、任务期限、纸质日历。

## 操作步骤

1. 建立 today、3-7 days、weeks、months 四栏，把每项需求放入实际发生时距。
2. 将补给标为 CONFIRMED、EXPECTED、UNCERTAIN 或 NONE。
3. CONFIRMED 仍只写预计到达日，不在实物验收前加入 usable quantity。
4. EXPECTED 使用历史延迟或较低产出修正；UNCERTAIN 只进入 best case。
5. 为每种补给写失约 trigger，例如超过日期、连续低产出或收获失败。
6. expected 方案用于正常计划，worst 方案检查 safety minimum 是否仍能保持。
7. 每个补给窗口到达、延误或证据变化时，重算 reserve 和任务安排。

## 判断标准

- 四个时距都有需求和限制项。
- 补给等级、来源、日期和置信度可追踪。
- 预计或不确定补给未被计为现货。
- 失约后的动作和 recheck 已写明。

## 风险提示

天气、农业收获和外部供应都有失败可能。Forecast 不是 guarantee，不能用乐观输入释放关键 reserve。

## 停止条件

worst case 会跌破饮水、关键能源、必要医疗耗材或安全设备最低线时，停止新增普通任务并进入降级规划。补给来源涉及危险设备或污染物时先转专业域。

## 替代方案

历史数据不足时，把最早日期后移一个复查周期、把数量按较低可信值记录，并保留 UNKNOWN。

## 记录建议

记录资源、时距、补给等级、来源、数量区间、失约 trigger、影响任务和下一复核。

## 领域边界

Energy Deepening、Agriculture、Water 等域提供产出或补给证据；Resource Allocation 只决定如何把不确定输入带入预算。

## 对应 Guide

- DG-0927｜未来 7 天跨资源预算
- DG-0929｜补给中断后的替代与降级分配
