import unittest
from unittest.mock import patch

from api.response.prompts import _format_guide_evidence, _format_wiki_evidence
from api.response.safety import sanitize_ai_answer
from api.retrieval_v2.fetchers import apply_kiwix_semantic_policy, fetch_related_wiki_candidates
from api.retrieval_v2.orchestrator import _ensure_planned_guide_selected
from api.retrieval_v2.schemas import EvidenceCandidate, EvidenceSelection, RetrievalPlan, SourcePlanItem
from api.services.wiki_service import normalize_wiki_articles_for_ai


class RetrievalRootContractTests(unittest.TestCase):
    def test_wiki_normalization_preserves_slug_and_record_id(self):
        item = normalize_wiki_articles_for_ai([{
            "id": "pb123", "slug": "water-test-001", "title": "测试",
            "category": "water", "risk_level": "high", "guide_links": ["DG-0001"],
        }])[0]
        self.assertEqual(item["id"], "water-test-001")
        self.assertEqual(item["slug"], "water-test-001")
        self.assertEqual(item["record_id"], "pb123")
        self.assertEqual(item["guide_links"], ["DG-0001"])

    @patch("api.services.wiki_service.get_wiki_articles_by_slugs_for_ai")
    def test_related_wiki_candidate_has_traceable_contract(self, loader):
        loader.return_value = [{
            "id": "water-test-001", "slug": "water-test-001", "record_id": "pb123",
            "title": "测试 Wiki", "category": "water", "risk_level": "caution",
            "summary": "判断边界", "content": "正文", "guide_links": [],
        }]
        guide = EvidenceCandidate(
            source_type="guide", id="DG-0001", title="测试 Guide",
            raw={"related_wiki": ["water-test-001"], "risk_level": "high"},
        )
        candidate = fetch_related_wiki_candidates([guide])[0]
        self.assertEqual(candidate.id, "water-test-001")
        self.assertEqual(candidate.raw["source_reason"], "guide_related_wiki")
        self.assertEqual(candidate.raw["source_role"], "guide_support")
        self.assertEqual(candidate.raw["linked_guide_id"], "DG-0001")
        self.assertIn("DG-0001", candidate.raw["guide_links"])

    def test_guide_prompt_exposes_risk_and_action_contract(self):
        text = _format_guide_evidence([{
            "id": "DG-0001", "title": "测试", "category": "water", "domains": ["water"],
            "priority": "P0", "risk_level": "high", "scenario": "异味", "goal": "停用",
            "tools": ["标签"], "steps": ["隔离"], "check": ["仍有异味"],
            "common_mistakes": ["试饮"], "fallback": ["改用储水"],
            "stop_or_escalate": ["无法确认时停止"], "related_wiki": ["water-test-001"],
        }])
        for term in ["DG-0001", "风险等级", "high", "判断标准", "本地降级方案", "停止", "water-test-001"]:
            self.assertIn(term, text)

    def test_wiki_prompt_exposes_slug_and_relation_reason(self):
        text = _format_wiki_evidence([{
            "slug": "water-test-001", "title": "测试", "source_reason": "guide_related_wiki",
            "linked_guide_id": "DG-0001", "summary": "判断边界",
        }])
        self.assertIn("water-test-001", text)
        self.assertIn("guide_related_wiki", text)
        self.assertIn("DG-0001", text)

    def test_external_dependency_filter_replaces_default_but_keeps_negation(self):
        cleaned = sanitize_ai_answer("- 绕行或等待救援。\n- 不要等待救援才行动。", "emergency")
        self.assertIn("现场可控", cleaned)
        self.assertIn("不要等待救援才行动", cleaned)
        self.assertNotIn("绕行或等待救援", cleaned)

    def test_kiwix_gas_profile_excludes_transport_homonym(self):
        candidate = EvidenceCandidate(
            source_type="kiwix", id="k1", title="RTL型美铁燃气轮机动车组",
            summary="铁路车辆历史", snippet="一种燃气轮机列车", raw={},
        )
        result = apply_kiwix_semantic_policy([candidate], "房间有燃气味，解释为什么不能开灯")[0]
        self.assertIn("gas_safety", result.raw["semantic_excluded_reason"])

    def test_kiwix_gas_profile_excludes_dated_incident(self):
        candidate = EvidenceCandidate(
            source_type="kiwix", id="k2", title="2022年某地燃气泄漏爆燃事故",
            summary="事故经过", snippet="燃气泄漏", raw={},
        )
        result = apply_kiwix_semantic_policy([candidate], "解释燃气泄漏为什么不能开灯")[0]
        self.assertIn("gas_safety", result.raw["semantic_excluded_reason"])

    def test_planned_guide_contract_keeps_top_guide(self):
        guide = EvidenceCandidate(source_type="guide", id="DG-0001", title="行动卡", raw={})
        wiki = EvidenceCandidate(source_type="wiki", id="wiki-001", title="背景", raw={})
        plan = RetrievalPlan(source_plan=[SourcePlanItem(source_type="guide")])
        selected, selection = _ensure_planned_guide_selected(
            plan=plan, candidates=[guide, wiki], selected_evidence=[wiki],
            selection=EvidenceSelection(),
        )
        self.assertIn(guide, selected)
        self.assertEqual(selection.selected[0].id, "DG-0001")


if __name__ == "__main__":
    unittest.main()
