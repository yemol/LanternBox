# Batch6-C Communication & Field Network Retrieval Field Test Report

生成时间：2026-07-16T06:08:00.012108+00:00

## 1. 测试范围

本阶段只测试 Batch6-B 新增 Communication / Field Network Guide/Wiki 是否稳定进入本地 Retrieval evidence。脚本默认不调用 LLM，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。

覆盖：无线电通信前检查、无回应排查、天线松动和潮湿边界、LoRa 节点部署和异常、临时通信链路、通信窗口、低功耗通信、短波未核信息、Mesh 节点角色、通信日志和进水停用边界。

## 2. 汇总

- 用例总数：19
- strict / observation：14 / 5
- pass / partial / fail：13 / 6 / 0
- Guide 命中率（严格用例，含 allowed secondary）：85.7%
- 主 Guide 命中率（严格用例，仅 expected）：78.6%
- Wiki 命中率（严格用例）：71.4%
- Guide-Wiki 精准组合率（严格用例）：85.7%
- safety boundary 覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- dangerous suggestion 数量：0
- Kiwix 越权数量：0
- cross domain 数量：0

## 3. Case 明细

| case | type | verdict | Guide | Wiki | profiles | safety | fallback | record | cross domain | root cause |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| communication_listen_before_transmit | strict | pass | DG-0858 无线电通信前检查、DG-0801 先确认雨水收集点是否可用、DG-0802 先判断井口水是否可安全使用 | communication-antenna-wet-weather-stop-001 雨天潮湿天线停止线、communication-antenna-connection-check-001 天线连接检查、communication-radio-listen-before-transmit-001 无线电发射前先监听、communication-radio-basic-terms-001 无线电通信基本原理、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_radio_no_reply_check | strict | pass | DG-0855 通信设备无法连接排查、DG-0857 LoRa 节点部署检查、DG-0441 异常解除确认 | communication-device-fault-tree-001 通信设备故障排查顺序、communication-wet-device-stop-use-001 通信设备进水停用边界、communication-radio-no-receive-check-001 无线电收不到信号排查、communication-radio-no-transmit-check-001 无线电发不出或无人确认排查、communication-antenna-connection-check-001 天线连接检查、communication-radio-interference-noise-check-001 无线电噪声和干扰初查 | communication_device_failure_isolation | 是 | 是 | 是 | 无 | 无 |
| communication_loose_antenna_transmit | strict | pass | DG-0858 无线电通信前检查、DG-0855 通信设备无法连接排查、DG-0120 断电前安全关闭 | communication-antenna-wet-weather-stop-001 雨天潮湿天线停止线、communication-antenna-connection-check-001 天线连接检查、communication-radio-listen-before-transmit-001 无线电发射前先监听、communication-radio-basic-terms-001 无线电通信基本原理、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界 | communication_antenna_weather_safety | 是 | 是 | 是 | 无 | 无 |
| communication_wet_antenna_rain | strict | pass | DG-0858 无线电通信前检查、DG-0855 通信设备无法连接排查、DG-0120 断电前安全关闭 | communication-antenna-wet-weather-stop-001 雨天潮湿天线停止线、communication-antenna-connection-check-001 天线连接检查、communication-radio-listen-before-transmit-001 无线电发射前先监听、communication-radio-basic-terms-001 无线电通信基本原理、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界 | communication_antenna_weather_safety | 是 | 是 | 是 | 无 | 无 |
| communication_lora_node_placement | strict | pass | DG-0857 LoRa 节点部署检查、DG-0639 LoRa 节点每日状态记录、DG-0855 通信设备无法连接排查 | communication-lora-node-placement-001 LoRa 节点部署位置、communication-mesh-node-role-map-001 Mesh 网络基础节点角色、communication-lora-node-id-map-001 LoRa 节点编号和位置图、communication-lora-range-test-log-001 LoRa 距离测试记录、communication-lora-store-forward-delay-001 LoRa 延迟和转发误判、communication-lora-message-size-priority-001 LoRa 短消息优先级 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_lora_missed_twice | strict | pass | DG-0857 LoRa 节点部署检查、DG-0855 通信设备无法连接排查、DG-0639 LoRa 节点每日状态记录 | communication-lora-node-placement-001 LoRa 节点部署位置、communication-mesh-node-role-map-001 Mesh 网络基础节点角色、communication-lora-node-id-map-001 LoRa 节点编号和位置图、communication-lora-range-test-log-001 LoRa 距离测试记录、communication-lora-store-forward-delay-001 LoRa 延迟和转发误判、communication-lora-message-size-priority-001 LoRa 短消息优先级 | communication_device_failure_isolation | 是 | 是 | 是 | 无 | 无 |
| communication_lora_unknown_node | strict | partial | DG-0857 LoRa 节点部署检查、DG-0639 LoRa 节点每日状态记录、DG-0681 未知植物不入口 | communication-lora-node-placement-001 LoRa 节点部署位置、communication-mesh-node-role-map-001 Mesh 网络基础节点角色、communication-lora-node-id-map-001 LoRa 节点编号和位置图、communication-lora-range-test-log-001 LoRa 距离测试记录、communication-lora-store-forward-delay-001 LoRa 延迟和转发误判、communication-lora-message-size-priority-001 LoRa 短消息优先级 | 无 | 是 | 是 | 是 | 无 | selector 问题 |
| communication_field_temp_link | strict | pass | DG-0856 野外建立临时通信链路、DG-0129 营地边界标记、DG-0196 无通信外出规则 | communication-check-in-missed-window-001 错过通信窗口后的动作、communication-radio-channel-plan-minimum-001 最小频道计划、communication-lora-node-id-map-001 LoRa 节点编号和位置图、communication-radio-range-line-of-sight-001 无线电距离和遮挡判断、communication-radio-message-format-short-001 无线电短消息格式、communication-antenna-placement-height-safety-001 天线高度影响 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_missed_checkin_window | strict | partial | DG-0859 通信设备低功耗运行、DG-0856 野外建立临时通信链路、DG-0636 断网家庭固定开机窗口 | communication-battery-radio-runtime-budget-001 无线电和 LoRa 电量预算、communication-qrp-power-budget-001 QRP 概念和电量预算、communication-qrp-contact-window-discipline-001 QRP 窗口纪律、communication-wet-device-stop-use-001 通信设备进水停用边界、communication-power-saving-001 低电量通信优先级、communication-device-daily-check-001 通讯设备每日检查 | 无 | 是 | 是 | 是 | 无 | selector 问题 |
| communication_low_power_schedule | strict | partial | DG-0582 手机低电量：固定时间开机联系、DG-0561 手机低电量：保通信模式、DG-0857 LoRa 节点部署检查 | communication-device-daily-check-001 通讯设备每日检查、communication-power-saving-001 低电量通信优先级、communication-sms-002 短信电话和耗电差异、energy-phone-002 手机省电为什么优先短信、communication-lora-node-placement-001 LoRa 节点部署位置、communication-mesh-node-role-map-001 Mesh 网络基础节点角色 | 无 | 是 | 是 | 是 | 无 | selector 问题、ranking 问题 |
| communication_shortwave_unverified_route | strict | partial | DG-0640 短波收听时段和信息记录、DG-0700 外部消息交叉验证记录、DG-0707 判断外部信息可信度 | communication-contact-window-001 固定开机窗口节省电量的原理、communication-offline-communication-001 断网后集合点为什么比设备重要、communication-sms-001 短信电话和即时消息的可靠性差异、safety-barter-001 物资交换前的信息最小化、safety-safety-knowledge-001 交换地点为什么不选核心住所、safety-stranger-contact-001 陌生人求助的边界判断 | 无 | 是 | 是 | 是 | 无 | 数据缺口、selector 问题 |
| communication_mesh_node_roles | strict | pass | DG-0857 LoRa 节点部署检查、DG-0856 野外建立临时通信链路、DG-0639 LoRa 节点每日状态记录 | communication-lora-node-placement-001 LoRa 节点部署位置、communication-mesh-node-role-map-001 Mesh 网络基础节点角色、communication-lora-node-id-map-001 LoRa 节点编号和位置图、communication-lora-range-test-log-001 LoRa 距离测试记录、communication-lora-store-forward-delay-001 LoRa 延迟和转发误判、communication-lora-message-size-priority-001 LoRa 短消息优先级 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_log_minimum_fields | strict | pass | DG-0860 通信信息记录与交接、DG-0859 通信设备低功耗运行、DG-0561 手机低电量：保通信模式 | communication-emergency-net-opening-check-001 应急通信网开网检查、communication-message-priority-levels-001 紧急信息优先级、communication-check-in-missed-window-001 错过通信窗口后的动作、communication-radio-log-minimum-fields-001 通信日志最小字段、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_wet_device_power_on | strict | pass | DG-0855 通信设备无法连接排查、DG-0859 通信设备低功耗运行、DG-0842 低压设备异常停用 | communication-device-fault-tree-001 通信设备故障排查顺序、communication-wet-device-stop-use-001 通信设备进水停用边界、communication-radio-no-receive-check-001 无线电收不到信号排查、communication-radio-no-transmit-check-001 无线电发不出或无人确认排查、communication-antenna-connection-check-001 天线连接检查、communication-radio-interference-noise-check-001 无线电噪声和干扰初查 | energy_low_voltage_fault_stop、communication_wet_device_safety | 是 | 是 | 是 | 无 | 无 |
| communication_observe_network_node_down | observation | pass | DG-0855 通信设备无法连接排查、DG-0581 手机没信号网络断了：家庭通讯计划和集合留言、DG-0857 LoRa 节点部署检查 | communication-device-fault-tree-001 通信设备故障排查顺序、communication-wet-device-stop-use-001 通信设备进水停用边界、communication-radio-no-receive-check-001 无线电收不到信号排查、communication-radio-no-transmit-check-001 无线电发不出或无人确认排查、communication-antenna-connection-check-001 天线连接检查、communication-radio-interference-noise-check-001 无线电噪声和干扰初查 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_observe_antenna_cable_broken | observation | pass | DG-0858 无线电通信前检查、DG-0857 LoRa 节点部署检查、DG-0855 通信设备无法连接排查 | communication-antenna-wet-weather-stop-001 雨天潮湿天线停止线、communication-antenna-connection-check-001 天线连接检查、communication-radio-listen-before-transmit-001 无线电发射前先监听、communication-radio-basic-terms-001 无线电通信基本原理、communication-radio-message-format-short-001 无线电短消息格式、communication-radio-call-sign-identity-check-001 呼号和身份确认边界 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_observe_low_power_comms | observation | pass | DG-0859 通信设备低功耗运行、DG-0122 壳中灯设备低功耗运行、DG-0639 LoRa 节点每日状态记录 | communication-battery-radio-runtime-budget-001 无线电和 LoRa 电量预算、communication-qrp-power-budget-001 QRP 概念和电量预算、communication-qrp-contact-window-discipline-001 QRP 窗口纪律、communication-wet-device-stop-use-001 通信设备进水停用边界、communication-power-saving-001 低电量通信优先级、communication-device-daily-check-001 通讯设备每日检查 | 无 | 是 | 是 | 是 | 无 | 无 |
| communication_observe_shortwave_noise | observation | partial | DG-0640 短波收听时段和信息记录、DG-0347 工具噪声控制、DG-0348 儿童和宠物噪声管理 | communication-contact-window-001 固定开机窗口节省电量的原理、communication-offline-communication-001 断网后集合点为什么比设备重要、communication-sms-001 短信电话和即时消息的可靠性差异 | 无 | 是 | 是 | 是 | 无 | 数据缺口、selector 问题、合理 partial |
| communication_observe_team_split_checkin | observation | partial | DG-0192 固定报平安时间、DG-0710 处理同行请求、DG-0072 撤离队伍顺序 | safety-route-planning-001 陌生路线邀请为什么谨慎 | 无 | 是 | 是 | 是 | 无 | 数据缺口、profile 缺口、合理 partial |

## 4. 逐条复盘

### communication_listen_before_transmit

- query：电台发射前是否要先监听？
- 类型：strict
- focus：无线电通信前检查，发射前先监听。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0858
- allowed secondary：无
- selected Guide：DG-0858、DG-0801、DG-0802
- selected Wiki：communication-antenna-wet-weather-stop-001、communication-antenna-connection-check-001、communication-radio-listen-before-transmit-001、communication-radio-basic-terms-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- profiles：无
- domains：power、records、security、comms、water
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_radio_no_reply_check

- query：无线电没人回，先检查什么？
- 类型：strict
- focus：无人回应时按电源、天线、频道、窗口顺序排查。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0855
- allowed secondary：无
- selected Guide：DG-0855、DG-0857、DG-0441
- selected Wiki：communication-device-fault-tree-001、communication-wet-device-stop-use-001、communication-radio-no-receive-check-001、communication-radio-no-transmit-check-001、communication-antenna-connection-check-001、communication-radio-interference-noise-check-001
- profiles：communication_device_failure_isolation
- domains：records、power、repair、comms、hygiene、water、medical、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_loose_antenna_transmit

- query：天线松了还能不能继续发射？
- 类型：strict
- focus：天线松动时通信安全主导，repair 不应抢主位。
- watch conflicts：repair
- verdict：pass
- expected Guide：DG-0855、DG-0858
- allowed secondary：无
- selected Guide：DG-0858、DG-0855、DG-0120
- selected Wiki：communication-antenna-wet-weather-stop-001、communication-antenna-connection-check-001、communication-radio-listen-before-transmit-001、communication-radio-basic-terms-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- profiles：communication_antenna_weather_safety
- domains：power、records、security、comms、repair、evacuation
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_wet_antenna_rain

- query：雨天接口潮湿还能不能架天线？
- 类型：strict
- focus：潮湿接口和天线安全边界，能源或维修只能 secondary。
- watch conflicts：electronics、repair
- verdict：pass
- expected Guide：DG-0855、DG-0858
- allowed secondary：无
- selected Guide：DG-0858、DG-0855、DG-0120
- selected Wiki：communication-antenna-wet-weather-stop-001、communication-antenna-connection-check-001、communication-radio-listen-before-transmit-001、communication-radio-basic-terms-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- profiles：communication_antenna_weather_safety
- domains：power、records、security、comms、repair、evacuation
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_lora_node_placement

- query：LoRa 节点应该放哪里？
- 类型：strict
- focus：LoRa 节点部署位置。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0857
- allowed secondary：DG-0856
- selected Guide：DG-0857、DG-0639、DG-0855
- selected Wiki：communication-lora-node-placement-001、communication-mesh-node-role-map-001、communication-lora-node-id-map-001、communication-lora-range-test-log-001、communication-lora-store-forward-delay-001、communication-lora-message-size-priority-001
- profiles：无
- domains：power、records、comms、repair
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_lora_missed_twice

- query：LoRa 节点两次收不到消息怎么办？
- 类型：strict
- focus：LoRa 节点失联、延迟和距离测试记录。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0857
- allowed secondary：DG-0855
- selected Guide：DG-0857、DG-0855、DG-0639
- selected Wiki：communication-lora-node-placement-001、communication-mesh-node-role-map-001、communication-lora-node-id-map-001、communication-lora-range-test-log-001、communication-lora-store-forward-delay-001、communication-lora-message-size-priority-001
- profiles：communication_device_failure_isolation
- domains：power、records、comms、repair
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_lora_unknown_node

- query：未知 LoRa 节点出现怎么办？
- 类型：strict
- focus：未知节点隔离和记录。
- watch conflicts：无
- verdict：partial
- expected Guide：DG-0857
- allowed secondary：无
- selected Guide：DG-0857、DG-0639、DG-0681
- selected Wiki：communication-lora-node-placement-001、communication-mesh-node-role-map-001、communication-lora-node-id-map-001、communication-lora-range-test-log-001、communication-lora-store-forward-delay-001、communication-lora-message-size-priority-001
- profiles：无
- domains：power、records、comms、wild_food
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：selector 问题
- failure reasons：未命中预期 Wiki

### communication_field_temp_link

- query：外出队如何建立临时通信链路？
- 类型：strict
- focus：两点通信、临时节点、中继和备用路径。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0856
- allowed secondary：DG-0858
- selected Guide：DG-0856、DG-0129、DG-0196
- selected Wiki：communication-check-in-missed-window-001、communication-radio-channel-plan-minimum-001、communication-lora-node-id-map-001、communication-radio-range-line-of-sight-001、communication-radio-message-format-short-001、communication-antenna-placement-height-safety-001
- profiles：无
- domains：records、evacuation、shelter、comms、hygiene、disaster、security、power
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_missed_checkin_window

- query：检查点错过通信窗口怎么办？
- 类型：strict
- focus：错过通信窗口后的记录、等待和复核。
- watch conflicts：无
- verdict：partial
- expected Guide：DG-0860
- allowed secondary：DG-0856
- selected Guide：DG-0859、DG-0856、DG-0636
- selected Wiki：communication-battery-radio-runtime-budget-001、communication-qrp-power-budget-001、communication-qrp-contact-window-discipline-001、communication-wet-device-stop-use-001、communication-power-saving-001、communication-device-daily-check-001
- profiles：无
- domains：power、records、comms、evacuation、shelter
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：selector 问题
- failure reasons：未命中预期 Wiki

### communication_low_power_schedule

- query：电量只剩一点，手机、LoRa、电台怎么安排？
- 类型：strict
- focus：低功耗通信、通信窗口和保底电量。
- watch conflicts：electronics、energy
- verdict：partial
- expected Guide：DG-0859
- allowed secondary：无
- selected Guide：DG-0582、DG-0561、DG-0857
- selected Wiki：communication-device-daily-check-001、communication-power-saving-001、communication-sms-002、energy-phone-002、communication-lora-node-placement-001、communication-mesh-node-role-map-001
- profiles：无
- domains：comms、power、records
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：selector 问题、ranking 问题
- failure reasons：未命中预期或允许 Guide、未命中预期 Wiki

### communication_shortwave_unverified_route

- query：短波听到未经确认的消息能不能改变路线？
- 类型：strict
- focus：短波未确认信息、记录和路线决策边界，navigation 不应抢主位。
- watch conflicts：gps、navigation
- verdict：partial
- expected Guide：DG-0860
- allowed secondary：无
- selected Guide：DG-0640、DG-0700、DG-0707
- selected Wiki：communication-contact-window-001、communication-offline-communication-001、communication-sms-001、safety-barter-001、safety-safety-knowledge-001、safety-stranger-contact-001
- profiles：无
- domains：comms、external_contact
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：数据缺口、selector 问题
- failure reasons：未命中预期或允许 Guide、未命中预期 Wiki

### communication_mesh_node_roles

- query：Mesh 节点角色怎么安排？
- 类型：strict
- focus：Mesh 节点角色、位置和中继。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0856、DG-0857
- allowed secondary：无
- selected Guide：DG-0857、DG-0856、DG-0639
- selected Wiki：communication-lora-node-placement-001、communication-mesh-node-role-map-001、communication-lora-node-id-map-001、communication-lora-range-test-log-001、communication-lora-store-forward-delay-001、communication-lora-message-size-priority-001
- profiles：无
- domains：power、records、comms、evacuation、shelter
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_log_minimum_fields

- query：通信日志至少写什么？
- 类型：strict
- focus：通信日志最小字段、回执和交接。
- watch conflicts：无
- verdict：pass
- expected Guide：DG-0860
- allowed secondary：无
- selected Guide：DG-0860、DG-0859、DG-0561
- selected Wiki：communication-emergency-net-opening-check-001、communication-message-priority-levels-001、communication-check-in-missed-window-001、communication-radio-log-minimum-fields-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- profiles：无
- domains：records、security、comms、power
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_wet_device_power_on

- query：通信设备进水还能不能开机测试？
- 类型：strict
- focus：通信设备进水停用边界。
- watch conflicts：electronics、repair
- verdict：pass
- expected Guide：DG-0855
- allowed secondary：DG-0859
- selected Guide：DG-0855、DG-0859、DG-0842
- selected Wiki：communication-device-fault-tree-001、communication-wet-device-stop-use-001、communication-radio-no-receive-check-001、communication-radio-no-transmit-check-001、communication-antenna-connection-check-001、communication-radio-interference-noise-check-001
- profiles：energy_low_voltage_fault_stop、communication_wet_device_safety
- domains：records、power、repair、comms、tools、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_observe_network_node_down

- query：网络节点连不上
- 类型：observation
- focus：观察 computer/data 是否抢主位。
- watch conflicts：computer、data
- verdict：pass
- expected Guide：DG-0855、DG-0857
- allowed secondary：无
- selected Guide：DG-0855、DG-0581、DG-0857
- selected Wiki：communication-device-fault-tree-001、communication-wet-device-stop-use-001、communication-radio-no-receive-check-001、communication-radio-no-transmit-check-001、communication-antenna-connection-check-001、communication-radio-interference-noise-check-001
- profiles：无
- domains：records、power、repair、comms
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_observe_antenna_cable_broken

- query：天线线缆坏了
- 类型：observation
- focus：观察 repair 是否抢主位。
- watch conflicts：repair
- verdict：pass
- expected Guide：DG-0855、DG-0858
- allowed secondary：无
- selected Guide：DG-0858、DG-0857、DG-0855
- selected Wiki：communication-antenna-wet-weather-stop-001、communication-antenna-connection-check-001、communication-radio-listen-before-transmit-001、communication-radio-basic-terms-001、communication-radio-message-format-short-001、communication-radio-call-sign-identity-check-001
- profiles：无
- domains：power、records、security、comms、repair
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_observe_low_power_comms

- query：低功耗通信
- 类型：observation
- focus：观察 energy 是否完全覆盖 communication。
- watch conflicts：energy、electronics
- verdict：pass
- expected Guide：DG-0859
- allowed secondary：无
- selected Guide：DG-0859、DG-0122、DG-0639
- selected Wiki：communication-battery-radio-runtime-budget-001、communication-qrp-power-budget-001、communication-qrp-contact-window-discipline-001、communication-wet-device-stop-use-001、communication-power-saving-001、communication-device-daily-check-001
- profiles：无
- domains：power、records、comms、water、medical、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：无
- failure reasons：无

### communication_observe_shortwave_noise

- query：短波噪声太大
- 类型：observation
- focus：观察 survival 是否泛化。
- watch conflicts：survival
- verdict：partial
- expected Guide：DG-0855
- allowed secondary：DG-0860
- selected Guide：DG-0640、DG-0347、DG-0348
- selected Wiki：communication-contact-window-001、communication-offline-communication-001、communication-sms-001
- profiles：无
- domains：comms、tools、hygiene、shelter、security
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：数据缺口、selector 问题、合理 partial
- failure reasons：未命中预期或允许 Guide、未命中预期 Wiki

### communication_observe_team_split_checkin

- query：队伍分散怎么报平安
- 类型：observation
- focus：观察 communication 是否优先，evacuation/navigation 是否只是补充。
- watch conflicts：evacuation、navigation
- verdict：partial
- expected Guide：DG-0856、DG-0860
- allowed secondary：无
- selected Guide：DG-0192、DG-0710、DG-0072
- selected Wiki：safety-route-planning-001
- profiles：无
- domains：comms、evacuation、records、tools、security、external_contact、disaster、medical、shelter
- safety / fallback / record：是 / 是 / 是
- dangerous suggestions：无
- Kiwix 越权：否
- cross domain：无
- root cause：数据缺口、profile 缺口、合理 partial
- failure reasons：未命中预期或允许 Guide、未命中预期 Wiki

## 5. Cross Domain 统计

- computer 冲突：0
- electronics 冲突：0
- gps 冲突：0
- repair 冲突：0
- survival 冲突：0
- energy 冲突：0
- navigation 冲突：0
- evacuation 冲突：0
- data 冲突：0

未发现非通信 Guide 完全抢主位。

## 6. Root Cause 初步分类

- selector 问题：5
- ranking 问题：1
- 数据缺口：3
- 合理 partial：2
- profile 缺口：1

## 7. 是否建议进入 Batch6-D Root Cause Review

建议进入 Batch6-D Root Cause Review：本批只记录 evidence 表现，不直接修复。Review 应判断问题属于数据缺口、profile 缺口、selector 问题、ranking 问题、Guide 设计问题或合理 partial。

## 8. 验证命令

本轮按要求运行：

```text
python3 -m py_compile scripts/test_communication_field.py
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py
python3 scripts/test_communication_field.py --no-answer
```

已确认审计和既有 retrieval 合约测试通过；Field Test 的 partial/fail 仅记录，不在本批修复。

## 9. 边界声明

- 本批没有修改 Wiki 正文、Guide 正文、Guide-Wiki 关系、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、PocketBase schema 或生产数据。
- 本批没有修复 partial/fail，也没有新增 communication query profile。
- 本脚本只调用本地 Guide/Wiki fetchers，不调用 LLM；Kiwix 越权按未进入本地证据选择路径记录为 0。
