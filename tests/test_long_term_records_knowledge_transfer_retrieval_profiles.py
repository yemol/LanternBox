import json
from pathlib import Path

import pytest

from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_related_wiki_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem


ROOT = Path(__file__).resolve().parents[1]
PROFILE_PATH = ROOT / "data/retrieval_query_profiles.json"
RECORDS_PROFILES = {
    "records_capture_trust_summary",
    "records_post_event_review",
    "records_long_term_trend",
    "records_archive_index",
    "records_skill_evidence",
}


def _profile_names(question: str) -> set[str]:
    return {str(item["name"]) for item in _matching_query_profiles(question)}


def _guide_candidates(question: str):
    return fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )


def _guide_ids(question: str) -> list[str]:
    return [str(guide.id) for guide in _guide_candidates(question)]


def _related_wiki_slugs(question: str) -> set[str]:
    guides = _guide_candidates(question)[:3]
    return {
        str((wiki.raw or {}).get("slug") or wiki.id)
        for wiki in fetch_related_wiki_candidates(guides)
    }


def test_exactly_five_records_profiles_are_registered():
    data = json.loads(PROFILE_PATH.read_text(encoding="utf-8"))
    names = {
        str(profile["name"])
        for profile in data["profiles"]
        if str(profile.get("name", "")).startswith("records_")
    }
    assert names == RECORDS_PROFILES


@pytest.mark.parametrize(
    ("profile", "question", "expected_top1"),
    [
        (
            "records_capture_trust_summary",
            "这条设备自动记录不是我亲眼看到的，最少信息和 verification 应该怎么写？",
            {"DG-0912"},
        ),
        (
            "records_post_event_review",
            "刚才发生了一次险情，现场已经处理完并恢复安全，事后怎么记录原因和整改？",
            {"DG-0913"},
        ),
        (
            "records_long_term_trend",
            "过去几个月电池容量多次变化，我想比较历史记录找长期趋势。",
            {"DG-0914"},
        ),
        (
            "records_archive_index",
            "SD、纸本和 Core 都有 ARCHIVED 记录，怎么做长期索引和多介质备份？",
            {"DG-0917"},
        ),
        (
            "records_skill_evidence",
            "很久没练的 INDEPENDENT 技能已经过期，复训后怎么重新确认独立资格？",
            {"DG-0918"},
        ),
    ],
)
def test_records_profile_positive_boundaries(profile, question, expected_top1):
    assert profile in _profile_names(question)
    assert _guide_ids(question)[0] in expected_top1


@pytest.mark.parametrize(
    ("profile", "question"),
    [
        ("records_capture_trust_summary", "今天的无线电日志怎么交接？"),
        ("records_post_event_review", "near miss 正在发生，先做什么？"),
        ("records_post_event_review", "事故还没有恢复安全，正在大量流血，先做什么？"),
        ("records_post_event_review", "工坊事故还没有恢复安全，火花还在冒，先做什么？"),
        ("records_long_term_trend", "锯子现在坏了，怎么检查和修？"),
        ("records_archive_index", "同一个 SOP 有多个版本，哪个 CURRENT 可以执行？"),
        ("records_skill_evidence", "成员已经疲劳，还能继续高风险任务吗？"),
    ],
)
def test_records_profile_negative_boundaries(profile, question):
    assert profile not in _profile_names(question)


@pytest.mark.parametrize(
    ("question", "expected_top1"),
    [
        ("有人手臂正在大量流血，现在应该先做什么？", "DG-0010"),
        ("我要做一个木架，木板具体怎么切和连接？", "DG-0873"),
        ("锯子坏了，现在怎么检查和修？", "DG-0837"),
        ("储藏区今天换班，库存和异常交接卡应该怎么写？", "DG-0902"),
        ("下一班要接手工坊里还没完成的半成品，应该怎么交接？", "DG-0910"),
        ("今天电量不够，哪些设备先停，电池怎么分配？", "DG-0562"),
    ],
)
def test_current_domain_observations_keep_existing_primary(question, expected_top1):
    assert not (_profile_names(question) & RECORDS_PROFILES)
    assert _guide_ids(question)[0] == expected_top1


def test_energy_trend_can_keep_energy_context_with_records_evidence():
    question = "我想比较过去几个月电池可用容量和太阳能产出的变化趋势。"
    guide_ids = _guide_ids(question)
    assert "records_long_term_trend" in _profile_names(question)
    assert "DG-0914" in guide_ids[:3]
    assert "records-energy-capacity-trend-001" in _related_wiki_slugs(question)


def test_skill_co_domain_keeps_organization_top1_with_records_support():
    question = "一个成员只跟着做过几次，还没有独立完成过，现在应该算什么技能等级？"
    guide_ids = _guide_ids(question)
    assert guide_ids[0] == "DG-0400"
    assert "DG-0918" in guide_ids[:3]


def test_medical_observation_handover_debt_is_not_claimed_by_records_profiles():
    question = "这个病人的观察记录怎么交给下一位照护者？"
    assert not (_profile_names(question) & RECORDS_PROFILES)
    assert not (set(_guide_ids(question)[:3]) & {f"DG-{number:04d}" for number in range(911, 919)})
