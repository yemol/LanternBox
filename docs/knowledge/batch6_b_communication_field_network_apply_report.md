# Batch6-B Communication & Field Network Expansion Apply Report

生成日期：2026-07-16

## 1. 新增 Wiki 清单

本批新增 Communication Wiki 32 篇，覆盖无线电基础、天线、应急通信、LoRa / Mesh、短波、QRP、低功耗通信和通信日志。

|slug|title|priority|risk_level|Guide|
|---|---|---|---|---|
|`communication-radio-basic-terms-001`|无线电通信基本原理|P0|caution|DG-0858|
|`communication-radio-listen-before-transmit-001`|无线电发射前先监听|P0|high|DG-0858|
|`communication-radio-message-format-short-001`|无线电短消息格式|P0|caution|DG-0858, DG-0856, DG-0860|
|`communication-radio-call-sign-identity-check-001`|呼号和身份确认边界|P0|caution|DG-0858, DG-0860|
|`communication-radio-channel-plan-minimum-001`|最小频道计划|P0|caution|DG-0858, DG-0856|
|`communication-radio-range-line-of-sight-001`|无线电距离和遮挡判断|P0|caution|DG-0858, DG-0856|
|`communication-radio-interference-noise-check-001`|无线电噪声和干扰初查|P0|caution|DG-0855|
|`communication-radio-no-receive-check-001`|无线电收不到信号排查|P0|caution|DG-0855|
|`communication-radio-no-transmit-check-001`|无线电发不出或无人确认排查|P0|high|DG-0855|
|`communication-antenna-connection-check-001`|天线连接检查|P0|high|DG-0855, DG-0858|
|`communication-antenna-placement-height-safety-001`|天线高度影响|P0|high|DG-0858, DG-0856|
|`communication-antenna-wet-weather-stop-001`|雨天潮湿天线停止线|P0|high|DG-0858|
|`communication-antenna-cable-strain-relief-001`|天线方向和线缆拉力影响|P1|caution|DG-0858, DG-0856|
|`communication-lora-node-placement-001`|LoRa 节点部署位置|P0|caution|DG-0857, DG-0856|
|`communication-lora-node-id-map-001`|LoRa 节点编号和位置图|P0|caution|DG-0857, DG-0856|
|`communication-lora-message-size-priority-001`|LoRa 短消息优先级|P0|caution|DG-0857, DG-0856, DG-0860|
|`communication-lora-unknown-node-isolation-001`|未知 LoRa 节点隔离|P0|high|DG-0857|
|`communication-lora-range-test-log-001`|LoRa 距离测试记录|P1|normal|DG-0857|
|`communication-lora-store-forward-delay-001`|LoRa 延迟和转发误判|P1|caution|DG-0857|
|`communication-device-fault-tree-001`|通信设备故障排查顺序|P0|caution|DG-0855|
|`communication-wet-device-stop-use-001`|通信设备进水停用边界|P0|high|DG-0855, DG-0859|
|`communication-battery-radio-runtime-budget-001`|无线电和 LoRa 电量预算|P1|caution|DG-0859|
|`communication-emergency-net-opening-check-001`|应急通信网开网检查|P0|caution|DG-0860|
|`communication-check-in-missed-window-001`|错过通信窗口后的动作|P0|caution|DG-0860, DG-0856|
|`communication-message-priority-levels-001`|紧急信息优先级|P0|caution|DG-0860|
|`communication-shortwave-receive-log-verification-001`|短波通信基础|P1|caution|DG-0860|
|`communication-shortwave-antenna-placement-safe-001`|短波接收天线安全摆放|P1|high|DG-0858|
|`communication-shortwave-noise-source-check-001`|短波噪声来源初查|P1|caution|DG-0855|
|`communication-qrp-power-budget-001`|QRP 概念和电量预算|P1|caution|DG-0859|
|`communication-qrp-contact-window-discipline-001`|QRP 窗口纪律|P1|caution|DG-0859|
|`communication-mesh-node-role-map-001`|Mesh 网络基础节点角色|P1|caution|DG-0857, DG-0856|
|`communication-radio-log-minimum-fields-001`|通信日志最小字段|P0|caution|DG-0860|

另更新 3 篇既有 Wiki 的 `guide_links`，用于与新增 Guide 建立双向关系：

- `communication-device-daily-check-001` -> 新增 DG-0859。
- `communication-lora-node-daily-001` -> 新增 DG-0857。
- `communication-power-saving-001` -> 新增 DG-0859。

## 2. 新增 Guide 清单

本批新增 6 个 Guide。

|Guide|title|priority|risk_level|目标|
|---|---|---|---|---|
|DG-0855|通信设备无法连接排查|P0|high|无信号、无法连接、设备状态、天线、电源排查。|
|DG-0856|野外建立临时通信链路|P0|caution|两点通信、临时节点、中继规划、备用路径。|
|DG-0857|LoRa 节点部署检查|P0|high|节点位置、天线、电源、中继、未知节点隔离。|
|DG-0858|无线电通信前检查|P0|high|电池、天线、频道、监听、身份和消息格式。|
|DG-0859|通信设备低功耗运行|P0|high|节电策略、通信窗口、设备休眠、保底电量。|
|DG-0860|通信信息记录与交接|P0|caution|频率记录、联系记录、团队通信日志和回执交接。|

未新增复杂网络工程、商业通信系统或依赖互联网服务的 Guide。

## 3. Guide-Wiki 关系

本批新增 Guide 只关联直接支撑其行动入口的 Wiki，没有为了补数量强行匹配。

关系校验结果：

```text
forward_edges=1925
reverse_edges=1925
single_forward_without_reverse=0
single_reverse_without_forward=0
invalid_guide_id=0
invalid_wiki_slug=0
```

## 4. PocketBase 同步结果

本批同步了新增 32 篇 Wiki 的 content 与 metadata 到本地 PocketBase `wiki_articles`。

同步结果：

```text
created=32
updated=32
Markdown total=871
PocketBase wiki_articles=871
communication Markdown=64
communication PocketBase=64
```

说明：`updated=32` 是修正 summary / content 后对新增 32 篇再次 upsert 的结果。PocketBase schema 未修改。

## 5. 审计结果

已运行：

```text
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

```text
Wiki audit:
Articles: markdown=871 pocketbase=871 categories=24
Issues: errors=0 warnings=0 advisories=0

build_guides:
Generated 790 Guides
Generated 790 Guide Index Items

Guide audit:
Guides: 790
Issues: errors=0 warnings=0 advisories=0
```

## 6. 未解决 Retrieval 风险

本批没有修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking 或 fallback。

仍需在 Batch6-C Field Test 中观察：

- `computer / data`：节点连不上、网络分区、终端收不到消息可能被服务启动、数据备份或离线页面 Guide 抢主位。
- `electronics / power`：通信设备掉电、进水、发热可能被能源安全完全主导；预期应由通信 Guide 主导，能源作为安全补充。
- `GPS / navigation`：外出队报位置、检查点和返回路线可能只命中 navigation，缺通信回执和窗口动作。
- `repair`：天线线缆松动、接口受力可能被 repair 主导；预期通信安全边界优先。
- `survival / evacuation`：队伍分散、荒野联系、失联等待可能泛化到撤离或生存建议，需要通信行动入口稳定进入 evidence。

本批不提前新增 query profile，也不提前优化通信排序。若 Field Test 显示“正确 Guide 已存在但无法进 selected top3”，再进入 Batch6-D Root Cause Review。

## 7. 下一步 Field Test 建议

建议进入 Batch6-C Communication Field Test。

建议 strict cases：

1. 电台发射前是否要先监听。
2. 无线电没人回，先查什么。
3. 天线松了还能不能继续发射。
4. 雨天接口潮湿还能不能架天线。
5. LoRa 节点放哪里更稳。
6. LoRa 节点两次收不到消息怎么办。
7. 未知 LoRa 节点出现怎么办。
8. 外出队如何建立临时通信链路。
9. 检查点错过通信窗口怎么办。
10. 电量只剩一点，手机 / LoRa / 电台怎么排窗口。
11. 短波听到未经确认的消息能不能立刻改变路线。
12. Mesh 节点角色怎么安排。
13. 通信日志至少写什么。
14. 通信设备进水还能不能开机测试。

建议 observation cases：

1. “网络节点连不上”是否被 computer/data 抢主位。
2. “天线线缆坏了”是否被 repair 抢主位。
3. “低功耗通信”是否被 energy 完全吞掉。
4. “短波噪声太大”是否误入泛 survival。
5. “队伍分散怎么报平安”是否由 communication 主导，evacuation / navigation 只补充。

验收目标：

- fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- cross domain 可解释。
- safety / fallback / record-check = 100%。
- 新增 Guide 的 related_wiki 确定性进入 evidence。
