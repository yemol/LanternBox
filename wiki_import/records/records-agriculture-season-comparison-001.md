---
title: 农业跨季对比
slug: records-agriculture-season-comparison-001
category: 信息保存与长期重建
priority: P1
risk_level: normal
summary: 按地点、品种、天气、投入、病虫害和产量比较多个种植季，保留反例并把当前种植行动交给 Agriculture。
tags: 农业季节, 跨季对比, 产量记录, 种植经验
guide_links: [DG-0914]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 农业跨季对比

## 用途
用于同一作物或地块经历多个季节后，判断哪些条件可能影响成功与失败。对比用于提出下一季假设，不提供当前播种、施肥或病虫害处置。

## 操作步骤
1. 为每季确定 season_id、地点、品种、种子批次和日期范围。
2. 汇总天气/环境、土壤、灌溉、投入、病虫害、收获和产量记录。
3. 统一单位和阶段定义，无法比较的数据明确排除。
4. 列出各季共同点、差异、成功因素、失败因素和反例。
5. 把相关性写成待验证假设，设计下一季只改变必要变量的观察。
6. 由 Agriculture reviewer 检查假设和安全边界，结果继续回写。

## 判断标准
- 每季能够回到原始种植和产量记录。
- 地点、品种、天气和批次差异没有被省略。
- 输出不是立即改变当前种植操作的命令。

## 风险提示
停止条件：当前出现不明化学污染、食用安全、病虫害扩散或种子批次隔离问题时，停止历史对比并进入 Agriculture/Waste/Food Safety 当前判断。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
纸本每季一张卡，固定写地点、品种、天气、投入、产量、病虫害、成功/失败和下一假设。

## 记录与交接
保存 comparison_id、season_ids、排除数据、单位、模式、反例、hypothesis 和 reviewer。

## 相邻领域边界
Agriculture 负责怎么种、土壤、病虫害、种子和收获；Records 只做跨季历史。

## 对应 Guide
- DG-0914
