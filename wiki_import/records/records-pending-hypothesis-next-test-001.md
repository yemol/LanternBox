---
title: 待验证假设与下一次测试
slug: records-pending-hypothesis-next-test-001
category: 信息保存与长期重建
priority: P1
risk_level: caution
summary: 把尚未证实的原因写成可被下一次观察否定或支持的假设，避免猜测在多次转述后变成正式经验。
tags: 待验证假设, 下一次测试, 反证条件, 经验验证
guide_links: [DG-0914]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 待验证假设与下一次测试

## 用途
用于复盘发现可能原因但证据不足时。假设必须与事实分开，并预先说明什么结果会支持或否定它。

## 操作步骤
1. 给假设分配 hypothesis_id，并链接支持它的原记录。
2. 用一句可检验语句描述可能原因，不写成已确认事实。
3. 列出替代解释、已知反证和需要保持不变的条件。
4. 设计一次低风险、最小改变的观察或测试，写明停止线。
5. 预先定义支持、否定和无法判断三种结果。
6. 完成后由 reviewer 记录结果；未验证假设不能升为 Lesson。

## 判断标准
- 假设和事实使用不同字段。
- 测试只改变必要变量且不越过业务域安全边界。
- 无论结果如何都保留原假设与测试记录。

## 风险提示
停止条件：测试需要绕过安全停止线、消耗唯一关键资源、对人员进行危险尝试或缺少对应域授权时，停止测试。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
无法安全测试时保持 pending，继续收集自然发生的观察，不制造事件。

## 记录与交接
保存 hypothesis_id、supporting_records、alternative_explanations、test_plan、stop_line、result 和 reviewer。

## 相邻领域边界
具体测试方法必须由对应域批准；Records 只管理假设、证据和结果状态。

## 对应 Guide
- DG-0914
