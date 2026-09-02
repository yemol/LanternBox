---
title: 冲突记录保留与复核
slug: records-contradictory-evidence-handling-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 在多条记录互相矛盾时保留原件、标出冲突点并安排复核，不通过覆盖旧记录制造虚假的单一结论。
tags: 冲突记录, 证据保留, 复核队列, 更正历史
guide_links: [DG-0912, DG-0913]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 冲突记录保留与复核

## 用途
用于时间、数量、人员、原因、地点或设备结果不一致的记录。冲突本身是一项事实，应进入 review queue，而不是被静默清理。

## 操作步骤
1. 为每条原记录保留独立 record_id，不合并正文。
2. 建立 conflict_id，列出冲突字段和各记录来源。
3. 检查是否由时区、单位、设备漂移、转录、批次混淆或观察位置不同造成。
4. 指定 reviewer、所需补证和 review_date；原记录者可以补充说明但不能改写他人记录。
5. 复核后追加 resolution，说明支持、否定或仍不确定的依据。
6. 任何派生 Lesson 同时链接冲突记录和解决记录。

## 判断标准
- 冲突双方仍可读取。
- 解决结论能回到证据和 reviewer。
- 尚未解决的冲突不会进入 CURRENT 知识摘要。

## 风险提示
停止条件：冲突涉及人员安全、医疗、火灾、能源、食品、化学品或结构判断时，停止依据争议结论行动并升级到对应域。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
纸本把冲突页夹在同一编号封套，使用红色“CONFLICT”索引卡，不撕页、不涂黑。

## 记录与交接
保存 conflict_id、contradictory_record、冲突字段、可能解释、补证任务、reviewer、状态和关闭日期。

## 相邻领域边界
Records 保证冲突可追踪；对应业务域决定哪些事实足以支持当前行动。

## 对应 Guide
- DG-0912
- DG-0913
