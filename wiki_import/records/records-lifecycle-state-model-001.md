---
title: 记录生命周期状态模型
slug: records-lifecycle-state-model-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 用两条状态轴区分记录可信度成长与正式知识版本，防止原始日志未经人工复核就进入当前行动入口。
tags: 记录生命周期, 知识审核, 版本状态, 离线记录
guide_links: [DG-0912, DG-0915, DG-0916]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 记录生命周期状态模型

## 用途
用于判断一条现场信息现在处于什么成熟度，以及它是否有资格参与正式知识变更。记录成熟度采用 RAW、REVIEWED、VERIFIED、LESSON、SOP_CANDIDATE；正式知识另用 CANDIDATE、CURRENT、SUPERSEDED、ARCHIVED。APPROVED 只保存为一次审核决定，不作为长期状态。

## 操作步骤
1. 创建记录时先标为 RAW，并保留原始文字、音频或设备来源。
2. 检查来源、时间、对象和上下文后，由人工把记录推进到 REVIEWED。
3. 只有重复观察、独立来源、实际复现或可靠依据支持时，才由 reviewer 标为 VERIFIED。
4. 把适用条件、结果和可复用改变写清后，才能形成 LESSON。
5. 可能改变行动步骤、检查或停止线时，建立 SOP_CANDIDATE，不直接改正式知识。
6. 候选经过审核与验证后创建有版本号的 CURRENT；旧版转 SUPERSEDED 或 ARCHIVED。

## 判断标准
- 每个状态都有进入依据、责任人和日期。
- 记录成熟度没有与正式文档生效状态混用。
- 从 candidate 到 CURRENT 存在人工审核和验证证据。

## 风险提示
停止条件：状态来源不明、审核人缺失、证据冲突未处理或高风险候选未验证时，停止晋升并保留在原状态。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
没有电子系统时，在纸页右上角写成熟度，在知识卡封面写版本状态；状态改变时追加日期和签名，不擦除旧状态。

## 记录与交接
最少保留 record_id、前后状态、变更理由、reviewer、review_date、关联 evidence 和下一复查日。

## 相邻领域边界
Records 管理状态和证据链，不判断医疗、结构、电池、食品或消防内容是否专业正确；这些判断回到对应业务域。

## 对应 Guide
- DG-0912
- DG-0915
- DG-0916
