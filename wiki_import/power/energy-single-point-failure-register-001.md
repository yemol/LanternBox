---
title: 能源系统单点故障清单
slug: energy-single-point-failure-register-001
category: 能源
priority: P1
risk_level: caution
summary: 逐段列出只有一个发电源、电池、转换器、线缆或接头即可使关键负载停止的单点，并安排低风险替代。
tags: 单点故障, 能源冗余, 关键负载, 备用路径, 故障清单
guide_links: DG-0923
kiwix_topics: single point of failure, off-grid redundancy, critical load resilience
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 能源系统单点故障清单

## 用途

用于识别一个部件失效就会让通信、最低照明、医疗照护或记录设备失去供电的位置，并区分真正冗余与只存在于清单上的备用件。

## 适用场景

适用于已有低压供电路径图的小型离网系统。不适用于市电、并网、高压 DC、工业 UPS 或需要工程计算的大型系统。

## 准备材料

最新路径图、关键负载清单、备用件台账、最近验证记录和纸表。

## 操作步骤

1. 从每个关键负载逆向追到发电源，列出储能、转换器、保护件、线缆和接头。
2. 对每个对象假设“此件不可用”，写出受影响负载和可维持时长。
3. 检查是否有电压、极性和接口均匹配的成品备用模块，以及它最近何时验证。
4. 将没有可用替代的对象标为 S1 单点，将有未验证备用的对象标为 S2 待验证。
5. 优先为高影响单点准备低风险整件替换、无电替代或缩减负载方案。
6. 部件、路径或关键负载变化后更新清单。

## 判断标准

- 真正冗余：备用源和路径独立、匹配、近期验证，且切换不需带电改线。
- 名义冗余：备用件未测、共用同一接头或依赖同一个控制器。
- 高优先单点：失效会同时中断多个关键负载且无人工替代。

## 风险提示

停止条件：验证需要带电拆线、并联未知电源、绕过保护、未知极性或进入市电/高压范围时停止。不得用危险改装消除单点。

## 替代方案

无法增加备用件时减少同时依赖该路径的关键负载，准备纸本、手动或轮流运行方案。

## 记录建议

记录 failure_point_id、affected_loads、impact、spare_id、shared_dependency、last_test、fallback 和 owner。

## 相邻领域边界

Repair 负责怎样替换部件；Energy Management 负责当天资源竞争；Energy Deepening 负责单点影响和冗余规划。

## 对应 Guide

- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：识别和降低关键供电单点。
