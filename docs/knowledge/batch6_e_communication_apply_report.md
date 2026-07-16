# Batch6-E Communication Retrieval Minimal Apply Report

生成日期：2026-07-16

## 1. 新增 Profile

本批只新增 3 个 communication query profile，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking、fallback 或 schema。

|profile|目标|主域|secondary 边界|
|---|---|---|---|
|`communication_wet_device_safety`|通信设备、LoRa 节点、电台、防水盒、接头进水/受潮/雨淋后，优先进入通信停用隔离入口。|communication|electronics / power 只作为安全补充。|
|`communication_antenna_weather_safety`|天线、馈线、接头、电台在雨天、潮湿、雷雨、风大、户外架设/继续使用场景中，优先进入通信天线安全入口。|communication|repair / electronics 只作为接口与带电风险补充。|
|`communication_device_failure_isolation`|无线电、LoRa、通信节点无信号、无人回应、连接失败、节点离线时，优先进入通信故障排查与隔离入口。|communication|computer / electronics 只作为补充。|

Profile 调整原则：

- 使用现有 profile 机制，不新增 pipeline 逻辑。
- target domain 收紧到 `comms` / `records` / `security` 等通信相关域，避免 power/repair 被同等提升为主入口。
- 通过 `mismatch_unless_context` 保留插线板、电路板、焊接、测电压、服务器、互联网服务等非通信场景边界。

## 2. Guide-Wiki 调整

本批未新增 Guide-Wiki 关系，只调整少量 `related_wiki` 顺序，让已存在的高风险通信 Wiki 更容易进入 selected evidence。

|Guide|调整|
|---|---|
|DG-0855 通信设备无法连接排查|将 `communication-wet-device-stop-use-001` 前移到故障树之后；故障树、进水停用、收不到/发不出、天线连接保持优先。|
|DG-0858 无线电通信前检查|将 `communication-antenna-wet-weather-stop-001` 和 `communication-antenna-connection-check-001` 前移，服务雨天架天线、接口潮湿、天线松动场景。|
|DG-0857 LoRa 节点部署检查|将 `communication-mesh-node-role-map-001`、距离测试和延迟转发前移，服务 Mesh / 节点角色场景。|
|DG-0856 野外建立临时通信链路|将 `communication-check-in-missed-window-001` 和 `communication-lora-node-id-map-001` 前移，服务外出队通信窗口和节点编号。|

说明：用户计划中的 `communication-device-power-check-001`、`communication-weather-antenna-check-001`、`communication-radio-precheck-001`、`communication-antenna-basic-001` 等是方向性名称，本仓库当前不存在这些 slug；本批没有新增 Wiki，而是映射到 Batch6-B 已存在的实际通信 Wiki。

## 3. 未新增 Wiki / Guide

本批没有新增 Wiki，没有新增 Guide，没有修改 Wiki 正文。

原因：

- Batch6-D 已确认湿环境通信安全、通信设备进水停用、天线天气风险 Wiki 都已存在。
- fail 根因是 profile 缺失和 related_wiki evidence priority，不是知识内容空洞。
- 最小 Apply 目标是让已有 Guide/Wiki 稳定进入 evidence。

## 4. 测试结果

已运行：

```text
python3 tools/audit_wiki.py
python3 scripts/audit_guides.py

env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 \
venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_communication_retrieval_profiles.py

python3 scripts/test_communication_field.py --no-answer
```

结果：

```text
Wiki audit:
Articles: markdown=871 pocketbase=871 categories=24
Issues: errors=0 warnings=0 advisories=0

Guide audit:
Guides: 790
Issues: errors=0 warnings=0 advisories=0

pytest:
15 passed

Communication Field Test:
total=19
strict / observation=14 / 5
pass / partial / fail=13 / 6 / 0
Guide hit=85.7%
Expected Guide hit=78.6%
Wiki hit=71.4%
Guide-Wiki precise=85.7%
safety=100%
fallback=100%
record/check=100%
dangerous suggestion=0
Kiwix override=0
cross domain=0
```

新增 contract test：

- `tests/test_communication_retrieval_profiles.py`
- 覆盖雨淋电台开机测试、LoRa 节点盒内进水、雨天架天线、天线接口氧化信号差、无线电无人回应、节点离线但设备未坏。

## 5. Batch6-C -> Batch6-E 变化

|指标|Batch6-C|Batch6-E|变化|
|---|---:|---:|---:|
|pass|8|13|+5|
|partial|9|6|-3|
|fail|2|0|-2|
|Guide 命中率|78.6%|85.7%|+7.1pp|
|主 Guide 命中率|71.4%|78.6%|+7.2pp|
|Wiki 命中率|42.9%|71.4%|+28.5pp|
|Guide-Wiki 精准组合率|78.6%|85.7%|+7.1pp|
|cross domain|2|0|-2|
|dangerous suggestion|0|0|不变|
|Kiwix 越权|0|0|不变|
|safety / fallback / record-check|100% / 100% / 100%|100% / 100% / 100%|不变|

原 fail 修复状态：

- `communication_wet_antenna_rain`：fail -> pass，DG-0858 / DG-0855 进入通信主入口，天线潮湿 Wiki 进入 evidence。
- `communication_wet_device_power_on`：fail -> pass，DG-0855 / DG-0859 进入通信主入口，通信设备进水停用 Wiki 进入 evidence。

## 6. 剩余 Partial 分类

本批不继续扩 profile。剩余 partial 记录如下：

|case|类型|原因|建议|
|---|---|---|---|
|communication_lora_unknown_node|strict partial|DG-0857 正确主导，但 `communication-lora-unknown-node-isolation-001` 被 LoRa placement / Mesh / id map 等前置 Wiki 挤出。|后续如需可微调 DG-0857 related_wiki，但当前无安全风险。|
|communication_missed_checkin_window|strict partial|DG-0859 / DG-0856 主导，`communication-check-in-missed-window-001` 未进 selected Wiki。|后续可评估是否让 DG-0860 或 missed-window Wiki 更前置。|
|communication_low_power_schedule|strict partial|旧手机低电量 Guide 仍主导；DG-0859 未进 selected。|暂不新增 `communication_low_power_multi_device` profile，留到后续批次。|
|communication_shortwave_unverified_route|strict partial|旧 DG-0640 / 外部信息 Guide 主导，DG-0860 未主导。|短波未核信息边界需后续单独复盘。|
|communication_observe_shortwave_noise|observation partial|短波噪声被旧短波/噪声 Guide 主导。|合理 observation，不在本批修。|
|communication_observe_team_split_checkin|observation partial|报平安/撤离/同行请求是合理竞争域。|合理 observation，不在本批修。|

## 7. 是否达到 v0.1 Stable Candidate

结论：达到 **Communication Retrieval v0.1 stable candidate**。

理由：

- strict fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- cross domain = 0。
- safety / fallback / record-check = 100%。
- 两个高风险 wet / antenna fail 均已转为 pass。

保留说明：

- v0.1 stable candidate 不代表通信全部完成。
- 低功耗多设备调度、短波未核信息、队伍分散报平安仍建议作为后续 Batch6-F / Batch6-G 观察项。

## 8. 边界声明

- 未新增 Wiki。
- 未新增 Guide。
- 未修改 Wiki 正文。
- 未修改 Retrieval Pipeline。
- 未修改 Prompt。
- 未修改 top_k。
- 未修改 selector limit。
- 未修改 ranking。
- 未修改 fallback。
- 未修改 schema。
