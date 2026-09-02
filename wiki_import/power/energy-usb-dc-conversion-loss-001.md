---
title: USB 与低压 DC 转换损耗检查
slug: energy-usb-dc-conversion-loss-001
category: 能源
priority: P1
risk_level: caution
summary: 记录 USB 与低压 DC 从储能到负载的转换级数和端到端实际能耗，用安全成品模块比较路径损耗。
tags: USB损耗, DC转换, 转换效率, 多级转换, 端到端测量
guide_links: DG-0919, DG-0925
kiwix_topics: USB conversion loss, DC-DC efficiency, end-to-end energy measurement
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# USB 与低压 DC 转换损耗检查

## 用途

用于发现同一负载经过多个成品转换器、长线缆或重复升降压后，实际运行时间明显缩短的问题，并为容量模型选择较短的安全路径。

## 适用场景

只适用于 5V、USB 和已知低压 DC 成品模块的外部比较。不设计 DC-DC 电路，不拆转换器，不测试市电或逆变器输出。

## 准备材料

供电路径图、USB power meter、同一电源、同一负载和模式、同类线缆、计时工具、纸表。

## 操作步骤

1. 画出电源到负载的每一级转换和线缆，记录接口、标称电压及模块编号。
2. 固定同一电源、负载模式、运行时间和环境条件。
3. 在安全可用的短路径与多级路径分别记录输入/输出 Wh 或完成同一任务的耗能。
4. 每条路径至少重复两次，保留较差结果，不用单次瞬时 W 代替累计损耗。
5. 优先选择转换级数更少、无异常温升且任务完成稳定的成品路径。
6. 把差异写入容量盘点；路径或线缆变化后重新验证。

## 判断标准

- 可比较：电源、负载、模式、时长和测量条件相同。
- 损耗可疑：多级路径持续消耗更多 Wh 或使 runtime 明显缩短。
- 不可使用：模块、线缆或接头出现温升、掉电、焦味或保护动作。

## 风险提示

停止条件：未知电压/极性、发热、异味、火花、潮湿、裸露导体或需要拆壳测量时停止并隔离。不要绕过保护件或自制转换器追求效率。

## 替代方案

无仪表时比较同一电池完成固定任务的实际时长，并把结论标为低置信度；无法确认时采用较短、已验证路径。

## 记录建议

记录 path_id、conversion_stages、cable、source、load_mode、input_Wh、output_Wh、runtime、temperature_note 和 reviewer。

## 相邻领域边界

Repair 负责损坏线缆和转换器；Energy 只比较成品低压路径对容量的影响。

## 对应 Guide

- DG-0919｜小型离网能源容量盘点｜使用时机：把转换级数和保守损耗纳入容量模型。
- DG-0925｜低压电压、电流、功率与运行时长现场测量｜使用时机：执行端到端低压测量。
