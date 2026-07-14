# Tools Repair Profile Conflict Apply Report

生成时间：2026-07-14

## 1. 本阶段修改范围

本阶段是 Batch4-L review 后的最小 Apply，只修复两个剩余 partial 的 profile conflict / candidate anchor 问题。

实际涉及：

- `data/retrieval_query_profiles.json`
- `tests/test_tools_repair_retrieval_profiles.py`
- `docs/knowledge/tools_repair_retrieval_field_test_results.json`
- `docs/knowledge/tools_repair_retrieval_field_test_report.md`
- `docs/knowledge/tools_repair_profile_conflict_apply_report.md`

未执行：

- 未新增 Wiki
- 未新增 Guide
- 未修改 Guide 正文
- 未修改 Wiki 正文
- 未修改 Guide-Wiki 关联
- 未修改 Retrieval Prompt
- 未扩大 top_k
- 未扩大 selector_candidate_limit
- 未修改 PocketBase schema
- 未修改 risk_level 体系
- 未删除或削弱 DG-0833
- 未让 Kiwix 覆盖本地行动建议

## 2. repair_anchor_fastening 修改摘要

目的：避免“旧钉子 + 承重位置”触发固定点失效 profile。

调整内容：

- 从 `repair_anchor_fastening.object_triggers` 移除 `钉子`
- 未将 `螺丝 / 钉子 / 紧固件` 这类泛紧固件词作为 anchor object
- 保留强固定点对象：
  - `固定点`
  - `墙角`
  - `支架`
  - `挂点`
  - `绑点`
  - `棚布`
  - `膨胀螺丝`
  - `挂钩`

效果：

- “旧钉子 + 承重位置”不再触发 `repair_anchor_fastening`
- 固定点松动、墙角固定、支架挂点等明确 anchor 场景仍正常触发
- DG-0833 在明确固定点/支架/挂点问题中仍保持主位

## 3. repair_rope_load 修改摘要

目的：让“旧绳 / 绳子 / 绳结 / 磨损 / 磨毛 / 发白 / 火烫 / 烧焦 / 断股 / 霉烂 / 轻拉测试”等现场表达稳定命中绳索 Guide 与绳索 Wiki。

调整内容：

- 增强 aliases：
  - `绳索承重前检查`
  - `绳子承重前检查`
  - `绳结固定`
  - `绳结滑脱检查`
  - `绳子全长摸查`
  - `磨损绳停用`
  - `磨毛发白`
  - `烧焦绳`
  - `烫过的绳`
  - `断股绳`
  - `霉烂绳`
  - `局部变细`
  - `禁承重`
  - `降级为捆扎`
  - `轻拉测试`
- 增强 candidate anchors：
  - `绳子`
  - `绳索`
  - `绳结`
  - `磨损`
  - `磨毛`
  - `发白`
  - `烧焦`
  - `烫过`
  - `断股`
  - `霉烂`
  - `局部变细`
  - `全长检查`
  - `全长摸查`
  - `轻拉测试`
  - `滑脱`
  - `禁承重`
  - `降级捆扎`
- 保留 `承重 / 吊 / 拉重物` 作为 state 线索，不把它们作为脱离绳索对象的决定性 anchor。

效果：

- 绳索磨损承重 case 命中 `repair_rope_load`
- 不触发 `repair_anchor_fastening`
- DG-0156 和绳索 Wiki 优先于 DG-0833

## 4. repair_fastener_reuse 修改摘要

目的：让“旧螺丝 / 旧钉子 / 螺丝头 / 钉帽 / 头花了 / 滑牙 / 弯曲 / 锈 / 复用 / 承重位置”稳定导向紧固件复用证据。

调整内容：

- 增强 aliases：
  - `旧螺丝复用`
  - `旧钉子复用`
  - `紧固件复用检查`
  - `螺丝头花了`
  - `钉帽变形`
  - `滑牙禁用`
  - `弯曲禁用`
  - `锈蚀降级`
  - `承重位置禁用`
  - `旧紧固件复用边界`
  - `螺丝钉分类保存`
- 增强 candidate anchors：
  - `旧螺丝`
  - `旧钉子`
  - `螺丝头`
  - `钉帽`
  - `头花了`
  - `滑牙`
  - `弯曲`
  - `锈蚀`
  - `生锈`
  - `复用检查`
  - `承重位置`
  - `紧固件`
  - `垫片`
  - `禁用`
  - `降级`

效果：

- 旧螺丝旧钉子复用 case 命中 `repair_fastener_reuse`
- 不因“钉子 + 承重位置”触发 `repair_anchor_fastening`
- `repair-used-screw-nail-reuse-check-001` 进入候选并在独立 Wiki 候选中保持主位
- DG-0833 不再抢旧紧固件复用主位

## 5. Profile Conflict Tests 结果

已覆盖附件要求的精确输入：

1. `这根旧绳有几段磨毛发白，还有一处像被火烫过，今天能不能继续拿来吊水桶或拉重物？`
   - `repair_rope_load` 触发
   - `repair_anchor_fastening` 不触发
   - DG-0156 为主 Guide
   - `repair-rope-full-length-inspection-001` 和 `repair-knot-slip-check-001` 进入 Wiki 证据

2. `拆下来的旧螺丝和旧钉子有点锈，有的头也花了，哪些还能复用，哪些不能再用在承重位置？`
   - `repair_fastener_reuse` 触发
   - `repair_anchor_fastening` 不触发
   - DG-0833 不为主位
   - `repair-used-screw-nail-reuse-check-001` 进入 Wiki 证据

3. `棚布绑在墙角固定点上会晃，拉紧时还有咔咔响，这个点还能继续绑东西吗？`
   - `repair_anchor_fastening` 仍触发
   - DG-0833 为主 Guide
   - 固定点失效 Wiki 为主证据

4. `临时支架的挂点有点松，挂上东西会晃，这个挂点还能承重吗？`
   - `repair_anchor_fastening` 触发
   - DG-0833 为主 Guide
   - 固定点 / 支架相关 Wiki 优先

5. `旧钉子有锈，钉帽也变形了，还能不能用在承重位置？`
   - `repair_fastener_reuse` 触发
   - `repair_anchor_fastening` 不触发
   - `repair-used-screw-nail-reuse-check-001` 为主 Wiki 候选

测试结果：

```bash
venv/bin/python -m pytest -q tests/test_tools_repair_retrieval_profiles.py
# 15 passed
```

Targeted pytest：

```bash
env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q \
  tests/test_retrieval_traceability.py \
  tests/test_retrieval_root_contract.py \
  tests/test_tools_repair_retrieval_profiles.py
# 24 passed
```

## 6. 原 10 条工具维修 Field Test 回归结果

运行：

```bash
venv/bin/python scripts/test_tools_repair_retrieval_field.py --no-answer
```

结果：

- 用例总数：10
- pass / partial / fail：10 / 0 / 0
- Batch4-H 新 Wiki 命中率：100.0%
- Guide 命中率：100.0%
- Guide-Wiki 精准组合率：100.0%
- high/caution 安全边界表现：100.0%
- fallback / 降级用途覆盖率：100.0%
- 记录 / 复查建议覆盖率：100.0%
- 外部依赖违规：0
- Kiwix 越权：0

## 7. 与 Batch4-K 对比

| 指标 | Batch4-K | Batch4-M |
| --- | ---: | ---: |
| pass / partial / fail | 8 / 2 / 0 | 10 / 0 / 0 |
| Batch4-H 新 Wiki 命中率 | 80% | 100% |
| Guide-Wiki 精准组合率 | 达标 | 100% |
| high/caution 安全边界 | 100% | 100% |
| fallback / 降级用途 | 100% | 100% |
| 记录 / 复查建议 | 100% | 100% |
| 外部依赖违规 | 0 | 0 |
| Kiwix 越权 | 0 | 0 |

两个剩余 partial 均已清除。

## 8. 是否仍有 Partial

当前无剩余 partial。

- 绳索磨损承重：pass
- 旧螺丝旧钉子复用：pass

## 9. 是否需要新增“旧紧固件复用边界”Guide

本批结论：暂不需要。

原因：

- profile conflict 修复后，旧紧固件 case 已达 pass
- 现有 DG-0376 可作为小件分类和降级入口
- `repair-used-screw-nail-reuse-check-001` 已能提供复用边界判断
- 当前问题是 profile conflict，不是必须新增 Guide 才能解决的行动入口缺失

后续如果产品层希望把“旧紧固件复用边界”作为独立可点击行动卡，可在下一批单独评估。

## 10. 硬编码检查

- 是否硬编码测试题到生产逻辑：否
- 是否硬编码 Guide ID 到生产逻辑：否
- 是否硬编码 Wiki slug 到生产逻辑：否
- 是否扩大 top_k / selector limit：否
- 是否修改 Prompt：否
- 是否批量改 tools/repair 排序：否

Guide ID / Wiki slug 仅出现在测试断言和报告中。

## 11. Audit / Build 结果

```bash
python3 tools/audit_wiki.py
# Articles: markdown=733 pocketbase=733 categories=24
# Issues: errors=0 warnings=0 advisories=0

python3 tools/build_guides.py
# Generated 765 Guides
# Generated 765 Guide Index Items

python3 scripts/audit_guides.py
# Guides: 765
# Issues: errors=0 warnings=0 advisories=0
```

Guide-Wiki 关系保持通过 audit，无新增单边关系、无效 Guide ID 或无效 Wiki slug。

## 12. 稳定判定

当前达到稳定判定条件：

- pass / partial / fail = 10 / 0 / 0
- Batch4-H 新 Wiki 命中率 >= 80%
- high/caution 安全边界 = 100%
- fallback / 降级用途 = 100%
- 记录 / 复查建议 = 100%
- 外部依赖违规 = 0
- Kiwix 越权 = 0
- Wiki audit = 0/0/0
- Guide audit = 0/0/0
- targeted pytest 全通过

建议将 Tools & Repair Retrieval 标记为 v0.1 stable。

## 13. 下一批建议

1. 若继续扩容工具维修知识，优先新增真实知识缺口，不继续围绕本批两条 partial 调 profile。
2. 若未来用户界面需要更明确的行动入口，再评估新增“旧紧固件复用边界”Guide。
3. 保留本批 profile conflict tests，作为后续知识扩容的回归保护。

## 14. 不应继续修复的问题

- 不继续削弱 DG-0833；它在明确固定点/支架/挂点场景中是正确主 Guide。
- 不通过扩大候选数量掩盖 profile 冲突。
- 不新增重复工具维修 Wiki。
- 不让 Kiwix 覆盖本地 Guide/Wiki 行动建议。
- 不把测试题、Guide ID 或 Wiki slug 写入生产逻辑。
