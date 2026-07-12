#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Audit LanternBox wiki Markdown files against naming/content rules and PB data."""

from __future__ import annotations

import argparse
import hashlib
import re
import sqlite3
from collections import Counter, defaultdict
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parent.parent
WIKI_DIR = ROOT / "wiki_import"
PB_DB = ROOT / "pocketbase" / "pb_data" / "data.db"
DEFAULT_REPORT = ROOT / "docs" / "knowledge" / f"wiki_audit_{date.today().isoformat()}.md"

REQUIRED_META = ["title", "slug", "category", "summary", "tags", "risk_level", "status", "source"]
ALLOWED_RISK_LEVELS = {"normal", "caution", "high"}
ALLOWED_STATUSES = {"draft", "published"}
ALLOWED_DOMAINS = {
    "water",
    "food",
    "medical",
    "energy",
    "repair",
    "hygiene",
    "safety",
    "communication",
    "shelter",
    "navigation",
    "clothing",
    "fire",
    "tools",
    "agriculture",
    "wildlife",
    "organization",
    "psychology",
    "education",
    "general",
}
SLUG_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*-[0-9]{3}$")
H1_RE = re.compile(r"^#\s+(.+?)\s*$", re.M)
H2_RE = re.compile(r"^##\s+(.+?)\s*$", re.M)

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
    "联系相关部门",
    "拨打救援电话",
    "等待专业人员",
    "上网搜索",
    "联系厂家",
    "咨询客服",
    "前往医院",
]
ABSOLUTE_TERMS = ["万无一失", "一定有效", "绝对安全", "完全安全", "保证安全"]
SUMMARY_TEMPLATE_TERMS = [
    "原理、判断边界与常见误区",
    "原理、判断边界和常见误区",
    "供 Guide 行动卡和后续 Kiwix/ZIM 检索使用",
    "帮助选择对应 Guide 行动卡",
]
PLACEHOLDER_TERMS = [
    "原文未单列准备材料",
    "原文未单列替代方案",
    "本次不新增处置建议",
]


@dataclass(frozen=True)
class Article:
    path: Path
    meta: dict[str, str]
    content: str

    @property
    def slug(self) -> str:
        return self.meta.get("slug", "")

    @property
    def title(self) -> str:
        return self.meta.get("title", "")

    @property
    def summary(self) -> str:
        return self.meta.get("summary", "")

    @property
    def category(self) -> str:
        return self.meta.get("category", "")

    @property
    def risk_level(self) -> str:
        return self.meta.get("risk_level", "")

    @property
    def status(self) -> str:
        return self.meta.get("status", "")


def iter_markdown_files(target: Path) -> Iterable[Path]:
    for path in sorted(target.glob("*/*.md")):
        yield path


def parse_front_matter(path: Path) -> Article:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---"):
        return Article(path, {}, text)

    parts = text.split("---", 2)
    if len(parts) < 3:
        return Article(path, {}, text)

    meta_text = parts[1].strip()
    content = parts[2].lstrip("\n")
    meta: dict[str, str] = {}

    for line in meta_text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or ":" not in line:
            continue
        key, value = line.split(":", 1)
        meta[key.strip()] = value.strip().strip('"').strip("'")

    return Article(path, meta, content)


def load_articles() -> list[Article]:
    return [parse_front_matter(path) for path in iter_markdown_files(WIKI_DIR)]


def split_tags(value: str) -> list[str]:
    value = value.replace("，", ",").replace("；", ",").replace(";", ",")
    return [part.strip() for part in value.split(",") if part.strip()]


def slug_domain(slug: str) -> str:
    return slug.split("-", 1)[0] if "-" in slug else slug


def slug_topic_key(slug: str) -> str:
    parts = slug.split("-")
    return "-".join(parts[:-1]) if len(parts) > 1 and parts[-1].isdigit() else slug


def normalize_text(text: str) -> str:
    text = re.sub(r"```.*?```", "", text, flags=re.S)
    text = re.sub(r"^#+\s+.*$", "", text, flags=re.M)
    text = re.sub(r"[0-9a-zA-Z_\-`~!@#$%^&*()+=\[\]{}\\|;:'\",.<>/?，。！？、；：“”‘’（）《》【】\s]+", "", text)
    for term in [
        "用途",
        "适用场景",
        "准备材料",
        "操作步骤",
        "核心原则",
        "判断标准",
        "检查点",
        "风险提示",
        "常见错误",
        "停止条件",
        "替代方案",
        "记录建议",
        "备注",
        "原文未单列准备材料执行前按下方操作步骤准备所需物品",
        "原文未单列替代方案本次不新增处置建议",
    ]:
        text = text.replace(term, "")
    return text


def canonical_body_text(article: Article) -> str:
    """Normalize a body to catch title-swapped template clones."""
    text = article.content
    text = re.sub(r"^#\s+.*$", "", text, flags=re.M)
    text = re.sub(r"## 对应 Guide\n.*?(?=\n## |\Z)", "", text, flags=re.S)
    text = re.sub(r"## Kiwix/ZIM 可继续查询\n.*?(?=\n## |\Z)", "", text, flags=re.S)
    if article.title:
        text = text.replace(article.title, "<TITLE>")
    text = re.sub(r"「[^」]{2,50}」", "「<TITLE>」", text)
    text = re.sub(r"“[^”]{2,80}”", "“<TITLE>”", text)
    return normalize_text(text)


def shingles(text: str, size: int = 4) -> set[str]:
    normalized = normalize_text(text)
    if len(normalized) <= size:
        return {normalized} if normalized else set()
    return {normalized[i : i + size] for i in range(len(normalized) - size + 1)}


def jaccard(left: set[str], right: set[str]) -> float:
    if not left or not right:
        return 0.0
    return len(left & right) / len(left | right)


def issue_external_dependency(content: str, term: str, article_title: str) -> bool:
    """Return true only when a forbidden external term appears as an action path."""
    action_markers = ["应", "先", "优先", "建议", "需要", "立即", "尽快", "可以", "必须"]
    negation_markers = ["不要", "不应", "不能", "不可", "避免", "不依赖", "不作为", "不得", "无法"]

    for raw_line in content.splitlines():
        line = raw_line.strip()
        if term not in line:
            continue
        if article_title and article_title in line:
            continue
        if not line or line.startswith("#") or line.startswith("- " + term) or line == article_title:
            continue
        if any(marker in line for marker in negation_markers):
            continue
        if any(marker in line for marker in action_markers):
            return True
    return False


def content_hash(content: str) -> str:
    normalized = re.sub(r"\s+", "\n", content.strip())
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def sqlite_tables(db_path: Path) -> set[str]:
    if not db_path.exists():
        return set()
    with sqlite3.connect(db_path) as con:
        return {row[0] for row in con.execute("select name from sqlite_master where type='table'")}


def load_pb_data(db_path: Path) -> tuple[dict[str, dict[str, str]], dict[str, str]]:
    if not db_path.exists():
        return {}, {}

    with sqlite3.connect(db_path) as con:
        categories = {
            row[0]: row[1]
            for row in con.execute("select id, name from wiki_categories")
        }
        articles: dict[str, dict[str, str]] = {}
        for row in con.execute(
            """
            select id, title, slug, category, summary, content, tags, risk_level, status, source
            from wiki_articles
            """
        ):
            article_id, title, slug, category_id, summary, content, tags, risk_level, status, source = row
            articles[slug] = {
                "id": article_id,
                "title": title,
                "slug": slug,
                "category": categories.get(category_id, category_id),
                "summary": summary,
                "content": content,
                "tags": tags,
                "risk_level": risk_level,
                "status": status,
                "source": source,
            }
        return articles, categories


def check_metadata(articles: list[Article], category_names: set[str]) -> list[dict[str, str]]:
    issues: list[dict[str, str]] = []
    slug_counts = Counter(article.slug for article in articles if article.slug)
    title_counts = Counter(article.title for article in articles if article.title)

    for article in articles:
        rel = str(article.path.relative_to(ROOT))

        if not article.meta:
            issues.append({"level": "error", "file": rel, "issue": "缺少或无法解析 front matter"})
            continue

        for key in REQUIRED_META:
            if not article.meta.get(key):
                issues.append({"level": "error", "file": rel, "issue": f"缺少必填字段 {key}"})

        if article.slug and article.path.stem != article.slug:
            issues.append({"level": "error", "file": rel, "issue": "文件名 stem 与 slug 不一致"})

        if article.slug:
            if not SLUG_RE.match(article.slug):
                issues.append({"level": "error", "file": rel, "issue": "slug 不符合正则 ^[a-z0-9]+(?:-[a-z0-9]+)*-[0-9]{3}$"})
            domain = slug_domain(article.slug)
            if domain not in ALLOWED_DOMAINS:
                issues.append({"level": "error", "file": rel, "issue": f"slug domain 不在固定列表：{domain}"})
            if slug_counts[article.slug] > 1:
                issues.append({"level": "error", "file": rel, "issue": f"slug 重复：{article.slug}"})

        if article.title and title_counts[article.title] > 1:
            issues.append({"level": "warning", "file": rel, "issue": f"title 重复：{article.title}"})

        if article.category and article.category not in category_names:
            issues.append({"level": "error", "file": rel, "issue": f"category 不存在于 PocketBase wiki_categories：{article.category}"})

        if article.risk_level and article.risk_level not in ALLOWED_RISK_LEVELS:
            issues.append({"level": "error", "file": rel, "issue": f"risk_level 非法：{article.risk_level}"})

        if article.status and article.status not in ALLOWED_STATUSES:
            issues.append({"level": "error", "file": rel, "issue": f"status 非法：{article.status}"})

        if article.summary:
            length = len(article.summary)
            if length < 35 or length > 110:
                issues.append({"level": "warning", "file": rel, "issue": f"summary 长度偏离 40-80 字左右：{length} 字"})
            if article.title and (
                article.summary.startswith(article.title + "：")
                or article.summary.startswith(article.title + ":")
            ):
                issues.append({"level": "warning", "file": rel, "issue": "summary 不应重复 title 开头"})
            for term in SUMMARY_TEMPLATE_TERMS:
                if term in article.summary:
                    issues.append({"level": "warning", "file": rel, "issue": f"summary 含模板化描述：{term}"})

        if article.meta.get("tags") and len(split_tags(article.meta["tags"])) < 3:
            issues.append({"level": "error", "file": rel, "issue": "tags 少于 3 个"})

    return issues


def check_content(articles: list[Article]) -> list[dict[str, str]]:
    issues: list[dict[str, str]] = []

    for article in articles:
        rel = str(article.path.relative_to(ROOT))
        content = article.content.strip()
        headings = set(H2_RE.findall(content))
        h1_match = H1_RE.search(content)

        if not content:
            issues.append({"level": "error", "file": rel, "issue": "正文为空"})
            continue

        if h1_match and article.title and h1_match.group(1).strip() != article.title:
            issues.append({"level": "warning", "file": rel, "issue": "正文 H1 与 front matter title 不一致"})
        if not h1_match:
            issues.append({"level": "warning", "file": rel, "issue": "正文缺少 H1"})

        required_heading_groups = {
            "明确用途": ["用途", "目的"],
            "具体操作": ["操作步骤", "步骤", "做法"],
            "判断标准": ["判断标准", "检查点"],
            "风险提示": ["风险提示", "停止条件", "常见错误"],
        }
        for name, options in required_heading_groups.items():
            if not any(option in headings for option in options):
                issues.append({"level": "error", "file": rel, "issue": f"正文缺少最低要求：{name}"})

        compact_len = len(re.sub(r"\s+", "", content))
        if compact_len < 300:
            issues.append({"level": "warning", "file": rel, "issue": f"正文低于建议短条目长度：{compact_len} 字"})
        if compact_len > 5000:
            issues.append({"level": "warning", "file": rel, "issue": f"正文很长，可能需要拆分：{compact_len} 字"})

        if article.risk_level in {"caution", "high"} and "停止条件" not in content and "不适用" not in content:
            issues.append({"level": "error", "file": rel, "issue": "中高风险条目缺少停止条件或不适用边界"})

        for term in EXTERNAL_DEPENDENCY_TERMS:
            if issue_external_dependency(content, term, article.title):
                issues.append({"level": "warning", "file": rel, "issue": f"正文含外部支援依赖表达：{term}"})

        for term in ABSOLUTE_TERMS:
            if term in content:
                issues.append({"level": "warning", "file": rel, "issue": f"正文含绝对化表达：{term}"})

        placeholder_hits = [term for term in PLACEHOLDER_TERMS if term in content]
        if placeholder_hits:
            issues.append({"level": "advisory", "file": rel, "issue": "含占位式补充语：" + "；".join(placeholder_hits)})

    return issues


def compare_with_pb(articles: list[Article], pb_articles: dict[str, dict[str, str]]) -> list[dict[str, str]]:
    issues: list[dict[str, str]] = []
    file_by_slug = {article.slug: article for article in articles if article.slug}

    for slug, article in file_by_slug.items():
        rel = str(article.path.relative_to(ROOT))
        pb = pb_articles.get(slug)
        if not pb:
            issues.append({"level": "error", "file": rel, "issue": "Markdown 中存在，但 PocketBase wiki_articles 缺失"})
            continue

        fields = ["title", "category", "summary", "tags", "risk_level", "status", "source"]
        for field in fields:
            if article.meta.get(field, "") != pb.get(field, ""):
                issues.append({"level": "error", "file": rel, "issue": f"Markdown 与 PocketBase 字段不一致：{field}"})

        if article.content.strip() != pb.get("content", "").strip():
            issues.append({"level": "error", "file": rel, "issue": "Markdown 正文与 PocketBase content 不一致"})

    for slug in sorted(set(pb_articles) - set(file_by_slug)):
        issues.append({"level": "error", "file": f"PocketBase:{slug}", "issue": "PocketBase 中存在，但 Markdown 源文件缺失"})

    return issues


def find_overlap(articles: list[Article]) -> tuple[list[dict[str, str]], list[tuple[str, list[Article]]]]:
    issues: list[dict[str, str]] = []
    hash_groups: dict[str, list[Article]] = defaultdict(list)
    canonical_body_groups: dict[tuple[str, str], list[Article]] = defaultdict(list)
    title_summary_groups: dict[str, list[Article]] = defaultdict(list)
    topic_groups: dict[str, list[Article]] = defaultdict(list)

    for article in articles:
        if not article.slug:
            continue
        hash_groups[content_hash(article.content)].append(article)
        canonical_body_groups[(slug_domain(article.slug), canonical_body_text(article))].append(article)
        key = normalize_text(article.title + article.summary)
        title_summary_groups[key].append(article)
        topic_groups[slug_topic_key(article.slug)].append(article)

    for group in hash_groups.values():
        if len(group) > 1:
            issues.append({
                "level": "error",
                "file": ", ".join(str(item.path.relative_to(ROOT)) for item in group),
                "issue": "正文完全重复",
            })

    for group in title_summary_groups.values():
        if len(group) > 1:
            issues.append({
                "level": "warning",
                "file": ", ".join(str(item.path.relative_to(ROOT)) for item in group),
                "issue": "title+summary 完全重复",
            })

    canonical_clone_slugs: set[str] = set()
    canonical_clones = [
        sorted(group, key=lambda item: item.slug)
        for group in canonical_body_groups.values()
        if len(group) > 1
    ]
    canonical_clones.sort(key=lambda group: (-len(group), group[0].slug))
    for group in canonical_clones:
        canonical_clone_slugs.update(item.slug for item in group)
        shown = ", ".join(item.slug for item in group[:16])
        if len(group) > 16:
            shown += f", ... +{len(group) - 16}"
        issues.append({
            "level": "warning",
            "file": f"{slug_domain(group[0].slug)} canonical clone group",
            "issue": (
                "主体内容高度重复，仅标题/主题词不同；建议合并为一个总条目，"
                f"或为每个 slug 重写独立边界：{len(group)} 篇；{shown}"
            ),
        })

    shingle_cache = {
        article.slug: shingles(article.title + "\n" + article.summary + "\n" + article.content[:2500])
        for article in articles
        if article.slug
    }
    sorted_articles = [article for article in articles if article.slug]
    adjacency: dict[str, set[str]] = defaultdict(set)
    for index, left in enumerate(sorted_articles):
        if left.slug in canonical_clone_slugs:
            continue
        left_domain = slug_domain(left.slug)
        for right in sorted_articles[index + 1 :]:
            if right.slug in canonical_clone_slugs:
                continue
            if slug_domain(right.slug) != left_domain:
                continue
            score = jaccard(shingle_cache[left.slug], shingle_cache[right.slug])
            if score >= 0.82:
                adjacency[left.slug].add(right.slug)
                adjacency[right.slug].add(left.slug)

    slug_to_article = {article.slug: article for article in sorted_articles}
    seen: set[str] = set()
    for slug in sorted(adjacency):
        if slug in seen:
            continue
        stack = [slug]
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
        members = [slug_to_article[item] for item in sorted(component)]
        domain = slug_domain(members[0].slug)
        shown = ", ".join(item.slug for item in members[:12])
        if len(members) > 12:
            shown += f", ... +{len(members) - 12}"
        issues.append({
            "level": "warning",
            "file": f"{domain} cluster",
            "issue": f"同 domain 高相似簇，可能存在模板化重叠：{len(members)} 篇；{shown}",
        })

    notable_topic_groups = [
        (key, sorted(group, key=lambda item: item.slug))
        for key, group in topic_groups.items()
        if len(group) > 1
    ]
    notable_topic_groups.sort(key=lambda item: (-len(item[1]), item[0]))
    return issues, notable_topic_groups


def render_issue_table(issues: list[dict[str, str]], limit: int | None = None) -> list[str]:
    rows = ["|级别|文件|问题|", "|---|---|---|"]
    selected = issues[:limit] if limit else issues
    for issue in selected:
        rows.append(f"|{issue['level']}|`{issue['file']}`|{issue['issue']}|")
    if limit and len(issues) > limit:
        rows.append(f"|...|...|另有 {len(issues) - limit} 条，详见脚本输出或调高报告限制|")
    return rows


def render_report(
    articles: list[Article],
    metadata_issues: list[dict[str, str]],
    content_issues: list[dict[str, str]],
    pb_issues: list[dict[str, str]],
    overlap_issues: list[dict[str, str]],
    topic_groups: list[tuple[str, list[Article]]],
    pb_articles: dict[str, dict[str, str]],
    category_names: set[str],
    lanternbox_tables: set[str],
) -> str:
    all_blocking = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "error"
    ]
    warnings = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "warning"
    ]
    advisories = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "advisory"
    ]

    slug_domains = Counter(slug_domain(article.slug) for article in articles if article.slug)
    categories = Counter(article.category for article in articles if article.category)
    canonical_clone_issues = [
        issue
        for issue in overlap_issues
        if "主体内容高度重复" in issue["issue"]
    ]
    canonical_clone_article_count = 0
    for issue in canonical_clone_issues:
        match = re.search(r"：(\d+) 篇", issue["issue"])
        if match:
            canonical_clone_article_count += int(match.group(1))

    lines: list[str] = []
    lines.append(f"# Wiki Audit Report ({date.today().isoformat()})")
    lines.append("")
    lines.append("## 结论")
    if all_blocking:
        lines.append(f"- 发现 {len(all_blocking)} 个阻断问题，需要修复后再作为稳定起点。")
    else:
        lines.append("- 未发现阻断级问题：slug 唯一性、必填字段、分类引用、Markdown 与 PocketBase 内容一致性均通过。")
    lines.append(f"- 发现 {len(warnings)} 个 warning，需要人工复核；发现 {len(advisories)} 个 advisory，多为内容可进一步增强项。")
    if canonical_clone_issues:
        lines.append(
            f"- 内容层面发现 {len(canonical_clone_issues)} 个主体重复组，覆盖 {canonical_clone_article_count} 篇。"
            "这些条目通常只是替换了标题/主题词，建议在进入稳定知识库前合并或重写。"
        )
    lines.append("- 未发现正文完全重复或 title+summary 完全重复。"
                 if not any("完全重复" in issue["issue"] for issue in overlap_issues)
                 else "- 存在完全重复项，见下方范围重叠检查。")
    lines.append("")

    lines.append("## 覆盖范围")
    lines.append(f"- Markdown wiki 条目：{len(articles)}")
    lines.append(f"- PocketBase wiki_articles：{len(pb_articles)}")
    lines.append(f"- PocketBase wiki_categories：{len(category_names)}")
    lines.append(f"- data/lanternbox.db 表：{', '.join(sorted(lanternbox_tables)) or '未找到'}")
    if not {"wiki_articles", "wiki_categories"} & lanternbox_tables:
        lines.append("- 说明：`data/lanternbox.db` 不承载 wiki；当前 wiki 数据库为 `pocketbase/pb_data/data.db`。")
    lines.append("")

    lines.append("## Slug Domain 分布")
    lines.append("|domain|数量|")
    lines.append("|---|---:|")
    for domain, count in sorted(slug_domains.items()):
        lines.append(f"|{domain}|{count}|")
    lines.append("")

    lines.append("## 分类分布")
    lines.append("|category|数量|")
    lines.append("|---|---:|")
    for category, count in sorted(categories.items()):
        lines.append(f"|{category}|{count}|")
    lines.append("")

    lines.append("## 元数据与 Slug 检查")
    if metadata_issues:
        lines.extend(render_issue_table(metadata_issues, limit=80))
    else:
        lines.append("- 通过：必填字段、slug 正则、slug domain、文件名一致性、分类引用、risk_level/status、tag 数量均未发现问题。")
    lines.append("")

    lines.append("## 正文内容检查")
    blocking_content = [issue for issue in content_issues if issue["level"] != "advisory"]
    if blocking_content:
        lines.extend(render_issue_table(blocking_content, limit=120))
    else:
        lines.append("- 通过：每篇条目均覆盖用途、操作、判断标准、风险提示；中高风险条目均含停止条件或不适用边界；未发现外部依赖或绝对化表达。")
    if advisories:
        lines.append("")
        lines.append("### 内容增强建议")
        lines.append(f"- {len(advisories)} 篇含占位式补充语，主要是“准备材料/替代方案未单列”。这不是入库阻断项，但后续可逐步补实。")
    lines.append("")

    lines.append("## 文件与数据库一致性")
    if pb_issues:
        lines.extend(render_issue_table(pb_issues, limit=120))
    else:
        lines.append(f"- 通过：{len(articles)} 个 Markdown slug 与 PocketBase wiki_articles 一一对应，标题、分类、摘要、标签、风险等级、状态、来源和正文均一致。")
    lines.append("")

    lines.append("## 重复与范围重叠")
    if canonical_clone_issues:
        lines.append(
            "- 下面的 `canonical clone group` 是最明确的修复清单：脚本去掉 H1、对应 Guide、Kiwix 列表，"
            "并把标题短语替换为占位符后，主体仍然相同。"
        )
        lines.append("- 建议处理优先级：先处理大组；每组选择“合并为一个总条目 + 删除/弃用重复 slug”，或“保留 slug 但重写成独立适用场景、操作步骤、判断标准和停止条件”。")
        lines.append("")
    if overlap_issues:
        lines.extend(render_issue_table(overlap_issues, limit=120))
    else:
        lines.append("- 通过：未发现 slug 重复、正文完全重复、title+summary 完全重复或同 domain 高相似条目。")
    lines.append("")
    lines.append("### 主题族边界复核")
    if topic_groups:
        lines.append("- 以下为同一 `domain-topic` 下有多个序号的主题族；这通常是合理拆分，不自动判为重复，但适合人工做范围边界抽检。")
        lines.append("|topic key|数量|slugs|")
        lines.append("|---|---:|---|")
        for key, group in topic_groups[:80]:
            lines.append(f"|`{key}`|{len(group)}|{', '.join('`' + item.slug + '`' for item in group)}|")
        if len(topic_groups) > 80:
            lines.append(f"|...|...|另有 {len(topic_groups) - 80} 个主题族|")
    else:
        lines.append("- 未发现多序号主题族。")
    lines.append("")

    lines.append("## 复跑方式")
    lines.append("```bash")
    lines.append("python3 tools/audit_wiki.py")
    lines.append("```")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    args = parser.parse_args()

    articles = load_articles()
    pb_articles, category_by_id = load_pb_data(PB_DB)
    category_names = set(category_by_id.values())
    lanternbox_tables = sqlite_tables(ROOT / "data" / "lanternbox.db")

    metadata_issues = check_metadata(articles, category_names)
    content_issues = check_content(articles)
    pb_issues = compare_with_pb(articles, pb_articles)
    overlap_issues, topic_groups = find_overlap(articles)

    report = render_report(
        articles,
        metadata_issues,
        content_issues,
        pb_issues,
        overlap_issues,
        topic_groups,
        pb_articles,
        category_names,
        lanternbox_tables,
    )

    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(report, encoding="utf-8")

    errors = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "error"
    ]
    warnings = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "warning"
    ]
    advisories = [
        issue
        for issue in metadata_issues + content_issues + pb_issues + overlap_issues
        if issue["level"] == "advisory"
    ]

    print(f"Report: {args.report}")
    print(f"Articles: markdown={len(articles)} pocketbase={len(pb_articles)} categories={len(category_names)}")
    print(f"Issues: errors={len(errors)} warnings={len(warnings)} advisories={len(advisories)}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
