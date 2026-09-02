---
title: 记录复核与验证检查表
slug: records-review-verification-checklist-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 通过来源、上下文、独立证据与实际复现检查，区分已阅读记录和真正得到支持的事实，避免虚假确定性。
tags: 记录复核, 事实验证, 证据检查, 人工审核
guide_links: [DG-0912]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 记录复核与验证检查表

## 用途
用于把 RAW 记录推进到 REVIEWED 或 VERIFIED。REVIEWED 说明字段和上下文已检查；VERIFIED 说明事实得到额外证据支持，两者不能互换。

## 操作步骤
1. 核对 record_id、原件、记录者、时间和对象，确认没有转录错位。
2. 标出亲历、转述、设备输出和由其他记录推导的部分。
3. 检查记录发生时的天气、材料、工具、人员、地点或设备状态。
4. 寻找真正独立的第二来源、重复观察、实际复现或可靠 CURRENT 对照。
5. 列出 contradictory_record，不以多数票删除少数记录。
6. reviewer 写明结论、依据、未解决问题和 review_date，再决定 REVIEWED、VERIFIED 或退回补证。

## 判断标准
- REVIEWED 条目不被描述为已经证实。
- VERIFIED 条目至少有一种可追溯支持方式。
- reviewer 与原记录者的角色和可能偏差可识别。

## 风险提示
停止条件：证据依赖同一传言、设备状态异常、冲突会改变安全判断或 reviewer 不具备高风险域能力时，停止标为 VERIFIED。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
离线纸本用复核章或签名栏记录“已读字段”“支持证据”“冲突”“结论”，原件继续单独保存。

## 记录与交接
保存 verification_status、支持记录 ID、冲突记录 ID、reviewer、review_date 和下一验证动作。

## 相邻领域边界
Records 检查证据链是否完整；具体病情、材料强度、电池状态或食物安全结论由对应域 reviewer 判断。

## 对应 Guide
- DG-0912
