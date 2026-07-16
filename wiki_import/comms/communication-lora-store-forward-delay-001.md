---
title: LoRa 延迟和转发误判
slug: communication-lora-store-forward-delay-001
category: 通讯
priority: P1
summary: 说明低速、延迟、转发和重复消息风险，不把迟到消息当成最新状态，并要求时间标记。
tags: LoRa, Mesh, 延迟, 转发, 误判
guide_links: DG-0857
risk_level: caution
status: published
source: LanternBox internal curated knowledge
---
# LoRa 延迟和转发误判

## 用途

用于 LoRa 或 Mesh 消息不是实时到达时的判断。低速链路可能延迟、重复、乱序或经过中继后才出现。

## 操作步骤

1. 每条消息都带发送时间和发送者编号。
2. 收到迟到消息时先看时间，不立即按当前状态处理。
3. 同一内容重复出现时标记为重复，不再次执行动作。
4. 重要消息要求回执和下一窗口确认。
5. 出现乱序时按最新时间和已确认状态整理日志。

## 判断标准

- 可继续：消息有时间、回执和状态，能区分新旧。
- 需复核：同一节点消息重复、延迟或缺少时间。
- 停止条件：消息时间不明却要求移动、分发物资或改变路线时，先暂停行动并等待确认。

## 风险提示

- 不要把迟到的“安全”消息当作当前安全。
- 不要对重复消息重复派人。
- 不要让延迟消息覆盖现场观察。
