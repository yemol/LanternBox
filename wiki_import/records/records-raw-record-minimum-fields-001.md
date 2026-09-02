---
title: 原始现场记录最小字段
slug: records-raw-record-minimum-fields-001
category: 信息保存与长期重建
priority: P0
risk_level: normal
summary: 给纸本、FT-02 与 Core 规定同一组原始记录字段，让现场事实能够离线保存、追溯来源并进入后续复核。
tags: 原始记录, 现场日志, 纸本记录, 来源追溯
guide_links: [DG-0911, DG-0912]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 原始现场记录最小字段

## 用途
用于现场刚发生事件、观察、任务结果或设备采集时，先留下足以回查的 RAW 记录。允许信息暂时不完整，但不能用推测补齐缺失字段。

## 操作步骤
1. 分配稳定 record_id；纸本可用记录本号、页号和序号组合。
2. 写 timestamp、recorder、source_type，以及具体人员、设备、文件或纸页来源。
3. 把 direct_observation 标为 yes、no 或 mixed，并把事实与推测分栏。
4. 按需要填写 related_event、related_task、related_batch 和 location；无有效 GNSS 时写地点代号。
5. 补充已采取动作、applicable_conditions、confidence、access_class 和下一复查时间。
6. 音频或设备文件只写引用 ID 和校验信息，不把附件本身概括成已验证事实。

## 判断标准
- 接手人能够找到原始证据。
- 时间、来源、事实和已做动作可以分开阅读。
- 缺失项明确标为 unknown 或 approximate，没有被猜测填充。

## 风险提示
停止条件：记录会影响当前高风险行动而来源、时间或对象无法确认时，不得按该记录继续行动，先转对应域当前 Guide。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
纸张不足时使用八栏：ID、时地、记录者/来源、事实、已做动作、可信度、下一复查、访问级别。

## 记录与交接
RAW 条目只追加更正，不覆盖原文；转录到 Core 后保留纸本页号或 FT-02 source id。

## 相邻领域边界
本条只规定最小记录字段，不替代 Production 批次卡、Medical 观察表、Storage 库存卡或 Energy 日志的专业字段。

## 对应 Guide
- DG-0911
- DG-0912
