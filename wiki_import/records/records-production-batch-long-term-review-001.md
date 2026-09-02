---
title: 生产批次长期回顾
slug: records-production-batch-long-term-review-001
category: 信息保存与长期重建
priority: P1
risk_level: caution
summary: 跨多个生产批次比较原料、产出、不合格、质检和改进结果，形成长期趋势而不接管当批放行。
tags: 生产批次, 长期回顾, 质量趋势, 改进历史
guide_links: [DG-0914]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 生产批次长期回顾

## 用途
用于多个已结束批次之间比较一致性、失败和改进效果。当前批次标签、WIP、每件检查、返工和放行继续由 Production Guide 执行。

## 操作步骤
1. 定义产品或任务范围，收集可比较的 batch_id 和使用的步骤卡版本。
2. 记录各批原料来源、数量、产出、不合格、返工、报废和质量结果。
3. 比较材料、工具、人员、环境、尺寸标准和生产版本差异。
4. 检查改进项实施前后是否降低同类缺陷，并保留失败批次。
5. 形成趋势、反例和下一批待验证检查点，不直接改 Production SOP。
6. 需要改正式步骤时转 SOP_CANDIDATE 和对应 reviewer。

## 判断标准
- 纳入批次已经完成当批质量记录。
- 长期趋势能回到 batch_id 和质检 evidence。
- 未检成品不因历史表现良好而放行。

## 风险提示
停止条件：当前批次仍有未隔离不合格品、成品未经检查、承重裂纹、污染或状态不明时，停止长期分析并先回到 Production 隔离/质检。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
纸本按批次一行记录原料、产出、不合格、原因、改进和版本，至少保留最近若干可比较批次。

## 记录与交接
保存 review_id、纳入批次、比较字段、趋势、反例、改进验证、reviewer 和下一批检查。

## 相邻领域边界
Production 主导当前批次、WIP、质检和班次；Manufacturing 主导具体制作；Records 只做历史比较。

## 对应 Guide
- DG-0914
