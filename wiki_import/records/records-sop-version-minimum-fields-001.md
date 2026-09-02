---
title: SOP 与知识卡版本最小字段
slug: records-sop-version-minimum-fields-001
category: 信息保存与长期重建
priority: P0
risk_level: high
summary: 为正式操作知识规定稳定文档编号、版本、状态、生效日、取代关系、审核者和证据，支持完全离线查验。
tags: SOP版本, 知识卡, 版本字段, 离线索引
guide_links: [DG-0916]
kiwix_topics: records version control, offline archive, knowledge governance
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# SOP 与知识卡版本最小字段

## 用途
用于任何可能进入现场默认行动入口的 SOP 或 Knowledge Card。版本字段必须让使用者在无网络时确认这是什么文档、当前是否有效以及依据来自哪里。

## 操作步骤
1. 分配跨版本稳定的 document_id，不把文件名或标题当作唯一身份。
2. 设置 version、status、effective_date 和 applicable_scope。
3. 记录 supersedes、reason_for_change、修改内容和旧版去向。
4. 填写 reviewer、risk_level、evidence 与 validation 记录。
5. 把 CURRENT 条目加入当前版索引，并在纸本和 FieldManual 包中使用同一版本信息。
6. 每次复制、导出或迁移后抽查 document_id、version 和 checksum 是否一致。

## 判断标准
- 所有 CURRENT 文档都有完整版本字段和生效日。
- evidence 能回到原始记录或 Field Test。
- 同一 document_id 在同一适用范围只有一个默认 CURRENT。

## 风险提示
停止条件：版本号、状态、reviewer、适用范围或 evidence 缺失，或者出现两个无法区分的 CURRENT 时，停止把该文档作为默认行动依据。

任何自动整理都不能替代 reviewer、版本批准和适用条件检查；历史版本不得因整理方便而删除。

## 离线替代方案
纸本封面使用九栏：document_id、version、status、生效日、取代版本、变更理由、reviewer、risk、证据位置。

## 记录与交接
版本登记同时保存创建日期、复制位置、校验结果和当前索引更新时间。

## 相邻领域边界
Records 管理版本身份与证据，不改写 Medical、Fire、Energy、Production 或其他域的专业步骤。

## 对应 Guide
- DG-0916
