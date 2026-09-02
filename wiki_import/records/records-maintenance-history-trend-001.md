---
title: 维修履历与重复故障趋势
slug: records-maintenance-history-trend-001
category: 信息保存与长期重建
priority: P1
risk_level: caution
summary: 聚合同一对象的故障、维修、替代材料和复查结果，识别重复模式并把当前修理步骤留给 Repair 域。
tags: 维修履历, 重复故障, 设备历史, 趋势复查
guide_links: [DG-0914]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 维修履历与重复故障趋势

## 用途
用于设备、工具或结构多次故障后回看完整履历。它回答“过去发生过什么和是否重复”，不回答“现在具体怎么修”。

## 操作步骤
1. 用稳定 object_id 汇总故障、停用、维修、材料替代和复查记录。
2. 按日期列出症状、使用条件、当时版本、临时/永久修复和再次故障间隔。
3. 核对不同故障是否真是同一对象、同一部位和可比较条件。
4. 标出重复表现、共同材料、环境、负载和维护间隔，同时保留反例。
5. 建立待验证 failure_mode 和下一次检查，不自行规定修法。
6. 把趋势交给 Repair reviewer，后续结果回写履历。

## 判断标准
- 每次维修能回到原始记录和对象。
- 临时修复与永久修复明确区分。
- 趋势不替代当前停用和维修判断。

## 风险提示
停止条件：对象正在失效、承重件开裂、工具破损、电气发热或当前风险未隔离时，停止查历史并先按 Repair/Safety Guide 停用。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
在对象卡背面按日期写“表现、处理、材料、复查、再次故障”，空间满后换新卡并保留前卡编号。

## 记录与交接
保存 object_id、failure_id、repair_id、材料、修复性质、复查、再次故障、候选模式和 Repair reviewer。

## 相邻领域边界
Repair 决定停用、修理和替代方法；Manufacturing 决定制作；Records 只聚合履历。

## 对应 Guide
- DG-0914
