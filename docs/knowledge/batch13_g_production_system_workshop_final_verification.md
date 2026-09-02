# Batch13-G Production System / Workshop Retrieval Final Verification

日期：2026-08-24

## 1. 最终结论

Batch13-G 只执行最终验证与冻结记录。未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback、schema、tests、fixture、PocketBase、`.env`、`data/lanternbox.db` 或无关 dirty 文件。

最终验收结果满足 Production 范围的全部目标：

- Production strict：`20 pass / 0 partial / 0 fail`
- Retrieval contracts：`26 passed / 1 xfailed / 0 unexpected fail`
- Wiki audit：`0 errors / 0 warnings / 0 advisories`
- Guide audit：`0 errors / 0 warnings / 0 advisories`
- dangerous suggestion：`0`
- Kiwix 越权：`0`
- safety / fallback / record-check：`100% / 100% / 100%`

据此正式标记：

> **Production System / Workshop Retrieval v0.1 stable**

全量 Field Test 保持 `21 pass / 3 partial / 2 fail`。两条 fail 均为已知相邻域入口问题，且 Production 未占主位，不构成 Production v0.1 阻断项。

## 2. Batch13-B 至 Batch13-G 演进总结

| Batch | 阶段 | 结果 |
|---|---|---|
| Batch13-B | Coverage Planning | 明确 Production 不是继续扩 Manufacturing；规划 41 篇 Wiki、8 个 Guide，以及开工/收工、流转、质检、小批量、安全记录和任务交接六类入口。 |
| Batch13-C | Knowledge Apply | 新增 41 篇 Production Wiki、8 个 Guide（`DG-0903` 至 `DG-0910`）和 57 条双向 evidence 边；Markdown / PocketBase 均为 1107；审计清零。 |
| Batch13-D | Retrieval Field Test | 初始基线为全量 `7/5/14`、strict `6/3/11`；暴露 Production Guide 主位不足、相关 Wiki 被旧入口截断，以及相邻域边界偏移。 |
| Batch13-E | Root Cause Review | 确认知识和双向关系无缺口；根因为 query profile / 主位选择、top1 related Wiki 截断、两份 Guide evidence 顺序和一条 fixture 过窄。明确禁止通过新增知识、扩大 top_k 或让 Production 吞域修复。 |
| Batch13-F | Minimal Apply | 新增 6 个窄范围 Production profile；仅重排 `DG-0905` / `DG-0910` 的既有 related Wiki；修正一条 fixture；strict 提升至 `20/0/0`。 |
| Batch13-G | Final Verification | 重跑 audits、contracts 和 Field Test；结果无回退，边界保持，完成 v0.1 stable 标记与冻结。 |

## 3. 基础审计

### Wiki Audit

执行：

```text
python3 tools/audit_wiki.py
```

结果：

```text
Articles: markdown=1107 pocketbase=1107 categories=24
Issues: errors=0 warnings=0 advisories=0
```

本阶段未同步或修改 PocketBase；审计只确认现有 Markdown 与 PocketBase 数量一致。

### Guide Audit

执行：

```text
python3 scripts/audit_guides.py
```

结果：

```text
Guides: 836
Issues: errors=0 warnings=0 advisories=0
```

Guide-Wiki 关系继续通过真实性与双向对称校验。

## 4. Retrieval Contracts

执行：

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 \
venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_production_system_workshop_retrieval_profiles.py
```

结果：

```text
26 passed, 1 xfailed, 1 warning in 2.29s
```

- 无 unexpected fail。
- 既有 xfailed 为 `Medical / Workshop injury` 已知跨域缺口。
- warning 仅为当前沙箱无法写入 `.pytest_cache`，不影响测试结果。
- 本阶段未新增或修改测试。

## 5. Production Field Test

执行：

```text
python3 scripts/test_production_system_workshop_field.py --no-answer
```

### 最终指标

| 指标 | 结果 | 验收 |
|---|---:|---|
| Strict pass / partial / fail | `20 / 0 / 0` | 通过 |
| Full pass / partial / fail | `21 / 3 / 2` | 允许的冻结状态 |
| Production primary rate | `100%` | 通过 |
| Expected Guide hit | `100%` | 通过 |
| Production Guide hit | `100%` | 通过 |
| Wiki any hit | `100%` | 通过 |
| Wiki full hit | `65%` | 与 Batch13-F 一致 |
| Guide-Wiki precise | `100%` | 通过 |
| Dangerous suggestion | `0` | 通过 |
| Kiwix 越权 | `0` | 通过 |
| Safety | `100%` | 通过 |
| Fallback | `100%` | 通过 |
| Record / check | `100%` | 通过 |

Production strict 20 条全部由预期 Production Guide 主导，并至少有一篇预期 Wiki 进入 evidence。未出现回退、危险建议或来源越权。

## 6. Batch13-F Profiles 状态确认

`data/retrieval_query_profiles.json` 当前共 74 个 profile。Batch13-F 新增的 6 个 profile 均存在且通过 contracts 与 Field Test：

1. `production_workshop_start_close`
2. `production_material_wip_flow`
3. `production_batch_repetition`
4. `production_quality_release`
5. `production_workshop_safety_incident`
6. `production_shift_handover`

本阶段未修改这些 profile。最终结果确认它们在开工/收工、物料与 WIP 流转、小批量重复制作、质检放行、工坊安全事件和班次交接六类场景中保持稳定。

## 7. 边界验证

| Case | 预期主域 | 最终 top1 | Production 主位 | 结论 |
|---|---|---|---|---|
| `observation_make_shelf` | Manufacturing | `DG-0873` 木材切割前固定与支撑 | 否 | Manufacturing 正确主导；Production 未吞域。 |
| `observation_broken_saw` | Repair / Tools | `DG-0837` 危险工具临时隔离 | 否 | Repair / Tools 正确主导；Production 未吞域。 |
| `observation_long_term_finished_storage` | Long-Term Storage | `DG-0895` 储藏区基础分区与标签 | 否 | Storage 正确主导；Production 未吞 Long-Term Storage。 |
| `observation_workshop_injury` | Medical | `DG-0839` 刀锯作业前清场 | 否 | Medical 尚未主导，但 Production 未吞域；保留为 Medical cross-domain gap。 |
| `observation_salvaged_wood` | Waste / Manufacturing | `DG-0830` 草木灰来源审查和禁用 | 否 | 主域仍异常，但 Production 未抢材料可用性判断；保留为 Waste / Manufacturing semantic drift。 |

边界结论：Production 没有吞掉 Manufacturing、Repair / Tools、Long-Term Storage、Medical 或 Waste / Manufacturing。后两项的目标主域仍未命中，但错误主位来自既有相邻域入口，不应通过扩大 Production 处理。

## 8. 已知剩余问题

### observation_salvaged_wood

- 性质：Waste / Manufacturing 旧入口语义漂移。
- 当前：top1 为 `DG-0830`，不是预期 Waste / Manufacturing 主位。
- Production：未占 top1，未触发新增 Production profile。
- 处理：留待后续 Waste / Manufacturing cross-domain review；不在 Production v0.1 修复。

### observation_workshop_injury

- 性质：Medical / Workshop injury cross-domain gap。
- 当前：top1 为 `DG-0839`，不是 Medical。
- Production：未占 top1，未触发新增 Production profile。
- 处理：已受伤、流血场景应由 Medical 主导；留待后续 Medical cross-domain review，不用 Production 修。

## 9. Stable 判定

Production System / Workshop v0.1 的职责是让生产过程可重复、可检查、可交接、可复盘，不替代具体制造、工具维修、长期储藏、材料可用性或医疗处置。

本次最终验证确认：

- 20 条 Production strict case 全部通过。
- Production primary、expected Guide、Wiki any、Guide-Wiki precise 均为 100%。
- 安全停止线、fallback 和 record/check 完整性均为 100%。
- dangerous suggestion 与 Kiwix 越权均为 0。
- Manufacturing、Repair / Tools 与 Long-Term Storage 主位边界正确。
- 两条已知跨域 fail 均未被 Production 吞掉，且按验收要求不作为 Production v0.1 阻断项。

因此判定：**Production System / Workshop Retrieval v0.1 stable**。

## 10. 冻结建议

自 Batch13-G 起冻结 Production System / Workshop Retrieval v0.1：

- 不继续扩 Production profile。
- 不新增 Production Wiki / Guide。
- 不继续调整 Guide-Wiki 顺序。
- 不修改 Retrieval Pipeline、Prompt、top_k、selector limit、ranking 或 fallback 来追逐剩余 observation。
- `salvaged_wood` 仅进入 Waste / Manufacturing cross-domain review。
- `workshop_injury` 仅进入 Medical cross-domain review。
- 后续若无新的真实 Field Test 证据，不重新打开 Production v0.1 检索入口调整。

## 11. Dirty 文件说明

`data/lanternbox.db` 及其他 Batch13-G 开始前已存在的 dirty 文件均未处理、未清理、未提交。本阶段命令仅刷新既有 audit / Field Test 输出，并新增本验证报告；没有修改任何禁止范围内的知识、配置、检索实现、测试或数据库。
