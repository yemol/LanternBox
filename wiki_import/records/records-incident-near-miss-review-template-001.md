---
title: 事故与 near miss 上层复盘
slug: records-incident-near-miss-review-template-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 在现场完成处置后统一整理事故时间线、直接动作、复工条件、整改和重复风险，不替代当前危险处置。
tags: 事故复盘, near miss, 整改验证, 复工条件
guide_links: [DG-0913]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 事故与 near miss 上层复盘

## 用途
用于事故或 near miss 的现场危险已经隔离之后，连接不同业务域记录并建立长期学习入口。当前受伤、火灾、漏电或污染处置不在本条执行。

## 操作步骤
1. 确认现场已由对应域处置并记录谁判断可以开始复盘。
2. 分配 event_id，按发现、通知、立即动作、隔离、结果建立时间线。
3. 链接人员、任务、批次、设备、地点和每个原始 record_id。
4. 区分事实、可能原因、未确认信息和不同成员证言。
5. 写复工条件、整改项、负责人、验证方法和观察期限。
6. 检查历史是否有同类 event；完成验证后关闭或重新打开。

## 判断标准
- 复盘开始前危险已隔离。
- 原因假设没有覆盖原始时间线。
- 复工和整改都有可验证条件。

## 风险提示
停止条件：人员仍需医疗行动、危险源未隔离、现场证据会因继续操作丢失或对应域尚未给出安全边界时，停止复盘并返回现场处置。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
信息混乱时先封存原件和事实时间线，原因分析延后；没有统一表格时使用 event_id 关联多张纸。

## 记录与交接
保存 event_id、时间线、直接动作、持续风险、证言、可能原因、复工条件、整改和关闭证据。

## 相邻领域边界
Medical、Fire、Energy、Safety、Production 等域负责当前处置与复工专业判断；Records 负责长期证据链。

## 对应 Guide
- DG-0913
