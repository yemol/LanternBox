---
title: 低压电压、电流、功率、Wh/Ah 与运行时长验证
slug: energy-low-voltage-field-measurement-runtime-verify-001
category: 能源
priority: P0
risk_level: high
summary: 在 5V、USB 和已知低压 DC 范围内，记录 V、A、W、Wh/Ah 与 runtime 的接法、条件和重复样本。
tags: 低压测量, USB Power Meter, multimeter, WhAh, 运行时长
guide_links: DG-0925
kiwix_topics: low voltage measurement, USB power meter, voltage current power energy
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 低压电压、电流、功率、Wh/Ah 与运行时长验证

## 用途

用于在已知低压范围内取得可追溯的原始 V、A、W、Wh/Ah 和运行时长，为容量、转换损耗和趋势判断提供数据。

## 适用场景

只允许 5V、USB 和明确标注的低压 DC。市电、110V/220V、高压 DC、太阳能高压串、未知电压、潮湿或裸露高风险导体不适用。

## 准备材料

完好 USB power meter，或量程和表笔均完好的 multimeter；设备编号、负载模式、计时工具、记录纸。

## 操作步骤

1. 先确认电源类型、已知电压范围、极性、仪表量程和表笔插孔；任何未知项都停止。
2. USB power meter 按成品说明串在已知 USB 电源与负载之间，记录 V、A、W 和累计 Wh/Ah。
3. multimeter 测电压时选择合适直流电压档并接在已知低压测点两端，不触碰裸露部分。
4. 测电流需要串入已断电的已知低压回路；**电流档不得直接并接电源两端**。
5. 固定负载模式、线缆、环境和起止条件，记录稳定值、短时峰值、累计能量和 runtime。
6. 至少重复一次；单次读数只作 observation，不自动转成容量或健康结论。

## 判断标准

- 合格：instrument、device、time、load_mode、conditions、单位和接法齐全。
- 待复查：仪表未校验、条件变化、只有一次读数或百分比显示与实测冲突。
- 禁止继续：量程/插孔不明、电流档并接、表笔破损或可能短接电池。

## 风险提示

停止条件：未知电压、破损表笔、电流档并接、短接电池、市电、110V/220V、高压 DC、潮湿、裸露高风险导体、火花或异常热时立即 STOP、断开并隔离。不得写成住宅市电测量教程。

## 替代方案

无法安全量测时记录铭牌下限和固定任务实际 runtime，按更保守容量规划，不猜测缺失值。

## 记录建议

记录 measurement_id、instrument_id、device_id、time、connection、V、A、W、Wh、Ah、runtime、load_mode、conditions 和 confidence。

## 相邻领域边界

Records 保存原始样本和趋势；Energy 解释保守容量；仪表或设备维修归 Repair。FT-02 固定 UI 百分比不是可信 Fuel Gauge 数据。

## 对应 Guide

- DG-0925｜低压电压、电流、功率与运行时长现场测量｜使用时机：取得低压现场测量与 runtime evidence。
