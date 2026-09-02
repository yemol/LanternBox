---
title: 知识版本变更日志
slug: records-version-change-log-001
category: 信息保存与长期重建
priority: P1
risk_level: caution
summary: 用追加式日志记录每次知识版本为何变化、改变了什么、何时生效、谁审核以及旧版如何处理。
tags: 变更日志, 版本历史, 修订理由, 审核记录
guide_links: [DG-0916]
kiwix_topics: records version control, offline archive, knowledge governance
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 知识版本变更日志

## 用途
用于 SOP、Knowledge Card、Guide 或 Wiki 的候选、发布、取代、撤回和回滚事件。日志是版本索引的审计线，不复制整篇正文。

## 操作步骤
1. 每次变更分配 change_id，并填写 document_id、from_version 和 to_version。
2. 写 reason_for_change，链接触发事件、复盘、用户反馈或 Field Test。
3. 列出实际修改字段、步骤、停止线或适用范围，不只写“优化”。
4. 记录 proposer、reviewer、决定、decision_date 和 effective_date。
5. 写明旧版状态、存放位置和当前索引更新时间。
6. 驳回、撤回和回滚也追加记录，不删除失败变更。

## 判断标准
- 时间线可以重建每个版本如何成为或离开 CURRENT。
- 变更理由与 evidence 相连。
- 纸本和数字索引使用相同 change_id。

## 风险提示
停止条件：变更内容、审核决定或旧版去向不清时，不得更新 CURRENT 索引。

任何自动整理都不能替代 reviewer、版本批准和适用条件检查；历史版本不得因整理方便而删除。

## 离线替代方案
纸本使用一行一事件的追加台账，正文另存；空间不足时至少保留版本、理由、reviewer、生效日和旧版状态。

## 记录与交接
日志本身定期备份并校验，任何更正使用新的 change_id 引用原事件。

## 相邻领域边界
本条记录知识变化，不替代代码版本控制，也不处理应用、固件或数据库 schema 变更。

## 对应 Guide
- DG-0916
