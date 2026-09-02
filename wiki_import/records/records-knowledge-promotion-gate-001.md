---
title: 经验晋升正式知识门槛
slug: records-knowledge-promotion-gate-001
category: 信息保存与长期重建
priority: P0
risk_level: high
summary: 规定记录、复核、验证、经验与正式知识之间的人工门禁，禁止一次日志或自动摘要直接修改 Guide、Wiki 和 CURRENT SOP。
tags: 知识晋升, 人工审核, SOP候选, 正式知识门禁
guide_links: [DG-0915]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 经验晋升正式知识门槛

## 用途
用于 LESSON 可能改变行动步骤、检查项、停止线或知识卡时。正常路径为 record、review、verification、lesson、candidate、human review、validation、CURRENT。

## 操作步骤
1. 确认原始 record_id、来源、适用条件、成功/失败结果和至少一次复核可回查。
2. 检查与现有 CURRENT Guide/Wiki、已知反例和冲突记录的关系。
3. 指定风险等级、domain reviewer、候选适用范围、停止线和 fallback。
4. 建立 SOP_CANDIDATE 或 Knowledge Card Candidate，不改写当前正式文件。
5. 按风险安排实际复现、Field Test 或可靠依据验证，并记录结果。
6. 审核通过后创建有版本号和生效日的 CURRENT；驳回或证据不足时保留 candidate、LESSON 或 VERIFIED。

## 判断标准
- 候选能够回到原始 evidence。
- reviewer 与风险域匹配。
- 发布动作同时处理旧版和 CURRENT 索引。

## 风险提示
停止条件：Medical、Fire、Energy、Chemical、Food Safety 或 Structural Safety 候选缺少对应域 reviewer、可靠 CURRENT 对照、停止线或实际验证时，禁止进入 CURRENT。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
证据不足时保留在 Knowledge Inbox 或候选清单，允许继续收集记录，但不进入默认行动检索。

## 记录与交接
保存 candidate_id、来源记录、风险、适用条件、冲突检查、reviewer、validation、决定事件、版本和生效时间。

## 相邻领域边界
Records 管理晋升证据和状态；专业内容的正确性由对应 stable domain 审核，系统不得自动批准。

## 对应 Guide
- DG-0915
