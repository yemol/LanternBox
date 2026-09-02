---
title: 离网能源 blackout / degraded 模式卡
slug: energy-blackout-degraded-mode-card-001
category: 能源
priority: P0
risk_level: caution
summary: 用一张公共模式卡定义普通、降级和最低维持状态的进入条件、允许负载、复核频率与恢复门槛。
tags: blackout mode, degraded mode, 最低维持, 负载切除, 恢复条件
guide_links: DG-0920
kiwix_topics: blackout mode, degraded operation, minimum energy survival
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 离网能源 blackout / degraded 模式卡

## 用途

用于让小团队在长期无补电、储能下降或部分发电部件失效时，对当前模式有同一理解，避免每个人按自己的优先级重新抢电。

## 适用场景

适用于安全状态已确认的低压离网系统。设备鼓包、冒烟、火花、进水或未知接线时先停用隔离，不能先进入模式卡继续供电。

## 准备材料

公共模式卡、容量盘点、关键负载最低线、负载断开顺序、负责人和复核时钟。

## 操作步骤

1. 写出 NORMAL、DEGRADED、MINIMUM 三个状态及各自进入条件。
2. 为每个状态列出允许负载、暂停负载、运行窗口和不可挪用保留量。
3. 标记唯一发电源、唯一电池和唯一转换器失效后要停掉的负载。
4. 指定每日或每班复核人，只依据可追溯容量和产出记录变更状态。
5. 从高模式降到低模式时按既有断开顺序执行；恢复时一次只增加一个负载并观察。
6. 把状态变更、原因和下次复核写在同一张卡上。

## 判断标准

- 合格：当前状态、允许负载、保留量、负责人和复核时间可见。
- 待复查：状态只靠口头宣布，或没有退出条件。
- 禁止恢复：只有一次高产出、备用源未验证，或关键储能仍低于保留线。

## 风险提示

停止条件：任何供电路径出现异味、快速升温、火花、潮湿、裸露导体或异常电池时，停止该路径并转安全隔离。不得为维持模式绕过保险或并联未知电源。

## 替代方案

没有可靠容量数据时直接采用 MINIMUM，保留通信、最低照明、医疗照护和记录，其他任务改用无电方案。

## 记录建议

记录 mode、start_time、trigger、allowed_loads、suspended_loads、reserve、reviewer、next_review 和 recovery_evidence。

## 相邻领域边界

每日细分预算归 Energy Management；当前危险归 Safety；模式历史归 Records；本条只定义长期系统状态。

## 对应 Guide

- DG-0920｜连续阴天与长期发电不足降级运行｜使用时机：进入、维持或退出长期降级模式。
