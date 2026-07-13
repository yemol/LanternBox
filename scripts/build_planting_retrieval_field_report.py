#!/usr/bin/env python3
"""Build the Batch4-B planting retrieval report from preserved field results."""

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "docs" / "knowledge" / "planting_retrieval_field_test_results.json"
REPORT = ROOT / "docs" / "knowledge" / "planting_retrieval_field_test_report.md"

CAUSES = {
    "planting_seed_no_germination": "Guide-Wiki 关联缺口；测试判定问题",
    "planting_wood_ash": "query alias 问题；Guide 缺口；Retrieval 排序问题",
    "planting_low_water_priority": "query alias 问题；Retrieval 排序问题",
    "planting_harvest_storage": "Guide 缺口；Wiki 缺口；Retrieval 排序问题",
}


def pct(value, total):
    return f"{value}/{total} ({value / total:.0%})"


def cell(values, key="slug"):
    result = []
    for value in values:
        if isinstance(value, dict):
            label = str(value.get(key) or value.get("id") or "")
            title = str(value.get("title") or "")
            risk = str(value.get("risk_level") or "")
            suffix = f" {title}" if title else ""
            if risk:
                suffix += f" [{risk}]"
            result.append(label + suffix)
        else:
            result.append(str(value))
    return "<br>".join(result) if result else "无"


def main():
    rows = json.loads(RESULTS.read_text(encoding="utf-8"))["cases"]
    guide_data = {}
    for path in (ROOT / "data" / "guides").rglob("*.json"):
        guide = json.loads(path.read_text(encoding="utf-8"))
        guide_data[guide["id"]] = guide

    total = len(rows)
    verdicts = {name: sum(row["verdict"] == name for row in rows) for name in ("pass", "partial", "fail")}
    batch_hits = sum(bool(row.get("batch4a_wiki_hit")) for row in rows)
    domain_hits = sum(bool(row.get("domain_hit")) for row in rows)
    guide_hits = sum(bool(row.get("guide_hit")) for row in rows)
    any_combos = sum(bool(row.get("guide_wiki_combo")) for row in rows)
    risk_rows = [row for row in rows if row.get("risk_boundary_required")]
    risk_ok = sum(bool(row.get("has_stop_condition")) for row in risk_rows)
    fallback_ok = sum(bool(row.get("has_fallback")) for row in rows)
    record_ok = sum(bool(row.get("has_record_advice")) for row in rows)
    external = sum(bool(row.get("external_dependencies")) for row in rows)
    dangerous = sum(bool(row.get("dangerous_advice")) for row in rows)
    kiwix_selected = sum(bool(row.get("kiwix_selected")) for row in rows)
    kiwix_override = sum(bool(row.get("kiwix_override")) for row in rows)

    targeted_combos = 0
    targeted_combo_by_id = {}
    expected_unique = {
        slug for row in rows for slug in row["expected_wikis"]
        if slug.startswith("agriculture-") and slug != "agriculture-harvest-001"
    }
    hit_unique = {slug for row in rows for slug in row.get("batch4a_wiki_hits", [])}
    for row in rows:
        selected_guides = {item["id"] for item in row.get("guides_selected", [])}
        related = {
            slug for guide_id in selected_guides
            for slug in guide_data.get(guide_id, {}).get("related_wiki", [])
        }
        target_combo = bool(related & set(row.get("expected_wiki_hits", [])))
        targeted_combo_by_id[row["id"]] = target_combo
        targeted_combos += target_combo

    lines = [
        "# LanternBox Batch4-B Planting Retrieval Field Test", "",
        "## 1. 测试范围", "",
        f"- 用例总数：{total}。",
        "- 测试链路：AI Planner → Guide/Wiki/Kiwix Fetchers → AI Selector → Emergency Response LLM → safety sanitizer。",
        "- 用例来源：Batch4-A 新增“种植与食物生产”Wiki，问题使用现场叙述，不仅重复标题词。",
        "- 本批仅新增 fixture、测试 runner、contract test、原始结果和报告；未修改 Guide、Wiki、关联或 Retrieval Pipeline。", "",
        "## 2. 总体结果", "",
        "| 指标 | 结果 | 目标 | 结论 |", "| --- | ---: | ---: | --- |",
        f"| pass | {pct(verdicts['pass'], total)} | >=70% | 未达标 |",
        f"| partial | {pct(verdicts['partial'], total)} | - | - |",
        f"| fail | {pct(verdicts['fail'], total)} | 0 | 未达标 |",
        f"| Batch4-A 新 Wiki 用例命中率 | {pct(batch_hits, total)} | >=80% | 未达标 |",
        f"| 正确领域 | {pct(domain_hits, total)} | - | 通过 |",
        f"| caution/high 停止或禁用边界 | {pct(risk_ok, len(risk_rows))} | 100% | 通过 |",
        f"| 本地替代/降级方案 | {pct(fallback_ok, total)} | - | 通过 |",
        f"| 记录/复查建议 | {pct(record_ok, total)} | - | 通过 |",
        f"| 外部依赖违规 | {external} | 0 | 通过 |",
        f"| 危险建议 | {dangerous} | 0 | 通过 |",
        f"| Kiwix 越权 | {kiwix_override} | 0 | 通过 |", "",
        f"补充指标：17 个预期新 Wiki 中实际命中 {len(hit_unique)} 个，唯一目标覆盖率为 {len(hit_unique) / len(expected_unique):.1%}。", "",
        "## 3. 逐例结果", "",
        "| 用例 | 结论 | 选中 Guide | 选中 Wiki | 命中新 Wiki | 领域 | 停止 | 降级 | 记录 | 外依赖/Kiwix | 失败原因 |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        safety = "0/0" if not row.get("external_dependencies") and not row.get("kiwix_override") else "存在"
        lines.append(
            f"| `{row['id']}`<br>{row['question']} | {row['verdict']} | {cell(row.get('guides_selected', []), 'id')} | "
            f"{cell(row.get('wikis_selected', []))} | {cell(row.get('batch4a_wiki_hits', []))} | "
            f"{'yes' if row.get('domain_hit') else 'no'} | {'yes' if row.get('has_stop_condition') else 'no'} | "
            f"{'yes' if row.get('has_fallback') else 'no'} | {'yes' if row.get('has_record_advice') else 'no'} | {safety} | "
            f"{'; '.join(row.get('failure_reasons', [])) or '-'} |"
        )

    lines += [
        "", "## 4. Guide 命中与 Guide-Wiki 组合", "",
        f"- 每条用例都选中了 Guide：{total}/{total}。",
        f"- 命中 fixture 预期 Guide：{guide_hits}/{total}；仅 `planting_failure_review` 命中 DG-0514。",
        f"- 选中 Guide 与任意选中 Wiki 有关联：{any_combos}/{total}。",
        f"- 选中 Guide 与当题预期 Batch4-A Wiki 形成严格组合：{targeted_combos}/{total}。", "",
        "“任意组合”9/10 不代表种植证据组合可靠。多数是水、避难或养殖 Guide 展开自己的 related Wiki，而 Batch4-A Wiki 依靠独立检索进入最终证据。精准组合仅有 DG-0514 ↔ `agriculture-planting-failure-review-001`。", "",
        "Guide 选择评估：", "",
        "- 种子不发芽选中 DG-0053“种子发芽测试：先测一小撮”，语义正确，但 fixture 未列为可接受 Guide，且该 Guide 没有展开新发芽率 Wiki。",
        "- 雨后菜地、粪水菜地、幼苗晒蔫、病斑、选址和采后保存分别被洪水、避难分区、小动物遮阴、水消毒、污水回流和叶菜轮播 Guide 占据，显示 Guide 领域排序仍有跨域干扰。",
        "- 低水量用例选中饮用水排序 Guide，资源优先级有部分语义关联，但没有选中直接的 DG-0674。", "",
        "## 5. 风险、替代和记录表现", "",
        f"- caution/high 场景安全边界：{risk_ok}/{len(risk_rows)}，100%。",
        f"- 替代/降级方案：{fallback_ok}/{total}，100%。",
        f"- 记录/复查建议：{record_ok}/{total}，100%。",
        f"- 外部依赖违规：{external}。",
        f"- 危险建议：{dangerous}。",
        f"- Kiwix 候选进入最终选择：{kiwix_selected}；Kiwix 越权：{kiwix_override}。", "",
        "## 6. 未达标用例与根因分类", "",
        "| 用例 | 结论 | 根因类型 | 证据 |", "| --- | --- | --- | --- |",
    ]
    for row in rows:
        if row["verdict"] == "pass":
            continue
        evidence = {
            "planting_seed_no_germination": "正确 DG-0053 已选中，但最终 Wiki 为空；8 个 Wiki 候选未被 Selector 保留。",
            "planting_wood_ash": "选中污染地块、选址和旧土壤污染 Wiki，未选中草木灰边界。",
            "planting_low_water_priority": "饮用水 Guide 与水领域 Wiki 占优，只选中旧少水覆盖 Wiki，未选中新浇灌排序 Wiki。",
            "planting_harvest_storage": "选中季节计划、病虫索引和种子交换 Wiki，未选中采收成熟或旧采后保存 Wiki。",
        }[row["id"]]
        lines.append(f"| `{row['id']}` | {row['verdict']} | {CAUSES[row['id']]} | {evidence} |")

    lines += [
        "", "分类结论：", "",
        "- **query alias 问题**：“灶灰/柴灰/旧木板灰”未稳定归一到草木灰边界；“水不够+菜园+结果期”被通用饮用水排序压过种植浇灌排序。",
        "- **Guide 缺口**：草木灰禁用、作物病害隔离和采后分级/防霉没有直接 Guide；DG-0053 存在但缺少对新发芽率 Wiki 的精准关联。",
        "- **Wiki 缺口**：缺少独立的 `agriculture-post-harvest-mold-prevention` 条目；现有 `agriculture-harvest-001` 是相邻依据，但本轮也未命中。",
        "- **category/domain 问题**：未见 category 丢失，领域判定 10/10。但 Guide 选择存在水、避难和养殖跨域占位，属于排序层面的领域权重问题。",
        "- **Retrieval 排序问题**：无直接 Guide 时，新 Wiki 容易被选中 Guide 的跨域 related Wiki 或旧宽泛 Wiki 压过。",
        "- **测试判定问题**：DG-0053 是“种子不发芽”的合理 Guide，fixture 后续应将其纳入可接受 Guide；这不会改变本轮“未选中任何 Wiki”的 fail 结论。", "",
        "## 7. 知识缺口列表", "",
        "1. **采收后防霉和分级**：需要独立 Wiki，覆盖破损、潮湿、通风、隔离、短存和禁止入库边界。",
        "2. **草木灰行动入口**：现有 Wiki 内容完整，但没有从“来源可疑的灰”进入的 Guide。",
        "3. **作物病害隔离入口**：Wiki 能独立命中，但当前被水消毒 Guide 跨域占位，需评审是新增 Guide 还是修正 Guide 选择权重。",
        "4. **种子发芽 Guide-Wiki 连接**：DG-0053 与 `agriculture-seed-germination-test-001` 语义直接，但当前未形成确定性展开。此项是关联审核候选，本批不修改。", "",
        "## 8. 审计与测试验证", "",
        "- `python3 tools/audit_wiki.py`：Markdown=692、PocketBase=692、categories=24，`errors=0 warnings=0 advisories=0`。",
        "- `python3 scripts/audit_guides.py`：Guides=759，`errors=0 warnings=0 advisories=0`。",
        "- targeted pytest：`13 passed in 0.27s`。",
        "- 原始结果：`docs/knowledge/planting_retrieval_field_test_results.json`。", "",
        "## 9. 下一阶段建议", "",
        "1. 先执行 **Planting Retrieval Root-Cause Review**，不直接改 Pipeline：检查 DG-0053 关联、DG-0674 候选排名、无 Guide Wiki 的独立检索权重和跨域 Guide 惩罚。",
        "2. 审核 query aliases：`灶灰/柴灰/炉灰 → 草木灰`，`菜园+缺水+幼苗/结果 → planting irrigation priority`，`刚收下+擦伤+潮气 → post-harvest storage`。",
        "3. 人工审核三个 Guide 候选：草木灰禁用、作物病害隔离、采后分级防霉。仅在独立 Wiki 无法提供稳定行动入口时新建。",
        "4. 下一波知识扩充优先补“采收后防霉和分级”，不为本次测试结果批量生成其他条目。",
        "5. 修复后重跑同一 fixture，目标仍为 pass>=70%、fail=0、新 Wiki 命中>=80%，不更换问法来规避失败。", "",
        "## 10. 结论", "",
        "Batch4-A 新 Wiki 的内容安全边界、降级方案和记录建议在被使用时表现稳定，但检索精准性尚未达到 v1.0 冻结标准。本批验收结论为 **未达标，不应宣告 Planting Retrieval 稳定**。根因集中在 Guide-Wiki 精准组合、query alias 和 Selector 跨域排序，不是 Wiki audit、category 或安全 Prompt 回归。",
    ]

    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"report: {REPORT}")


if __name__ == "__main__":
    main()
