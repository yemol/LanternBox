---
title: 经验适用条件与上下文字段
slug: records-applicable-conditions-context-001
category: 信息保存与长期重建
priority: P1
risk_level: normal
summary: 记录经验成立时的环境、材料、设备、人员与限制条件，避免把局部成功或失败无条件复制到其他场景。
tags: 适用条件, 经验边界, 上下文记录, 知识复用
guide_links: [DG-0912, DG-0914, DG-0915]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 经验适用条件与上下文字段

## 用途
用于 LESSON、趋势结论和 SOP_CANDIDATE 的上下文说明。经验只有在使用者知道何时适用、何时不适用时才可复用。

## 操作步骤
1. 写明任务目标、对象、地点和日期范围。
2. 记录会改变结果的材料批次、设备型号、工具状态和操作版本。
3. 记录天气、温湿度、照明、能源、人员技能与疲劳等相关条件。
4. 区分必要条件、已知限制和尚未验证的变量。
5. 列出反例或失败条件，并链接对应 record_id。
6. 新场景与原条件不匹配时，先按候选测试处理，不直接套用结论。

## 判断标准
- 读者能判断自己的场景是否落在适用范围。
- 条件不是泛写“正常环境”，而是可观察或可核对。
- 未知变量和反例没有从摘要中消失。

## 风险提示
停止条件：高风险任务缺少关键适用条件、设备或材料版本不同且未复核时，停止复用经验并回到 CURRENT 对应域知识。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
纸卡只保留“适用于”“不适用于”“还不知道”三栏，并写原记录位置。

## 记录与交接
把 applicable_conditions、excluded_conditions、related_batch、location、版本和反例 ID 写入 Lesson 或 candidate。

## 相邻领域边界
Records 记录条件和差异，不决定农业配方、维修方法、制造尺寸或医疗处置是否应该改变。

## 对应 Guide
- DG-0912
- DG-0914
- DG-0915
