# Batch17-C Resource Allocation Apply Verification + Field Test Fixture Freeze

Date: 2026-09-01

Authority:

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch17_a_resource_allocation_coverage_plan.md`
- `docs/knowledge/batch17_b_resource_allocation_knowledge_apply_report.md`

This Batch independently verifies the Batch17-B knowledge layer and freezes the Resource Allocation Field Test v0.1 fixture and runner. It does not run the formal 28-case Retrieval Field Test.

## 1. Batch17-B Baseline

|Item|Frozen baseline|Batch17-C verification|
|---|---:|---|
|Resource Allocation Wiki|10|10 present|
|PocketBase Resource Allocation Wiki|10|10 exact|
|Resource Allocation Guide|5|5 present|
|New bidirectional Guide-Wiki edges|48|48 symmetric|
|Wiki audit|0 / 0 / 0|0 / 0 / 0|
|Guide audit|0 / 0 / 0|0 / 0 / 0|
|Query profiles|80|80 unchanged|
|Contract change|0|0|
|Retrieval change|0|0|

The temporary Batch17-B superuser remains deleted. Batch17-C did not start PocketBase, synchronize data, create an administrator, or write a database.

## 2. Wiki Verification

Each Wiki was checked for exact slug and title, required metadata, independent actionable content, check criteria, fallback, stop boundary, records handoff, adjacent-domain boundary, Guide link, uniqueness, and PocketBase parity.

|Slug|Title|Markdown|PocketBase exact|Result|
|---|---|---|---|---|
|`resource-allocation-available-resource-status-001`|可分配资源状态卡|PASS|PASS|PASS|
|`resource-allocation-consumption-horizon-baseline-001`|跨资源消耗基线与可维持时长|PASS|PASS|PASS|
|`resource-allocation-safety-minimum-register-001`|关键资源最低保障线登记|PASS|PASS|PASS|
|`resource-allocation-reserve-class-policy-001`|运行、应急与战略储备分级|PASS|PASS|PASS|
|`resource-allocation-resource-specific-priority-001`|按资源建立用途优先级|PASS|PASS|PASS|
|`resource-allocation-horizon-replenishment-001`|分配时距与补给置信度|PASS|PASS|PASS|
|`resource-allocation-substitution-degraded-mode-001`|替代路径与降级模式选择|PASS|PASS|PASS|
|`resource-allocation-trigger-action-recheck-001`|分配触发线、动作与复查条件|PASS|PASS|PASS|
|`resource-allocation-review-loop-001`|资源分配复查循环|PASS|PASS|PASS|
|`resource-allocation-uncertainty-scenarios-001`|Best / Expected / Worst 离线手算卡|PASS|PASS|PASS|

Result: **10 / 10 PASS**. Slug duplicates: 0. PocketBase metadata/content mismatches: 0.

The ten entries preserve the v0.1 distinctions: available is not total inventory; reserve is not unused stock; allocation is not storage; degraded mode begins before exhaustion; expected replenishment is not guaranteed stock.

## 3. Guide Verification

Each Guide was checked for exact ID/title, intended scope, executable steps, risk level, check, fallback, stop/escalation, ethical boundary, and evidence references.

|Guide|Scope|Risk|Structure and stop line|Evidence|Result|
|---|---|---|---|---|---|
|`DG-0926` 有限资源快速盘点与可用量确认|Usable/unavailable/reserved/uncertain inventory|high|PASS|7 links|PASS|
|`DG-0927` 未来 7 天跨资源预算|Cross-resource seven-day horizon and replenishment|high|PASS|10 links|PASS|
|`DG-0928` 关键资源最低保障线与储备分级|Safety minimum, three reserve classes, purpose priority|high|PASS|12 links|PASS|
|`DG-0929` 补给中断后的替代与降级分配|Verified substitution, delay, cancellation, degraded mode|high|PASS|9 links|PASS|
|`DG-0930` 长期短缺资源重分配与周期复查|Plan/Observe/Record/Compare/Adjust review loop|caution|PASS|10 links|PASS|

Result: **5 / 5 PASS**. Guide ID duplicates: 0.

## 4. Evidence Verification

The 48 relationships were checked in both directions and then reviewed semantically against each Guide goal and the Wiki title/summary/body. Classification rules:

- `VALID`: directly supplies a decision input, safety boundary, cross-domain baseline, action, or recheck evidence used by the Guide.
- `WEAK`: adjacent but not sufficient for the decision role claimed by the edge.
- `INVALID`: unrelated, contradictory, or creates false domain ownership.

|Guide|Semantically reviewed Wiki edges|VALID|WEAK|INVALID|
|---|---|---:|---:|---:|
|`DG-0926`|available-resource-status; uncertainty-scenarios; storage inventory card; inventory consumption; off-grid capacity; food inventory; water quantity|7|0|0|
|`DG-0927`|consumption-horizon; horizon-replenishment; uncertainty-scenarios; device consumption; off-grid capacity; solar seasonal; food inventory; drinking-water rate; water usage; planting inventory link|10|0|0|
|`DG-0928`|safety-minimum; reserve-class; resource-specific-priority; essential-load reserve; critical-device minimum; daily energy budget; emergency ration reserve; staple-food baseline; food priority; drinking-water minimum; water priority; production material reserve|12|0|0|
|`DG-0929`|horizon-replenishment; substitution/degraded-mode; trigger/action/recheck; cloudy-solar degraded mode; generation-failure path; water-shortage response; production daily budget; production material reserve; agriculture water/fertility budget|9|0|0|
|`DG-0930`|review-loop; uncertainty-scenarios; consumption-horizon; trigger/action/recheck; inventory consumption; energy trend; solar seasonal baseline; production daily budget; planting inventory link; agriculture water/fertility budget|10|0|0|
|Total|48 edges|48|0|0|

Graph verification:

- Guide to Wiki edges: 2482
- Wiki to Guide edges: 2482
- Batch17-B edges: 48
- missing relation: 0
- orphan relation: 0
- direction mismatch: 0

No weak edge affects a core decision because `WEAK = 0`. Hard gate `INVALID = 0` passes.

## 5. Capability Matrix

|Capability|Wiki anchor|Guide action entry|Status|v0.1 judgement|
|---|---|---|---|---|
|RA-01 Resource Inventory|available-resource-status|DG-0926|FULL|Usable quantity and unknown state are explicit|
|RA-02 Consumption Baseline|consumption-horizon-baseline|DG-0927|FULL|Normal/minimum/high-load daily and weekly baselines|
|RA-03 Safety Minimum|safety-minimum-register|DG-0928|FULL|Professional owner defines; RA protects|
|RA-04 Reserve Policy|reserve-class-policy|DG-0928|FULL|Operational/emergency/strategic with release and recheck|
|RA-05 Priority Allocation|resource-specific-priority|DG-0928|FULL|Per-resource purpose ordering, no person-value model|
|RA-06 Allocation Horizon|consumption-horizon + horizon-replenishment|DG-0927, DG-0930|FULL|Today, 3-7 days, weeks, months|
|RA-07 Replenishment|horizon-replenishment|DG-0927, DG-0929|FULL|Confirmed/expected/uncertain/none|
|RA-08 Substitution|substitution-degraded-mode|DG-0929|FULL|Only professionally verified options|
|RA-09 Degraded Mode|substitution-degraded-mode|DG-0929|FULL|Entry, allowed/paused uses, recovery condition|
|RA-10 Trigger Threshold|trigger-action-recheck|DG-0929, DG-0930|FULL|Trigger, action, recheck, restoration|
|RA-11 Allocation Review Loop|review-loop|DG-0930|FULL|Plan/Observe/Record/Compare/Adjust with versions|
|RA-12 Uncertainty|uncertainty-scenarios|DG-0926, DG-0927, DG-0930|FULL|Best/expected/worst offline conservative model|

Result: `FULL = 12`, `PARTIAL = 0`, `MISSING = 0`. No Knowledge Apply gap was found.

## 6. Domain Boundary Verification

|Boundary pair|Frozen ownership rule|Verification|
|---|---|---|
|RA vs Storage|RA allocates verified usable stock; Storage owns preservation, rotation, isolation, and custody|PASS|
|RA vs Energy Management|RA handles cross-resource/cross-horizon conflicts; Energy Management owns same-day electrical scheduling|PASS|
|RA vs Energy Deepening|RA consumes capacity conclusions; Deepening owns generation, storage, degradation, topology, and long-term capacity|PASS|
|RA vs Records|RA adjusts decisions from evidence; Records owns facts, trust, version, trend, and archive lifecycle|PASS|
|RA vs Production|RA grants/defers resource windows; Production owns workshop execution, flow, inspection, and process|PASS|
|RA vs Manufacturing|RA selects an already verified option; Manufacturing owns how to make it|PASS|
|RA vs Repair|RA handles reserve/substitution during downtime; Repair owns diagnosis and repair method|PASS|
|RA vs Medical|RA may budget inventory/reserve/replenishment; Medical owns patient assessment, treatment, contraindication, and clinical priority|PASS|
|RA vs Team Operations|RA states resource windows; Team Operations owns staffing, duty, skill authorization, and scheduling|PASS|

Resource Allocation remains limited to where resources go, how much is protected, when degraded mode begins, and when the decision must be re-evaluated.

## 7. Safety Verification

|Scenario|Required owner/action|Static evidence result|
|---|---|---|
|Only device is smoking|Safety / Fire first; stop, disconnect, isolate; no scarcity override|PASS|
|Last battery is swollen|Energy Safety first; stop and isolate; battery is unavailable|PASS|
|Medical supplies are insufficient|Inventory/reserve/replenishment allowed; concrete treatment remains Medical|PASS|
|A person is currently injured|Medical / Safety first; Resource Allocation is deferred|PASS|

The new Wiki/Guide set explicitly covers smoke, swelling, abnormal heat, leakage, sparks, contamination, and human harm. Uniqueness, cost, scarcity, or lack of a replacement never authorizes continued hazardous operation. Cross-Domain Contract Limited Production v1.0 remains unchanged.

Ethical verification also passes: the knowledge allocates by purpose, safety minimum, system function, and time constraint, not identity, status, compliance, or perceived human value.

## 8. Field Test Fixture Design

Frozen artifact:

`tests/fixtures/resource_allocation_field_cases.json`

- schema version: 1.0
- frozen date: 2026-09-01
- formal cases: 28
- strict cases: 20
- observation cases: 8
- duplicate case IDs: 0
- invalid Guide IDs: 0
- invalid Wiki slugs: 0

Strict cases determine v0.1 Retrieval performance. Observation cases expose cross-domain competition without allowing their result to rewrite the frozen query, expected ownership, or accepted Guides.

Preferred `expected_guide_ids` and broader `accepted_guide_ids` are separate. Cases without a defensible exact Guide, such as the generic pump-repair boundary, freeze domain ownership and leave preferred Guide empty rather than inventing an exact expectation.

## 9. Twenty Strict Cases

|Case|Focus|Expected primary|Preferred / accepted Guide|
|---|---|---|---|
|R01|Record 40 vs physical 31|Resource Allocation|DG-0926 / DG-0926, DG-0927|
|R02|Emergency water reserve|Resource Allocation|DG-0928 / DG-0928|
|R03|Water consumption increase|Resource Allocation|DG-0927 / DG-0927, DG-0930|
|R04|Food 14 days vs energy 5 days|Resource Allocation|DG-0927 / DG-0927, DG-0928|
|R05|Lighting/communication/entertainment minimum|Resource Allocation|DG-0928 / DG-0928, DG-0845, DG-0844|
|R06|Fuel reserve release|Resource Allocation|DG-0928 / DG-0928, DG-0930|
|R07|Confirmed replenishment in five days|Resource Allocation|DG-0927 / DG-0927|
|R08|Uncertain replenishment|Resource Allocation|DG-0927 / DG-0927, DG-0926|
|R09|No replenishment for one month|Resource Allocation|DG-0929 / DG-0929, DG-0930, DG-0927|
|R10|Consumption spike trigger|Resource Allocation|DG-0930 / DG-0930, DG-0927|
|R11|Verified substitution|Resource Allocation|DG-0929 / DG-0929|
|R12|Delay high-energy Production task|Resource Allocation|DG-0929 / DG-0929, DG-0927|
|R13|Same-day load shutdown|Energy Management|DG-0844, DG-0845 / DG-0844, DG-0845, DG-0619|
|R14|Three-month generation decline|Energy Deepening|DG-0919, DG-0922 / plus DG-0914|
|R15|Seed moisture protection for six months|Storage|DG-0897 / DG-0897, DG-0513|
|R16|Turn three-month logs into a trusted trend|Records|DG-0914 / DG-0914, DG-0912|
|R17|Repair the only water pump|Repair|domain gate / DG-0383 acceptable|
|R18|Medical inventory and replenishment|Resource Allocation|DG-0928 / DG-0928, DG-0926, DG-0927|
|R19|Only solar controller is smoking|Energy Safety|DG-0842 / DG-0842, DG-0579|
|R20|Early seven-day plan recalculation|Resource Allocation|DG-0930 / DG-0930|

## 10. Eight Observation Cases

|Case|Observed competition|Frozen primary / boundary|
|---|---|---|
|O01|Food abundance vs fuel shortage|RA primary; Energy Management/Food supplemental|
|O02|Use or reserve the only spare part|RA primary; Storage/Repair supplemental|
|O03|Inventory record vs physical count|RA primary; Records/Storage evidence|
|O04|Possible two-week cloudy period|Energy Deepening primary; RA/Management supplemental acceptable|
|O05|Material sufficient for only one product|RA constraint; Production execution|
|O06|Hygiene consumables for half a month|RA primary; WASH/Medical safety minimum protected|
|O07|Give more resources to the most skilled person|Reject human-value ranking; Team Operations retains assignment|
|O08|Last battery is swollen|Energy Safety primary; RA/Management/Deepening forbidden primary|

Observation results are reported separately as pass/partial/fail in Batch17-D and do not retroactively change the Knowledge Apply decision.

## 11. Fixture Schema

Every case freezes the required fields:

- `case_id`
- `case_type`
- `query`
- `expected_primary_domain`
- `accepted_secondary_domains`
- `expected_capabilities`
- `expected_guide_ids`
- `accepted_guide_ids`
- `required_evidence_topics`
- `forbidden_primary_domains`
- `safety_required`
- `stop_condition_required`
- `dangerous_advice_forbidden`
- `notes`

The fixture additionally freezes `expected_wiki_slugs` so Batch17-D can report Wiki any/full and precise Guide-Wiki evidence without deriving expectations after seeing results.

## 12. Runner Design

Runner:

`scripts/test_resource_allocation_field.py`

Properties:

- read-only for Knowledge, PocketBase, profiles, Contract, and Retrieval;
- never calls an LLM;
- selects local Guide/Wiki evidence using the existing Retrieval interfaces;
- reports strict and observation pass/partial/fail separately;
- reports Guide hit/top1/accepted, Wiki any/full, and precise evidence;
- reports all 12 capability rates;
- reports Storage, Energy Management, Energy Deepening, Records, Production, Manufacturing, Repair, Medical, Team Operations, and Energy Safety boundaries;
- reports safety priority, stop condition, dangerous suggestion, scarcity-overrides-safety, human-value ranking, Kiwix override, fallback, and record/check;
- supports repository-contained `--results-path` and `--report-path` without overwriting the fixture;
- defaults to independent Batch17-D result/report paths.

Validation commands run in Batch17-C:

```bash
PYTHONPYCACHEPREFIX=/tmp/lanternbox_batch17c_pycache \
  python3 -m py_compile scripts/test_resource_allocation_field.py

python3 scripts/test_resource_allocation_field.py --help
python3 scripts/test_resource_allocation_field.py --validate-only
```

Validation result:

- fixture validation: PASS
- formal / strict / observation: 28 / 20 / 8
- retrieval cases executed: 0
- result artifacts written: 0

The default Batch17-D results and report files remain absent. Formal Field Test was not run.

## 13. Protected Hashes

Hash method for aggregate sets: SHA-256 over each sorted repository path, NUL separator, file bytes, and NUL separator. `__pycache__`, `.pyc`, and `.orig` are excluded. Before and after values are identical.

|Protected set|Files / edges|Before SHA-256|After SHA-256|Result|
|---|---:|---|---|---|
|Resource Allocation Wiki|10|`e356b1fca2d1d15c5afca31ad489ad0ad934dabd2d54e005504448d4eb89d423`|same|UNCHANGED|
|Resource Allocation Guides|5|`16fc933539303f6391ce9987e037394752b5bcd9cfd6bb7293145707b2fb9531`|same|UNCHANGED|
|Batch17-B relations|48|`6af01d113929a9fa384aec29856b41b6217b63ff848925335372f54dda0b9b6e`|same|UNCHANGED|
|Query profiles|1 file / 80 profiles|`8bb38c0e76acdbe7ba50fe4c1ae626d65d11b6a2a95c9021789131295b58efdf`|same|UNCHANGED|
|Cross-Domain Contract|1 file|`b0ed7d0208dee4380abb431a4b84ea1582ade7b172e65ea511217533a1b72cb3`|same|UNCHANGED|
|Retrieval core|13 source files|`2f38bbe29557743285b4e52e2e7a9de701b610302833b03ef21ba9b4721fa063`|same|UNCHANGED|

Retrieval core scope is `api/retrieval_v2` source plus `api/config.py`, `api/models.py`, and `api/routes.py`.

New frozen artifacts:

- fixture SHA-256: `521a5bfef6fae3aa71efe9501ef7e7b1eb6a2b9f317c0e00b38c9a38135468b0`
- runner SHA-256: `a1df8a8f5bab56cb431325e4d0584d5bc6845653c16c7ebf68fbcbb8aca971f1`

## 14. Audit Results

|Audit|Scope|Errors|Warnings|Advisories|
|---|---:|---:|---:|---:|
|`python3 tools/audit_wiki.py`|1167 Markdown / 1167 PocketBase|0|0|0|
|`python3 scripts/audit_guides.py`|856 Guides|0|0|0|
|Guide-Wiki relation audit|2482 forward / 2482 reverse|0|0|0|

Required metadata, slug uniqueness, Guide ID uniqueness, PocketBase parity, and relationship direction all pass.

## 15. Profile Protection

- profiles before: 80
- profiles after: 80
- profile increment: 0
- profile aggregate hash: unchanged

No Resource Allocation profile was added. No Prompt, ranking, `top_k`, selector, selector limit, fallback, generation, Contract, or Retrieval behavior was modified.

## 16. Changed Files

Intentional Batch17-C additions:

- `tests/fixtures/resource_allocation_field_cases.json`
- `scripts/test_resource_allocation_field.py`
- `docs/knowledge/batch17_c_resource_allocation_apply_verification.md`

The required Wiki and Guide audits refreshed the existing date-stamped audit reports. No Wiki, Guide, Guide-Wiki edge, Guide build output, profile, Contract, Retrieval source, PocketBase content, `.env`, `data/lanternbox.db`, FT-02, Core, or unrelated dirty file was modified by Batch17-C.

No files were staged or committed.

## 17. Readiness Judgement

Acceptance gates:

- Wiki Apply complete: PASS
- Guide Apply complete: PASS
- evidence `INVALID = 0`: PASS
- capability `MISSING = 0`: PASS
- domain boundary violation: 0
- safety boundary violation: 0
- Wiki audit: 0 / 0 / 0
- Guide audit: 0 / 0 / 0
- Guide-Wiki relation audit: 0 / 0 / 0
- query profile increment: 0
- formal Field Test executed: no

Fixture and runner are frozen for Batch17-D. Expected ownership and accepted Guides must not be changed after seeing Retrieval results; any failure proceeds to Root Cause Review rather than fixture tuning.

**READY FOR FIELD TEST**
