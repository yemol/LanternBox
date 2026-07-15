# Batch5-C Energy Safety Retrieval Root Cause Review

生成时间：2026-07-14

## 1. 范围

本报告复核 `docs/knowledge/batch5_b_energy_safety_field_test_report.md` 中 8 个未达标 case。  
本阶段只分析，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit 或 ranking。

参考文件：

- `docs/knowledge/batch5_b_energy_safety_field_test_report.md`
- `docs/knowledge/batch5_b_energy_safety_field_test_results.json`
- `docs/knowledge/batch5_a1_energy_safety_apply_report.md`
- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

## 2. 总体判断

Batch5-A1 的 18 篇 P0 能源安全 Wiki 和 3 个 Guide 已存在，且 Guide-Wiki 关系在内容层面基本成立。Batch5-B 的主要问题不是继续缺 Wiki，而是：

1. 新增能源安全主题缺少 query profile，导致高危问题没有被稳定拉入能源安全候选池。
2. 旧能源 Guide 和旧 Wiki 在 selector 中优先于 Batch5-A1 新 Guide/Wiki。
3. 少量问题存在 Guide 设计/测试期望冲突，例如“红黑线接反”当前挂在 DG-0843，但测试期望 DG-0842。
4. “停电三天优先给什么设备充电”属于能源 P1 设备优先级和能源预算，不应在 P0 安全边界批次中硬修。

Batch5-B 没有出现危险建议，`stop_or_escalate`、fallback、record/check 大多由旧证据兜住；但这不能掩盖新增 P0 安全知识未进入 evidence 的问题。

## 3. Case 复盘

| case | 当前 evidence | 理想 evidence | 根因分类 | 是否需要 Apply |
| --- | --- | --- | --- | --- |
| `energy_battery_power_bank_swollen` | DG-0563、DG-0338、DG-0564；旧移动电源/太阳能 Wiki；旧 `energy-swollen-lithium-battery-stop-use-001` 进入 evidence | DG-0841；`energy-damaged-power-bank-quarantine-001`、`energy-battery-storage-temperature-boundary-001` | selector 排序问题 + profile 缺口。DG-0841 已在候选第 4 位，但未进最终 Guide；新 Wiki 未被独立召回 | 是 |
| `energy_spare_battery_hot_after_storage` | DG-0562、DG-0618、DG-0616；旧电池容量、轮换、充电记录 Wiki | DG-0841；`energy-battery-storage-temperature-boundary-001`、`energy-battery-leak-corrosion-isolation-001` | selector 排序问题 + profile 缺口。DG-0841 在候选第 4 位；“备用电池/存放后发热/是否测试”未提升异常隔离 | 是 |
| `energy_battery_leak_handling` | DG-0112、DG-0618、DG-0841；最终 Wiki 仍是旧电池条目 | DG-0841；`energy-battery-leak-corrosion-isolation-001` | evidence priority/selector 问题。DG-0841 已选中，且关联列表包含目标 Wiki，但旧 Guide/Wiki 占据前 6 个 selected wiki | 是，但不需要新增 Wiki |
| `energy_usb_device_overheating` | DG-0563、DG-0107、DG-0382；移动电源优先级和维修材料 Wiki | DG-0842；`energy-low-voltage-system-stop-boundary-001`、`energy-device-smell-heat-no-restart-001` | selector 排序问题 + profile 缺口。DG-0842 在候选第 6 位，USB/发烫/继续用未稳定触发低压异常停用 | 是 |
| `energy_wire_getting_hot_after_wiring` | DG-0006 医疗感染、DG-0276 备份、DG-0426 离线资料；医疗/general Wiki | DG-0842；`energy-short-circuit-warning-signs-001`、`energy-wire-heating-load-limit-001` | profile 缺口 + 跨域竞争。`线越来越热` 被医疗“红肿热痛”误吸，DG-0842 未进入前 8 | 是 |
| `energy_red_black_reverse_polarity` | 无 Guide、无 Wiki、无候选 | DG-0842；`energy-dc-polarity-reverse-check-001` | profile 缺口 + Guide 设计/测试期望冲突。Wiki 存在，但当前关联 DG-0843；测试期望 DG-0842 | 是 |
| `energy_unknown_matching_port` | DG-0620、DG-0546、DG-0804；接口潮湿、进水、浑浊水 Wiki | DG-0843；`energy-unknown-adapter-stop-use-001`、`energy-loose-connector-stop-use-001` | selector 排序问题 + profile 缺口。DG-0843 在候选第 5 位，未知接口/匹配/通电测试未提升未知电源判断 | 是 |
| `energy_three_day_blackout_charge_priority` | DG-0107、DG-0563、DG-0564；设备充电优先级、移动电源、太阳能 Wiki | 观察型 case，无 P0 预期 | 合理 partial / P1 范围。当前命中内容语义合理；跨通信是设备优先级天然涉及通信设备 | 否，留到能源 P1 |

## 4. 现有 Wiki 是否存在但未进入 evidence

存在。

- `energy-damaged-power-bank-quarantine-001` 存在，关联 DG-0841，但 `energy_battery_power_bank_swollen` 中未进入 selected evidence。
- `energy-battery-storage-temperature-boundary-001` 存在，关联 DG-0841，但电池鼓起和备用电池发热 case 中未进入 selected evidence。
- `energy-battery-leak-corrosion-isolation-001` 存在，关联 DG-0841；漏液 case 中 DG-0841 已被选中，关联列表也包含该 Wiki，但最终 selected wiki 被旧条目占满。
- `energy-low-voltage-system-stop-boundary-001`、`energy-device-smell-heat-no-restart-001`、`energy-short-circuit-warning-signs-001`、`energy-wire-heating-load-limit-001` 均存在，关联 DG-0842，但 USB 发烫和接线发热 case 未进入 selected evidence。
- `energy-dc-polarity-reverse-check-001` 存在，但当前关联 DG-0843；红黑线接反 case 没有任何候选。
- `energy-unknown-adapter-stop-use-001` 存在，关联 DG-0843；接口匹配 case 中 DG-0843 仅在候选第 5 位，未进入最终 evidence。
- `energy-loose-connector-stop-use-001` 存在，关联 DG-0842；接口匹配 case 期望它作为松动/匹配风险补充，但当前没有进入 evidence。

结论：Batch5-B 报告中的“数据缺口”多数应重分类为“已有数据未进入 evidence”，不是内容缺失。

## 5. 现有 Guide 是否存在但没有优先

存在。

- DG-0841 在电池鼓起、备用电池发热、电池漏液三个 case 中均进入候选或 selected，但排序不稳定。鼓起和发热 case 排在第 4 位，漏液 case 进入 selected 第 3 位但 Wiki priority 不足。
- DG-0842 在 USB 发烫 case 中进入候选第 6 位，但未被选中；在接线发热 case 中没有进入前 8，被医疗/通用备份类 Guide 抢占。
- DG-0843 在未知接口匹配 case 中进入候选第 5 位，但未被选中；旧接口潮湿、插线板进水、水处理 Guide 抢占。
- 红黑线接反 case 没有候选。这里不仅是排序问题，还存在触发词缺失；同时该 Wiki 当前挂 DG-0843，而测试期望 DG-0842。

## 6. 需要新增 Profile 的问题

建议新增少量能源安全 profile，不新增大量规则。

### 6.1 电池异常隔离 profile

建议名称：`energy_battery_abnormal_isolation`

覆盖 case：

- `energy_battery_power_bank_swollen`
- `energy_spare_battery_hot_after_storage`
- `energy_battery_leak_handling`

核心触发：

- object：电池、备用电池、移动电源、充电宝、电池盒
- state：鼓包、鼓起来、胀、发热、烫、漏液、腐蚀、异味、摔裂、放了很久、存放半年
- action：继续充、继续用、测试、还能不能用

优先 evidence：

- DG-0841
- `energy-damaged-power-bank-quarantine-001`
- `energy-battery-storage-temperature-boundary-001`
- `energy-battery-leak-corrosion-isolation-001`
- `energy-battery-chemistry-mix-stop-001`

### 6.2 低压设备异常停用 profile

建议名称：`energy_low_voltage_fault_stop`

覆盖 case：

- `energy_usb_device_overheating`
- `energy_wire_getting_hot_after_wiring`
- 可兼容红黑线接反中的低压接线语义

核心触发：

- object：USB、低压设备、设备、线、线缆、电线、接线、红黑线
- state：发烫、发热、越来越热、异味、火花、短路、接反、反接
- action：继续用、通电、测试、是不是功率不够

优先 evidence：

- DG-0842
- `energy-low-voltage-system-stop-boundary-001`
- `energy-device-smell-heat-no-restart-001`
- `energy-short-circuit-warning-signs-001`
- `energy-wire-heating-load-limit-001`
- `energy-dc-polarity-reverse-check-001`（若决定把极性问题也纳入 DG-0842）

需要避免：

- 医疗“红肿热痛”在没有电/线/接线/USB/设备锚点时被能源 profile 抢走。

### 6.3 未知电源/接口匹配 profile

建议名称：`energy_unknown_power_adapter_match`

覆盖 case：

- `energy_unknown_matching_port`
- 稳定已 pass 的 `energy_unknown_old_charger_voltage`

核心触发：

- object：旧充电器、适配器、接口、插头、电源、线缆
- state：不知道多少伏、不知道匹配、能插进去、参数不明、极性不明
- action：试一下、通电看看、能不能插、能不能用

优先 evidence：

- DG-0843
- `energy-unknown-adapter-stop-use-001`
- `energy-temporary-wiring-precheck-001`
- `energy-fuse-protection-no-bypass-001`
- `energy-loose-connector-stop-use-001`（作为接口松动/匹配风险补充）

## 7. 应留到能源 P1 的问题

`energy_three_day_blackout_charge_priority` 应留到能源 P1。

原因：

- 该 case 是设备充电优先级、能源预算、长期停电策略，不是 Batch5-A1 的 P0 电气危险边界。
- 当前 selected evidence 包含 DG-0107、DG-0563、DG-0564，语义上合理。
- `power_vs_communication` 在该场景中不一定是错误；“给什么设备充电”自然会涉及通信、照明、水处理、记录等关键设备。
- 不建议为了让观察型 case 变成 pass 而调整 P0 profile，否则会把安全边界 profile 扩到能源调度类问题。

太阳能阴天 case 已 pass，也应留给后续 P1/P2 扩展，不在 Batch5-D 修。

## 8. Selector / Ranking 问题

当前不建议修改 selector、ranking、top_k 或 selector limit。

可先通过少量 profile 将正确 Guide/Wiki 提前，观察是否足够解决：

- 电池鼓起、备用电池发热：DG-0841 已在第 4 位，profile boost 很可能足够。
- USB 发烫：DG-0842 已在第 6 位，profile boost 很可能足够。
- 未知接口匹配：DG-0843 已在第 5 位，profile boost 很可能足够。
- 漏液 case：DG-0841 已选中，但目标 Wiki 未进 selected wiki；若 profile 将 DG-0841 提到第 1 位，关联 Wiki 顺序可能自然改善。

如果 Batch5-D 后漏液 case 仍只选旧 Wiki，再单独评审 related_wiki ordering，而不是扩大 selector limit。

## 9. Guide 设计冲突

`energy_red_black_reverse_polarity` 是唯一 fail，也是唯一明显 Guide 归属冲突。

当前内容设计：

- `energy-dc-polarity-reverse-check-001` 关联 DG-0843 未知电源安全判断。
- Batch5-B 期望 DG-0842 低压设备异常停用。

判断：

- “红黑线接反”既可以归入未知电源/极性不明，也可以归入低压设备异常停用。
- 如果测试期望保持 DG-0842，则需要在 Batch5-D 中做最小 Guide-Wiki 关系调整：让 `energy-dc-polarity-reverse-check-001` 同时关联 DG-0842，或将 DG-0842 作为该 Wiki 的补充 Guide。
- 如果不允许改 Guide-Wiki，则只能通过 profile 召回 DG-0843，但仍会与 Batch5-B 对 DG-0842 的期望不一致。

不建议新增 Wiki。现有 Wiki 足够。

## 10. Batch5-D 最小 Apply 范围

建议进入 Apply。

最大修改文件数量建议：

- 必要：2 个
  - `data/retrieval_query_profiles.json`
  - 新增或扩展能源 profile 测试文件，例如 `tests/test_energy_safety_retrieval_profiles.py`
- 可选：2 个
  - Guide-Wiki 数据文件，仅在保留 DG-0842 期望时，为 `energy-dc-polarity-reverse-check-001` 增加 DG-0842 关系。
  - Batch5-B field test result/report 刷新文件。
- 报告：1 个
  - `docs/knowledge/batch5_d_energy_safety_apply_report.md`

预计新增 profile 数量：

- 2 到 3 个。

建议新增：

1. `energy_battery_abnormal_isolation`
2. `energy_low_voltage_fault_stop`
3. `energy_unknown_power_adapter_match`

预计新增 Guide-Wiki 关系数量：

- 0 到 1 条。
- 仅当继续要求红黑线接反命中 DG-0842 时，新增 `DG-0842 <-> energy-dc-polarity-reverse-check-001`。

不建议：

- 新增 Wiki。
- 新增 Guide。
- 修改 Retrieval Pipeline。
- 修改 Prompt。
- 扩大 top_k 或 selector limit。
- 调整全局 ranking 架构。
- 为能源 P1 设备优先级提前做 profile。

## 11. 是否可以保持 Energy Safety v0.1 架构不变

可以。

现有架构中，P0 能源安全内容已经通过 3 个 Guide 分组：

- DG-0841 电池异常隔离
- DG-0842 低压设备异常停用
- DG-0843 未知电源安全判断

问题集中在 retrieval entry points，而不是知识结构本身。Batch5-D 应保持架构不变，只补最小 profile 和必要的一个 Guide-Wiki 归属修正。

## 12. 不应修复的问题

以下问题不应在 Batch5-D 中处理：

- 不要修 P1 低电量策略、能源预算、停电三天设备优先级。
- 不要修太阳能阴天展开、角度、遮挡、防风固定。
- 不要合并旧电池 Wiki 或重写旧 Guide。
- 不要为了通过 field test 扩大 selector limit。
- 不要把医疗“发热”整体降权；只应在明确出现电/线/USB/接线对象时提升能源。
- 不要把所有“接口”问题都归能源；潮湿接口、进水插线板在旧 Guide 中仍有合理场景。
- 不要新增重复的电池鼓包、漏液、接线百科条目。

## 13. 结论

Batch5-B 的 8 个未达标 case 中：

- 需要 Batch5-D Apply：7 个。
- 合理 partial / 留到 P1：1 个（`energy_three_day_blackout_charge_priority`）。
- profile 缺口为主：`energy_wire_getting_hot_after_wiring`、`energy_red_black_reverse_polarity`。
- selector 排序为主：电池鼓起、备用电池发热、USB 发烫、未知接口匹配。
- evidence priority 为主：电池漏液。
- Guide 设计冲突：红黑线接反。

建议进入 Batch5-D，但 Apply 范围应限制在能源安全 profile + 最多一条 Guide-Wiki 关系修正，不进入 Pipeline、Prompt、top_k、selector limit 或 P1 内容扩展。
