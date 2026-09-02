---
title: 发电部件故障后的隔离与降级路径
slug: energy-generation-component-failure-degraded-path-001
category: 能源
priority: P0
risk_level: high
summary: 面板、控制器、转换器或发电线缆失效时，先隔离异常部件，再按剩余产能进入最低负载与备用路径。
tags: 发电故障, 部件隔离, 降级路径, 控制器异常, 最低负载
guide_links: DG-0920, DG-0923, DG-0924, DG-0929
kiwix_topics: generation failure, solar controller fault, degraded power path
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 发电部件故障后的隔离与降级路径

## 用途

用于小型离网系统的面板、控制器、转换器或线缆部分失效后，先把异常从系统中移出，再根据剩余安全产能选择备用或长期降级。

## 适用场景

适用于成品低压模块可整体断开的系统。冒烟、火花、快速升温、积水或裸露导体属于当前安全事件，不进行故障试跑。

## 准备材料

路径图、停用标签、容量盘点、关键负载清单、已验证备用件和事件记录。

## 操作步骤

1. 停止相关负载，断开异常模块的输入和输出，不反复重启或插拔试错。
2. 贴上 STOP 标签，记录是面板、控制器、转换器、线缆还是未知段。
3. 在路径图上划掉已隔离部件，重新计算剩余发电、储能和关键负载可维持时间。
4. 只有已知低压、电压极性匹配、近期验证的成品备用模块才能按单路切换流程接入。
5. 没有安全备用时进入 DEGRADED 或 MINIMUM，暂停所有非关键负载。
6. 维修或替换后先做低风险验证，连续稳定后才恢复普通模式。

## 判断标准

- 隔离完成：异常模块不再与系统相连，标签和路径图同步。
- 可备用：备用路径独立、匹配、有保护且近期验证。
- 需长期降级：剩余产出持续无法覆盖关键负载。

## 风险提示

停止条件：控制器冒烟、火花、焦味、积水、面板破裂、裸露导体、未知高压或保护件动作时立即 STOP、扩大隔离并转 Energy Safety / Fire。禁止带电拆修。

## 替代方案

无法确认故障点时停用整段路径，只保留独立且已验证的最少关键负载；准备纸本和人工替代。

## 记录建议

记录 incident_id、failed_component、symptom、isolated_at、remaining_capacity、mode、backup_path、repair_status 和 recovery_check。

## 相邻领域边界

Safety 处理当前危险；Repair 负责故障检查和更换；Energy 负责隔离后容量、冗余和模式选择。

## 对应 Guide

- DG-0920｜连续阴天与长期发电不足降级运行｜使用时机：故障导致长期产能不足。
- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：隔离后选择已验证备用路径。
- DG-0924｜低压能源系统周期维护｜使用时机：维护中发现发电部件状态异常。
