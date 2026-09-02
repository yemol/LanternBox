---
title: SUPERSEDED 旧版隔离
slug: records-superseded-version-isolation-001
category: 信息保存与长期重建
priority: P0
risk_level: high
summary: 保留被取代版本的历史证据，同时将其从纸本操作夹、默认索引和 FieldManual 当前入口中明确隔离。
tags: SUPERSEDED, 旧版隔离, 历史保留, 默认索引
guide_links: [DG-0916, DG-0917]
kiwix_topics: records version control, offline archive, knowledge governance
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# SUPERSEDED 旧版隔离

## 用途
用于新 CURRENT 生效后处理被取代版本。SUPERSEDED 仍有审计和学习价值，但不得继续出现在默认行动入口。

## 操作步骤
1. 在旧版写入 status=SUPERSEDED、取代它的新版本和生效日期。
2. 从当前版清单、常用卡夹、默认搜索包和现场张贴位置移除旧版。
3. 在纸本首页盖明显“旧版，不作为默认行动”标记，不遮盖正文。
4. 把旧版移入历史索引，保留 document_id、version、checksum 和存放位置。
5. 检查成员私有副本、FT-02 离线包和备份介质是否仍把旧版当 CURRENT。
6. 交接时说明旧版存在原因和新 CURRENT 位置。

## 判断标准
- 默认入口只显示新 CURRENT。
- 旧版仍能按 document_id 和 version 找到。
- 旧版不会因标题相同被误选。

## 风险提示
停止条件：无法确认所有现场副本状态、旧版包含已知危险步骤或新旧版本在高风险停止线上冲突时，暂停对应行动并完成隔离。

任何自动整理都不能替代 reviewer、版本批准和适用条件检查；历史版本不得因整理方便而删除。

## 离线替代方案
无法立即更新设备时，暂停该设备上的相关 FieldManual 入口，使用已核验纸本 CURRENT 和版本清单。

## 记录与交接
保存 supersede 事件、新旧版本、隔离位置、受影响副本、执行人、复查人和完成日期。

## 相邻领域边界
Storage 管物资隔离；本条只处理知识版本隔离，不接管库存、工具或成品状态。

## 对应 Guide
- DG-0916
- DG-0917
