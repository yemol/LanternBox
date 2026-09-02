---
title: 备用供电路径切换检查
slug: energy-backup-power-path-switch-check-001
category: 能源
priority: P0
risk_level: high
summary: 对已知低压、匹配极性和电压的成品备用路径执行断开、检查、单路接入、空载验证再接关键负载。
tags: 备用供电, 路径切换, 低压DC, 单路接入, 关键负载
guide_links: DG-0923
kiwix_topics: backup power switching, low voltage failover, critical load transfer
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 备用供电路径切换检查

## 用途

用于主低压供电路径失效后，把最少关键负载转到已经验证的成品备用源，避免临时并联、反送或带电改线。

## 适用场景

只适用于已知低压、电压匹配、极性确认、接口明确、可整体断开的单一路径。未知电源、市电、110V/220V、高压 DC 和并网不适用。

## 准备材料

供电路径图、支路标签、备用源就绪记录、关键负载清单、停用标签。

## 操作步骤

1. 停止负载并断开原输入，确认原路径不再带电。
2. 检查备用源身份、已知电压、极性、接口、保护件、安全外观和最近验证日期。
3. 保持其他电源断开，只接入一条备用路径；禁止多源直接并联。
4. 先在无关键负载或低风险测试负载下验证输出稳定、无发热和无异味。
5. 一次只接一个最关键负载，观察掉电、重启、接头温升和保护动作。
6. 写下切换时间、当前路径、允许负载和回退条件；问题出现立即断开。

## 判断标准

- 可切换：低压、电压、极性、接口、保护和备用状态全部已知。
- 待验证：备用源久未复测、标签与实物不一致或路径含 UNKNOWN。
- 禁止：需要带电改线、并联未知电源、绕过保险或产生市电反送。

## 风险提示

停止条件：任何未知极性、火花、焦味、快速升温、潮湿、裸露导体、保护件缺失、高压 DC 或市电接触点都必须 STOP、断开并隔离。无法确认时只保留无电替代或停用。

## 替代方案

没有安全备用路径时关闭所有非关键负载，转纸本和人工方案，不自行拼接电池、转换器或充电器。

## 记录建议

记录 switch_id、from_path、to_path、source_id、voltage、polarity、test_load、critical_load、result、fallback 和 reviewer。

## 相邻领域边界

具体接头修理归 Repair；当前危险归 Energy Safety；本条不提供市电、并网或高压切换方法。

## 对应 Guide

- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：主路径失效后的低压单路切换。
