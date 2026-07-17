---
title: GPS 轨迹最小字段
slug: navigation-gps-track-minimum-fields-001
category: 地图地形与环境监测
priority: P0
risk_level: caution
summary: 解释 track、session、path point、时间和来源等最小字段，让轨迹能被复盘和转成返回路线。
tags: GPS track, FT, session, path point, 导航, 地图地形与环境监测, 长期断供, 离线生存
guide_links: DG-0870
kiwix_topics: GPS tracking, GPX
last_reviewed: 2026-07-16
status: published
source: LanternBox internal curated knowledge
---
# GPS 轨迹最小字段

## 用途

用于理解 FT 或手机记录的一段轨迹，确认它是否足够支撑返回、交接或路线复盘。

## 适用场景

- FT 已记录 path_points.jsonl。
- 外出后需要把轨迹交给留守人。
- 要把轨迹转换成纸质路线摘要。

## 准备材料

- FT 或定位设备
- session 记录
- 纸笔
- 路线卡

## 操作步骤

1. 确认轨迹所属 session 和起止时间。
2. 识别 base、转折点、危险点、资源点和等待点。
3. 检查每个关键点是否有时间和备注。
4. 不要只看线条，必须读点位含义。
5. 把关键点转成地标和方向说明。

## 判断标准

- 可用：session 清楚，关键点有时间，路线和实际地标一致。
- 需复查：只有连续点，没有备注或起止不明。
- 停止条件：base 不可信、session 混乱或轨迹缺关键段时，不按轨迹直接返回。

## 风险提示

- 不要把自动记录的每个点都当检查点。
- 不要忽略轨迹中的停留和跳点。

## 替代方案

- 轨迹字段不足时，用外出者口述补地标顺序。
- 无法读取设备时，用纸质路线卡和检查点记录。

## 记录建议

记录 session、起止时间、base、关键 path point、备注和导出人。

## 对应 Guide

- DG-0870

## Kiwix/ZIM 可继续查询

- GPS tracking
- GPX
