# Batch5-D Energy Safety Retrieval Minimal Apply Report

生成时间：2026-07-14

## 1. 修改文件

本批只处理 Batch5-C 确认的 retrieval entry point 缺口，未新增 Wiki，未新增 Guide，未修改 Retrieval Pipeline、Prompt、top_k、selector limit、全局 ranking 或 fallback 架构。

修改/刷新文件：

- `data/retrieval_query_profiles.json`
- `data/guides/power/DG-0842.json`
- `data/emergency_guides.json`
- `data/guide_index.json`
- `wiki_import/power/energy-dc-polarity-reverse-check-001.md`
- `tests/test_energy_safety_retrieval_profiles.py`
- `scripts/test_energy_safety_field.py`
- `docs/knowledge/batch5_b_energy_safety_field_test_results.json`
- `docs/knowledge/batch5_b_energy_safety_field_test_report.md`
- `docs/knowledge/wiki_audit_2026-07-14.md`
- `docs/knowledge/guide_audit_2026-07-14.md`
- `docs/knowledge/batch5_d_energy_safety_apply_report.md`

同步说明：

- `energy-dc-polarity-reverse-check-001` 的 PocketBase `wiki_articles.content` 已同步到本地 SQLite，保持 Markdown 与 PocketBase 一致。
- 未修改 PocketBase schema。

## 2. 新增 Profile

新增 3 个最小 Energy Safety profile。

### `energy_battery_abnormal_isolation`

覆盖：

- 移动电源鼓包/鼓起来还能不能继续充。
- 备用电池存放后发热是否还能测试。
- 电池漏液/腐蚀处理。

优先入口：

- Guide：DG-0841 电池异常隔离
- Wiki：
  - `energy-damaged-power-bank-quarantine-001`
  - `energy-battery-storage-temperature-boundary-001`
  - `energy-battery-leak-corrosion-isolation-001`

调整要点：

- 使用“电池异常、禁用标签、可用区、受摔、摔裂、存放温度、漏液和腐蚀隔离”等 A1 新安全入口锚点。
- 避免只用“电池/发热/漏液”这类泛词，让旧电源保管或旧漏液 Guide 抢主位。

### `energy_low_voltage_fault_stop`

覆盖：

- USB 设备突然发烫是否还能继续用。
- 接线后线越来越热。
- 红黑线接反/反接等部分 polarity 场景。

优先入口：

- Guide：DG-0842 低压设备异常停用
- Wiki：
  - `energy-low-voltage-system-stop-boundary-001`
  - `energy-device-smell-heat-no-restart-001`
  - `energy-short-circuit-warning-signs-001`
  - `energy-wire-heating-load-limit-001`
  - `energy-dc-polarity-reverse-check-001`

调整要点：

- 目标 domain 收窄到 `power`，避免医疗“红肿热痛”因 `tools/security` 等泛域获得 boost。
- 用“低压设备、线缆发热、短路、接线、红黑线、接反、停用、断开”等锚点稳定低压异常入口。

### `energy_unknown_power_adapter_match`

覆盖：

- 旧充电器电压不明是否能试。
- 接口能插进去但不知道是否匹配，是否能通电看看。

优先入口：

- Guide：DG-0843 未知电源安全判断
- Wiki：
  - `energy-unknown-adapter-stop-use-001`
  - `energy-temporary-wiring-precheck-001`
  - `energy-fuse-protection-no-bypass-001`
  - `energy-loose-connector-stop-use-001`

调整要点：

- 用“未知电源、适配器、充电器、参数不明、电压、极性、临时接线、保护件、停用”等锚点压过旧接口潮湿/进水场景。

## 3. Guide-Wiki 关系变化

处理 Batch5-C 标出的 polarity 归属冲突。

新增关系：

- DG-0842 -> `energy-dc-polarity-reverse-check-001`
- `energy-dc-polarity-reverse-check-001` -> DG-0842

保留关系：

- `energy-dc-polarity-reverse-check-001` 继续保留 DG-0843。

理由：

- “红黑线接反”同时属于未知电源/极性不明，也属于低压设备异常停用。
- 本批不删除 DG-0843，不新增 Guide，只补一条双向关系。

## 4. Contract Test 结果

新增 `tests/test_energy_safety_retrieval_profiles.py`，覆盖 8 个入口：

1. 移动电源鼓包 -> DG-0841
2. 备用电池发热 -> DG-0841
3. 电池漏液 -> DG-0841
4. USB 设备发热 -> DG-0842
5. 接线发热 -> DG-0842
6. 红黑线接反 -> DG-0842 或 DG-0843，允许双归属
7. 未知充电器 -> DG-0843
8. 未知接口匹配 -> DG-0843

结果：

```text
tests/test_energy_safety_retrieval_profiles.py
8 passed
```

完整 targeted pytest：

```text
tests/test_retrieval_traceability.py
tests/test_retrieval_root_contract.py
tests/test_energy_safety_retrieval_profiles.py
17 passed
```

## 5. Batch5-B 前后变化

### Apply 前

- pass / partial / fail：2 / 7 / 1
- Guide 命中率：25.0%
- Wiki 命中率：12.5%
- Guide-Wiki 精准组合：25.0%
- 安全边界覆盖：90.0%
- fallback 覆盖：90.0%
- record/check 覆盖：90.0%
- 危险建议：0
- 跨域竞争：3

### Apply 后

- pass / partial / fail：8 / 2 / 0
- Guide 命中率：100.0%
- Wiki 命中率：87.5%
- Guide-Wiki 精准组合：100.0%
- 安全边界覆盖：100.0%
- fallback 覆盖：100.0%
- record/check 覆盖：100.0%
- 危险建议：0
- 跨域竞争：1

回归命令：

```bash
venv/bin/python scripts/test_energy_safety_field.py
```

结果：

- fail = 0
- safety boundary = 100.0%
- fallback = 100.0%
- external dependency violation = 0
- Kiwix overreach = 0

## 6. Field Test Harness 修正

回归中发现测试脚本把 Wiki 正文中的“**不继续充电**看能否恢复”误判为危险建议“继续充”。

处理：

- 在 `scripts/test_energy_safety_field.py` 的否定/安全语境标记中加入 `不继续`。
- 这是 field test 判定修正，不改变 retrieval pipeline、fallback 架构或回答生成逻辑。

验证：

```bash
python3 -m py_compile scripts/test_energy_safety_field.py
```

结果：通过。

## 7. 剩余 Partial

本批不继续扩 profile，剩余 2 个 partial 进入后续 review。

### `energy_unknown_matching_port`

状态：

- Guide 命中 DG-0843。
- 安全边界、fallback、record/check 均满足。
- 未命中预期 Wiki：`energy-unknown-adapter-stop-use-001`、`energy-loose-connector-stop-use-001`。

原因：

- selector / evidence priority 问题。
- DG-0843 的 related Wiki 已包含 `energy-unknown-adapter-stop-use-001`，但排序靠后，前 6 个 selected wiki 被极性、保险丝、临时接线、无人值守充电、充电区和电源线临时绝缘占满。
- `energy-loose-connector-stop-use-001` 来自 DG-0842，作为第三 Guide 的 related wiki 更靠后。

建议：

- 不扩大 selector limit。
- 下一轮只评审 related_wiki ordering 或 Guide-Wiki evidence priority。

### `energy_three_day_blackout_charge_priority`

状态：

- 当前命中 DG-0107、DG-0563、DG-0564。
- 安全边界、fallback、record/check 均满足。
- partial 原因是 `power_vs_communication`。

原因：

- 这是 Batch5-C 已判定的合理 partial / 能源 P1 范围。
- “停电三天优先给什么设备充电”天然涉及通信、照明、记录等关键设备，不属于 P0 电气危险边界。

建议：

- 留到能源 P1：低电量策略、能源预算、关键设备优先级。
- 不在 P0 safety profile 中继续扩展。

## 8. Audit 结果

已运行：

```bash
python3 tools/audit_wiki.py
```

结果：

- Articles：markdown=781 pocketbase=781 categories=24
- Issues：errors=0 warnings=0 advisories=0

已运行：

```bash
python3 tools/build_guides.py
```

结果：

- Generated 773 Guides
- Generated 773 Guide Index Items

已运行：

```bash
python3 scripts/audit_guides.py
```

结果：

- Guides：773
- Issues：errors=0 warnings=0 advisories=0

## 9. 是否达到 Energy Safety v0.1 Stable

达到 Energy Safety Retrieval v0.1 stable 的最低验收线：

- fail = 0
- high/caution safety boundary = 100%
- fallback = 100%
- dangerous suggestion = 0
- Kiwix overreach = 0
- Wiki audit = 0/0/0
- Guide audit = 0/0/0
- targeted pytest = 17 passed

保留说明：

- 仍有 2 个 partial，但均已分类，不应通过继续扩 profile 处理。
- `energy_unknown_matching_port` 是 evidence priority 问题。
- `energy_three_day_blackout_charge_priority` 是能源 P1 范围。

## 10. 未做内容

本批没有：

- 修改 Wiki 正文安全判断。
- 新增 Wiki。
- 新增 Guide。
- 修改 Prompt。
- 修改 Retrieval Pipeline。
- 修改 top_k。
- 修改 selector limit。
- 修改全局 ranking。
- 修改 fallback 架构。
- 实现能源 P1 低电量策略或太阳能扩展。
