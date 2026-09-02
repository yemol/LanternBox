---
title: 太阳能预期与实际产出偏差检查
slug: energy-solar-expected-actual-output-check-001
category: 能源
priority: P1
risk_level: caution
summary: 在同类天气、时段和负载下比较太阳能保守基线与实际 Wh，逐项排除遮挡、污染、摆放和路径变化。
tags: 太阳能产出, 预期偏差, 实际Wh, 控制器状态, 发电复核
guide_links: DG-0922
kiwix_topics: solar output diagnostics, expected versus actual yield, low voltage solar
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 太阳能预期与实际产出偏差检查

## 用途

用于太阳能标称功率看似足够、实际补电却长期偏低时，用可比较条件定位偏差范围。单次低读数只形成待复测项，不直接宣告面板故障。

## 适用场景

适用于已知低压小型面板、成品控制器和 USB 充电路径。高压串联、并网、屋顶高处和带电拆修不适用。

## 准备材料

季节基线、天气记录、panel_id、控制器状态、路径图、USB power meter 或任务计时、纸表。

## 操作步骤

1. 选择与基线相近的天气、时段、温度、面板位置和负载。
2. 记录实际 Wh 或固定充电任务用时，同时保留控制器显示和测量工具编号。
3. 依次检查新增遮挡、表面污染、方向/角度变化、固定点和线缆拉扯。
4. 断开后目视检查接头、保险和控制器是否有松动、腐蚀、水汽、热痕或告警。
5. 不改变多个变量；一次只做一个安全调整，并记录调整前后结果。
6. 连续可比样本仍低时，降低系统发电预期并把可疑模块交 Repair 或专业边界。

## 判断标准

- 可比较：天气、时段、负载、位置和单位一致或差异已注明。
- 偏差成立：多个可比样本持续低于本季保守基线。
- 未证实故障：只有一次低值，或天气、遮挡和负载条件不同。

## 风险提示

停止条件：破裂面板、裸露导体、控制器冒烟、火花、积水、接头快速升温、高压串联或需要屋顶高处作业时停止并隔离。

## 替代方案

无法安全排查时按较低实际产出规划，缩小负载并使用已验证备用源；不要反复插拔可疑路径。

## 记录建议

记录 panel_id、baseline、actual_Wh、weather、time、load、shade、soiling、controller_state、change、result 和 reviewer。

## 相邻领域边界

Records 管理长期样本；Repair 处理部件故障；Energy Safety 处理当前冒烟、火花和过热；Energy 解释产出偏差。

## 对应 Guide

- DG-0922｜太阳能实际产出与预期偏差检查｜使用时机：低产出持续偏离保守基线。
