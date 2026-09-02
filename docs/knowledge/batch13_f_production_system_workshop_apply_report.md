# Batch13-F Production System / Workshop Retrieval Minimal Apply Report

日期：2026-08-24

## 1. 范围与结论

本批只实施 Batch13-E 建议的最小检索入口修正：新增 6 个 Production query profile、重排 `DG-0905` / `DG-0910` 的既有关联 Wiki、修正 1 条 Field Test fixture，并新增契约测试。未新增 Wiki 或 Guide，未修改 Wiki / Guide 正文，未新增或删除 Guide-Wiki 关系，也未修改 Retrieval Pipeline、Prompt、query profile 以外的检索参数、schema、PocketBase 或数据库。

Production strict cases 最终为 `20 / 20 pass`，strict fail 为 0。Production System / Workshop Retrieval v0.1 达到 **stable candidate（Production 范围）**。全量 26 条 Field Test 仍有 2 条 observation fail，均为任务明确要求留待后续跨域审查的旧入口问题，不是 Production 吞域。

## 2. 新增 Production Profiles

`data/retrieval_query_profiles.json` 从 68 个 profile 增至 74 个，新增且仅新增以下 6 个：

| Profile | 主入口 | 覆盖范围 | 边界保护 |
|---|---|---|---|
| `production_workshop_start_close` | `DG-0903`, `DG-0904` | 开工、收工、工具/材料清点、风险扫描、封存 | 工具损坏仍归 Repair；具体加工仍归 Manufacturing |
| `production_material_wip_flow` | `DG-0905` | 原料入场、WIP、待检区、废料转交、批次标签 | 材料可用性归 Waste/Manufacturing；长期储藏归 Storage |
| `production_batch_repetition` | `DG-0906` | 首件、模板、重复制作、逐件检查、批次一致性 | 具体制作方法仍归 Manufacturing |
| `production_quality_release` | `DG-0907`, `DG-0908` | 待检、尺寸/稳定性/锐边/松动/功能、返工与报废 | 明确“长期/保存”语境条件让位给 Storage；受伤不触发 Production |
| `production_workshop_safety_incident` | `DG-0909` | 低照明、疲劳、near miss、粉尘火花、复工记录 | 已受伤归 Medical；实际火灾/漏电归相邻安全域 |
| `production_shift_handover` | `DG-0910` | 班次、工具状态、下一步、失败原因、WIP 交接 | 储藏交接归 Storage；长期知识归 Records；人员制度归 Organization |

未新增独立 `production_nonconforming_rework_scrap` profile。

## 3. Guide Evidence 顺序调整

只调整 `related_wiki` 顺序，关联集合未变化。重建 `data/emergency_guides.json` 与 `data/guide_index.json` 后，聚合结果仍为 836 Guides / 836 Index Items。

### DG-0905 原料 / 半成品 / 成品流转

1. `production-workshop-raw-material-intake-card-001`
2. `production-workshop-wip-zone-layout-001`
3. `production-workshop-wip-handover-tag-001`
4. `production-workshop-finished-goods-hold-zone-001`
5. `production-workshop-waste-transfer-card-001`
6. `production-workshop-batch-label-minimum-001`
7. `production-workshop-material-count-before-work-001`
8. `production-workshop-material-reservation-line-001`
9. `production-workshop-finished-goods-storage-transfer-001`

### DG-0910 工坊任务交接

1. `production-workshop-shift-handover-card-001`
2. `production-workshop-tool-state-handover-001`
3. `production-workshop-next-action-handover-001`
4. `production-workshop-wip-handover-tag-001`
5. `production-workshop-unfinished-item-tag-001`
6. `production-workshop-task-board-minimum-001`
7. `production-workshop-end-of-day-status-log-001`
8. `production-workshop-fatigue-stop-line-001`
9. `production-workshop-daily-resource-budget-001`

Guide audit 确认所有 `related_wiki` 均为真实 slug，Guide-Wiki 前后向关系完全对称。

## 4. Fixture 修正

只修改 `production_unfinished_item_tag`：

- 修改前：`DG-0904`, `DG-0910`
- 修改后：`DG-0904`, `DG-0905`, `DG-0910`

`DG-0905` 负责半成品流转，且对应 WIP 标签 evidence，属于合理 Production 主位。其他 strict expected 与全部 observation expected 均未修改。

## 5. Contract Tests

新增 `tests/test_production_system_workshop_retrieval_profiles.py`，覆盖 13 条正向查询，以及 Manufacturing、Repair/Tools、Storage、Medical 四类边界。

要求的测试命令结果：

```text
26 passed, 1 xfailed, 1 warning in 2.21s
```

其中 strict xfail 记录已知 `Medical / Workshop injury` 跨域缺口；硬断言已确认受伤查询不会激活 6 个 Production profile，且 Production 不占 top1。pytest warning 仅为沙箱无法写 `.pytest_cache`，不影响测试结果。

额外运行相邻域 profile 套件得到 `44 passed, 6 failed`。6 条失败查询均未激活任何新增 Production profile，因此本批未修改相邻域 profile、Guide 或 pipeline；该补充结果不属于指定验收命令。

## 6. Field Test 前后变化

执行：

```text
python3 scripts/test_production_system_workshop_field.py --no-answer
```

| 指标 | Batch13-D 基线 | Batch13-F 最终 |
|---|---:|---:|
| 总 pass / partial / fail | 7 / 5 / 14 | 21 / 3 / 2 |
| Strict pass / partial / fail | 6 / 3 / 11 | 20 / 0 / 0 |
| Observation pass / partial / fail | 1 / 2 / 3 | 1 / 3 / 2 |
| Production Guide hit | 55% | 100% |
| Expected Guide hit | 45% | 100% |
| Wiki full hit | 20% | 65% |
| Wiki any hit | 35% | 100% |
| Guide-Wiki precise | 30% | 100% |
| Production strict primary | 35% | 100% |

全量 `fail=0` 未达到，原因仅为两条指令明确要求本批不修的 observation。Production strict `fail=0` 已达到。

## 7. Observation 边界结果

| Case | Top1 | 结果 | 说明 |
|---|---|---|---|
| `observation_make_shelf` | `DG-0873` 木材切割前固定与支撑 | partial | Manufacturing 主导；Production 未吞域，仅未补充 evidence |
| `observation_broken_saw` | `DG-0837` 危险工具临时隔离 | partial | Repair / Tools 主导；Production 未吞域 |
| `observation_salvaged_wood` | `DG-0830` 草木灰来源审查和禁用 | fail | Waste / Manufacturing 旧入口语义漂移，按要求留待 cross-domain review |
| `observation_long_term_finished_storage` | `DG-0895` 储藏区基础分区与标签 | partial | Storage 主导；Production 未吞域，仅未补充 evidence |
| `observation_workshop_injury` | `DG-0839` 刀锯作业前清场 | fail | Production 未吞域，但 Medical 未主导；留待 Medical / Workshop injury cross-domain review |
| `observation_low_energy_schedule` | `DG-0910` 工坊任务交接 | pass | 允许的 Energy / Production 主域范围内 |

## 8. 安全与越权

- Dangerous suggestion：`0`
- Kiwix 越权：`0`
- Safety present：`100%`
- Fallback present：`100%`
- Record / check present：`100%`

## 9. Audits

```text
python3 tools/audit_wiki.py
Articles: markdown=1107 pocketbase=1107 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 836
Issues: errors=0 warnings=0 advisories=0
```

本批未执行 PocketBase 同步，也未修改 Wiki；Markdown 与 PocketBase 数量保持 `1107 = 1107`。

## 10. 未处理事项与 Dirty 文件

1. `observation_salvaged_wood`：Waste / Manufacturing 旧入口语义漂移，留待后续 cross-domain review。
2. `observation_workshop_injury`：Medical / Workshop injury 跨域入口缺口，禁止用 Production profile 修补。
3. 补充相邻域测试的 6 条失败：查询均未匹配新增 Production profile，本批不扩大范围处理。
4. `data/lanternbox.db` 在 Batch13-F 开始前已为 dirty；本批未读取写入同步流程、未清理、未提交。
5. `tools/audit_wiki.py`、Batch13-B 至 Batch13-E 产物、Production Wiki/Guide、审计报告及其他既有 dirty 文件均按原状态保留；本批没有回滚或清理无关改动。

本批没有新增 Wiki / Guide，没有修改正文，没有 Field Test 之外的检索系统变更，也没有继续扩展 profile。
