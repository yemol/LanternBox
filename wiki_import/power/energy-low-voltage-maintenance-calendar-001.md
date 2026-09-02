---
title: 低压能源系统周期维护表
slug: energy-low-voltage-maintenance-calendar-001
category: 能源
priority: P1
risk_level: caution
summary: 按周、月和季记录面板、线缆、接头、电池、控制器、标签与备件的低风险状态检查和下次日期。
tags: 周期维护, 低压能源, 状态检查, 维护日历, 备件
guide_links: DG-0924
kiwix_topics: low voltage maintenance schedule, off-grid system inspection, preventive maintenance
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 低压能源系统周期维护表

## 用途

用于把分散的太阳能、电池、线缆、接头、控制器和备件检查变成固定周期，尽早发现状态变化并在危险前整体停用。

## 适用场景

只覆盖可视、可触达、断电后的低风险状态检查。不包含拆壳、带电维修、屋顶高处、电池拆包或高压系统。

## 准备材料

维护表、对象编号、路径图、停用标签、软布、手套和最近一次记录。

## 操作步骤

1. 按对象风险和环境分配周、月、季检查频率，写负责人和备用复核人。
2. 面板检查污染、遮挡、固定点和安全可达性；线缆检查磨损、夹压和变色。
3. 接头检查松动、腐蚀、水汽和热痕；电池只看外观、气味和异常温升记录。
4. 控制器记录告警、外壳状态和端口异常，不拆壳确认。
5. 核对路径标签、保护件标识、备用件状态和最近验证日期。
6. 每项写 OK、WATCH、STOP 或 UNKNOWN，附下一日期；WATCH 不得自动恢复为 OK。

## 判断标准

- 合格：对象、状态、观察、动作、责任人和下次日期齐全。
- 待复查：记录缺失、标签不符、备用件过期未验证。
- 停用：烧焦、冒烟、裸露导体、异常热、鼓包、漏液或固定结构不稳。

## 风险提示

停止条件：需要拆壳、带电维修、屋顶高处、处理鼓包电池，或发现裸露导体、烧焦、冒烟、火花时立即停止并隔离；具体修理交 Repair。

## 替代方案

缺少维修能力时停用可疑模块，缩小系统范围，使用已验证替换件或进入降级模式。

## 记录建议

记录 maintenance_id、object_id、interval、status、observation、action、next_date、owner 和 verification。

## 相邻领域边界

Repair 负责修理方法；Records 保存维护履历；Energy 决定部件状态对容量、单点和降级模式的影响。

## 对应 Guide

- DG-0924｜低压能源系统周期维护｜使用时机：执行和交接周期状态检查。
