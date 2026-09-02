---
title: 电池实际容量周期复测
slug: energy-battery-capacity-retest-cycle-001
category: 能源
priority: P1
risk_level: high
summary: 在相同或可比较的低压负载和环境条件下，非侵入式复测电池运行时长或累计 Wh，形成保守容量趋势。
tags: 电池容量, 周期复测, 非侵入式, 运行时长, Wh
guide_links: DG-0921
kiwix_topics: battery capacity retest, runtime verification, non-invasive battery health
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 电池实际容量周期复测

## 用途

用于同一电池或移动电源的实际运行时间逐步缩短时，以非侵入方式复核可用容量。复测只支持保守规划，不能证明异常电池可以继续使用。

## 适用场景

只适用于外观正常、无异常热、无异味、无漏液和无摔裂的成品低压电池模块。禁止拆包、换芯、点焊、改 BMS 或修理鼓包电池。

## 准备材料

电池编号、可比较负载、USB power meter 或计时工具、温度记录、起止条件卡、停用标签。

## 操作步骤

1. 先做外观、气味、温度和接口检查；任何异常立即停止并隔离。
2. 选择与历史相同或可比较的负载、模式、线缆和环境温度。
3. 写明开始状态、停止阈值和测试中不得改变的条件。
4. 记录 runtime 或累计 Wh，不以显示百分比单独作为容量。
5. 至少在不同日期重复两次，并与同条件历史比较；取偏低结果作为保守可用值。
6. 把明显下降标为待复核，按健康分级决定降级用途、替换或退役。

## 判断标准

- 可比：负载、环境、起止条件和仪表一致或差异已注明。
- 趋势：至少三个可比样本呈同方向变化。
- 不可下结论：只有一次读数、设备模式改变或温差明显。

## 风险提示

停止条件：鼓包、漏液、腐蚀、摔裂、异常热、异味、冒烟或接口快速升温时，立即 STOP、断开并隔离，交 Energy Safety。禁止为完成复测继续充放电。

## 替代方案

没有仪表时用同一设备完成固定任务的时长比较；条件不可比时只降低规划容量，不宣布退化率。

## 记录建议

记录 battery_id、date、temperature、load_mode、start_condition、stop_condition、runtime、Wh、instrument、异常和 reviewer。

## 相邻领域边界

异常电池处置归 Energy Safety；历史样本归 Records；Storage 管理封存环境；Energy 只解释安全样本的容量趋势。

## 对应 Guide

- DG-0921｜电池实际容量下降复查｜使用时机：复核运行时长下降和可用容量趋势。
