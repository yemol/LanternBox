---
title: 亲历、转述与设备来源边界
slug: records-source-direct-reported-boundary-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 区分本人观察、他人转述、设备输出与派生记录，保留完整来源链，防止二手消息和自动 metadata 冒充事实。
tags: 来源类型, 亲历观察, 转述信息, 设备证据
guide_links: [DG-0912]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 亲历、转述与设备来源边界

## 用途
用于给人员、设备、文档和自动整理产生的记录标明来源类型。来源标签说明信息如何到达系统，不等于真实性评分。

## 操作步骤
1. 本人直接看到、听到或测量时标 person/direct，并写观察条件。
2. 他人报告时标 person/reported，写原报告者和转述链；无法找到源头时明确 unknown。
3. 设备产生时标 device，保留 device_id、时间来源、原始值和设备状态。
4. 从音频转写、摘要或其他记录整理出的内容标 derived，并链接每个输入。
5. 同一条记录混有亲历和转述时标 mixed，在正文逐句或分栏标识。
6. reviewer 检查来源是否被复制成多个看似独立的条目。

## 判断标准
- 每个关键事实都能回答“谁或什么直接产生了它”。
- 自动时间、GNSS 和文件哈希只作为 provenance。
- 转写内容能够回到 audio id 和 transcription version。

## 风险提示
停止条件：来源链断裂、设备时间明显错误、GNSS 无有效 fix 或二手信息会触发高风险行动时，停止把它作为确认依据。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
纸本用 P-D、P-R、DEV、DER 四种符号，并在首页保留图例；混合来源拆成多行。

## 记录与交接
保留 source_type、recorder、device/source、direct_observation、原始附件 ID、导入批次和不确定性。

## 相邻领域边界
FT-02 和 Core 可以附加来源 metadata，但不能自动把记录改成 VERIFIED，也不能代替人工确认说话人或事件含义。

## 对应 Guide
- DG-0912
