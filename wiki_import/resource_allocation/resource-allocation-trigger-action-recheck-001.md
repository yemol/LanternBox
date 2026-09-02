---
title: 分配触发线、动作与复查条件
slug: resource-allocation-trigger-action-recheck-001
category: 风险决策
priority: P0
risk_level: high
summary: 把 reserve 下降、消耗偏快、补给失约和关键资源不可用写成明确的触发线、立即动作和复查条件，减少临时争论。
tags: 资源分配, 触发线, 再预算, 复查条件, 短缺响应
guide_links: DG-0929, DG-0930
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# 分配触发线、动作与复查条件

## 用途

把“情况不好再说”改成可观察的 trigger、预先同意的 action 和明确 recheck，确保策略在资源耗尽前改变。

## 适用场景

适用于 reserve 低于线、消耗超出 baseline、补给未到、预测短缺、关键资源被判不可用或任务需求突然增加。

## 准备材料

资源状态卡、消耗基线、reserve 表、补给日历、任务清单和公开模式卡。

## 操作步骤

1. 每个 trigger 只使用可观察事实，例如剩余量、时间、连续周期或专业域状态。
2. 为 trigger 写立即动作：冻结普通用途、缩短窗口、暂停任务、改用已验证替代或重新盘点。
3. 写明 action 不得触碰的 safety minimum 和 reserve class。
4. 指定 recheck 时间或证据，不用“以后再看”。
5. 若动作无效，预先写下一等级 degraded mode；若恢复，写逐步退出条件。
6. 记录触发前后可用量、实际消耗和被影响用途。
7. 多个 trigger 同时出现时，先执行 Safety / Medical / Fire 边界，再做资源调整。

## 判断标准

- trigger 可被两人用同一证据判断。
- action 明确到暂停或缩小的用途。
- recheck 有时间、责任和所需 evidence。
- 恢复不是一次乐观读数，而是满足预设条件。

## 风险提示

分配 trigger 不能替代火灾、伤害、电气或污染 stop line；Active Hazard 的动作由 Safety Contract 决定。

## 停止条件

设备冒烟、火花、异常热、电池鼓包、人员伤害、化学危险或污染资源出现时，停止普通 trigger 流程并执行专业安全响应。禁止把危险状态写成“资源紧张可继续”。

## 替代方案

无法量化阈值时，使用明确事件 trigger，例如一次补给窗口失约或唯一零件被隔离，并缩短 recheck 周期。

## 记录建议

记录 trigger、观察值、动作、保护线、影响用途、fallback level、recheck 和关闭证据。

## 领域边界

Safety/Fire/Medical trigger 优先；Records 保存事件；Resource Allocation 只定义非危险短缺和 reserve 触发后的分配动作。

## 对应 Guide

- DG-0929｜补给中断后的替代与降级分配
- DG-0930｜长期短缺资源重分配与周期复查
