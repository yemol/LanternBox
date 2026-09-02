---
title: 资源分配复查循环
slug: resource-allocation-review-loop-001
category: 风险决策
priority: P1
risk_level: caution
summary: 用 Plan、Observe、Record、Compare、Adjust 的离线循环检查分配决定，保留原版本、偏差原因和下一轮生效条件。
tags: 资源分配, 周期复查, 决策记录, 偏差调整, 离线循环
guide_links: DG-0930
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# 资源分配复查循环

## 用途

让资源分配成为可纠正的周期决定，而不是一次口头安排。复查关注计划与实际的偏差，以及偏差是否需要改变 reserve、任务或 degraded mode。

## 适用场景

适用于持续数天至数月的短缺、消耗不稳定、补给变化、设备效率下降和长期任务资源冲突。

## 准备材料

当前 allocation plan、资源状态卡、实际消耗、补给记录、异常事件、上一版本和纸笔。

## 操作步骤

1. PLAN：写明用途、数量、horizon、safety minimum、reserve 和假设。
2. OBSERVE：在固定时点读取实物余量、消耗、补给和专业域状态。
3. RECORD：保存原始数值、单位、来源、时间和不确定项。
4. COMPARE：比较计划与实际，区分测量误差、需求变化、浪费、补给偏差和设备效率变化。
5. ADJUST：只改有 evidence 支持的分配量、任务窗口、reserve 或 degraded level。
6. 保留旧版本，写生效时间、变更原因和下一触发线。
7. 复查后由第二人确认 safety minimum 和专业域边界仍被保护。

## 判断标准

- 原计划和新计划都可追踪。
- 偏差原因与调整动作对应。
- 记录事实和分配决定没有混写。
- 每轮都有下一复查和退出条件。

## 风险提示

复查不能把一次异常读数当长期趋势，也不能因库存数字改善而恢复危险设备或污染物资。

## 停止条件

原始 evidence 缺失、多个单位无法比较、计划已跌破最低线、危险状态未关闭或具体患者处置混入时，停止普通调整并转相应专业域。

## 替代方案

没有完整历史时保留当前版本，只做较小、可回退的调整，并把未知项列入下一轮观察。

## 记录建议

记录版本、计划、实际、偏差、原因、调整、决定人、专业域确认、下一 trigger 和 recheck。

## 领域边界

Records 负责可信度、版本和长期归档；Resource Allocation 负责根据 evidence 更新分配决定，不改写专业知识。

## 对应 Guide

- DG-0930｜长期短缺资源重分配与周期复查
