---
title: 运行、应急与战略储备分级
slug: resource-allocation-reserve-class-policy-001
category: 风险决策
priority: P0
risk_level: caution
summary: 将 reserve 分为运行、应急和战略三类，分别写明日常补位、异常释放和长期风险用途，避免把所有未使用库存都称为备用。
tags: 资源分配, 运行储备, 应急储备, 战略储备, 释放条件
guide_links: DG-0928
kiwix_topics: reserve policy, emergency reserve, strategic reserve
last_reviewed: 2026-09-01
status: published
source: LanternBox internal curated knowledge
---
# 运行、应急与战略储备分级

## 用途

明确 reserve 不是“暂时没用的库存”，而是已经绑定风险、释放条件和恢复要求的资源承诺。

## 适用场景

适用于饮水、食物、能源、燃料、医疗与卫生耗材、关键零件和生产材料。具体保存、轮换和质量检查仍由 Storage 或对应专业域负责。

## 准备材料

可分配量状态卡、最低保障线、风险清单、补给置信度、任务计划和 reserve 标签。

## 操作步骤

1. `OPERATIONAL RESERVE` 用于吸收正常波动，例如一次延迟补给或短时高消耗，并规定补回周期。
2. `EMERGENCY RESERVE` 只在明确异常发生时释放，例如关键供水中断或必要通信窗口。
3. `STRATEGIC RESERVE` 对应数周至数月风险、唯一零件或恢复能力，不能因普通效率需求动用。
4. 每笔 reserve 写资源、数量、目的、释放 trigger、允许用途、决定依据和下一复查。
5. 同一数量只能属于一个 reserve class，不重复计入可分配量。
6. 释放时记录实际使用，并立即重新计算 safety minimum、剩余时长和补回计划。
7. 风险消失或补给恢复后，根据 evidence 调整 reserve，不因长期未用自动取消。

## 判断标准

- 三类 reserve 的用途和时间尺度能清楚区分。
- 每笔 reserve 有释放 trigger 和 release 后 recheck。
- reserve 数量未被普通预算重复使用。
- 战略储备的调整有跨周期证据。

## 风险提示

reserve 不能掩盖过期、污染、损坏或未验证状态。不可用物品不是 reserve，预计补给也不是 reserve。

## 停止条件

动用后会跌破专业域 safety minimum、资源状态不明、释放理由只为舒适或赶工、或危险物资被建议继续使用时，停止释放并升级复核。

## 替代方案

无法精确分级时，先保护 safety minimum 和唯一恢复能力，其余资源暂列 operational，24 小时内复核。

## 记录建议

记录 reserve class、数量、位置、风险、release trigger、批准证据、释放量、补回状态和 recheck。

## 领域边界

Storage 负责 reserve 的保存和轮换，Records 保存变更历史；Resource Allocation 只定义用途承诺与释放决策。

## 对应 Guide

- DG-0928｜关键资源最低保障线与储备分级
