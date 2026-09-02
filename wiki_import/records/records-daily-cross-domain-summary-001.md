---
title: 每日跨域运行摘要
slug: records-daily-cross-domain-summary-001
category: 信息保存与长期重建
priority: P0
risk_level: normal
summary: 从各业务域原始记录中提取当天异常、未完成事项、风险和次日第一步，并保留可回查的记录编号。
tags: 每日摘要, 跨域记录, 未完成事项, 离线交接
guide_links: [DG-0911]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 每日跨域运行摘要

## 用途
用于每天结束或换日时建立一页运行摘要。摘要帮助团队找到重要原记录，不替代 Production、Storage、Medical、Energy 等域的当班操作卡。

## 操作步骤
1. 确定摘要日期、覆盖时间、整理人和使用的记录范围。
2. 按人员安全、物资、生产、维修、农业、能源、环境和知识变更扫描原记录。
3. 每类只写发生了什么、当前状态和 record_id，不复制完整正文。
4. 列出未完成事项、明确风险、禁用对象和下一次复查时间。
5. 为次日写第一步、责任人和对应业务域 Guide；无法确认的内容标待复核。
6. 接手人按抽样 record_id 找到原件并复述关键事项。

## 判断标准
- 摘要中的关键句都有原记录 ID。
- 当前急症或危险没有被摘要流程延误。
- 明日第一步和待复核项能够直接交接。

## 风险提示
停止条件：发现当前医疗急症、火灾、电气异常、污染、人员失联或其他未隔离危险时，停止整理摘要并先进入对应域行动 Guide。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
无设备时在纸上用五栏“发生、未完、异常、风险、明日第一步”，每项写原记录页号。

## 记录与交接
保存 summary_id、日期范围、纳入记录、遗漏说明、整理人、接手人和抽查结果。

## 相邻领域边界
Production 班次、Storage 交接、Energy 调度和 Medical 照护仍使用各自 Guide；Records 只做跨域日终索引。

## 对应 Guide
- DG-0911
