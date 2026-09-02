---
title: 技能授权记录最小字段
slug: records-skill-authorization-minimum-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 记录成员在特定技能下允许、禁止和必须监督的任务，以及练习证据与复测日期，回答当前能否安全独立执行。
tags: 技能授权, 允许任务, 禁止任务, 复测记录
guide_links: [DG-0401, DG-0918]
kiwix_topics: skill transfer, supervised practice, offline training records
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 技能授权记录最小字段

## 用途
用于小团队在离线环境中保存技能事实和授权历史。它不是岗位评价，也不自动安排人员，只回答某人在明确范围内现在可以做什么。

## 操作步骤
1. 使用稳定 skill_name，写 learner、trainer 和对应业务域。
2. 记录 current_level：OBSERVE、ASSIST、SUPERVISED 或 INDEPENDENT。
3. 分别列 allowed_tasks、prohibited_tasks 和 supervised_tasks，避免只写“会”。
4. 链接最近实际练习、任务结果、停止线表现和 reviewer evidence。
5. 填写适用设备/材料、last_practice、next_review 和可能失效条件。
6. 每次升级、降级、暂停或恢复都追加授权事件，不覆盖历史。

## 判断标准
- 授权范围能落到具体任务。
- INDEPENDENT 有实际执行 evidence 和人工确认。
- 高风险任务的业务域授权与监督要求清楚。

## 风险提示
停止条件：成员没有 evidence、不能复述停止线、设备/流程已变化、技能过期或曾发生相关事故未复训时，不得标为 INDEPENDENT。

不得使用自动评分、成员排名或系统推断授权；INDEPENDENT 必须有实际执行 evidence 和人工确认。

## 离线替代方案
不确定时降级为 OBSERVE、ASSIST 或 SUPERVISED，并安排低风险练习。

## 记录与交接
保存 authorization_id、skill_name、learner、trainer、level、allowed/prohibited/supervised tasks、evidence、reviewer 和 next_review。

## 相邻领域边界
Organization 决定人员分工，对应域决定技能标准；Records 只保存能力事实和授权历史，不做绩效排名。

## 对应 Guide
- DG-0401
- DG-0918
