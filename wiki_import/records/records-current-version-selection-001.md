---
title: CURRENT 版本选择规则
slug: records-current-version-selection-001
category: 信息保存与长期重建
priority: P0
risk_level: high
summary: 在多个副本和历史版本并存时按状态、适用范围、验证与生效日选择唯一当前版本，避免误用候选或旧版。
tags: CURRENT版本, 版本选择, 默认行动, 冲突检查
guide_links: [DG-0916]
kiwix_topics: records version control, offline archive, knowledge governance
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# CURRENT 版本选择规则

## 用途
用于行动前发现纸本、SD、Core 或不同成员持有多份同名 SOP 时。只有状态为 CURRENT、适用条件匹配且证据链完整的版本可以默认指导行动。

## 操作步骤
1. 先按 document_id 聚合同一知识的所有副本，不只比较标题。
2. 排除 CANDIDATE、SUPERSEDED 和 ARCHIVED 的默认行动资格。
3. 核对 CURRENT 的 applicable_scope、risk_level、effective_date、reviewer 和 checksum。
4. 比较现场设备、材料、地点和人员条件是否落在适用范围。
5. 发现多个 CURRENT 时暂停选择，按版本变更日志和批准事件复核。
6. 确认后在当前版索引写明唯一版本和核对日期；历史副本移出现场操作夹。

## 判断标准
- 选择依据包含状态和适用范围，而非只看版本数字最大。
- 当前版索引与实际文件一致。
- 高风险条目在行动前完成版本确认。

## 风险提示
停止条件：没有可验证 CURRENT、多个 CURRENT 冲突、checksum 不符或现场条件超出适用范围时，停止依据争议文档执行高风险任务。

任何自动整理都不能替代 reviewer、版本批准和适用条件检查；历史版本不得因整理方便而删除。

## 离线替代方案
继续使用最后一个已验证且条件匹配的 CURRENT；若它被标记危险或已撤回，则暂停对应任务并转业务域安全降级。

## 记录与交接
记录 selection_id、document_id、候选版本、排除理由、所选 CURRENT、核对人、时间和现场条件。

## 相邻领域边界
本条只选正式版本，不判断具体治疗、修理、生产、储藏或用电步骤。

## 对应 Guide
- DG-0916
