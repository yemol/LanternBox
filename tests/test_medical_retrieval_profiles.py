from api.retrieval_v2.fetchers import (
    _matching_query_profiles,
    fetch_guide_candidates,
    fetch_wiki_candidates,
)
from api.retrieval_v2.schemas import SourcePlanItem


def _profile_names(question: str) -> set[str]:
    return {item["name"] for item in _matching_query_profiles(question)}


def _guide_candidates(question: str):
    return fetch_guide_candidates(
        SourcePlanItem(source_type="guide", query=question, limit=8),
        user_message=question,
    )


def _wiki_ids(question: str) -> set[str]:
    return {
        wiki.id
        for wiki in fetch_wiki_candidates(
            SourcePlanItem(source_type="wiki", query=question, limit=8),
            user_message=question,
        )
    }


def _guide_ids(question: str) -> list[str]:
    return [guide.id for guide in _guide_candidates(question)]


def test_choking_no_breathing_prefers_medical_airway_chain():
    question = "有人噎住不能呼吸怎么办"
    assert "medical_airway_breathing_emergency" in _profile_names(question)

    guides = _guide_candidates(question)
    assert "DG-0013" in [guide.id for guide in guides[:4]]

    choking = next(guide for guide in guides if guide.id == "DG-0013")
    assert "medical-choking-airway-obstruction-001" in set(
        choking.raw.get("related_wiki") or []
    )
    assert "medical-choking-airway-obstruction-001" in _wiki_ids(question)


def test_unresponsive_but_breathing_prefers_recovery_position():
    question = "人没有反应但还有呼吸"
    assert "medical_airway_breathing_emergency" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0014"
    assert "medical-recovery-position-breathing-observation-001" in _wiki_ids(question)


def test_no_normal_breathing_prefers_cpr_boundary():
    question = "没有正常呼吸怎么办"
    assert "medical_airway_breathing_emergency" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    assert guides[0].id == "DG-0015"
    assert "medical-cpr-no-normal-breathing-001" in _wiki_ids(question)


def test_major_bleeding_enters_critical_bleeding_guide_chain():
    question = "大量出血怎么处理"
    assert "medical_trauma_bleeding_fracture" in _profile_names(question)

    guides = _guide_candidates(question)
    assert "DG-0004" in [guide.id for guide in guides[:4]]

    bleeding = next(guide for guide in guides if guide.id == "DG-0004")
    related = set(bleeding.raw.get("related_wiki") or [])
    assert "medical-bleeding-control-001" in related
    assert "medical-bleeding-control-002" in related


def test_possible_fracture_no_reposition_prefers_fracture_guide():
    question = "腿可能骨折不能掰怎么办"
    assert "medical_trauma_bleeding_fracture" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    fracture = next((guide for guide in guides[:3] if guide.id == "DG-0009"), None)
    assert fracture is not None
    assert "medical-fracture-immobilize-no-reposition-001" in set(
        fracture.raw.get("related_wiki") or []
    )


def test_battery_leak_near_eye_prefers_medical_chemical_exposure():
    question = "电池漏液弄到眼睛附近怎么办"
    assert "medical_poisoning_chemical_exposure" in _profile_names(question)

    guides = _guide_candidates(question)
    guide_ids = [guide.id for guide in guides]
    assert any(guide_id in {"DG-0112", "DG-0064"} for guide_id in guide_ids[:3])

    exposure_guides = [guide for guide in guides if guide.id in {"DG-0112", "DG-0064"}]
    assert any(
        "medical-chemical-skin-eye-exposure-001" in set(guide.raw.get("related_wiki") or [])
        for guide in exposure_guides[:2]
    )


def test_unknown_medication_ingestion_prefers_medication_guide():
    question = "误服不知道的药怎么办"
    assert "medical_poisoning_chemical_exposure" in _profile_names(question)

    guides = _guide_candidates(question)
    assert guides
    medication = next((guide for guide in guides[:3] if guide.id == "DG-0215"), None)
    assert medication is not None
    assert "medical-accidental-medication-ingestion-001" in set(
        medication.raw.get("related_wiki") or []
    )


def test_repeated_diarrhea_dehydration_prefers_dehydration_guides():
    question = "腹泻很多次如何判断脱水"
    assert "medical_infection_dehydration_fever" in _profile_names(question)

    guide_ids = _guide_ids(question)
    assert guide_ids[0] in {"DG-0020", "DG-0559"}
    assert any(guide_id in {"DG-0020", "DG-0559"} for guide_id in guide_ids[:2])

    wikis = _wiki_ids(question)
    assert "medical-oral-rehydration-001" in wikis
    assert "medical-diarrhea-dehydration-risk-001" in wikis
