---
title: 备用电池就绪检查
slug: energy-spare-battery-readiness-check-001
category: 能源
priority: P1
risk_level: high
summary: 在不拆修电池的前提下，复核备用成品电池的身份、安全外观、保守容量、接口匹配和最近验证日期。
tags: 备用电池, 就绪检查, 容量复核, 接口匹配, 电池轮换
guide_links: DG-0921, DG-0923
kiwix_topics: spare battery readiness, backup power, battery rotation
last_reviewed: 2026-08-25
status: published
source: LanternBox internal curated knowledge; Batch15-A coverage plan
---
# 备用电池就绪检查

## 用途

用于确认台账中的备用电池在真正需要切换时能否安全承担指定负载，而不是只因“有一块备用”就把它计入系统冗余。

## 适用场景

适用于已知型号、已知低压接口、可整体断开的成品电池模块。未知化学体系、改装包、外壳破损或需要拆包验证的对象不适用。

## 准备材料

备用电池台账、设备编号、接口标签、已验证低风险负载、计时工具、停用标签。

## 操作步骤

1. 核对 battery_id、型号、接口、电压范围、上次轮换和上次容量复测日期。
2. 检查鼓包、漏液、腐蚀、摔裂、异味、异常热和接头损伤。
3. 只用已知匹配的低压负载做短时就绪验证，记录 runtime 或任务完成情况。
4. 将结果与该备用源要承担的关键负载最低线比较。
5. 标记 READY、LIMITED、UNKNOWN 或 ISOLATED，并写下允许用途。
6. 更新轮换日期和下一次验证时间；切换路径另按断开后单路接入执行。

## 判断标准

- READY：身份、接口、安全外观和保守能力均有近期证据。
- LIMITED：只能承担明确的低风险短任务，不计入关键负载冗余。
- UNKNOWN：记录缺失或久未验证，不能直接承诺关键任务。

## 风险提示

停止条件：鼓包、漏液、异常热、异味、腐蚀、摔裂或接口烧焦时立即停止、隔离并交 Energy Safety。禁止拆包、换芯、点焊或改 BMS 来恢复“就绪”。

## 替代方案

没有可验证备用源时，减少关键负载数量、缩短运行窗口，并把系统标为存在单点故障。

## 记录建议

记录 battery_id、status、checked_at、接口、电压范围、test_load、runtime、allowed_use、next_check 和 reviewer。

## 相邻领域边界

Storage 管理长期存放条件；Energy Safety 处理异常；Energy Deepening 只确认可用性和冗余贡献。

## 对应 Guide

- DG-0921｜电池实际容量下降复查｜使用时机：验证备用电池容量和健康状态。
- DG-0923｜单点故障、备用路径与关键负载切换｜使用时机：把已验证备用源纳入切换计划。
