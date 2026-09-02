---
title: 跨资源消耗基线与可维持时长
slug: resource-allocation-consumption-horizon-baseline-001
category: 风险决策
priority: P1
risk_level: caution
summary: 把食物、饮水、能源和关键耗材的正常、最低与高负载消耗换算到日和周，比较不同时间范围内的保守可维持时长。
tags: 资源分配, 消耗基线, 可维持时长, 七天预算, 离线手算
guide_links: DG-0927, DG-0930
kiwix_topics: consumption baseline, days of supply, offline budgeting
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# 跨资源消耗基线与可维持时长

## 用途

用相同时间单位比较不同资源还能维持多久，并识别最先触线的限制项。基线不是固定配额，而是根据真实记录形成的预算输入。

## 适用场景

适用于同时比较食物、水、电、燃料、医疗耗材、零件和生产材料的 3 至 7 天或更长计划。单一资源的专业计算继续使用对应域 Guide。

## 准备材料

可分配量状态卡、最近三天或一周领用记录、任务计划、人员与天气变化记录、计算纸。

## 操作步骤

1. 为每种资源选定可核对单位，例如升/日、份/日、Wh/日、件/周。
2. 分别写 `NORMAL`、`SAFETY MINIMUM` 和 `HIGH LOAD` 三种消耗，不用平均值掩盖峰值任务。
3. 用最近可比记录修正人数、天气、病人照护、生产任务和设备效率变化。
4. 以 `USABLE` 扣除不可释放 reserve 后，按三种消耗估算可维持时长。
5. 分别检查 today、3-7 days、weeks、months；超出可信记录范围的结果标低置信度。
6. 找出最先达到最低线的资源，把它作为当前计划的限制项。
7. 写明下一次观察时间；实际消耗偏离基线时重新计算，不沿用旧结论。

## 判断标准

- 每个数值有单位、时间范围和来源。
- 正常、最低和高负载消耗彼此分开。
- 可维持时长只使用当前可分配量，不把预计补给当作已到货。
- 最先触线的资源及其影响任务清楚可见。

## 风险提示

最低消耗必须来自 Water、Food、Energy、Medical 等专业证据；Resource Allocation 不自行降低饮水、照明、通信或医疗安全线。

## 停止条件

关键资源已低于专业域最低线、原始数量无法核对、单位混乱或消耗记录明显失真时，停止普通分配并先盘点。出现 Active Hazard 时由 Safety / Fire / Medical 先处理。

## 替代方案

记录不足时用最近一个完整周期和最差可信消耗做临时基线，明确标注 `PROVISIONAL`，在下一周期复测。

## 记录建议

记录资源、三档消耗、修正原因、可维持时长、限制项、置信度和下一复算时间。

## 领域边界

Energy Management 负责今天的电力调度，Energy Deepening 负责长期容量，Water/Food 负责域内最低需求，Records 保存原始消耗；本条只做跨资源时距比较。

## 对应 Guide

- DG-0927｜未来 7 天跨资源预算
- DG-0930｜长期短缺资源重分配与周期复查
