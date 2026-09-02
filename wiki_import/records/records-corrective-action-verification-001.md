---
title: 整改项验证与关闭
slug: records-corrective-action-verification-001
category: 信息保存与长期重建
priority: P0
risk_level: caution
summary: 为失败或事故后的整改项规定负责人、验证方法和关闭证据，防止“已经处理”的口头结论掩盖重复风险。
tags: 整改验证, 事件关闭, 复工条件, 责任追踪
guide_links: [DG-0913, DG-0914]
kiwix_topics: offline records, knowledge transfer, records verification
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 整改项验证与关闭

## 用途
用于复盘已经提出改进动作后，确认动作是否完成、风险是否降低以及同类问题是否再次出现。关闭记录不等于永久消除风险。

## 操作步骤
1. 把每个整改项连接到 event_id、failure_id 或 trend_id。
2. 写清具体改变、负责人、期限和需要的业务域复核。
3. 预先定义可观察的验证方法和失败标准。
4. 实施后由不同于执行者的成员在条件允许时复查。
5. 记录验证结果、残余风险、是否允许复工及下一轮观察期。
6. 观察期内再发生同类问题时重新打开，不创建新的孤立整改项。

## 判断标准
- 完成动作和验证结果分开记录。
- 关闭有 evidence、reviewer 和日期。
- 残余风险及后续检查没有从交接摘要中消失。

## 风险提示
停止条件：危险仍存在、验证条件未满足、整改只停留在口头或高风险复工缺少对应域确认时，不得关闭。

不要以删除原记录、模糊来源或口头确认代替证据；自动 metadata 只能说明记录来自哪里，不能说明记录是真的。

## 离线替代方案
无法立即验证时标为 implemented_not_verified，暂停受影响的高风险任务或采用对应域安全降级。

## 记录与交接
保存 action_id、原事件、负责人、due_date、verification_method、结果、残余风险、close/reopen 日期。

## 相邻领域边界
Records 记录整改生命周期；Production、Safety、Medical、Energy、Repair 等域决定现场是否可复工或继续。

## 对应 Guide
- DG-0913
- DG-0914
