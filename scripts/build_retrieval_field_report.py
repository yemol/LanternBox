#!/usr/bin/env python3
"""Build the Batch3-F Markdown report from captured field-test results."""

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "docs" / "knowledge" / "retrieval_field_test_results.json"
REPORT = ROOT / "docs" / "knowledge" / "retrieval_field_test_report.md"


def yes(value):
    return "是" if value else "否"


def pct(value, total):
    return f"{value}/{total}（{value / total * 100:.1f}%）"


def compact_guides(row):
    items = row.get("guides_selected", [])
    return "<br>".join(f"`{item['id']}` {item['title']} ({item['risk_level']})" for item in items) or "无"


def compact_wikis(row):
    items = row.get("wikis_selected", [])
    return "<br>".join(f"`{item['slug']}` {item['title']}" for item in items) or "无"


def main():
    rows = json.loads(RESULTS.read_text(encoding="utf-8"))["cases"]
    total = len(rows)
    verdicts = Counter(row["verdict"] for row in rows)
    guide_hits = sum(bool(row.get("guide_hit")) for row in rows)
    domain_hits = sum(bool(row.get("domain_hit")) for row in rows)
    wiki_selected = sum(bool(row.get("wikis_selected")) for row in rows)
    wiki_hits = sum(bool(row.get("wiki_hit")) for row in rows)
    stops = sum(bool(row.get("has_stop_condition")) for row in rows)
    fallbacks = sum(bool(row.get("has_fallback")) for row in rows)
    records = sum(bool(row.get("has_record_advice")) for row in rows)
    high_rows = [row for row in rows if any(item.get("risk_level") in {"high", "critical"} for item in row.get("guides_selected", []))]
    high_stops = sum(bool(row.get("has_stop_condition")) for row in high_rows)

    lines = [
        "# LanternBox Batch3-F AI Retrieval 实战测试报告", "",
        "## 执行摘要", "",
        f"- 测试用例：{total}（21 条指定场景 + 1 条显式 Kiwix 对照）",
        f"- 严格通过：{pct(verdicts['pass'], total)}",
        f"- 部分通过：{pct(verdicts['partial'], total)}",
        f"- 失败：{pct(verdicts['fail'], total)}",
        f"- Guide 严格命中：{pct(guide_hits, total)}",
        f"- 正确领域：{pct(domain_hits, total)}",
        f"- 至少选择一个 Wiki：{pct(wiki_selected, total)}",
        f"- 所选 Wiki 与所选 Guide `related_wiki` 严格相交：{pct(wiki_hits, total)}",
        f"- high / critical 场景输出停止边界：{pct(high_stops, len(high_rows))}",
        f"- 回答包含替代方案：{pct(fallbacks, total)}",
        f"- 回答包含记录或复查建议：{pct(records, total)}", "",
        "本轮结果说明：行动 Guide 和高风险边界总体可用，但 Guide-Wiki 组合、fallback 注入、Kiwix 语义约束仍不足。严格通过率低主要由这三项共同造成，不代表 21 条回答全部不可用。", "",
        "## 测试方法", "",
        "每题调用真实生产链路：AI Planner → Guide/Wiki/Kiwix Fetchers → AI Selector → Emergency Response LLM。模型使用项目 `.env` 配置的局域网 `qwen3:8b`，Wiki 使用本地 PocketBase 只读数据库，Kiwix 使用现有 ZIM。", "",
        "判分采用预先声明的可接受 Guide ID、领域别名和 Guide `related_wiki` 交集。回答层检查停止边界、替代方案、记录建议、外部依赖及 Kiwix 是否压过行动证据。", "",
        "Wiki Retrieval 当前会在 `normalize_wiki_articles_for_ai()` 丢失 slug；测试器使用同一 PocketBase 数据库把候选 ID 只读映射回真实 slug 后判分。", "",
        "## 总体通过率", "",
        "| 结论 | 数量 | 比例 |", "|---|---:|---:|",
        f"| pass | {verdicts['pass']} | {verdicts['pass']/total*100:.1f}% |",
        f"| partial | {verdicts['partial']} | {verdicts['partial']/total*100:.1f}% |",
        f"| fail | {verdicts['fail']} | {verdicts['fail']/total*100:.1f}% |", "",
        "唯一严格通过案例为 `repair_wire`。`repair_rope` 没有命中 Guide 且领域错误，判为 fail；其余 20 条至少保留了正确行动证据或安全边界，判为 partial。", "",
        "## Guide 命中", "",
        f"Guide 严格命中率为 {pct(guide_hits, total)}，领域命中率为 {pct(domain_hits, total)}。", "",
        "未命中预设 Guide 的 4 条：", "",
        "- `water_boiled`：只选中 Wiki，未选 Guide。",
        "- `repair_rope`：只选中 Wiki，没有承重判断 Guide。",
        "- `records_consumption`：误选 `DG-0212 外伤耗材消耗记录`，范围过窄。",
        "- `team_disagreement`：选中 `DG-0726 长期压力下防止决策崩坏`，没有命中共同决定或风险分级 Guide。", "",
        "## Wiki 命中", "",
        f"有 Wiki 候选的案例为 {pct(wiki_selected, total)}；严格与命中 Guide 双向关联一致的案例仅 {pct(wiki_hits, total)}。", "",
        "严格组合成功的 4 条：`medical_burn`、`power_solar`、`fire_co`、`repair_wire`。其余情况分为两类：Selector 没有选择 Wiki，或选择了语义相关 Wiki 但该 slug 不在所选 Guide 的 `related_wiki` 中。", "",
        "这表明当前链路并不会从已选 Guide 确定性展开 `related_wiki`，而是让 Wiki 独立检索和独立选择，无法保证已建设的双向关系在回答阶段真正生效。", "",
        "## Risk Level", "",
        f"22 条回答全部出现至少一个停止、停用、隔离、撤离、禁止或不可继续边界。命中 high / critical Guide 的案例共 {len(high_rows)} 条，安全边界覆盖为 {pct(high_stops, len(high_rows))}。", "",
        "但是回答 Prompt 的 `_format_guide_evidence()` 没有传入 `risk_level`，也没有完整传入 `check`、`fallback` 和 `stop_or_escalate`。模型当前主要依靠标题和 steps 自行推断风险，而不是显式消费统一后的风险 schema。", "",
        "## High / Critical 安全表现", "",
        "高风险场景中，进水插线板、鼓包电池、燃气泄漏、室内燃烧、初起火灾、破皮电线、可疑水源等均给出了断电、停用、隔离或撤离动作。该项是本轮最稳定的能力。", "",
        "仍有两点需要修正：", "",
        "- `nav_flood` 回答出现“绕行或等待救援”，违反默认无外部支援原则。现有 `sanitize_ai_answer()` 词表没有覆盖“等待救援”。",
        f"- 只有 {pct(fallbacks, total)} 的回答被检测到本地替代或降级方案，原因与 fallback 未进入 Guide Prompt 直接相关。", "",
        "## Kiwix 使用", "",
        "21 条普通行动问题均未选择 Kiwix，没有出现百科覆盖 Guide/Wiki 的情况。", "",
        "显式 Kiwix 对照题仍以 `DG-0547` 的 critical 行动边界为主，回答没有使用错误百科内容覆盖撤离与禁开关建议；但 Kiwix 候选错误命中“RTL 型美铁燃气轮机动车组”，仅因“燃气”字面重合。结论：优先级控制有效，Kiwix 语义相关性不合格。", "",
        "## 分项结果", "",
        "| ID / 用户问题 | Guide | Wiki | 领域 | 外部依赖 | 停止 | 替代 | 记录 | Kiwix 越权 | 结论 / 原因 |",
        "|---|---|---|---|---|---|---|---|---|---|",
    ]

    for row in rows:
        reason = "通过" if not row.get("failure_reasons") else "；".join(row["failure_reasons"])
        lines.append(
            f"| `{row['id']}`<br>{row['question']} | {compact_guides(row)} | {compact_wikis(row)} | "
            f"{yes(row.get('domain_hit'))} | {', '.join(row.get('external_dependencies', [])) or '无'} | "
            f"{yes(row.get('has_stop_condition'))} | {yes(row.get('has_fallback'))} | {yes(row.get('has_record_advice'))} | "
            f"{yes(row.get('kiwix_override'))} | **{row['verdict']}**<br>{reason} |"
        )

    lines.extend(["", "## 失败案例", "",
        "### `repair_rope`", "",
        "问题“绳子还能不能承重怎么判断”未命中任何 Guide，只选中 Wiki“材料替代时的承重停止线”。说明 Guide 检索在‘绳子 + 承重 + 判断’组合上存在召回缺口，也暴露出当前 Guide 库缺少直接的绳索承重检查行动卡或同义词。", "",
        "### 关键 partial", "",
        "- `water_boiled`：Wiki 判断正确，但 Guide 为空，无法形成行动步骤 + 判断边界组合。",
        "- `records_consumption`：被“消耗记录”带到医疗耗材，缺少通用物资库存语义约束。",
        "- `team_disagreement`：命中压力决策 Guide，但没有命中风险分级或共同决定记录。",
        "- `nav_flood`：Guide 领域正确，但回答引入“等待救援”，且没有本地降级路线。",
        "- 18 条案例未形成所选 Guide 与所选 Wiki 的严格关联组合。", "",
        "## 下一阶段修复建议", "",
        "1. 在 Guide 被选中后，按其 `related_wiki` 确定性加载真实 Wiki，再允许 Selector 从这些关联 Wiki 中裁剪；不要让 Wiki 完全独立召回。",
        "2. `normalize_wiki_articles_for_ai()` 保留 `slug`，EvidenceCandidate 的 Wiki `id` 优先使用 slug，PocketBase ID 只作为内部 record ID。",
        "3. `_format_guide_evidence()` 显式传入 `risk_level`、steps、check、fallback、stop_or_escalate 和 related_wiki，禁止只取第一个可用正文字段。",
        "4. 回答 Prompt 对 high / critical 增加确定性前置规则：先输出停止/隔离/撤离/停用边界，再给后续动作。",
        "5. 扩展 `sanitize_ai_answer()`，覆盖“等待救援”“联系医院”“联系相关部门”“拨打电话”等默认外部依赖，并识别否定语境。",
        "6. 为通用物资消耗、团队风险分歧、煮沸边界和绳索承重补充 query aliases / negative keywords；如确属知识缺口，再进入内容扩充批次。",
        "7. Kiwix 增加领域锚点和标题语义过滤：燃气泄漏查询必须要求泄漏、爆燃、气体安全等锚点，排除交通工具、人物和同形词条。",
        "8. 将本脚本纳入固定回归测试，并为 Planner/Selector 使用可复现的模型参数或保存选择快照，降低 LLM 随机性。", "",
        "## 验证记录", "",
        "```text",
        "python3 -m pytest -q",
        "  system Python: No module named pytest",
        "",
        "venv/bin/python -m pytest -q",
        "  无范围收集超过 4 分钟无输出，已终止；根目录会递归发现 scripts/test_*.py。",
        "",
        "env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py",
        "  1 passed in 0.18s",
        "",
        "python3 scripts/audit_guides.py",
        "  errors=0 warnings=0 advisories=0",
        "",
        "python3 tools/audit_wiki.py",
        "  errors=0 warnings=0 advisories=0",
        "```", "",
        "本阶段没有修改 Guide 或 Wiki 正文，也没有修改 Retrieval 实现。发现的问题仅形成测试、证据与修复建议。",
    ])

    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"report: {REPORT}")


if __name__ == "__main__":
    main()
