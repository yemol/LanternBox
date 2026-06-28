import re
from typing import Any, Dict, Optional, List

from ..config import POCKETBASE_WIKI_ARTICLES
from ..pocketbase_client import pocketbase_get_records

WIKI_DOMAIN_TERMS = {
    "water": ["饮用水", "储水", "停水", "水源", "净水", "过滤", "消毒"],
    "food": ["食物", "储存", "粮食", "保质", "发霉", "烹饪"],
    "power": ["移动电源", "充电宝", "电量", "低电量", "手机", "充电", "断电", "照明", "手电", "通讯"],
    "medical": ["伤口", "发烧", "腹泻", "药品", "消毒", "隔离"],
    "safety": ["避难", "安全", "撤离", "火灾", "燃气", "风险"],
    "tools": ["工具", "维修", "修理", "手锯", "螺丝刀", "胶带"],
    "planting": ["种植", "种子", "土豆", "番茄", "育苗", "土壤"],
}

WIKI_STOP_TERMS = {
    "怎么办", "怎么", "如何", "一下", "现在", "已经", "是不是",
    "可以", "需要", "应该", "感觉", "问题", "情况",
    "这个", "那个", "我们", "家里", "不多", "都有",
}

def trim_text(text: str, max_chars: int = 800) -> str:
    text = (text or "").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "……"

def clean_wiki_search_term(term: str) -> str:
    term = (term or "").strip()
    term = re.sub(r"[，。！？、；：,.!?;:\s]+", "", term)
    return term

def filter_related_wikis_for_query(related_wikis, query_profile):
    """v0.7：按 query intent 轻量过滤 Wiki 噪声。
    这里只做标题级别收敛，不影响指南召回。
    """
    if not related_wikis:
        return related_wikis

    intents = set(query_profile.get("intents", []) if query_profile else [])

    def title_of(item):
        return str(item.get("title", ""))

    if "medical_dehydration_diarrhea" in intents:
        bad = ["停水后的用水优先级", "简易过滤", "煮沸饮水", "水质异常", "储水容器"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    if "power_water_electrical_safety" in intents:
        bad = ["冰箱食物", "食物变质", "干粮", "罐头", "雨水收集", "饮用水"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    if "shelter_damp_mold" in intents:
        bad = ["洪水", "撤离", "撤高", "干粮", "罐头"]
        return [
            item for item in related_wikis
            if not any(word in title_of(item) for word in bad)
        ]

    return related_wikis

def search_wiki_for_ai(
    keyword: str,
    detected_domains: Optional[List[str]] = None,
    limit: int = 3,
) -> List[dict]:
    keyword = (keyword or "").strip()

    if not keyword:
        return []

    # 第一层：先用原句直接搜索
    direct_articles = search_wiki_articles(keyword, limit=limit)

    if direct_articles:
        return normalize_wiki_articles_for_ai(direct_articles)

    # 第二层：原句搜不到，再拆动态搜索词
    terms = build_wiki_search_terms(
        text=keyword,
        detected_domains=detected_domains,
        max_terms=8,
    )

    if not terms:
        return []

    wiki_filter = build_wiki_or_filter_for_terms(terms)


    data = pocketbase_get_records(
        POCKETBASE_WIKI_ARTICLES,
        params={
            "page": 1,
            "perPage": limit,
            "sort": "-updated",
            "filter": wiki_filter,
        }
    )

    articles = [
        normalize_wiki_article(item)
        for item in data.get("items", [])
    ]

    return normalize_wiki_articles_for_ai(articles)

def search_wiki_articles(keyword: str, limit: int = 20) -> list[dict]:
    keyword = keyword.strip()

    if not keyword:
        return []

    safe_keyword = keyword.replace('"', '\\"')

    data = pocketbase_get_records(
        POCKETBASE_WIKI_ARTICLES,
        params={
            "page": 1,
            "perPage": limit,
            "sort": "-updated",
            "filter": (
                f'status = "published" && '
                f'(title ~ "{safe_keyword}" || '
                f'summary ~ "{safe_keyword}" || '
                f'content ~ "{safe_keyword}" || '
                f'tags ~ "{safe_keyword}")'
            ),
        }
    )

    return [
        normalize_wiki_article(item)
        for item in data.get("items", [])
    ]

def normalize_wiki_articles_for_ai(articles: List[dict]) -> List[dict]:
    results = []

    for article in articles:
        results.append({
            "id": article.get("id"),
            "title": article.get("title", ""),
            "summary": article.get("summary", ""),
            "content": trim_text(article.get("content", ""), 3000),
            "tags": article.get("tags", ""),
            "risk_level": article.get("risk_level", "normal"),
            "source": article.get("source", ""),
        })

    return results

def normalize_wiki_article(item: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "id": item.get("id"),
        "title": item.get("title", ""),
        "slug": item.get("slug", ""),
        "category": item.get("category", ""),
        "summary": item.get("summary", ""),
        "content": item.get("content", ""),
        "tags": item.get("tags", ""),
        "risk_level": item.get("risk_level", "normal"),
        "status": item.get("status", ""),
        "source": item.get("source", ""),
        "updated_note": item.get("updated_note", ""),
        "created": item.get("created", ""),
        "updated": item.get("updated", ""),
    }

def build_wiki_search_terms(
    text: str,
    detected_domains: Optional[List[str]] = None,
    max_terms: int = 8,
) -> List[str]:
    text = (text or "").strip()
    detected_domains = detected_domains or []

    terms: List[str] = []

    def add_term(term: str):
        term = clean_wiki_search_term(term)

        if not term:
            return

        if term in WIKI_STOP_TERMS:
            return

        if len(term) < 2:
            return

        if term not in terms:
            terms.append(term)

    # 1. 领域关键词优先：只加入用户句子中实际出现的领域词
    for domain in detected_domains:
        for word in WIKI_DOMAIN_TERMS.get(domain, []):
            if word in text:
                add_term(word)

    # 2. 通用领域词：即使 detected_domains 没传，也从全文里找明显词
    for words in WIKI_DOMAIN_TERMS.values():
        for word in words:
            if word in text:
                add_term(word)

    # 3. 提取连续中文片段
    chinese_chunks = re.findall(r"[\u4e00-\u9fff]{2,12}", text)

    for chunk in chinese_chunks:
        if len(chunk) <= 6:
            add_term(chunk)
        else:
            # 对较长句子切成 2-4 字窗口，避免整句搜不到
            for size in (4, 3, 2):
                for i in range(0, len(chunk) - size + 1):
                    add_term(chunk[i:i + size])
                    if len(terms) >= max_terms:
                        return terms[:max_terms]

    # 4. 最后兜底：原句前 12 个中文字符
    if not terms:
        fallback = "".join(chinese_chunks)[:12]
        add_term(fallback)

    return terms[:max_terms]

def build_wiki_or_filter_for_terms(terms: List[str]) -> str:
    parts = []

    for term in terms:
        safe_term = term.replace('"', '\\"')

        parts.append(
            "("
            f'title~"{safe_term}" || '
            f'summary~"{safe_term}" || '
            f'content~"{safe_term}" || '
            f'tags~"{safe_term}"'
            ")"
        )

    if not parts:
        return 'status="published"'

    return 'status="published" && (' + " || ".join(parts) + ")"

def get_wiki_categories_records() -> Dict[str, Any]:
    params = {
        "page": 1,
        "perPage": 100,
        "sort": "sort_order,name",
    }

    return pocketbase_get_records(
        "wiki_categories",
        params=params,
    )

def get_wiki_articles_by_category_records(
    category_id: str,
    *,
    page: int = 1,
    per_page: int = 10,
) -> Dict[str, Any]:
    page = max(page, 1)
    per_page = min(max(per_page, 1), 50)

    params = {
        "page": page,
        "perPage": per_page,
        "sort": "title",
        "filter": f'status = "published" && category = "{category_id}"',
    }

    return pocketbase_get_records(
        "wiki_articles",
        params=params,
    )