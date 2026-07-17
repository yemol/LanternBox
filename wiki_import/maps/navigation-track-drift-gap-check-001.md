---
title: 轨迹漂移和断点检查
slug: navigation-track-drift-gap-check-001
category: 地图地形与环境监测
priority: P0
risk_level: caution
summary: 识别轨迹漂移、跳点、断点和停留点，避免按坏轨迹返回、误报路线或误导下一次外出。
tags: 轨迹, 漂移, 断点, FT, 导航, 地图地形与环境监测, 长期断供, 离线生存
guide_links: DG-0870
kiwix_topics: GPS tracking, GPS error analysis
last_reviewed: 2026-07-16
status: published
source: LanternBox internal curated knowledge
---
# 轨迹漂移和断点检查

## 用途

用于轨迹复盘前判断数据是否连续可信。它保护返回决策，不修复设备或算法。

## 适用场景

- 轨迹突然穿过不可通行区域。
- 设备长时间无 fix 后重新定位。
- 外出者停留过，但轨迹看起来像直线穿越。

## 准备材料

- 轨迹列表
- 地图或草图
- 外出者口述
- 时间记录

## 操作步骤

1. 先看轨迹是否有明显跳点或长空白。
2. 检查断点前后的时间差和地标差。
3. 把停留点、等待点和危险点从普通轨迹中标出。
4. 发现漂移时，不用漂移段规划返回。
5. 把可信段和待核段分别写入摘要。

## 判断标准

- 可用：轨迹连续，点位和地标相符，断点原因明确。
- 需待核：短时无 fix 或局部偏离但可由地标解释。
- 停止条件：轨迹跨越明显不可能地形、base 错误或断点覆盖关键岔路时，不按轨迹返回。

## 风险提示

- 不要删除异常点后假装路线完整。
- 不要让第二个人按未核轨迹外出。

## 替代方案

- 用可信段、地标和时间重建路线摘要。
- 关键断点不明时回到最近确定点规则。

## 记录建议

记录漂移段、断点时间、判断依据、采用段和放弃段。

## 对应 Guide

- DG-0870

## Kiwix/ZIM 可继续查询

- GPS tracking
- GPS error analysis
