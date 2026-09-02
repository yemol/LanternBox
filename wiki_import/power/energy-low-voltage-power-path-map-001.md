---
title: 5V / USB / 低压 DC 供电路径图
slug: energy-low-voltage-power-path-map-001
category: 能源
priority: P1
risk_level: caution
summary: 用纸面路径图标出低压发电源、储能、转换器、保护件、支路和关键负载，识别未知段与单点故障。
tags: 低压DC, USB供电, 路径图, 单点故障, 支路隔离
guide_links: DG-0919, DG-0923
kiwix_topics: low voltage power map, USB power path, off-grid distribution
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 5V / USB / 低压 DC 供电路径图

## 用途

用于把低压能源从来源到负载的实际路径画清楚，支持容量盘点、故障隔离和备用切换，避免把多条未知线路当作冗余。

## 适用场景

只适用于 5V、USB 和已知可整体断开的低压 DC 成品模块。住宅配电、110V/220V、逆变器输出、高压太阳能串和并网不适用。

## 准备材料

纸、铅笔、设备标签、现有清单、极性和电压说明、停用标签。

## 操作步骤

1. 画出每个发电源、储能、转换器、保护件、连接器、支路和关键负载，并写固定编号。
2. 用箭头表示实际供电方向，在线旁写已知电压、接口和极性。
3. 标出每一级转换、保险/保护位置、可断开点和未知段。
4. 圈出只有一个部件的路径，以及多个关键负载共用的接头或转换器。
5. 为每条备用路径写明匹配条件和最近验证日期，不把未验证路径画成可用。
6. 实物变化后更新版本并保留旧图，不凭记忆覆盖历史。

## 判断标准

- 合格：图与实物编号一致，方向、保护、断开点和未知项可见。
- 待复查：线缆去向不明、标签脱落或备用路径未验证。
- 禁止使用：需要并联未知电源、绕过保险或跨入市电/高压部分。

## 风险提示

停止条件：绘图过程中发现发热、异味、火花、潮湿、裸露导体、未知极性或无保护件时，停止通电确认并隔离。不得带电拆线来追踪路径。

## 替代方案

无法确认的段落画成红色 UNKNOWN，并在该段前整体停用；关键负载改接已验证的单一路径。

## 记录建议

记录 map_version、对象编号、voltage、polarity、protection、disconnect_point、unknown_segment、single_point 和 reviewer。

## 相邻领域边界

Repair 负责线缆和接头检查方法；Records 保存版本；Energy 负责路径、容量和冗余判断。

## 对应 Guide

- DG-0919｜小型离网能源容量盘点｜使用时机：建立容量模型中的转换和负载路径。
- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：识别隔离点和备用路径。
