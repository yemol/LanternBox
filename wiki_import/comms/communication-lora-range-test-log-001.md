---
title: LoRa 距离测试记录
slug: communication-lora-range-test-log-001
category: 通讯
priority: P1
summary: 记录测试点、时间、天气、成功失败、延迟和电量，建立真实 LoRa 覆盖图。
tags: LoRa, 距离测试, 覆盖图, 记录, 节点
guide_links: DG-0857
risk_level: normal
status: published
source: LanternBox internal curated knowledge
---
# LoRa 距离测试记录

## 用途

用于把 LoRa 的真实覆盖从猜测变成记录。测试结果服务节点部署、外出路线和备用通信，不追求一次测出最大距离。

## 操作步骤

1. 选定固定节点和移动测试点。
2. 记录时间、天气、位置、节点编号和电量。
3. 发送固定格式测试消息，等待回执。
4. 写下成功、失败、延迟和是否重复收到。
5. 把可靠点、不稳定点和失败点标在纸图上。

## 判断标准

- 可继续：测试字段完整，别人能按记录复测。
- 需补测：只记录成功，不记录失败和电量。
- 停止条件：测试需要进入危险路线、暴露位置或消耗保底电量时，停止测试并保留现有记录。

## 风险提示

- 不要用敏感内容做测试消息。
- 不要把晴天测试结果当雨天结果。
- 不要因为一次成功就取消备用路径。

