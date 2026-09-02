---
title: 回滚与重新验证
slug: records-rollback-revalidation-001
category: 信息保存与长期重建
priority: P0
risk_level: high
summary: 当新版本失败或出现风险时，以旧内容为基础创建新的当前版本并重新审核，禁止简单恢复旧状态标签。
tags: 版本回滚, 重新验证, 风险撤回, 新版本发布
guide_links: [DG-0916]
kiwix_topics: records version control, offline archive, knowledge governance
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 回滚与重新验证

## 用途
用于 CURRENT 新版产生错误、现场条件变化或验证失败，需要恢复较早做法时。回滚是一次新的知识变更，不是把 SUPERSEDED 原样改回 CURRENT。

## 操作步骤
1. 先停止使用有问题的 CURRENT，并记录触发事件、风险和影响范围。
2. 找到拟恢复的旧版本及其当时 evidence、适用条件和被取代原因。
3. 检查旧内容是否仍适用于当前设备、材料、环境和业务域安全要求。
4. 以旧内容为基础创建新 version，写 rollback reason、evidence、reviewer 和 effective_date。
5. 完成对应风险等级要求的复核、实际复现或 Field Test。
6. 新版本批准后设为 CURRENT；有问题版本和原旧版都保留历史状态。

## 判断标准
- 回滚生成了新的 version。
- 旧版被取代的原始原因已经重新审查。
- 发布前有人工审核和验证证据。

## 风险提示
停止条件：触发风险仍未隔离、旧版也存在同类问题、适用条件不明或高风险域缺 reviewer 时，禁止回滚发布。

任何自动整理都不能替代 reviewer、版本批准和适用条件检查；历史版本不得因整理方便而删除。

## 离线替代方案
没有安全可验证版本时暂停对应高风险任务，保留事件记录并使用业务域认可的低风险替代。

## 记录与交接
保存 rollback_id、触发事件、受影响版本、候选基线、差异、验证、reviewer、批准决定和生效日。

## 相邻领域边界
Records 维护回滚证据和版本；对应域决定旧做法在当前条件下是否安全有效。

## 对应 Guide
- DG-0916
