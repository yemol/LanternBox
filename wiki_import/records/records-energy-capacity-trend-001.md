---
title: 能源容量与产出趋势
slug: records-energy-capacity-trend-001
category: 信息保存与长期重建
priority: P1
risk_level: caution
summary: 比较设备充入、消耗、太阳能产出和可用容量随时间的变化，记录测量条件但不指导当前电量分配。
tags: 能源趋势, 电池容量, 太阳能产出, 长期退化
guide_links: [DG-0914, DG-0919, DG-0921, DG-0922, DG-0930]
kiwix_topics: cross-domain records, longitudinal review, offline knowledge
last_reviewed: 2026-08-24
status: published
source: LanternBox internal curated knowledge; docs/KNOWLEDGE_STRATEGY.md; Batch14-B coverage plan
---
# 能源容量与产出趋势

## 用途
用于数周或数月回看电池、移动电源、太阳能或关键设备的容量与产出变化。当前充电、负载优先级和异常电池处置由 Energy Guide 主导。

## 操作步骤
1. 使用稳定 device_id 汇总充入、消耗、产出、可用容量和异常记录。
2. 核对单位、测量工具、温度、负载模式、天气和设备版本。
3. 按相同条件比较周期，标出无法比较和缺测区间。
4. 识别缓慢下降、突然变化、天气依赖和维护前后差异，同时保留反例。
5. 形成容量/产出趋势和待复测项，不把估算写成额定值。
6. 将异常趋势交给 Energy reviewer；任何调度改变另走 Energy 决策。

## 判断标准
- 趋势包含测量条件和单位。
- 缺测、设备更换和温度影响可见。
- 记录结论不要求拆修电池或绕过安全边界。

## 风险提示
停止条件：电池鼓包、漏液、过热、异味、接线发热或当前电量危及关键负载时，停止趋势整理并立即进入 Energy Safety。

长期记录不得覆盖当前业务域的安全动作；相关性只能形成待验证假设，不能自动成为原因或正式知识。

## 离线替代方案
纸卡按周记录设备、充入、消耗、天气、估算容量和异常；工具不足时保留相对变化并标低置信度。

## 记录与交接
保存 trend_id、device_id、测量条件、period、数据来源、异常、估算方法、reviewer 和下一复测。

## 相邻领域边界
Energy 主导当前预算、充电、电池隔离和负载优先级；Records 只保存长期退化事实。

## 对应 Guide
- DG-0914
