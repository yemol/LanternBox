# LanternBox Batch9-G：Manufacturing Retrieval Final Verification

日期：2026-07-18

阶段性质：最终验证和冻结报告。  
遵守：`docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

本阶段未修改 Wiki、Guide、Guide-Wiki、Retrieval Pipeline、Prompt、profile、top_k、selector limit、ranking、fallback 或 schema，未新增知识。

## 1. Batch9-C 到 Batch9-G 演进总结

|阶段|目标|结果|
|---|---|---|
|Batch9-C|Manufacturing & Production Foundation Apply|新增制造生产基础 Wiki / Guide，建立 Guide-Wiki 双向 evidence chain，不修改 Retrieval。|
|Batch9-D|Manufacturing Retrieval Field Test|18 cases 初测，结果为 pass / partial / fail = 11 / 2 / 5；安全指标 100%，dangerous suggestion = 0，Kiwix 越权 = 0。|
|Batch9-E|Root Cause Review|确认主要根因不是知识缺失，而是 manufacturing profile 缺失、repair/tools 抢主位、少量 selected Wiki top8 截断。|
|Batch9-F|Minimal Apply|新增 4 个 manufacturing query profile，最小调整 7 个 Guide 的 related_wiki 顺序，新增 contract tests；Field Test 提升到 16 / 2 / 0。|
|Batch9-G|Final Verification|重新运行 audit、pytest 和 Manufacturing Field Test；结果稳定，fail = 0。|

## 2. Manufacturing Retrieval 当前状态

当前状态：Manufacturing Retrieval v0.1 stable。

稳定能力：

- 工坊开工前安全检查。
- 材料再利用前安全判断。
- 木材切割前固定与支撑。
- 简易结构件制作检查。
- 胶粘 / 螺丝 / 捆扎连接方式选择。
- 废旧金属材料安全处理。
- 制作完成后的承重检查。
- 工坊收工与工具封存。

当前检索边界：

- Manufacturing 主导：从材料制作新物品、多件重复制作、工坊流程、连接方式选择、成品承重检查、材料再利用前判断。
- Repair / Tools 主导：修已有物品、单个工具损坏、临时修补、已有结构失效。
- Shelter / Fire / Medical / Safety 可作为风险补充，但不应抢走制造作业前的行动入口。

## 3. 最终验证结果

### Audit

```text
python3 tools/audit_wiki.py
Articles: markdown=950 pocketbase=950 categories=24
Issues: errors=0 warnings=0 advisories=0

python3 scripts/audit_guides.py
Guides: 804
Issues: errors=0 warnings=0 advisories=0
```

### Pytest

```text
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
tests/test_retrieval_traceability.py \
tests/test_retrieval_root_contract.py \
tests/test_manufacturing_retrieval_profiles.py

17 passed
```

### Manufacturing Field Test

```text
python3 scripts/test_manufacturing_field.py --no-answer

pass / partial / fail = 16 / 2 / 0
strict pass / partial / fail = 10 / 2 / 0
```

关键指标：

|指标|结果|
|---|---:|
|total|18|
|strict / observation|12 / 6|
|Guide 命中率|100.0%|
|主 Guide 命中率|100.0%|
|Wiki 全量命中率|83.3%|
|Wiki 任一命中率|100.0%|
|Guide-Wiki 精准组合率|100.0%|
|Manufacturing 主 Guide 进入率|83.3%|
|dangerous suggestion|0|
|Kiwix 越权|0|
|safety boundary|100.0%|
|fallback|100.0%|
|record/check|100.0%|

结果文件：

- `docs/knowledge/batch9_d_manufacturing_field_test_results.json`
- `docs/knowledge/batch9_d_manufacturing_field_test_report.md`

## 4. 剩余 partial 分类

|case|状态|分类|说明|
|---|---|---|---|
|`manufacturing_scrap_wood_shelf_judgement`|partial|selector / top8 evidence 截断|DG-0872、DG-0877、DG-0876 已进入 selected Guide；Guide-Wiki precise 为 true。selected Wiki 前 8 被 DG-0872 的材料判断链占满，`repair-manufacturing-load-test-001` 未进入前 8。|
|`manufacturing_shutdown_child_tool_safety`|partial|selector / top8 evidence 截断|DG-0871、DG-0878、DG-0838 已进入 selected Guide；但 selected Wiki 前 8 被 DG-0871 开工安全链占满，DG-0878 的收工清点 Wiki 未进入前 8。|

判断：

- 两个 partial 均不是 Guide 缺失。
- 两个 partial 均不是 Wiki 缺失。
- 两个 partial 均不是 dangerous suggestion。
- 两个 partial 均不是 Kiwix 越权。
- 两个 partial 均不建议继续扩 profile。

## 5. 是否达到 stable

结论：达到 Manufacturing Retrieval v0.1 stable。

通过项：

- Wiki audit = 0 / 0 / 0。
- Guide audit = 0 / 0 / 0。
- pytest 通过。
- Field Test fail = 0。
- dangerous suggestion = 0。
- Kiwix 越权 = 0。
- safety = 100%。
- fallback = 100%。
- record/check = 100%。
- Guide 命中率 = 100%。
- Guide-Wiki 精准组合率 = 100%。

稳定性说明：

- Batch9-D 的 5 个 fail 已全部清零。
- 剩余 2 个 partial 为 evidence top8 截断和 fixture 粒度问题，不影响行动入口稳定性。
- 当前不需要继续新增 profile，不需要修改 Retrieval Pipeline，不需要调整 top_k 或 selector limit。

## 6. 冻结建议

建议冻结：

- Manufacturing Retrieval v0.1 stable。
- 当前 4 个 manufacturing query profile。
- 当前 DG-0871 至 DG-0878 的 related_wiki 顺序。
- 当前 contract test：`tests/test_manufacturing_retrieval_profiles.py`。
- 当前 Field Test fixture 与脚本，作为 Manufacturing v0.1 回归基线。

后续不建议在 v0.1 内继续处理：

- 用新增 profile 清理剩余 partial。
- 用 top_k / selector limit 放大掩盖 evidence 截断。
- 为了清零 partial 硬改 fixture expected。
- 新增制造 Wiki 或 Guide。

后续可进入下一阶段：

- Batch10 农业深化，或按 `knowledge_coverage_map_v0_3_final.md` 的未来 Batch 优先级继续推进。
