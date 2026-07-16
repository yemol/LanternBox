# Batch6-A Communication & Field Network Coverage Planning

生成日期：2026-07-16

## 0. 范围

本阶段只做 Communication & Field Network 知识覆盖规划，不修改 Wiki、Guide、Guide-Wiki 关联、Retrieval、Prompt、query profile、top_k、selector limit、fallback、schema、PocketBase 或 tests。

参考：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/knowledge_coverage_map_v0_2.md`

只读检查对象：

- `wiki_import/comms/*.md`
- `data/guides/comms/*.json`
- `data/guides/**/*.json` 中 `domains` 含 `comms` 或通信信息相关 category 的 Guide
- `pocketbase/pb_data/data.db` 中 `wiki_articles` / `wiki_categories`
- `data/retrieval_query_profiles.json`

## 1. 当前覆盖审查

### 1.1 Wiki 覆盖

当前仓库状态：

|项目|数量|
|---|---:|
|Markdown Wiki 总数|839|
|PocketBase `wiki_articles`|839|
|PocketBase `wiki_categories`|24|
|`wiki_import/comms` Wiki|32|
|PocketBase 中 `communication-*` Wiki|32|
|通信 Wiki Markdown/PocketBase 缺失差异|0|

通信 Wiki priority / risk_level：

|维度|分布|
|---|---|
|priority|P0 = 29, P1 = 1, P2 = 2|
|risk_level|caution = 28, normal = 4|

通信 Wiki Guide 支撑：

|项目|数量|
|---|---:|
|有 `guide_links` 的通信 Wiki|32|
|无 `guide_links` 的通信 Wiki|0|
|通信 Wiki -> Guide 边|81|

已覆盖主题主要集中在：

- 断网后的家庭通信计划、集合点、留言点。
- 固定开机窗口和低电量手机通信。
- 纸条留言、近距离哨子 / 灯光 / 敲击信号。
- 分开行动前的信息最小集。
- LoRa 节点每日状态记录。
- 短波收听时段和信息记录。
- 无线电通信基础索引。

判断：现有通信 Wiki 结构健康，且均已进入 Guide-Wiki 链路；但“无线电 / 天线 / LoRa Mesh / 短波 / 故障排查 / 定位联动”的可执行深度不足，很多条目更像断网家庭协同，而不是 Field Network 运行体系。

### 1.2 PocketBase 同步情况

只读检查结果：

|检查项|结果|
|---|---|
|`communication-*` Markdown slug 数|32|
|PocketBase `communication-*` slug 数|32|
|Markdown 有、PocketBase 缺失|0|
|PocketBase 有、Markdown 缺失|0|
|PocketBase status|均为 `published`|

结论：通信相关 Wiki 当前已同步到 PocketBase。Batch6-A 不做 PocketBase 写入。

### 1.3 Guide 覆盖

`data/guides/comms` 当前有 17 个 Guide，其中 12 个带 `related_wiki`，5 个不带 `related_wiki`。需要注意：该目录里有少量非通信主题或早期归档项，例如鞋底检查、基础技能清单、动物异常观察等，不能把目录数量直接等同于通信能力。

通信相关 Guide 更广义统计：

|项目|数量|
|---|---:|
|`domains` 含 `comms` 或通信信息相关 category 的 Guide|106|
|其中带 `related_wiki`|28|
|其中无 `related_wiki`|78|
|相关 Guide -> Wiki 引用总数|100|

代表性现有 Guide：

|Guide|能力|判断|
|---|---|---|
|DG-0549 断网断信号：家庭通讯计划和集合约定|家庭通信计划、集合点、留言|可用，但偏家庭/断网场景。|
|DG-0582 手机低电量：固定时间开机联系|低电量手机通信窗口|可用，但没有扩展到无线电/LoRa 能源预算。|
|DG-0583 近距离信号：哨子灯光敲击约定|近距离低技术信号|可用。|
|DG-0584 纸条留言：位置格式和防水|纸条留言|可用。|
|DG-0585 分开行动前：时间方向目的地记录|外出通信与路线记录|可用，但缺无线/定位联动。|
|DG-0636 断网家庭固定开机窗口|固定开机窗口|可用，但 `related_wiki` 很多，容易成为总入口。|
|DG-0639 LoRa 节点每日状态记录|LoRa 节点维护|可用，但只覆盖每日记录，不覆盖部署、范围测试和故障排查。|
|DG-0640 短波收听时段和信息记录|短波接收与记录|可用，但不覆盖天线、噪声、收听验证链和低功耗安排。|

### 1.4 Guide-Wiki 关系情况

只读关系检查：

|关系|数量|结论|
|---|---:|---|
|通信 Wiki -> Guide `guide_links`|81|均可映射到现有 Guide。|
|通信相关 Guide -> `communication-*` Wiki|81|均可映射到现有 Wiki。|
|Wiki 引用 Guide、Guide 未反向引用 Wiki|0|无单边关系。|
|Guide 引用 Wiki、Wiki 未反向引用 Guide|0|无单边关系。|

结论：通信目录的双向关系健康。当前问题不是 schema 或同步缺口，而是能力覆盖缺口。

## 2. 缺口分析

### 2.1 P0 缺口

|缺口|当前状态|为什么是 P0|
|---|---|---|
|无线电基础|只有 `communication-radio-communication-basics-index-001`，priority P2，且偏索引。|断网后无线电是 Field Network 的核心入口；必须有可执行的“频道、监听、短消息、确认、停止线”。|
|天线基础|LoRa 节点 Wiki 提到天线检查，但没有独立天线条目。|天线松动、错误位置、潮湿和拉扯会直接造成失联或设备损坏。|
|通信故障排查|有手机无信号、通信失败不盲找；缺 radio/LoRa/电源/天线故障树。|失联后如果没有排查顺序，容易耗尽电量、移动节点、扩大风险。|
|应急通信流程|有家庭通讯计划、固定窗口；缺 field net 开网、收报、回执、优先级和交接。|多点位、小队外出、低功耗设备需要统一流程。|
|LoRa 基础|有 LoRa 信息适用性和每日状态；缺节点编号地图、布点、距离测试、未知节点隔离。|LoRa 是低功耗断网通信候选，但没有最小网络运行链。|

### 2.2 P1 缺口

|缺口|当前状态|为什么是 P1|
|---|---|---|
|短波通信|有短波收听时段和记录，但缺噪声排查、天线摆放、安全和信息验证。|短波主要用于外部信息获取；错误记录会引发路线/配给误判。|
|QRP|当前基本缺失。|低功耗通信与能源预算相关，但不应优先于 P0 的安全流程。|
|Mesh 网络|当前基本缺失，LoRa 也未形成 mesh / store-forward 运行条目。|适合中期扩展，但第一步应先建立节点角色、分区和降级。|
|低功耗通信|已有手机低电量、固定开机窗口；缺无线电/LoRa/短波统一电量预算。|能源和通信需要联合排班，否则通信抢占照明/医疗设备电量。|

### 2.3 P2 缺口

|缺口|当前状态|为什么是 P2|
|---|---|---|
|通信管理|已有纪律和同步节奏，但没有 field net 管理角色和设备借还。|需要，但不能先于基础通信链。|
|团队通信纪律|已有断供通信纪律；缺无线电简语、禁发内容、回读确认和误报复盘。|可作为 Guide 的 check/record 支撑。|
|通信日志|已有一些记录建议，但缺统一字段：时间、节点、频道、消息级别、确认、失败原因。|适合作为 P2 Wiki 支撑多个 Guide。|

## 3. 新增 Wiki 规划清单

建议规划 39 篇 Wiki。第一批 Apply 不必全做，优先从 P0/P1 中选择 30-32 篇。

|#|slug|title|category|priority|risk_level|summary|intended Guide relation|
|---:|---|---|---|---|---|---|---|
|1|communication-radio-basic-terms-001|无线电基础术语最小集|通讯|P0|caution|解释频道、频率、监听、发射、回执、呼号、噪声等最小行动术语，避免把术语误解成可稳定通信保证。|DG-0855|
|2|communication-radio-listen-before-transmit-001|无线电发射前先监听|通讯|P0|high|发射前先监听占用、危险信息和对方状态；不在不清楚频道用途时连续发射。|DG-0855|
|3|communication-radio-message-format-short-001|无线电短消息格式|通讯|P0|caution|把无线电消息压缩为对象、位置、状态、需求、确认和下次窗口，减少误解和耗电。|DG-0855, DG-0862|
|4|communication-radio-call-sign-identity-check-001|呼号和身份确认边界|通讯|P0|caution|用简单呼号或编号确认发送者和接收者，不在公开信道暴露成员全名、库存和住所。|DG-0855, DG-0862|
|5|communication-radio-channel-plan-minimum-001|最小频道计划|通讯|P0|caution|规划主频道、备用频道、监听窗口和禁用场景，避免所有人随意换频道。|DG-0855|
|6|communication-radio-range-line-of-sight-001|无线电距离和遮挡判断|通讯|P0|caution|用地形、建筑、树林、低洼和天气判断通信距离，不把标称距离当作承诺。|DG-0855, DG-0861|
|7|communication-radio-interference-noise-check-001|无线电噪声和干扰初查|通讯|P0|caution|区分电源噪声、位置遮挡、频道拥挤和设备故障，先记录再移动。|DG-0857|
|8|communication-radio-no-receive-check-001|无线电收不到信号排查|通讯|P0|caution|按电量、音量、静噪、频道、天线、位置、时间窗口顺序排查收不到信号。|DG-0857|
|9|communication-radio-no-transmit-check-001|无线电发不出或无人确认排查|通讯|P0|high|发送失败时先确认电量、频道、天线、按键、监听窗口和对方状态，不连续呼叫耗电。|DG-0857|
|10|communication-antenna-connection-check-001|天线连接和禁发边界|通讯|P0|high|检查天线是否接牢、接口是否松动或进水；天线异常时不继续发射。|DG-0856|
|11|communication-antenna-placement-height-safety-001|天线位置和高度安全|通讯|P0|high|选择高处、干燥、稳定、低暴露位置；避开电线、火源、通道和儿童可拉扯区域。|DG-0856|
|12|communication-antenna-wet-weather-stop-001|雨天潮湿天线停止线|通讯|P0|high|雨水、进水、雷声、接口潮湿或线缆破皮时停止架设或发射，改用备用路径。|DG-0856|
|13|communication-antenna-cable-strain-relief-001|天线线缆拉力缓解|通讯|P1|caution|给线缆留余量和固定点，避免风、门窗、行人和背包拉坏接口。|DG-0856|
|14|communication-lora-node-placement-001|LoRa 节点放置位置选择|通讯|P0|caution|选择高处、干燥、可复查、低暴露、远离金属遮挡的位置，并记录备用点。|DG-0858|
|15|communication-lora-node-id-map-001|LoRa 节点编号和位置图|通讯|P0|caution|把节点编号、位置、负责人、供电和覆盖方向画到纸面，避免失联后找不到节点。|DG-0858, DG-0861|
|16|communication-lora-message-size-priority-001|LoRa 短消息优先级|通讯|P0|caution|限制 LoRa 消息为报平安、位置、需求、危险和回执，不发送长文本和敏感库存。|DG-0858, DG-0862|
|17|communication-lora-unknown-node-isolation-001|未知 LoRa 节点隔离|通讯|P0|high|发现未知节点、身份不清或密钥不清时先隔离，不让它进入关键通信链路。|DG-0858|
|18|communication-lora-range-test-log-001|LoRa 距离测试记录|通讯|P1|normal|记录测试点、时间、天气、成功/失败、延迟和电量，建立真实覆盖图。|DG-0858|
|19|communication-lora-store-forward-delay-001|LoRa 延迟和转发误判|通讯|P1|caution|说明低速、延迟、转发和重复消息风险，不把迟到消息当作最新状态。|DG-0858|
|20|communication-device-fault-tree-001|通信设备故障排查顺序|通讯|P0|caution|按电源、接口、天线、设置、位置、窗口、对端状态排查，不同时改多个变量。|DG-0857|
|21|communication-wet-device-stop-use-001|通信设备进水停用边界|通讯|P0|high|设备进水、发热、异味、接口腐蚀时停用隔离，不为了通信继续开机测试。|DG-0857, DG-0860|
|22|communication-battery-radio-runtime-budget-001|无线电和 LoRa 电量预算|通讯|P1|caution|估算通信设备运行时间，分配监听、发射、收听和备用电量。|DG-0860|
|23|communication-emergency-net-opening-check-001|应急通信网开网检查|通讯|P0|caution|开始通信窗口前确认负责人、频道、时间、优先级、回执方式和结束条件。|DG-0862|
|24|communication-check-in-missed-window-001|错过通信窗口后的动作|通讯|P0|caution|错过窗口时先记录、等待下个窗口或转备用路径，不立刻盲目外出寻找。|DG-0862, DG-0861|
|25|communication-message-priority-levels-001|通信消息优先级分级|通讯|P0|caution|把消息分为生命危险、位置状态、资源需求、普通更新和待核信息，决定是否占用窗口。|DG-0862|
|26|communication-relay-runner-handover-001|人工传递消息交接|通讯|P1|caution|无线不可用时用人工传递，要求消息封装、接收确认、路线和返回时间记录。|DG-0862, DG-0861|
|27|communication-shortwave-receive-log-verification-001|短波收听信息核验表|通讯|P1|caution|记录频率、时间、来源、内容、是否复听和是否可行动，不把噪声猜测当事实。|DG-0859|
|28|communication-shortwave-antenna-placement-safe-001|短波接收天线安全摆放|通讯|P1|high|短波接收天线只在安全、干燥、稳定位置使用，避免电线、绊倒、拉扯和潮湿风险。|DG-0859, DG-0856|
|29|communication-shortwave-noise-source-check-001|短波噪声来源初查|通讯|P1|caution|用位置、时间、电源、设备距离和天线方向判断噪声，不凭猜测补全内容。|DG-0859|
|30|communication-qrp-power-budget-001|QRP 低功率通信电量预算|通讯|P1|caution|规划低功率通信的监听、短发射、休眠和备用电量，不抢占照明和医疗电量。|DG-0860|
|31|communication-qrp-contact-window-discipline-001|QRP 窗口纪律|通讯|P1|caution|低功率通信只在约定窗口和短消息格式下尝试，失败后记录而不是连续呼叫。|DG-0860, DG-0862|
|32|communication-mesh-node-role-map-001|Mesh 节点角色图|通讯|P1|caution|区分固定节点、移动节点、备用节点和记录点，避免所有节点同时移动。|DG-0858|
|33|communication-mesh-network-partition-fallback-001|Mesh 分区失联降级|通讯|P1|caution|Mesh 分区后按固定窗口、纸条点、人工传递和集合点降级，不临时重排所有节点。|DG-0858, DG-0862|
|34|communication-position-report-grid-landmark-001|位置报告的地标和网格格式|地图地形与环境监测|P0|caution|把位置报告写成地标、方向、距离、时间、返回路线，避免只说“我在这附近”。|DG-0861|
|35|communication-gps-coordinate-paper-backup-001|GPS 坐标纸质备份|地图地形与环境监测|P1|normal|把关键坐标、地标和备用路线写到纸面，不让定位只存在手机里。|DG-0861|
|36|communication-route-checkpoint-report-001|路线检查点通信报告|地图地形与环境监测|P0|caution|外出队按检查点报告位置、状态、风险和下一窗口，留守队按超时规则处理。|DG-0861|
|37|communication-radio-log-minimum-fields-001|通信日志最小字段|通讯|P2|normal|统一记录时间、设备、频道、节点、消息级别、是否确认、失败原因和下次动作。|DG-0862|
|38|communication-team-net-control-role-001|小团队通信负责人角色|通讯|P2|caution|规定谁开窗、谁记录、谁确认、谁保管备用设备，减少多人同时发话。|DG-0862|
|39|communication-equipment-inventory-checkout-001|通信设备借还清单|通讯|P2|normal|记录设备编号、电量、天线、线缆、借用人、归还时间和异常。|DG-0860, DG-0862|

## 4. Guide 候选

最多规划 8 个 Guide。Guide 必须是行动入口，不是百科入口。

### DG-0855 无线电基础联络流程

- scenario：小团队有对讲机、手持无线电或简易电台，需要在断网或外出时建立最小联络流程。
- steps：
  1. 先确认设备电量、频道计划、监听窗口和负责人。
  2. 发射前监听，确认频道未被占用或未出现危险信息。
  3. 使用短消息格式：对象、位置、状态、需求、下次窗口。
  4. 要求接收方回读关键字段。
  5. 未收到确认时记录失败，不连续呼叫耗电。
- check：
  - 主频道、备用频道、窗口和回执方式已写下。
  - 每条消息短、可复述、有时间。
  - 发送前完成监听。
- stop_or_escalate：
  - 频道用途不明、设备异常发热、天线异常、身份不清或连续无人确认时停止普通发射。
- fallback：
  - 改用固定开机窗口、纸条留言、近距离信号或人工传递。
- related_wiki：
  - `communication-radio-basic-terms-001`
  - `communication-radio-listen-before-transmit-001`
  - `communication-radio-message-format-short-001`
  - `communication-radio-call-sign-identity-check-001`
  - `communication-radio-channel-plan-minimum-001`
  - `communication-radio-range-line-of-sight-001`

### DG-0856 天线连接与架设安全检查

- scenario：准备使用无线电、LoRa 或短波接收设备前，需要检查天线、线缆、接口、位置和天气风险。
- steps：
  1. 检查天线接口是否接牢、干燥、无裂纹和无明显腐蚀。
  2. 选择高处、稳定、干燥、可复查的位置。
  3. 避开电线、火源、通道、睡眠区和儿童可拉扯范围。
  4. 给线缆留余量并做拉力缓解。
  5. 雨天、潮湿、雷声或接口异常时停止架设或发射。
- check：
  - 天线连接牢固。
  - 线缆不绊人、不受拉、不进水。
  - 天气和周边风险已确认。
- stop_or_escalate：
  - 接口进水、线缆破皮、天线松脱、雷声、靠近电线或人员无法隔离时停止使用。
- fallback：
  - 改用低功率短时窗口、移动到安全位置、纸条点或近距离信号。
- related_wiki：
  - `communication-antenna-connection-check-001`
  - `communication-antenna-placement-height-safety-001`
  - `communication-antenna-wet-weather-stop-001`
  - `communication-antenna-cable-strain-relief-001`
  - `communication-shortwave-antenna-placement-safe-001`

### DG-0857 通信故障排查顺序

- scenario：无线电、LoRa 或手机通信失败，需要判断是电源、天线、设置、位置、时间窗口还是对端问题。
- steps：
  1. 不同时改多个变量，先记录失败时间、地点、设备和电量。
  2. 按电源、接口、天线、音量/静噪、频道、位置、时间窗口、对端状态排查。
  3. 先做低成本动作：换位置、查接口、等窗口、发短测试。
  4. 连续失败时标记设备或节点降级，不作为关键联络路径。
  5. 写下下一次复查时间。
- check：
  - 失败原因有记录。
  - 排查顺序可复现。
  - 关键联络有备用路径。
- stop_or_escalate：
  - 设备进水、发热、异味、接口损坏、天线异常或电池异常时停用隔离。
- fallback：
  - 转用备用设备、纸条点、集合点、人工传递或下一固定窗口。
- related_wiki：
  - `communication-device-fault-tree-001`
  - `communication-radio-interference-noise-check-001`
  - `communication-radio-no-receive-check-001`
  - `communication-radio-no-transmit-check-001`
  - `communication-wet-device-stop-use-001`

### DG-0858 LoRa / Mesh 最小节点运行

- scenario：小团队使用 LoRa 或简易 Mesh 节点维持低功耗断网通信，需要布点、编号、测试、记录和降级。
- steps：
  1. 给每个节点编号，记录负责人、位置、供电和角色。
  2. 选择高处、干燥、可复查、低暴露位置。
  3. 只发送短消息：报平安、位置、需求、危险和回执。
  4. 做距离测试并记录成功/失败、延迟和电量。
  5. 未知节点或身份不清节点先隔离。
  6. 分区失联时按固定窗口、纸条点或人工传递降级。
- check：
  - 节点图可读。
  - 每个节点有编号和负责人。
  - 最近一次测试结果可查。
- stop_or_escalate：
  - 节点进水、发热、身份不清、连续失败或位置暴露时停止作为关键节点。
- fallback：
  - 减少开机频率，保留主节点；转纸条点、集合点或人工传递。
- related_wiki：
  - `communication-lora-node-placement-001`
  - `communication-lora-node-id-map-001`
  - `communication-lora-message-size-priority-001`
  - `communication-lora-unknown-node-isolation-001`
  - `communication-lora-range-test-log-001`
  - `communication-lora-store-forward-delay-001`
  - `communication-mesh-node-role-map-001`
  - `communication-mesh-network-partition-fallback-001`

### DG-0859 短波收听与信息核验

- scenario：团队使用短波收音或接收设备获取外部信息，需要控制耗电、噪声、误听和未经核验信息扩散。
- steps：
  1. 设定固定收听时段和收听人。
  2. 记录时间、频率、位置、电量和内容摘要。
  3. 区分天气、交通、供水、医疗、安全等可行动信息。
  4. 同一消息等待复听或另一来源印证。
  5. 收听后关闭设备并标记待核信息。
- check：
  - 每条信息有频率、时间和来源状态。
  - 待核和已确认分开。
  - 电量消耗已记录。
- stop_or_escalate：
  - 噪声太大、无法辨认、设备进水发热或消息要求高风险转移但无法核验时停止直接行动。
- fallback：
  - 缩短到最高价值时段，换安全位置，或等待下一窗口复听。
- related_wiki：
  - `communication-shortwave-receive-log-verification-001`
  - `communication-shortwave-antenna-placement-safe-001`
  - `communication-shortwave-noise-source-check-001`
  - `communication-shortwave-001`

### DG-0860 低功耗通信预算

- scenario：通信设备、电池、太阳能或充电能力有限，需要在报平安、监听、外出联络和关键信息之间分配电量。
- steps：
  1. 列出所有通信设备、电量、充电方式和关键用途。
  2. 区分监听、发射、短波收听、LoRa 节点和手机窗口的耗电。
  3. 设定每日通信窗口和最低保底电量。
  4. 电量低时优先短消息、位置、危险和回执。
  5. 记录每次窗口前后电量。
- check：
  - 通信电量预算不抢占医疗、照明和安全设备底线。
  - 每台设备有负责人和保底电量。
  - 失败窗口有记录。
- stop_or_escalate：
  - 电池发热、鼓包、进水、快速掉电或设备异常时停止使用并隔离。
- fallback：
  - 降为每日单窗口、纸条点、固定集合点或人工传递。
- related_wiki：
  - `communication-battery-radio-runtime-budget-001`
  - `communication-qrp-power-budget-001`
  - `communication-qrp-contact-window-discipline-001`
  - `communication-wet-device-stop-use-001`
  - `communication-equipment-inventory-checkout-001`
  - `communication-power-saving-001`

### DG-0861 定位报告与返回路线通信

- scenario：成员外出、分组、巡查或转移时，需要把位置、路线、检查点和返回条件通过通信链交接。
- steps：
  1. 出发前写明人员、目的地、主路线、返回路线和检查点。
  2. 位置报告使用地标、方向、距离、时间和下一窗口。
  3. GPS 坐标必须配纸质地标备份。
  4. 到检查点发送状态、风险和下一动作。
  5. 错过窗口时按等待、备用点、人工核查规则处理。
- check：
  - 留守者能复述路线和最晚返回时间。
  - 位置报告不用模糊词。
  - 纸质地图上有检查点。
- stop_or_escalate：
  - 无法复述返回路线、天气恶化、通信失败且路线不可回撤时取消或终止外出任务。
- fallback：
  - 无 GPS 时用地标顺序和步行时间；无通信时用纸条点和固定返回窗口。
- related_wiki：
  - `communication-position-report-grid-landmark-001`
  - `communication-gps-coordinate-paper-backup-001`
  - `communication-route-checkpoint-report-001`
  - `communication-check-in-missed-window-001`
  - `navigation-return-route-marking-001`
  - `navigation-landmark-selection-001`

### DG-0862 小团队应急通信网纪律

- scenario：小团队进入断网、外出、巡查或多点值守状态，需要统一开网、消息优先级、记录和结束规则。
- steps：
  1. 指定通信负责人、记录人和备用负责人。
  2. 开网前确认频道、窗口、设备、电量、消息优先级和回执规则。
  3. 先处理生命危险、位置状态、资源需求，再处理普通更新。
  4. 每条消息记录发送者、接收者、时间、是否确认和下次动作。
  5. 结束窗口后归档日志、检查设备并公布下一窗口。
- check：
  - 有通信负责人。
  - 消息优先级清楚。
  - 日志字段完整。
  - 未确认消息有后续动作。
- stop_or_escalate：
  - 生命危险、成员失联、身份不清、频道被陌生人干扰或敏感信息外泄时停止普通流程，进入应急处理。
- fallback：
  - 使用固定开机窗口、纸条点、近距离信号、人工传递和集合点。
- related_wiki：
  - `communication-emergency-net-opening-check-001`
  - `communication-message-priority-levels-001`
  - `communication-check-in-missed-window-001`
  - `communication-radio-log-minimum-fields-001`
  - `communication-team-net-control-role-001`
  - `communication-relay-runner-handover-001`

## 5. Retrieval 风险预测

本阶段只预测，不修改 Retrieval。

### 5.1 computer / data 混淆

风险 query：

- “节点连不上怎么办？”
- “网络分区了怎么恢复？”
- “终端收不到消息？”

可能误入：

- `computer` / `data` / `security` 方向的服务启动、备份、资料包、离线页面测试。

判断：

- 如果语义包含 LoRa、节点、电台、频道、天线、无线、报平安、外出联络，应由 communication 主导。
- 如果语义包含服务无法打开、Web、资料包、备份、数据目录，应由 computer/data/security 主导。

建议 profile：

- `communication_lora_mesh_node`

### 5.2 electronics / power 混淆

风险 query：

- “电台一发射就掉电怎么办？”
- “LoRa 节点电池不够怎么排班？”
- “通信设备进水还能不能开机？”

可能误入：

- 能源电池安全、低压设备、充电队列。

判断：

- 设备发热、鼓包、进水、电池异常时 power safety 应作为强安全补充。
- 但通信窗口、监听/发射预算、消息降级应由 communication 主导。

建议 profile：

- `communication_low_power_window`
- `communication_device_fault_safety`

### 5.3 GPS / navigation 混淆

风险 query：

- “外出队怎么报位置？”
- “GPS 坐标怎么和纸地图对应？”
- “检查点没报平安怎么办？”

可能误入：

- maps/navigation 独立路线知识。

判断：

- 纯地图读图、路线标注、危险区应由 navigation 主导。
- 带“报位置、回执、通信窗口、失联、检查点”的应由 communication + navigation 联合，communication 作为 action entry。

建议 profile：

- `communication_position_report_nav`

### 5.4 repair 混淆

风险 query：

- “天线线松了还能发吗？”
- “线缆被门夹了怎么固定？”
- “接口湿了还能不能试？”

可能误入：

- repair 线缆、固定点、胶带、材料替代。

判断：

- 涉及接口潮湿、天线异常、发射前安全，应由 communication/energy safety 主导，repair 只补充临时固定和降级。
- 不建议让 repair 给出继续使用建议。

建议 profile：

- `communication_antenna_fault_safety`

### 5.5 survival 泛化混淆

风险 query：

- “荒野里怎么联系？”
- “队伍分散怎么报平安？”
- “没有网络怎么组通信网？”

可能误入：

- general survival、evacuation、external_contact、team。

判断：

- 如果核心是通信链、消息、窗口、节点、设备，应由 communication 主导。
- 如果核心是撤离、路线选择、外部接触风险，则对应域补充。

建议 profile：

- `communication_field_network_ops`

### 5.6 是否需要新增 communication query profile

建议在 Apply 后、Field Test 前先不修改 Retrieval。原因：

- 当前缺口主要是知识和 Guide action entry 不足。
- 需要先新增 Wiki/Guide，再用 Field Test 判断正确 Guide 是否进 selected top3。
- 如果出现“正确 Guide 存在但被 computer/electronics/GPS/repair/survival 稳定压制”，再进入 Root Cause Review 规划 profile。

候选 profile 只列为后续评估：

1. `communication_field_network_ops`
2. `communication_lora_mesh_node`
3. `communication_antenna_fault_safety`
4. `communication_low_power_window`
5. `communication_position_report_nav`
6. `communication_shortwave_receive_verify`

## 6. 不建议加入内容

本批暂不加入：

- 复杂通信工程教程。
- 商业网络部署。
- 依赖互联网服务、云服务、运营商后台或商业平台的方案。
- 复杂基站、转发台、卫星链路或专业通信车建设。
- 具体品牌采购推荐。
- 高风险改装、功放制作、违规频段或绕过监管的操作。
- 复杂天线设计公式和长篇电磁学教程。
- 加密算法实现教程。
- 大规模组织通信指挥系统。
- 城市级 Mesh 网络部署。
- 无人机中继、气球中继等高风险或高维护方案。
- 把短波未核验信息直接作为撤离或配给依据的行动指南。
- 让通信建议覆盖安全、医疗、火源、转移等高风险停止线。

## 7. Apply 建议

### 7.1 第一批新增 Wiki

建议 Batch6-B Apply 第一批新增 32 篇 Wiki：

- P0：22-24 篇。
- P1：8-10 篇。
- P2：0-2 篇。

优先纳入：

- 无线电基础：1-9。
- 天线基础：10-13。
- LoRa 基础：14-19。
- 通信故障排查：20-22。
- 应急通信流程：23-26。
- 定位与通信结合：34、36。
- 低功耗通信：22、30、31 可视容量纳入。

可暂缓：

- Mesh 深度条目 32-33。
- 通信日志/负责人/设备借还 37-39。
- GPS 坐标纸质备份 35 可与导航批次联合处理。

### 7.2 Guide 是否同步

建议同步新增 6 个 Guide，不超过 8 个：

第一批优先：

1. DG-0855 无线电基础联络流程。
2. DG-0856 天线连接与架设安全检查。
3. DG-0857 通信故障排查顺序。
4. DG-0858 LoRa / Mesh 最小节点运行。
5. DG-0860 低功耗通信预算。
6. DG-0861 定位报告与返回路线通信。

可第二批：

- DG-0859 短波收听与信息核验。
- DG-0862 小团队应急通信网纪律。

理由：

- 当前 DG-0636 / DG-0637 / DG-0638 太容易成为泛通信入口。
- 新增 Wiki 如果没有 Guide 承接，会依赖自然召回，不稳定。
- Field Network 需要从“设备能不能用”直接进入行动链：Guide -> Wiki -> Evidence -> Action -> Safety boundary -> Fallback -> Record/check。

### 7.3 Field Test 设计

建议 Batch6-C Field Test 设计 18-22 个 cases。

Strict cases：

1. 电台发射前是否要先监听。
2. 无线电没人回，先查什么。
3. 天线松了还能不能继续发射。
4. 雨天接口潮湿还能不能架天线。
5. LoRa 节点放哪里更稳。
6. LoRa 节点两次收不到消息怎么办。
7. 未知 LoRa 节点出现怎么办。
8. 外出队如何报位置。
9. 检查点错过通信窗口怎么办。
10. 电量只剩一点，手机/LoRa/电台怎么排窗口。
11. 短波听到未经确认的消息能不能立刻改变路线。
12. Mesh 节点分区失联怎么降级。
13. 人工传递消息怎么避免误传。
14. 通信日志至少写什么。

Observation cases：

1. “网络节点连不上”是否被 computer/data 抢主位。
2. “天线线缆坏了”是否被 repair 抢主位。
3. “GPS 坐标怎么发回去”是否由 communication + navigation 组合。
4. “低功耗通信”是否被 energy 完全吞掉。
5. “短波噪声太大”是否误入 survival 泛知识。

验收目标：

- fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- cross domain 可解释，且不让 computer / electronics / GPS / repair / survival 覆盖通信主入口。
- safety / fallback / record-check = 100%。
- 新增 Guide 的 related_wiki 必须确定性进入 evidence。

### 7.4 不建议在 Apply 同步做的事

- 不新增 query profile。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不改 top_k。
- 不改 selector limit。
- 不改 fallback。
- 不压低 computer / power / repair / maps 旧 Guide。
- 不为 Field Test 预先硬编码答案。

建议流程：

1. Batch6-B：Apply 第一批 Wiki + Guide + 双向关系。
2. Batch6-C：Field Test。
3. Batch6-D：Root Cause Review。
4. 只有当正确 Guide 已存在但被稳定压制时，再考虑最小 communication query profile。
