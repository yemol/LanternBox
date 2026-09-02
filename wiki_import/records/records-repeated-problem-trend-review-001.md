---
title: 重复问题与趋势复查
slug: records-repeated-problem-trend-review-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 把同一对象或流程的多次记录按条件和版本排列，判断是否存在可重复模式，同时防止把相似表象误当同一原因。
tags: 重复问题, 趋势复查, 历史聚合, 模式识别
guide_links: [DG-0914]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 重复问题与趋势复查

## 用途
用于设备反复故障、跨批次缺陷、连续农业失败、容量下降或重复 near miss。目标是发现值得验证的模式，不是自动给出根因。

## 操作步骤
1. 定义要比较的对象、现象、时间范围和纳入标准。
2. 收集每次 record_id，并核对批次、地点、设备、版本和人员条件。
3. 把共同点、不同点、发生频率和未发生时的条件并排列出。
4. 排除同一事件重复转录、同一来源复制和单位不一致。
5. 形成一到三个待验证模式，写明反例。
6. 为下一次检查或对照测试指定负责人、时间和成功/失败标准。

## 判断标准
- 聚合记录来自多个真实事件。
- 趋势包含条件变化和反例。
- 输出是可验证假设，不是自动成为 SOP。

## 风险提示
停止条件：样本来源不独立、关键测量不可比、条件差异过大或趋势会触发高风险行动时，停止下结论并交对应域 reviewer。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
纸本按日期横排，每列只写条件、表现、版本和结果，用不同颜色标共同点与差异。

## 记录与交接
建立 trend_id，保存纳入/排除记录、比较字段、候选模式、反例、下一测试和 reviewer。

## 相邻领域边界
Repair 决定当前怎么修，Agriculture 决定当前怎么种，Energy 决定当前调度，Production 决定当前批次；Records 只聚合历史。

## 对应 Guide
- DG-0914
