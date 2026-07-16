---
title: LoRa 短消息优先级
slug: communication-lora-message-size-priority-001
category: 通讯
priority: P0
summary: 限制 LoRa 消息为报平安、位置、需求、危险和回执，不发送长文本和敏感库存。
tags: LoRa, 短消息, 优先级, 回执, 低功耗
guide_links: DG-0857, DG-0856, DG-0860
risk_level: caution
status: published
source: LanternBox internal curated knowledge
---
# LoRa 短消息优先级

## 用途

用于低速、低功耗 LoRa 通信时决定什么值得发送。LoRa 适合短状态，不适合长说明、争论和大量背景信息。

## 操作步骤

1. 只发送报平安、位置、危险、资源需求和回执。
2. 每条消息带时间、发送者编号和下次窗口。
3. 避免发送库存明细、住所细节和成员全名。
4. 长内容先写纸面摘要，再压缩成一条短消息。
5. 未确认消息在日志中标为未确认。

## 判断标准

- 可继续：消息短、能行动、能回执。
- 需压缩：消息包含解释、情绪、猜测或重复背景。
- 停止条件：消息暴露敏感位置、未知节点可见或密钥状态不清时，停止发送敏感内容。

## 风险提示

- 不要把 LoRa 当聊天工具。
- 不要连续重发同一条普通消息。
- 不要把迟到或重复消息当作最新状态。
