---
title: 电池健康分级与替换线
slug: energy-battery-health-grade-replacement-line-001
category: 能源
priority: P0
risk_level: high
summary: 用外观安全、可比容量趋势和任务重要性给成品电池模块分级，明确降级使用、替换、退役与立即隔离边界。
tags: 电池健康, 替换线, 退役, 容量趋势, 安全隔离
guide_links: DG-0921
kiwix_topics: battery health grading, replacement threshold, battery retirement
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 电池健康分级与替换线

## 用途

用于把外观正常但容量下降的电池，与已经出现安全异常的电池分开管理，避免健康判断和危险隔离混成同一条线。

## 适用场景

适用于成品低压电池、移动电源和电池盒的非侵入式管理。电池包拆解、电芯重组、点焊、BMS 改造和损坏电池维修不适用。

## 准备材料

电池编号、最近三次可比容量记录、任务分级、备用状态、停用标签和隔离位置。

## 操作步骤

1. 先查外观、温度、气味、接口和外壳；出现安全异常直接进入 ISOLATE，不再做容量分级。
2. 对安全外观正常的对象，比较最近三个可比 runtime 或 Wh 样本。
3. 标为 A：趋势稳定且可承担已验证任务；B：容量下降但仍可承担低风险短任务；C：低于本地保守线，只作非关键备用或等待替换。
4. 任何无法可靠维持关键任务的对象从关键电源清单移除。
5. 为 B/C 写替换优先级、替代模块和下一复测日期。
6. 退役对象贴清楚状态，避免重新混入可用区。

## 判断标准

- 分级依据包含安全状态、可比趋势和任务需求，不只看显示百分比。
- replacement line 使用本地已验证任务需求，不宣称通用百分比。
- 关键负载只使用 A 级或有明确保守余量的已验证模块。

## 风险提示

停止条件：鼓包、漏液、异常热、异味、腐蚀、摔裂、冒烟或接头烧焦时，立即 STOP、隔离并交 Energy Safety；这些对象没有 B/C 降级使用选项。

## 替代方案

数据不足时标为 UNKNOWN，按更低能力安排并优先准备替代成品模块；不要用拆包修理补足容量。

## 记录建议

记录 battery_id、grade、safety_check、trend_samples、allowed_tasks、replacement_reason、review_date 和隔离位置。

## 相邻领域边界

Energy Safety 决定异常隔离；Storage 负责退役物的安全存放；Records 保存版本和历史；Energy 决定任务能力与替换优先级。

## 对应 Guide

- DG-0921｜电池实际容量下降复查｜使用时机：把复测结果转成健康等级与替换线。
