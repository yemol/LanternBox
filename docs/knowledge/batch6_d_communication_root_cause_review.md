# Batch6-D Communication Retrieval Root Cause Review

生成日期：2026-07-16

## 1. Field Test 总结

本阶段只复盘 Batch6-C Communication & Field Network Retrieval Test，不修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback，也不新增 Guide/Wiki。

Batch6-C 当前结果：

|指标|结果|
|---|---:|
|total|19|
|strict / observation|14 / 5|
|pass / partial / fail|8 / 9 / 2|
|Guide 命中率|78.6%|
|主 Guide 命中率|71.4%|
|Wiki 命中率|42.9%|
|Guide-Wiki 精准组合率|78.6%|
|safety boundary|100%|
|fallback|100%|
|record/check|100%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|cross domain|2|

总体判断：

- 安全边界不是主要问题。所有 case 都有 safety / fallback / record-check。
- 失败集中在通信湿环境、通信设备进水、天线接口潮湿等 electronics / repair 相邻场景。
- 多数 partial 不是知识正文缺失，而是正确 Wiki 未进入 selected evidence 前 6，或新 Guide 被旧 Guide / 相邻领域 Guide 排在后面。
- Batch6-B 新增知识已经能支撑 LoRa、临时链路、通信日志等核心能力，但 communication retrieval 还不是 stable。

## 2. Fail Case 根因

### 2.1 通信设备进水还能不能开机测试

Batch6-C 结果：

|项目|结果|
|---|---|
|verdict|fail|
|selected Guide|DG-0842 低压设备异常停用；DG-0546 插线板和延长线进水后；DG-0337 充电优先级|
|selected Wiki|energy-low-voltage-system-stop-boundary-001 等 energy Wiki|
|expected Guide|DG-0855|
|expected Wiki|communication-wet-device-stop-use-001|
|cross domain|electronics / repair 抢主位|

现有通信知识检查：

- `communication-wet-device-stop-use-001` 已存在。
- 该 Wiki 已双向关联 DG-0855、DG-0859。
- Wiki 内容包含“进水后先停用隔离、不继续开机测试、改用备用通信、记录设备编号和复查时间”。
- DG-0855 的 `related_wiki` 包含 `communication-wet-device-stop-use-001`。
- DG-0859 的 `related_wiki` 也包含 `communication-wet-device-stop-use-001`。

根因判断：

|维度|判断|
|---|---|
|profile 缺失|是。当前存在 `energy_low_voltage_fault_stop` 等能源/低压安全 profile，但缺少 `communication_wet_device_safety`。|
|Guide priority|不是主要问题。DG-0855、DG-0859 都是 P0/high，但未进入 selected top3。|
|Wiki evidence priority|是次要问题。正确 Wiki 已存在且已关联，但只有正确 Guide 进入 selected 后才稳定加载。|
|selector / ranking|是。现有 query “通信设备 + 进水 + 开机测试”被低压设备/插线板进水路线吸收。|
|communication safety boundary 缺失|否。通信 Wiki 和 Guide 已有停用、隔离、备用路径、记录边界。|

结论：这是 **profile 缺失 + 跨域 ranking/selector 问题**，不是知识正文缺失。若目标是“通信设备是否继续使用、是否保护通信能力”，通信应优先；energy/electronics 作为安全补充。

### 2.2 雨天接口潮湿还能不能架天线

Batch6-C 结果：

|项目|结果|
|---|---|
|verdict|fail|
|selected Guide|DG-0620 接口潮湿和线头保护；DG-0594 潮湿寒冷；DG-0858 无线电通信前检查|
|selected Wiki|energy / tools / fire / communication-radio-basic-terms-001|
|expected Guide|DG-0858 或 DG-0855|
|expected Wiki|communication-antenna-wet-weather-stop-001|
|cross domain|electronics 抢主位，repair 相邻竞争|

现有通信知识检查：

- `communication-antenna-wet-weather-stop-001` 已存在。
- 该 Wiki 已关联 DG-0858。
- Wiki 内容包含潮湿接口、雷雨、线缆破皮、设备发热、停止发射、撤下临时天线、备用通信路径和记录。
- DG-0858 已进入 selected Guide 第 3 位，但由于前两位为 power/shelter，selected Wiki 前 6 被相邻领域 related Wiki 占用，未加载天线潮湿停止线。

根因判断：

|维度|判断|
|---|---|
|profile 缺失|是。缺少 `communication_antenna_weather_safety`，无法把“雨天 + 接口潮湿 + 天线/发射”稳定判为通信安全场景。|
|Guide priority|部分相关。DG-0858 已进入但不是主位，DG-0855 第 5 候选。|
|Wiki evidence priority|是。`communication-antenna-wet-weather-stop-001` 在 DG-0858 related_wiki 中位置偏后，当前前 6 selection 容易被无线电基础 Wiki 或相邻领域 Wiki 挤掉。|
|selector / ranking|是。DG-0620 的“接口潮湿”强匹配把场景拉到 power。|
|communication safety boundary 缺失|否。通信天线天气风险 Wiki 已有明确停止线。|

结论：这是 **profile 缺失 + Guide-Wiki evidence priority 问题**。通信安全应该优先，因为用户问的是“能不能架天线/继续通信”；power/repair 只应补充“接口潮湿不可带电测试”的安全理由。

## 3. Partial 分类

### A. 正确 Guide 存在，但没有进入 selected evidence

|case|表现|判断|
|---|---|---|
|communication_low_power_schedule|DG-0859 只在候选第 6，未进 selected；旧 DG-0582/DG-0561 抢先|低功耗通信 profile 或 Guide 入口权重不足|
|communication_shortwave_unverified_route|DG-0860 在候选第 8，旧 DG-0640/外部信息 Guide 抢先|短波未核信息应由记录/交接主导还是旧短波 Guide 主导需定边界|
|communication_observe_shortwave_noise|DG-0860 候选第 4，旧 DG-0640 和噪声管理 Guide 抢先|observation，可后续观察|

### B. Wiki 存在，但没有召回或没有进入 selected Wiki

|case|缺失 Wiki|主要原因|
|---|---|---|
|communication_radio_no_reply_check|communication-radio-no-receive-check-001 / communication-device-fault-tree-001|DG-0858 排第一，related_wiki 前 6 都是通信前检查基础 Wiki，DG-0855 的故障排查 Wiki 被挤出|
|communication_loose_antenna_transmit|communication-antenna-connection-check-001|DG-0858 related_wiki 中天线连接位置偏后|
|communication_wet_antenna_rain|communication-antenna-wet-weather-stop-001|power/shelter 抢主位，且天线潮湿 Wiki 在 DG-0858 中位置偏后|
|communication_missed_checkin_window|communication-check-in-missed-window-001|DG-0860 候选第 4，未进入 selected；DG-0859 related_wiki 抢前 6|
|communication_low_power_schedule|communication-battery-radio-runtime-budget-001|DG-0859 未进入 selected；旧手机低电量 Guide 抢主位|
|communication_shortwave_unverified_route|communication-shortwave-receive-log-verification-001|旧 DG-0640 进入但未关联新短波核验 Wiki；DG-0860 排后|
|communication_mesh_node_roles|communication-mesh-node-role-map-001|DG-0857/DG-0856 正确进入，但 Mesh Wiki 在 related_wiki 中位置偏后，wiki_candidates 虽命中但未进入前 6|
|communication_wet_device_power_on|communication-wet-device-stop-use-001|能源/低压 Guide 抢主位，通信 Guide 未进入 selected|
|communication_observe_antenna_cable_broken|communication-antenna-connection-check-001 / communication-antenna-cable-strain-relief-001|observation；正确 Guide 进入，Wiki 被前置基础 Wiki 挤出|
|communication_observe_shortwave_noise|communication-shortwave-noise-source-check-001|observation；旧短波/噪声 Guide 抢先，DG-0855 未进入|
|communication_observe_team_split_checkin|communication-radio-message-format-short-001 / communication-check-in-missed-window-001|observation；旧报平安/撤离 Guide 更强|

### C. 领域 profile 缺失

明确缺口：

- `communication_wet_device_safety`
- `communication_antenna_weather_safety`
- `communication_device_failure_isolation`

可能后续需要，但不建议 Batch6-E 一次铺太大：

- `communication_low_power_multi_device`
- `communication_shortwave_unverified_info`
- `communication_team_split_checkin`

### D. 合理 observation，不立即修

|case|理由|
|---|---|
|communication_observe_antenna_cable_broken|正确 Guide 已进入，repair 未抢主位；主要是 related_wiki 前 6 选择问题。|
|communication_observe_shortwave_noise|旧 DG-0640 本身是 comms，短波噪声场景可先观察。|
|communication_observe_team_split_checkin|报平安/撤离/同行请求是合理竞争域，是否强行通信主导需要更多 field case。|

## 4. Guide-Wiki 缺口

### DG-0855 通信设备无法连接排查

现有强项：

- 已覆盖设备故障树、无线电收不到/发不出、天线连接、进水停用、短波噪声。
- 安全边界足够，含“设备进水、发热、异味、电池异常、接口损坏或天线断裂时停止使用并隔离”。

缺口：

- `communication-wet-device-stop-use-001` 已关联，但在“通信设备进水”场景中 DG-0855 没有进入 selected。
- `communication-antenna-connection-check-001` 已关联，但在天线松动场景中容易被 DG-0858 前置基础 Wiki 挤出。
- 可以考虑在 Batch6-E 调整 DG-0855 / DG-0858 之间的 related_wiki 优先顺序，而不是新增 Wiki。

### DG-0858 无线电通信前检查

现有强项：

- 发射前监听、频道、身份、消息格式、天线、天气停止线都已覆盖。
- `communication-antenna-wet-weather-stop-001` 已关联。

缺口：

- 天线连接和雨天潮湿 Wiki 在 related_wiki 中偏后；当前 selection 容易先取无线电基础、频道、消息格式，导致高风险天线场景证据不进前 6。
- “雨天接口潮湿还能不能架天线”需要 profile 把 DG-0858 拉到主位，同时需要 related_wiki 让 `communication-antenna-wet-weather-stop-001` 更靠前。

### DG-0859 通信设备低功耗运行

现有强项：

- 已覆盖电量预算、QRP、电源保存、通信窗口、进水停用。
- observation “低功耗通信”通过，说明该 Guide 对短查询有效。

缺口：

- “手机、LoRa、电台怎么安排”被旧手机低电量 Guide 抢主位，DG-0859 只排第 6。
- 这是多设备通信调度 profile 缺口，不是 Wiki 缺失。

### DG-0857 LoRa 节点部署检查

现有强项：

- LoRa 节点位置、两次收不到消息、未知节点全部 pass。
- LoRa 基础能力已经可作为 v0.1 稳定点。

缺口：

- Mesh 节点角色 Wiki 已关联且 wiki_candidates 命中第 1，但 selected Wiki 前 6 被 LoRa placement/id/message/range 等挤掉。
- 建议 Batch6-E 只在需要时调整 related_wiki 顺序；不新增 Mesh Wiki。

### 是否缺少三类 Wiki

|检查项|结论|
|---|---|
|湿环境通信安全 Wiki|不缺。`communication-antenna-wet-weather-stop-001` 已覆盖。|
|通信设备进水停用 Wiki|不缺。`communication-wet-device-stop-use-001` 已覆盖。|
|天线天气风险 Wiki|不缺。雨天潮湿天线停止线和短波天线安全摆放已覆盖。|

当前不建议新增 Wiki。

## 5. Profile 判断

现有 `data/retrieval_query_profiles.json` 中没有专门 communication profile。相关竞争 profile/旧入口包括：

- `repair_wire_damage`：覆盖“潮湿、通电、线缆、接口”一类触发。
- `energy_low_voltage_fault_stop` 等能源低压安全逻辑：会优先解释“进水、开机测试、通电”。
- 旧 comms Guide：DG-0582、DG-0636、DG-0640、DG-0192 对手机低电量、固定窗口、短波、报平安仍有强入口。

建议 profile 判断：

|候选 profile|是否需要|理由|
|---|---|---|
|communication_wet_device_safety|需要|“通信设备 + 进水/潮湿/开机测试/充电测试”应优先进入 DG-0855/DG-0859 和 `communication-wet-device-stop-use-001`。|
|communication_antenna_weather_safety|需要|“天线 + 雨天/潮湿/接口/发射/架设”应优先进入 DG-0858/DG-0855 和 `communication-antenna-wet-weather-stop-001`。|
|communication_device_failure_isolation|需要但可合并|“无线电没人回、天线松、节点连不上、设备故障”需要把 DG-0855 作为故障入口；可与 wet_device_safety 合并为小范围 profile，避免过宽。|
|communication_low_power_multi_device|暂缓|低功耗通信 partial 未产生危险或跨域，旧 Guide 仍在 comms/power 内；可在 Batch6-F 再处理。|
|communication_shortwave_unverified_info|暂缓|短波场景主要是旧短波 Guide 与新记录 Guide 边界问题，先做 Guide-Wiki 最小关联观察。|
|communication_team_split_checkin|暂缓|observation，且 evacuation/navigation 合理竞争，不宜先扩 profile。|

## 6. 通信领域边界规则

### 6.1 设备进水

Communication 优先：

- 用户目标是“通信设备还能不能继续使用”。
- 用户目标是“如何保护通信能力/备用联络”。
- 对象是手机、对讲机、无线电、LoRa 节点、收音机、电池盒等通信设备。
- 问题包含开机测试、发射、通信窗口、备用通信、报平安。

Electronics / energy 优先：

- 用户目标是电路维修、元件损坏、测量电压、拆机、焊接。
- 对象是插线板、延长线、低压供电系统、裸线、电池异常。
- 问题核心是供电安全，而不是通信连续性。

推荐输出边界：

- 通信主导时，答案应先给“停用/隔离/备用通信/记录复查”。
- energy/electronics 作为补充时，只提供“不通电测试、不充电、隔离观察”的安全理由。

### 6.2 天线损坏和潮湿

Communication 优先：

- 用户目标是能否发射、能否架天线、信号为何下降、无线电/LoRa 是否可继续通信。
- 对象是天线、馈线、接口、LoRa 节点、无线电。
- 风险是通信失败、设备受损、暴露、耗电。

Repair / electronics 补充：

- 用户目标是焊接、接头维修、线缆替换、绝缘材料、机械固定。
- 问题明确要求修复方法而不是通信使用决策。

推荐输出边界：

- 天线松动、接口潮湿、线缆破皮时，通信 Guide 应主导“停止发射/撤下/备用路径/记录复查”。
- repair 只补充“不要用胶带掩盖进水、不要让线缆受力、修前断电”。

### 6.3 短波和外部信息

Communication / records 优先：

- 用户问“听到消息能不能改变路线/配给/行动”。
- 关键是未核信息、记录、回执、复听和交叉验证。

Navigation / evacuation 补充：

- 用户已经进入路线选择、撤离决策或危险区域判断。
- 通信证据只负责消息可信度和记录，不替代路线风险判断。

## 7. Batch6-E 最小 Apply 建议

建议选择：**C. profile 与 Guide-Wiki 都需要，但必须最小化。**

不建议 A 只调整 profile：

- 能把 DG-0858/DG-0855 拉回主位，但 `communication-antenna-wet-weather-stop-001`、`communication-antenna-connection-check-001`、`communication-mesh-node-role-map-001` 仍可能因为 related_wiki 顺序偏后而不进前 6。

不建议 B 只调整 Guide-Wiki：

- 不能解决“通信设备进水”被 DG-0842/DG-0546 抢主位的问题。
- 不能稳定解决“雨天接口潮湿”由 DG-0620 主导的问题。

不建议 D 暂不修复：

- 当前 strict fail = 2，且都是高风险停用场景；继续观察会留下 communication wet safety 的主入口缺陷。

### Batch6-E 最小范围

建议只做：

1. 新增少量 communication query profile，不改 Retrieval Pipeline / Prompt / top_k / selector limit / ranking / fallback。
   - `communication_wet_device_safety`
   - `communication_antenna_weather_safety`
   - 可选合并：`communication_device_failure_isolation`

2. 最小 Guide-Wiki 调整，不新增 Wiki，不新增 Guide。
   - DG-0858：将 `communication-antenna-connection-check-001`、`communication-antenna-wet-weather-stop-001` 在 related_wiki 中前移。
   - DG-0855：确保 `communication-device-fault-tree-001`、`communication-radio-no-receive-check-001`、`communication-antenna-connection-check-001`、`communication-wet-device-stop-use-001` 优先进入故障/进水/天线场景。
   - DG-0857 / DG-0856：将 `communication-mesh-node-role-map-001` 前移或建立更明确的 Mesh 场景 evidence 顺序。
   - 评估 DG-0640 是否应关联 `communication-shortwave-receive-log-verification-001`，用于旧短波入口与新核验 Wiki 的桥接。

3. 不做：
   - 不新增通信 Wiki。
   - 不新增 Guide。
   - 不压低旧 Guide。
   - 不改 selector limit / top_k。
   - 不硬编码测试答案。

### Batch6-E 验收建议

- 重新运行 Batch6-C communication field test。
- 目标先达到 strict fail = 0。
- safety / fallback / record-check 继续 100%。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- 允许 observation partial 保留，尤其是短波噪声、队伍分散报平安。

## 8. 本阶段边界

- 本报告只分析，不实施。
- 本阶段未运行生产同步。
- 本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback。
