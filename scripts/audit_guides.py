#!/usr/bin/env python3
"""Audit LanternBox Guide JSON files for structure, duplication, and overlap.

Read-only audit. This script does not modify Guide content, Retrieval v2,
prompt, schema, frontend, Kiwix, or scoring logic.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from collections import Counter, defaultdict
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parent.parent
GUIDE_DIR = ROOT / "data" / "guides"
WIKI_DIR = ROOT / "wiki_import"
DEFAULT_REPORT = ROOT / "docs" / "knowledge" / f"guide_audit_{date.today().isoformat()}.md"

REQUIRED_FIELDS = [
    "id",
    "title",
    "category",
    "domains",
    "priority",
    "risk_level",
    "scenario",
    "goal",
    "keywords",
    "tools",
    "steps",
    "check",
    "common_mistakes",
    "fallback",
    "stop_or_escalate",
    "notes",
]
LIST_FIELDS = ["domains", "keywords", "tools", "steps", "check", "common_mistakes", "fallback", "stop_or_escalate"]
ALLOWED_PRIORITIES = {"P0", "P1", "P2"}
ALLOWED_RISK_LEVELS = {"normal", "caution", "high", "critical"}
ID_RE = re.compile(r"^DG-\d{4}$")
GUIDE_ID_RE = re.compile(r"DG-\d{4}")
HIGH_RISK_BOUNDARY_RE = re.compile(
    r"停止|停用|禁用|禁止|不要|不得|不能|不可|隔离|撤离|离开|远离|放弃|"
    r"不进入|不继续|不饮用|不食用|不接触|不通电|切断|暂停|求援|升级|报废|退出|结束"
)
HIGH_RISK_SIGNAL_RE = re.compile(
    r"呼吸困难|意识异常|意识不清|叫不醒|抽搐|大量出血|持续高热|单侧无力|说话含糊|"
    r"火花|焦味|爆燃|浓烟|异味|油膜|污水|外溢|裂开|开裂|围堵|逼近|疼痛加重|"
    r"持续呕吐|血便|尿很少|无法饮水|头晕苍白|环境恶化|风险扩大"
    r"|自伤|伤人|严重绝望|拒食|连续失眠"
)
ABSOLUTE_TERMS = ["万无一失", "一定有效", "绝对安全", "完全安全", "保证安全"]
EXTERNAL_DEPENDENCY_TERMS = [
    "联系物业",
    "联系供水公司",
    "拨打客服",
    "等待救援",
    "上网查询",
    "在线购买",
    "联系医院",
    "联系消防",
    "依赖手机网络",
    "依赖云服务",
    "等待专业人员",
]
TEMPLATE_TERMS = [
    "相关情况出现，外部支援不稳定，需要小团队用本地资源先做保守处理",
    "降低直接风险，保留人员安全、关键物资和后续判断余地",
    "先确认“",
    "涉及的人员、地点、物品、时间和是否正在恶化",
    "按保命、饮水食物、通信照明、卫生隔离、物资保护的顺序处理",
    "无法确认安全时，选择停用、隔离、降级或等待复核",
    "凭感觉继续尝试",
    "只处理物品，不观察人员状态",
]


@dataclass(frozen=True)
class Guide:
    path: Path
    data: dict[str, Any]

    @property
    def guide_id(self) -> str:
        return str(self.data.get("id", ""))

    @property
    def title(self) -> str:
        return str(self.data.get("title", ""))

    @property
    def category(self) -> str:
        return str(self.data.get("category", ""))

    @property
    def priority(self) -> str:
        return str(self.data.get("priority", ""))

    @property
    def directory(self) -> str:
        try:
            return self.path.relative_to(GUIDE_DIR).parts[0]
        except (ValueError, IndexError):
            return "(root)"


def iter_guide_files() -> Iterable[Path]:
    return sorted(GUIDE_DIR.glob("**/*.json"))


def load_guides() -> tuple[list[Guide], list[dict[str, str]]]:
    guides: list[Guide] = []
    issues: list[dict[str, str]] = []

    for path in iter_guide_files():
        rel = str(path.relative_to(ROOT))
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            issues.append({"level": "error", "file": rel, "issue": f"JSON 无法解析：{exc}"})
            continue
        if not isinstance(data, dict):
            issues.append({"level": "error", "file": rel, "issue": "JSON 根节点不是对象"})
            continue
        guides.append(Guide(path, data))

    return guides, issues


def as_list(value: Any) -> list[str]:
    if isinstance(value, list):
        return [str(item) for item in value if str(item).strip()]
    if isinstance(value, str) and value.strip():
        return [value.strip()]
    return []


def text_value(value: Any) -> str:
    if isinstance(value, list):
        return "\n".join(text_value(item) for item in value)
    if isinstance(value, dict):
        return "\n".join(f"{key}: {text_value(val)}" for key, val in value.items())
    return str(value)


def guide_body_text(guide: Guide) -> str:
    fields = [
        "scenario",
        "goal",
        "tools",
        "steps",
        "check",
        "common_mistakes",
        "fallback",
        "stop_or_escalate",
        "notes",
    ]
    return "\n".join(text_value(guide.data.get(field, "")) for field in fields)


def normalize_text(text: str) -> str:
    text = re.sub(r"[0-9a-zA-Z_\-`~!@#$%^&*()+=\[\]{}\\|;:'\",.<>/?，。！？、；：“”‘’（）《》【】\s]+", "", text)
    return text


def canonical_text(guide: Guide) -> str:
    text = guide_body_text(guide)
    if guide.title:
        text = text.replace(guide.title, "<TITLE>")
    text = re.sub(r"“[^”]{2,80}”", "“<TITLE>”", text)
    text = re.sub(r"「[^」]{2,80}」", "「<TITLE>」", text)
    text = text.replace(guide.guide_id, "<ID>")
    return normalize_text(text)


def full_content_hash(guide: Guide) -> str:
    encoded = json.dumps(guide.data, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def body_hash(guide: Guide) -> str:
    normalized = re.sub(r"\s+", "\n", guide_body_text(guide).strip())
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def shingles(text: str, size: int = 4) -> set[str]:
    normalized = normalize_text(text)
    if len(normalized) <= size:
        return {normalized} if normalized else set()
    return {normalized[index : index + size] for index in range(len(normalized) - size + 1)}


def jaccard(left: set[str], right: set[str]) -> float:
    if not left or not right:
        return 0.0
    return len(left & right) / len(left | right)


def issue_external_dependency(content: str, term: str, title: str) -> bool:
    action_markers = ["应", "先", "优先", "建议", "需要", "立即", "尽快", "可以", "必须"]
    negation_markers = ["不要", "不应", "不能", "不可", "避免", "不依赖", "不作为", "不得", "无法"]

    for raw_line in content.splitlines():
        line = raw_line.strip()
        if term not in line:
            continue
        if title and title in line:
            continue
        if any(marker in line for marker in negation_markers):
            continue
        if any(marker in line for marker in action_markers):
            return True
    return False


def issue_absolute_expression(content: str, term: str) -> bool:
    negated_patterns = [
        rf"(?:不|无法|不能|不要|并非|不代表)\s*.{{0,10}}{re.escape(term)}",
        rf"不要.{{0,12}}当.{{0,4}}{re.escape(term)}",
    ]
    for raw_line in content.splitlines():
        line = raw_line.strip()
        if term not in line:
            continue
        if any(re.search(pattern, line) for pattern in negated_patterns):
            continue
        return True
    return False


def load_wiki_links() -> tuple[set[str], set[tuple[str, str]], list[dict[str, str]]]:
    slugs: set[str] = set()
    pairs: set[tuple[str, str]] = set()
    issues: list[dict[str, str]] = []

    for path in sorted(WIKI_DIR.glob("*/*.md")):
        rel = str(path.relative_to(ROOT))
        text = path.read_text(encoding="utf-8")
        parts = text.split("---", 2)
        if len(parts) != 3:
            issues.append({"level": "error", "file": rel, "issue": "Wiki frontmatter 无法解析"})
            continue
        meta: dict[str, str] = {}
        for line in parts[1].splitlines():
            if ":" not in line:
                continue
            key, value = line.split(":", 1)
            meta[key.strip()] = value.strip()
        slug = meta.get("slug", "")
        if not slug:
            issues.append({"level": "error", "file": rel, "issue": "Wiki 缺少 slug，无法校验 Guide 关联"})
            continue
        slugs.add(slug)
        for guide_id in GUIDE_ID_RE.findall(meta.get("guide_links", "")):
            pairs.add((slug, guide_id))
    return slugs, pairs, issues


def check_metadata(guides: list[Guide], parse_issues: list[dict[str, str]]) -> list[dict[str, str]]:
    issues = list(parse_issues)
    id_counts = Counter(guide.guide_id for guide in guides if guide.guide_id)
    title_counts = Counter(guide.title for guide in guides if guide.title)

    for guide in guides:
        rel = str(guide.path.relative_to(ROOT))

        for field in REQUIRED_FIELDS:
            if field not in guide.data or guide.data.get(field) in (None, "", []):
                issues.append({"level": "error", "file": rel, "issue": f"缺少或为空字段：{field}"})

        if guide.guide_id:
            if not ID_RE.match(guide.guide_id):
                issues.append({"level": "error", "file": rel, "issue": f"id 不符合 DG-0000 格式：{guide.guide_id}"})
            if guide.path.stem != guide.guide_id:
                issues.append({"level": "error", "file": rel, "issue": "文件名 stem 与 id 不一致"})
            if id_counts[guide.guide_id] > 1:
                issues.append({"level": "error", "file": rel, "issue": f"id 重复：{guide.guide_id}"})

        if guide.title and title_counts[guide.title] > 1:
            issues.append({"level": "warning", "file": rel, "issue": f"title 重复：{guide.title}"})

        if guide.priority and guide.priority not in ALLOWED_PRIORITIES:
            issues.append({"level": "error", "file": rel, "issue": f"priority 非法：{guide.priority}"})

        risk_level = str(guide.data.get("risk_level", ""))
        if risk_level and risk_level not in ALLOWED_RISK_LEVELS:
            issues.append({"level": "error", "file": rel, "issue": f"risk_level 非法：{risk_level}"})

        for field in LIST_FIELDS:
            value = guide.data.get(field)
            if not isinstance(value, list):
                issues.append({"level": "warning", "file": rel, "issue": f"{field} 应为数组"})

        if len(as_list(guide.data.get("keywords"))) < 3:
            issues.append({"level": "warning", "file": rel, "issue": "keywords 少于 3 个"})
        if len(as_list(guide.data.get("steps"))) < 3:
            issues.append({"level": "error", "file": rel, "issue": "steps 少于 3 步"})
        if len(as_list(guide.data.get("check"))) < 2:
            issues.append({"level": "warning", "file": rel, "issue": "check 少于 2 项"})
        if len(as_list(guide.data.get("stop_or_escalate"))) < 1:
            issues.append({"level": "error", "file": rel, "issue": "stop_or_escalate 为空"})
        if not isinstance(guide.data.get("fallback"), list) or not as_list(guide.data.get("fallback")):
            issues.append({"level": "error", "file": rel, "issue": "fallback 必须为非空数组"})
        if "related_wiki" not in guide.data or not isinstance(guide.data.get("related_wiki"), list):
            issues.append({"level": "error", "file": rel, "issue": "related_wiki 必须存在且为数组"})

        if risk_level in {"caution", "high", "critical"} and not as_list(guide.data.get("stop_or_escalate")):
            issues.append({"level": "error", "file": rel, "issue": f"{risk_level} Guide 缺少 stop_or_escalate"})
        if risk_level in {"high", "critical"}:
            boundary_text = "\n".join(
                text_value(guide.data.get(field, ""))
                for field in ["title", "steps", "common_mistakes", "fallback", "stop_or_escalate"]
            )
            stop_text = text_value(guide.data.get("stop_or_escalate", ""))
            if not HIGH_RISK_BOUNDARY_RE.search(boundary_text) and not HIGH_RISK_SIGNAL_RE.search(stop_text):
                issues.append({
                    "level": "error",
                    "file": rel,
                    "issue": f"{risk_level} Guide 缺少明确停止、禁用、隔离、撤离或升级边界",
                })

    return issues


def check_content(guides: list[Guide]) -> list[dict[str, str]]:
    issues: list[dict[str, str]] = []

    for guide in guides:
        rel = str(guide.path.relative_to(ROOT))
        content = guide_body_text(guide)
        compact_len = len(re.sub(r"\s+", "", content))

        if compact_len < 180:
            issues.append({"level": "warning", "file": rel, "issue": f"正文低于建议短行动卡长度：{compact_len} 字"})
        if compact_len > 1800:
            issues.append({"level": "warning", "file": rel, "issue": f"正文较长，可能需要拆分：{compact_len} 字"})

        for term in ABSOLUTE_TERMS:
            if issue_absolute_expression(content, term):
                issues.append({"level": "warning", "file": rel, "issue": f"含绝对化表达：{term}"})

        for term in EXTERNAL_DEPENDENCY_TERMS:
            if issue_external_dependency(content, term, guide.title):
                issues.append({"level": "warning", "file": rel, "issue": f"含外部支援依赖表达：{term}"})

        template_hits = [term for term in TEMPLATE_TERMS if term in content]
        if template_hits:
            issues.append({"level": "advisory", "file": rel, "issue": "含模板化行动卡语句：" + "；".join(template_hits[:4])})

        title_mentions = content.count(guide.title) if guide.title else 0
        if title_mentions >= 5:
            issues.append({"level": "advisory", "file": rel, "issue": f"标题短语在正文中重复较多：{title_mentions} 次"})

    return issues


def check_wiki_relations(guides: list[Guide]) -> list[dict[str, str]]:
    wiki_slugs, forward_pairs, issues = load_wiki_links()
    guide_ids = {guide.guide_id for guide in guides if guide.guide_id}
    reverse_pairs: set[tuple[str, str]] = set()

    for slug, guide_id in sorted(forward_pairs):
        if guide_id not in guide_ids:
            issues.append({
                "level": "error",
                "file": f"Wiki {slug}",
                "issue": f"guide_links 指向不存在 Guide：{guide_id}",
            })

    for guide in guides:
        rel = str(guide.path.relative_to(ROOT))
        related = guide.data.get("related_wiki", [])
        if not isinstance(related, list):
            continue
        for raw_slug in related:
            slug = str(raw_slug).strip()
            if slug not in wiki_slugs:
                issues.append({
                    "level": "error",
                    "file": rel,
                    "issue": f"related_wiki 指向不存在 Wiki slug：{slug}",
                })
                continue
            reverse_pairs.add((slug, guide.guide_id))

    for slug, guide_id in sorted(forward_pairs - reverse_pairs):
        issues.append({
            "level": "error",
            "file": f"Wiki {slug}",
            "issue": f"Guide-Wiki 非对称：Wiki 引用 {guide_id}，Guide 未反向引用 Wiki",
        })
    for slug, guide_id in sorted(reverse_pairs - forward_pairs):
        issues.append({
            "level": "error",
            "file": f"Guide {guide_id}",
            "issue": f"Guide-Wiki 非对称：Guide 引用 {slug}，Wiki 未反向引用 Guide",
        })

    return issues


def find_overlap(guides: list[Guide]) -> list[dict[str, str]]:
    issues: list[dict[str, str]] = []
    full_groups: dict[str, list[Guide]] = defaultdict(list)
    body_groups: dict[str, list[Guide]] = defaultdict(list)
    canonical_groups: dict[tuple[str, str], list[Guide]] = defaultdict(list)
    title_goal_groups: dict[str, list[Guide]] = defaultdict(list)

    for guide in guides:
        full_groups[full_content_hash(guide)].append(guide)
        body_groups[body_hash(guide)].append(guide)
        canonical_groups[(guide.directory, canonical_text(guide))].append(guide)
        title_goal_groups[normalize_text(guide.title + text_value(guide.data.get("goal", "")))].append(guide)

    for label, groups, level, issue_text in [
        ("完整 JSON", full_groups, "error", "完整 JSON 重复"),
        ("主体字段", body_groups, "warning", "主体行动内容完全重复"),
        ("title+goal", title_goal_groups, "warning", "title+goal 高度重复"),
    ]:
        for group in groups.values():
            if len(group) <= 1:
                continue
            shown = ", ".join(item.guide_id for item in sorted(group, key=lambda item: item.guide_id)[:20])
            issues.append({
                "level": level,
                "file": f"{label} duplicate group",
                "issue": f"{issue_text}：{len(group)} 篇；{shown}",
            })

    canonical_clone_ids: set[str] = set()
    canonical_clones = [
        sorted(group, key=lambda item: item.guide_id)
        for group in canonical_groups.values()
        if len(group) > 1
    ]
    canonical_clones.sort(key=lambda group: (-len(group), group[0].guide_id))
    for group in canonical_clones:
        canonical_clone_ids.update(item.guide_id for item in group)
        shown = ", ".join(item.guide_id for item in group[:20])
        if len(group) > 20:
            shown += f", ... +{len(group) - 20}"
        issues.append({
            "level": "warning",
            "file": f"{group[0].directory} canonical clone group",
            "issue": f"主体行动卡高度模板化，仅标题/主题词不同：{len(group)} 篇；{shown}",
        })

    shingle_cache = {
        guide.guide_id: shingles(guide.title + "\n" + text_value(guide.data.get("scenario", "")) + "\n" + guide_body_text(guide)[:3000])
        for guide in guides
        if guide.guide_id
    }
    sorted_guides = [guide for guide in sorted(guides, key=lambda item: item.guide_id) if guide.guide_id]
    adjacency: dict[str, set[str]] = defaultdict(set)
    for index, left in enumerate(sorted_guides):
        if left.guide_id in canonical_clone_ids:
            continue
        for right in sorted_guides[index + 1 :]:
            if right.guide_id in canonical_clone_ids:
                continue
            if left.directory != right.directory:
                continue
            score = jaccard(shingle_cache[left.guide_id], shingle_cache[right.guide_id])
            if score >= 0.84:
                adjacency[left.guide_id].add(right.guide_id)
                adjacency[right.guide_id].add(left.guide_id)

    id_to_guide = {guide.guide_id: guide for guide in sorted_guides}
    seen: set[str] = set()
    for guide_id in sorted(adjacency):
        if guide_id in seen:
            continue
        stack = [guide_id]
        component: set[str] = set()
        while stack:
            current = stack.pop()
            if current in seen:
                continue
            seen.add(current)
            component.add(current)
            stack.extend(sorted(adjacency[current] - seen))
        if len(component) < 2:
            continue
        members = [id_to_guide[item] for item in sorted(component)]
        shown = ", ".join(item.guide_id for item in members[:20])
        if len(members) > 20:
            shown += f", ... +{len(members) - 20}"
        issues.append({
            "level": "warning",
            "file": f"{members[0].directory} cluster",
            "issue": f"同目录高相似行动卡，可能存在范围重叠或模板化：{len(members)} 篇；{shown}",
        })

    return issues


def render_issue_table(issues: list[dict[str, str]], limit: int | None = None) -> list[str]:
    rows = ["|级别|文件|问题|", "|---|---|---|"]
    selected = issues[:limit] if limit else issues
    for issue in selected:
        rows.append(f"|{issue['level']}|`{issue['file']}`|{issue['issue']}|")
    if limit and len(issues) > limit:
        rows.append(f"|...|...|另有 {len(issues) - limit} 条，详见完整脚本输出|")
    return rows


def render_report(
    guides: list[Guide],
    metadata_issues: list[dict[str, str]],
    content_issues: list[dict[str, str]],
    overlap_issues: list[dict[str, str]],
    relation_issues: list[dict[str, str]],
) -> str:
    all_issues = metadata_issues + content_issues + overlap_issues + relation_issues
    errors = [issue for issue in all_issues if issue["level"] == "error"]
    warnings = [issue for issue in all_issues if issue["level"] == "warning"]
    advisories = [issue for issue in all_issues if issue["level"] == "advisory"]
    canonical_count = sum(1 for issue in overlap_issues if "主体行动卡高度模板化" in issue["issue"])

    lines: list[str] = []
    lines.append(f"# Guide Audit Report ({date.today().isoformat()})")
    lines.append("")
    lines.append("## 结论")
    if errors:
        lines.append(f"- 发现 {len(errors)} 个阻断级问题，需要先修复。")
    else:
        lines.append("- 未发现阻断级问题：JSON 可解析、id/文件名/priority/最低结构检查通过。")
    lines.append(f"- 发现 {len(warnings)} 个 warning；发现 {len(advisories)} 个 advisory。")
    if canonical_count:
        lines.append(f"- 发现 {canonical_count} 个 canonical clone / 高模板化重复组，需要后续逐组拆边界或重写。")
    else:
        lines.append("- 未发现 canonical clone 重复组。")

    lines.append("")
    lines.append("## 覆盖范围")
    lines.append(f"- Guide JSON：{len(guides)}")
    lines.append(f"- 目录数：{len(set(guide.directory for guide in guides))}")
    lines.append(f"- 分类数：{len(set(guide.category for guide in guides))}")

    lines.append("")
    lines.append("## 目录分布")
    lines.append("|directory|数量|")
    lines.append("|---|---:|")
    for directory, count in Counter(guide.directory for guide in guides).most_common():
        lines.append(f"|{directory}|{count}|")

    lines.append("")
    lines.append("## Priority 分布")
    lines.append("|priority|数量|")
    lines.append("|---|---:|")
    for priority, count in Counter(guide.priority for guide in guides).most_common():
        lines.append(f"|{priority}|{count}|")

    lines.append("")
    lines.append("## Risk Level 分布")
    lines.append("|risk_level|数量|")
    lines.append("|---|---:|")
    for risk_level in ["normal", "caution", "high", "critical"]:
        count = sum(1 for guide in guides if guide.data.get("risk_level") == risk_level)
        lines.append(f"|{risk_level}|{count}|")

    lines.append("")
    lines.append("## 结构检查")
    metadata_errors = [issue for issue in metadata_issues if issue["level"] == "error"]
    metadata_warnings = [issue for issue in metadata_issues if issue["level"] == "warning"]
    if not metadata_errors and not metadata_warnings:
        lines.append("- 通过：必填字段、id 格式、文件名一致性、priority 合法性和最低数组结构未发现 error/warning。")
    else:
        lines.extend(render_issue_table(metadata_errors + metadata_warnings, limit=120))

    lines.append("")
    lines.append("## 正文内容检查")
    content_blocking = [issue for issue in content_issues if issue["level"] in {"error", "warning"}]
    if not content_blocking:
        lines.append("- 通过：正文长度、外部依赖和绝对化表达未发现 error/warning。")
    else:
        lines.extend(render_issue_table(content_blocking, limit=120))
    content_advisories = [issue for issue in content_issues if issue["level"] == "advisory"]
    if content_advisories:
        lines.append("")
        lines.append("### 内容增强建议")
        lines.append(f"- {len(content_advisories)} 条 advisory，主要提示模板化语句或标题短语重复。")

    lines.append("")
    lines.append("## 重复与范围重叠")
    overlap_blocking = [issue for issue in overlap_issues if issue["level"] in {"error", "warning"}]
    if not overlap_blocking:
        lines.append("- 通过：未发现完整 JSON 重复、主体重复、canonical clone 或同目录高相似簇。")
    else:
        lines.extend(render_issue_table(overlap_blocking, limit=160))

    lines.append("")
    lines.append("## Guide-Wiki 关联检查")
    if not relation_issues:
        lines.append("- 通过：related_wiki 均为真实 Wiki slug，Guide-Wiki 前后向关系完全对称。")
    else:
        lines.extend(render_issue_table(relation_issues, limit=160))

    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT, help="Output Markdown report path")
    args = parser.parse_args()

    guides, parse_issues = load_guides()
    metadata_issues = check_metadata(guides, parse_issues)
    content_issues = check_content(guides)
    overlap_issues = find_overlap(guides)
    relation_issues = check_wiki_relations(guides)

    report = render_report(guides, metadata_issues, content_issues, overlap_issues, relation_issues)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(report, encoding="utf-8")

    all_issues = metadata_issues + content_issues + overlap_issues + relation_issues
    errors = sum(1 for issue in all_issues if issue["level"] == "error")
    warnings = sum(1 for issue in all_issues if issue["level"] == "warning")
    advisories = sum(1 for issue in all_issues if issue["level"] == "advisory")
    print(f"Report: {args.report}")
    print(f"Guides: {len(guides)}")
    print(f"Issues: errors={errors} warnings={warnings} advisories={advisories}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
