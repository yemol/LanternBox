import unittest

from api.retrieval_v2.orchestrator import _build_source_outputs
from api.retrieval_v2.schemas import EvidenceCandidate, EvidenceSelection, SelectedEvidence


class RetrievalTraceabilityTests(unittest.TestCase):
    def test_source_outputs_expose_kiwix_traceability_and_gap_note(self) -> None:
        candidates = [
            EvidenceCandidate(
                source_type="wiki",
                id="wiki:test",
                title="测试 Wiki",
                summary="本地知识说明",
                raw={"source_id": "wiki:test", "label": "Wiki"},
            ),
            EvidenceCandidate(
                source_type="kiwix",
                id="kiwix:test",
                title="背景资料",
                summary="背景补充",
                raw={
                    "source_id": "kiwix:test",
                    "label": "Kiwix / ZIM",
                    "zim_filename": "example.zim",
                    "article_path": "A/Test",
                    "usage_policy": "background_selectable",
                    "source_role": "background",
                    "language": "zh",
                    "relevance_score": 0.82,
                    "matched_terms": ["背景", "补充"],
                },
            ),
        ]
        selection = EvidenceSelection(
            selected=[SelectedEvidence(source_type="kiwix", id="kiwix:test", reason="背景补充")],
            excluded=[],
            answer_focus=["优先参考本地指南和 Wiki"],
            raw={},
        )

        selected_sources, excluded_sources, retrieval_decision = _build_source_outputs(
            candidates=candidates,
            selected_evidence=[candidates[1]],
            selection=selection,
        )

        self.assertEqual(len(selected_sources), 1)
        self.assertEqual(selected_sources[0]["source_type"], "kiwix")
        self.assertIn("selection_reason", selected_sources[0])
        self.assertEqual(selected_sources[0]["selection_reason"], "背景补充")
        self.assertEqual(selected_sources[0]["usage_policy"], "background_selectable")
        self.assertEqual(selected_sources[0]["zim_filename"], "example.zim")
        self.assertIn("knowledge_gap_note", retrieval_decision)
        self.assertIn("Wiki", retrieval_decision["knowledge_gap_note"])
        self.assertEqual(excluded_sources, [])


if __name__ == "__main__":
    unittest.main()
