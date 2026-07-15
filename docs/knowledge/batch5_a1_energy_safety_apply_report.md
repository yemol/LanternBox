# Batch5-A1 Energy Safety Boundary Apply Report

生成时间：2026-07-14

## 1. 范围

本批只处理 `docs/knowledge/batch5_a_energy_foundation_plan.md` 中 P0 能源安全 Wiki，主题限定为电池安全、低压系统安全、充电安全和临时接线安全。

本批未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、schema、PocketBase schema 或无关 Wiki。

## 2. 新增 Wiki 清单

本批新增 18 篇 Wiki。

| slug | title | category | priority | risk_level | Guide |
| --- | --- | --- | --- | --- | --- |
| `energy-low-voltage-system-stop-boundary-001` | 低压直流系统停用边界 | 能源 | P0 | high | DG-0842 |
| `energy-dc-polarity-reverse-check-001` | 直流正负极接反检查 | 能源 | P0 | high | DG-0843 |
| `energy-short-circuit-warning-signs-001` | 短路前兆和立即断开顺序 | 能源 | P0 | high | DG-0842 |
| `energy-wire-heating-load-limit-001` | 线缆发热和负载过大判断 | 能源 | P0 | high | DG-0842 |
| `energy-loose-connector-stop-use-001` | 电源接头松动停用边界 | 能源 | P0 | high | DG-0842 |
| `energy-fuse-protection-no-bypass-001` | 保险丝和保护件不可绕过 | 能源 | P0 | high | DG-0843 |
| `energy-temporary-wiring-precheck-001` | 临时接线前检查 | 能源 | P0 | high | DG-0843 |
| `energy-unattended-charging-stop-boundary-001` | 无人值守充电停止边界 | 能源 | P0 | high | DG-0843 |
| `energy-battery-parallel-series-boundary-001` | 电池串并联禁用边界 | 能源 | P0 | high | DG-0841 |
| `energy-battery-chemistry-mix-stop-001` | 不同电池类型混用停用边界 | 能源 | P0 | high | DG-0841 |
| `energy-battery-leak-corrosion-isolation-001` | 电池漏液和腐蚀隔离 | 能源 | P0 | high | DG-0841 |
| `energy-wet-low-voltage-device-no-test-001` | 潮湿低压设备也不盲目通电 | 能源 | P0 | high | DG-0842 |
| `energy-charging-zone-fire-isolation-001` | 集中充电区防火隔离 | 能源 | P0 | high | DG-0843 |
| `energy-damaged-power-bank-quarantine-001` | 受摔移动电源隔离观察 | 能源 | P0 | high | DG-0841 |
| `energy-device-smell-heat-no-restart-001` | 设备异味发热后不反复重启 | 能源 | P0 | high | DG-0842 |
| `repair-power-cable-temporary-insulation-boundary-001` | 电源线临时绝缘边界 | 维修 / 制作 / 替代 / 拆解再利用 | P0 | high | DG-0843 |
| `energy-unknown-adapter-stop-use-001` | 未知电源适配器停用判断 | 能源 | P0 | high | DG-0843 |
| `energy-battery-storage-temperature-boundary-001` | 电池存放温度边界 | 能源 | P0 | high | DG-0841 |

每篇均包含固定结构：

- `## 用途`
- `## 适用场景`
- `## 准备材料`
- `## 操作步骤`
- `## 判断标准`
- `## 风险提示`
- `## 替代方案`
- `## 记录建议`

所有 high 条目均包含停止条件和不适用边界。

## 3. 新增 Guide 清单

本批新增 3 个 Guide，未为每篇 Wiki 单独创建 Guide。

### DG-0841 电池异常隔离

用途：处理电池、移动电源、电池盒出现鼓包、漏液、腐蚀、摔裂、发热、异味，或电池类型、新旧、容量不一致仍想继续使用的场景。

关联 Wiki：

- `energy-battery-parallel-series-boundary-001`
- `energy-battery-chemistry-mix-stop-001`
- `energy-battery-leak-corrosion-isolation-001`
- `energy-damaged-power-bank-quarantine-001`
- `energy-battery-storage-temperature-boundary-001`

### DG-0842 低压设备异常停用

用途：处理 USB 灯、移动电源、太阳能小板、收音机、对讲机或低压供电线出现发热、异味、火花、掉电、潮湿、接头松动或反复重启的场景。

关联 Wiki：

- `energy-low-voltage-system-stop-boundary-001`
- `energy-short-circuit-warning-signs-001`
- `energy-wire-heating-load-limit-001`
- `energy-loose-connector-stop-use-001`
- `energy-wet-low-voltage-device-no-test-001`
- `energy-device-smell-heat-no-restart-001`

### DG-0843 未知电源安全判断

用途：处理旧适配器、未知充电器、临时线缆、极性不明接口、保护件缺失或集中充电区布置。

关联 Wiki：

- `energy-dc-polarity-reverse-check-001`
- `energy-fuse-protection-no-bypass-001`
- `energy-temporary-wiring-precheck-001`
- `energy-unattended-charging-stop-boundary-001`
- `energy-charging-zone-fire-isolation-001`
- `repair-power-cable-temporary-insulation-boundary-001`
- `energy-unknown-adapter-stop-use-001`

## 4. PocketBase 同步数量

同步方式：本地 PocketBase SQLite upsert，写入 `pocketbase/pb_data/data.db` 的 `wiki_articles` 表，包含 `content` 字段。

结果：

- 新增 `wiki_articles`：18
- 更新 `wiki_articles`：0
- 同步后 `wiki_articles` 总数：781
- 同步后 Markdown Wiki 总数：781

未修改 PocketBase schema。

## 5. Audit 结果

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

## 6. Guide-Wiki 关系变化

新增双向 Guide-Wiki 关系 18 条：

- DG-0841 -> 5 篇电池异常隔离 Wiki
- DG-0842 -> 6 篇低压设备异常停用 Wiki
- DG-0843 -> 7 篇未知电源、临时接线和充电区 Wiki

校验结果：

- 单边关系：0
- 无效 Guide ID：0
- 无效 Wiki slug：0

## 7. 未处理范围

本批未处理：

- P1 低电量策略、能源预算、设备优先级和能源记录扩容。
- P2 背景知识，例如 Wh/mAh、本地估算、直流/交流差异。
- 太阳能排程、阴天降级、角度遮挡、防风固定。
- 现有电池鼓包/发热条目的重复合并。
- 现有疑似医疗发热语境污染的旧能源条目质量修复。
- Retrieval Pipeline、Prompt、query profile、top_k、selector limit。
- Batch5 Field Test。

下一步建议进入 Batch5-A2 或 Batch5-B Field Test，先验证新增 P0 安全边界是否能被正确召回，再决定是否需要 profile 或 Guide-Wiki 微调。
