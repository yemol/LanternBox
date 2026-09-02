# Batch17-J Resource Allocation Remaining Strict Selection / Ownership Root Cause Review

## 1. Batch17-I baseline

- Authoritative run: `batch17_i_resource_allocation_field_test_results.json`.
- Full: **12 pass / 4 partial / 12 fail**.
- Strict: **10 pass / 3 partial / 7 fail**.
- Observation: **2 pass / 1 partial / 5 fail**.
- All 28 cases used the production retrieval path. Planner fallback and selector fallback were each used in 28/28 cases because the configured Ollama endpoint was unavailable.
- Safety remained intact: `dangerous suggestion = 0`, `scarcity override = 0`, `Safety Gate failure = 0`.
- Batch16 stable-domain comparison remained at the frozen baseline: 91/96 with the same five pre-existing failures and **NEW regression = 0**.
- This review made no Apply change. Wiki, Guide, Guide-Wiki, profiles, Retrieval, Contract, Safety Gate, fixture, database, and all existing dirty files were left untouched.

## 2. Frozen successful cases

The following Batch17-H results are frozen and excluded from further tuning:

| Case | Frozen result | Profile |
|---|---|---|
| R03 | `DG-0927` rank 1, evidence complete, PASS | `resource_allocation_consumption_horizon` |
| R06 | `DG-0928` rank 1, evidence complete, PASS | `resource_allocation_emergency_reserve_release` |
| R12 | `DG-0929` rank 1, evidence complete, PASS | `resource_allocation_high_energy_task_decision` |

The three profiles are not candidates for modification. Batch16 Limited Production v1.0, R19, O08, and their Safety-first ownership are also frozen.

## 3. Ten unresolved strict cases

| Case | Result | Main failure layer | Correct/accepted Guide candidate | Safety blocker |
|---|---|---|---|---|
| R01 | FAIL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0926` #3 | No |
| R02 | FAIL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0928` #2 | No |
| R05 | PARTIAL | H. FIXTURE / EXPECTED OWNERSHIP ISSUE | Yes, accepted `DG-0845` #1 | No |
| R08 | FAIL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0927` #2 | No |
| R09 | PARTIAL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0929` #3 | No |
| R11 | FAIL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0929` #3 | No |
| R13 | FAIL | D. GUIDE SELECTION / RANKING GAP | Yes, accepted `DG-0619` #7 | No |
| R14 | FAIL | F. DOMAIN OWNERSHIP CONFLICT | Yes, `DG-0919` #7 | No |
| R17 | FAIL | F. DOMAIN OWNERSHIP CONFLICT | No; accepted generic `DG-0383` absent | No |
| R20 | PARTIAL | D. GUIDE SELECTION / RANKING GAP | Yes, `DG-0930` #2 | No |

Classification totals: D x7, F x2, H x1. A/B/E/I/J x0. There is no Safety blocker.

## 4. Full retrieval trace per case

### R01 - physical inventory discrepancy and one-week allocation

- Query: `仓库记录有 40 份应急食品，但盘点只有 31 份，未来一周该按多少安排？`
- Expected primary / accepted secondary: Resource Allocation / Storage, Records, Food.
- Expected capabilities: inventory, uncertainty, allocation horizon.
- Expected / accepted Guides: `DG-0926`; accepted `DG-0926`, `DG-0927`.
- Actual primary / selected Guide / rank: Records / `DG-0860` 通信信息记录与交接 / #1. `DG-0926` was #3 and `DG-0927` #5.
- Candidate Guides: `DG-0860, DG-0212, DG-0926, DG-0126, DG-0927, DG-0127, DG-0870, DG-0909`.
- Selected Wiki: six Communication Wiki linked to `DG-0860`, including radio log and message-format evidence.
- Available expected Wiki: both `resource-allocation-available-resource-status-001` and `resource-allocation-uncertainty-scenarios-001` exist and are linked from `DG-0926`; neither entered Wiki candidates.
- Triggered profile: legacy `inventory_consumption` only.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / D. Correct Guide recall succeeded, but the legacy inventory profile and semantic score favored Records; selector fallback then retained the first Guide only.

### R02 - emergency water reserve release

- Query: `有 20 升水，其中 6 升是紧急储备，今天能按 20 升全部使用吗？`
- Expected primary / accepted secondary: Resource Allocation / Water, Storage.
- Expected capabilities: reserve, safety minimum, priority allocation.
- Expected / accepted Guide: `DG-0928`.
- Actual primary / selected Guide / rank: other/Storage-like / `DG-0470` 备用水封存 / #1; `DG-0928` #2.
- Candidate Guides: `DG-0470, DG-0928, DG-0463, DG-0655, DG-0562, DG-0593, DG-0849, DG-0802`.
- Selected Wiki: none.
- Available expected Wiki: `resource-allocation-reserve-class-policy-001` was a Wiki candidate; `resource-allocation-safety-minimum-register-001` exists through `DG-0928` but was not a candidate.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / D. The query asks when reserve may be released, not how water is preserved. The correct Guide was already #2, so this is selection competition, not recall.

### R05 - same-day low-power load retention

- Query: `电量很少，照明、通信和娱乐设备应该怎么留？`
- Expected primary / accepted secondary: Resource Allocation / Energy Management.
- Expected capabilities: safety minimum, reserve, priority allocation.
- Expected / accepted Guides: `DG-0928`; accepted `DG-0928`, `DG-0845`, `DG-0844`.
- Actual primary / selected Guide / rank: Energy Management / `DG-0845` 关键设备最低电量线 / #1. `DG-0928` was absent.
- Candidate Guides: `DG-0845, DG-0561, DG-0859, DG-0563, DG-0855, DG-0339, DG-0562, DG-0113`.
- Selected Wiki: `energy-critical-device-minimum-line-001`, `energy-essential-load-reserve-001`, `energy-low-battery-power-window-001`, `energy-night-lighting-runtime-plan-001`.
- Available expected Wiki: Energy essential-load reserve was selected; the RA safety-minimum register exists but was not a candidate.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: PARTIAL / H. This is a same-day load decision, so Energy Management is the stronger primary by the frozen boundary. The selected Guide and evidence are operationally correct; the fixture is narrower than the domain rule.

### R08 - uncertain future replenishment

- Query: `有人说一周后可能有补给，但并不确定，现在能提前多消耗一些吗？`
- Expected primary / accepted secondary: Resource Allocation / Records.
- Expected capabilities: replenishment, uncertainty, reserve.
- Expected / accepted Guides: `DG-0927`; accepted `DG-0927`, `DG-0926`.
- Actual primary / selected Guide / rank: Food / `DG-0599` 主食不足：低体力消耗配给 / #1; `DG-0927` #2.
- Candidate Guides: `DG-0599, DG-0927, DG-0930, DG-0127, DG-0212, DG-0929, DG-0350, DG-0596`.
- Selected Wiki: `food-food-knowledge-004`, `food-staple-food-002`.
- Available expected Wiki: `resource-allocation-uncertainty-scenarios-001` was a candidate; `resource-allocation-horizon-replenishment-001` exists through `DG-0927` but did not enter candidates.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / D. The correct Guide was already #2. Generic consumption language over-weighted a food rationing Guide despite the query being resource-neutral and uncertainty-led.

### R09 - month-long replenishment interruption

- Query: `未来一个月没有补给来源，当前库存应该怎么调整？`
- Expected primary / accepted secondary: Resource Allocation / Storage, Food, Water, Energy Deepening.
- Expected capabilities: allocation horizon, replenishment, degraded mode, reserve.
- Expected / accepted Guides: `DG-0929`; accepted `DG-0929`, `DG-0930`, `DG-0927`.
- Actual primary / selected Guide / rank: Resource Allocation / `DG-0927` 未来 7 天跨资源预算 / #1; `DG-0929` #3.
- Candidate Guides: `DG-0927, DG-0128, DG-0929, DG-0696, DG-0704, DG-0926, DG-0596, DG-0435`.
- Selected Wiki: six `DG-0927` Wiki covering horizon, replenishment uncertainty, and energy baselines.
- Available expected Wiki: `resource-allocation-horizon-replenishment-001` was selected; `resource-allocation-substitution-degraded-mode-001` exists through `DG-0929` but was not a candidate.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: PARTIAL / D. Ownership is correct, but the seven-day budget Guide displaced the more appropriate interruption/degraded-mode Guide already at #3.

### R11 - scarce consumable with a functional substitute

- Query: `某种耗材只剩很少，还有功能相近的替代品，应该怎么安排？`
- Expected primary / accepted secondary: Resource Allocation / Repair, Manufacturing, Storage.
- Expected capabilities: substitution, degraded mode, reserve.
- Expected / accepted Guide: `DG-0929`.
- Actual primary / selected Guide / rank: Food / `DG-0545` 低温环境食物摄入 / #1; `DG-0929` #3.
- Candidate Guides: `DG-0545, DG-0212, DG-0929, DG-0928, DG-0705, DG-0717, DG-0728, DG-0572`.
- Selected Wiki: none.
- Available expected Wiki: `resource-allocation-substitution-degraded-mode-001` exists and is linked from `DG-0929`, but did not enter Wiki candidates.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / D. The target Guide was present; generic “耗材/只剩很少” semantics produced an unrelated Food top1.

### R13 - same-day solar load shedding

- Query: `今天太阳能输出偏低，晚上有哪些负载应该关掉？`
- Expected primary / accepted secondary: Energy Management / Resource Allocation.
- Expected capabilities: priority allocation, safety minimum.
- Expected Guides: `DG-0844`, `DG-0845`; accepted additionally `DG-0619`.
- Actual primary / selected Guide / rank: Energy Management / `DG-0564` 太阳能充电：排程和保护 / #1. `DG-0619` was #7; `DG-0844` and `DG-0845` were absent.
- Candidate Guides: `DG-0564, DG-0617, DG-0109, DG-0922, DG-0132, DG-0337, DG-0619, DG-0920`.
- Selected Wiki: `energy-solar-charging-003`, `energy-solar-system-basics-index-001`.
- Available expected Wiki: `energy-daily-energy-budget-001` and `energy-critical-device-minimum-line-001` exist but were not candidates.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / D. Domain ownership is correct, but solar-output terms displaced the accepted load-disconnect Guide. Because an accepted Guide is present at #7, the hard rule excludes a new profile recommendation.

### R14 - three-month generation decline

- Query: `过去三个月发电越来越低，长期供能能力是不是需要重新规划？`
- Expected primary / accepted secondary: Energy Deepening / Records, Resource Allocation.
- Expected capabilities: baseline, allocation horizon, review loop.
- Expected Guides: `DG-0919`, `DG-0922`; accepted additionally `DG-0914`.
- Actual primary / selected Guide / rank: Medical / `DG-0006` 伤口感染：红肿热痛处理 / #1; `DG-0919` #7.
- Candidate Guides: `DG-0006, DG-0863, DG-0920, DG-0110, DG-0531, DG-0930, DG-0919, DG-0726`.
- Selected Wiki: three wound/infection Wiki linked from `DG-0006`.
- Available expected Wiki: `records-energy-capacity-trend-001` and `energy-solar-seasonal-output-baseline-001` exist and are linked from accepted Guides, but neither entered Wiki candidates.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / F. A long-term energy trend was captured by Medical. The Energy Deepening candidate exists, so this is ownership/selection drift rather than missing knowledge or RA recall.

### R17 - broken sole water pump

- Query: `唯一的水泵坏了，怎么修？`
- Expected primary / accepted secondary: Repair / Tools, Resource Allocation.
- Expected capability: substitution.
- Expected / accepted Guides: no frozen preferred Guide; accepted generic repair boundary `DG-0383`.
- Actual primary / selected Guide / rank: Resource Allocation / `DG-0929` 补给中断后的替代与降级分配 / #1. `DG-0383` was absent.
- Candidate Guides: `DG-0929, DG-0923, DG-0148, DG-0066, DG-0113, DG-0117, DG-0123, DG-0153`.
- Selected Wiki: six RA/degraded-mode Wiki linked from `DG-0929`.
- Available expected Wiki: none frozen by the fixture; `DG-0383` itself exists with reusable repair safety edges.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: FAIL / F. The verdict is caused by a forbidden RA primary on an explicit “怎么修” query. The absent generic Repair Guide is a secondary candidate-recall symptom, but one isolated pump query does not meet the profile gate and does not justify new repair knowledge in this batch.

### R20 - early recalculation trigger

- Query: `制定了 7 天资源计划以后，什么情况下必须提前重算？`
- Expected primary / accepted secondary: Resource Allocation / Records.
- Expected capabilities: trigger threshold, review loop, uncertainty.
- Expected / accepted Guide: `DG-0930`.
- Actual primary / selected Guide / rank: Resource Allocation / `DG-0927` 未来 7 天跨资源预算 / #1; `DG-0930` #2.
- Candidate Guides: `DG-0927, DG-0930, DG-0884, DG-0581, DG-0928, DG-0666, DG-0926, DG-0549`.
- Selected Wiki: six `DG-0927` Wiki covering baseline and horizon planning.
- Available expected Wiki: `resource-allocation-review-loop-001` and `resource-allocation-trigger-action-recheck-001` exist through `DG-0930`; neither entered Wiki candidates.
- Triggered profiles: none.
- Contract / Safety: `COMPARE_ONLY`, no ownership change / not applicable.
- Status / layer: PARTIAL / D. The plan-construction Guide displaced the plan-recalculation Guide already at #2.

## 5. Failure-layer classification

The former runner classifications describe symptoms but are not used as the final one-layer RCA. The final classifications are:

- **D - Guide selection/ranking, 7 cases:** R01, R02, R08, R09, R11, R13, R20.
- **F - Domain ownership conflict, 2 cases:** R14, R17.
- **H - Fixture/expected ownership issue, 1 case:** R05.
- **Safety blocker, 0 cases.**

No case proves a knowledge gap, Guide gap, selected-evidence starvation after correct Guide selection, runner defect, or new Contract/Safety regression.

## 6. Candidate recall analysis

Nine cases have an expected or accepted Guide in candidates. Seven of those Guides rank #2, #3, or #7 and lose before evidence selection. R05 has the semantically correct accepted Energy Management Guide at #1. R17 alone lacks its accepted generic Repair Guide.

`additional_profile_required: NO`

Reason:

1. The hard rule prohibits profiles where a correct/accepted Guide already appears, which excludes R01, R02, R08, R09, R11, R13, R14, and R20.
2. R05 is a fixture/domain-boundary issue, not recall.
3. R17 is one isolated cross-domain case, not multiple similar strict cases. A one-case profile would violate the profile gate and the root-cause policy.
4. The three Batch17-H profiles already provide stable rank-1 recall for their approved intent families and must remain frozen.

## 7. Selection/ranking analysis

The dominant mechanism is fallback selection, not candidate absence:

- All 28 Batch17-I cases entered planner and selector fallback.
- The fallback path chooses the first Guide candidate as the selected Guide and deterministically loads that Guide's `related_wiki`.
- Therefore a correct Guide at #2/#3/#7 receives no selected evidence, even where its Wiki edges are complete.
- R01 includes profile competition from the legacy `inventory_consumption` profile, which favors Records language around inventory logs.
- R02, R08, and R11 are semantic-score competitions against preservation or domain-specific consumption Guides.
- R09 and R20 are intra-RA intent competitions: budget construction versus degraded mode or review trigger.
- R13 is an intra-Energy competition: solar charging versus same-day load shedding.
- R14 and R17 expose ownership drift across stable domains.

This is not evidence-capacity proof and does not justify changing `top_k`, selector limit, or fallback capacity. The issue occurs before evidence loading: candidate ordering and ownership intent are insufficiently discriminated in fallback mode.

## 8. Evidence starvation analysis

No case meets the strict definition of **E. SELECTED-EVIDENCE STARVATION**: there is no unresolved case where the correct Guide was selected and its valid related Wiki was then displaced.

The missing evidence in R01, R02, R08, R09, R11, R13, R14, and R20 is downstream of selecting the wrong Guide. Loading more Wiki or reordering `related_wiki` would mask the selection error. R05 already selected coherent Energy Management evidence. R17 has no frozen expected Wiki.

Result: **NO RELATED_WIKI CHANGE**.

## 9. Ownership analysis

- **RA should be primary:** R01, R02, R08, R09, R11, R20. These ask how to allocate, release reserve, account for uncertain replenishment, enter degraded mode, substitute, or recalculate.
- **Energy Management should be primary:** R05 and R13. Both ask about same-day load retention/shedding.
- **Energy Deepening should be primary:** R14. It asks about a three-month trend and long-term capacity replanning.
- **Repair should be primary:** R17. “怎么修” is explicit repair intent; RA may supply a temporary degraded path only as secondary.
- **Storage remains secondary:** R02 concerns release conditions, not preservation method.
- **Records remains secondary:** R01/R08/R20 concern decisions supported by records, not archiving or trust.
- **Production, Manufacturing, and Medical have no valid primary claim in these ten cases.** Medical taking R14 is an ownership error.

The two direct ownership conflicts are cross-domain and cannot be safely fixed by broadening RA. The seven D cases also demonstrate that ownership intent is not reliably preserved by candidate ordering during fallback. Together they form a concentrated candidate-lane/ownership problem rather than ten independent content problems.

## 10. Fixture review

Only R05 has a fixture ownership defect supported by the frozen boundary:

- Original expected primary: Resource Allocation.
- Actual: Energy Management with accepted `DG-0845` and coherent critical-load evidence.
- Why actual is more correct: the query asks which loads to keep under low power now; same-day load scheduling belongs to Energy Management.
- Recommended future fixture change: set Energy Management primary, Resource Allocation secondary; retain `DG-0845`/`DG-0844` as accepted Guides and Energy minimum-line/reserve Wiki as evidence. Do not require the RA safety-minimum Wiki for a pass.

No other strict expected ownership should be changed merely to accept the current retrieval result. In particular, do not accept Food for R08/R11, Medical for R14, or RA for R17.

## 11. Observation review

| Case | Boundary result | Rationale |
|---|---|---|
| O01 | DEBT | Food/fuel scarcity creates RA, Energy Management, and Food competition; insufficient evidence for a safe fixture correction. |
| O02 | DEBT | “Use now or keep” is RA reserve-release intent, but Records captured primary. |
| O03 | ACCEPTABLE | RA inventory reconciliation selected `DG-0926` and passed. |
| O04 | ACCEPTABLE OBSERVATION AMBIGUITY | Two-week cloudy-solar planning can reasonably involve Energy Management and Energy Deepening. |
| O05 | WRONG OWNERSHIP | Choosing which item to produce from constrained workshop material is primarily Production; RA is supplemental. The fixture's RA-primary expectation is too broad. |
| O06 | DEBT | Hygiene-consumable conservation sits across RA and WASH/Medical; the selected contamination path does not resolve allocation intent. |
| O07 | ACCEPTABLE OBSERVATION AMBIGUITY | Skill-aware prioritization reasonably spans RA and Organization/Training; current RA partial is not a profile trigger. |
| O08 | ACCEPTABLE | Swollen last battery remains Safety first; scarcity does not override the hazard. |

Observation outcomes are not used to justify a new RA profile.

## 12. Additional profile recommendation

`additional_profile_required: NO`

No multi-case strict intent family has complete absence of the correct/accepted Guide. Adding profiles for R01/R02/R08/R09/R11/R13/R14/R20 would be ranking repair disguised as recall repair. Adding one for R17 would be a prohibited one-case profile and would modify a Repair boundary for an RA batch.

## 13. Related-wiki recommendation

`related_wiki_change_required: NO`

The 10 Resource Allocation Wiki and five Guides already contain the required inventory, uncertainty, reserve, horizon, substitution, degraded-mode, trigger, and review evidence. Their Guide-Wiki edges are present. No new trace shows a correct selected Guide losing its own evidence.

Result: **NO RELATED_WIKI CHANGE**.

## 14. Knowledge-gap recommendation

`knowledge_or_guide_addition_required: NO`

- Every RA intent in the unresolved set is covered by `DG-0926` through `DG-0930` and their current Wiki.
- Energy Management and Energy Deepening Guides already cover R05, R13, and R14.
- A reusable Repair safety Guide (`DG-0383`) exists for R17. The case does not prove that a new pump-repair Guide is necessary, only that Repair ownership did not enter the candidate lane.
- Adding Wiki or Guides would not correct the observed selection path and would violate the root-cause policy.

## 15. Minimal Apply scope

Recommendation buckets:

- **P1 - Fixture correction:** R05 only. Correct expected ownership to Energy Management with RA supplemental. O05 may be handled later as observation fixture debt, not as a blocker.
- **P2 - Narrow profile additions:** none.
- **P3 - Ownership/candidate-lane adjustment:** required before freeze, but not yet safe for a direct minimal Apply. It must cover RA decision intent versus Storage/Food/Records, preserve Energy Management for same-day load decisions, preserve Energy Deepening for long-term trends, and preserve Repair for explicit repair queries. It must be designed against fallback behavior without changing global ranking, `top_k`, selector limit, or evidence capacity.
- **P4 - Evidence/related Wiki:** none.
- **P5 - Knowledge/Guide addition:** none.
- **P6 - No fix/acceptable boundary:** keep R03/R06/R12/R19/O08 frozen; keep O04/O07 as observation ambiguity.

A second Apply consisting only of R05 fixture correction would not resolve the concentrated strict selection/ownership gap. Conversely, applying ad hoc query rules for the other nine cases would violate the root-cause policy. The next work item should therefore be a bounded candidate-lane/ownership design review, followed by a separately authorized minimal implementation if that design proves narrow and regression-safe.

## 16. Stable-candidate criteria

Resource Allocation v0.1 does not need mechanical 28/28 green. It may freeze when all of the following hold:

- Safety issue = 0.
- NEW regression = 0.
- Approved RA intent families have stable recall.
- No concentrated ownership or systemic retrieval gap remains in strict cases.
- Observation ambiguity is explicitly documented and does not let RA swallow stable domains.

Batch17-I meets the first three conditions but not the fourth. Seven strict selection/ranking gaps plus two ownership conflicts all occur on the fallback candidate lane. This is concentrated, not incidental. Fixture correction alone is insufficient for stable-candidate status.

## 17. Next-step recommendation

Do not Apply from this report. Open a narrowly scoped **Resource Allocation fallback candidate-lane / ownership design review** that:

1. Treats R01/R02/R08/R09/R11/R20 as RA decision-intent selection cases, not recall cases.
2. Preserves Energy Management for R05/R13, Energy Deepening for R14, and Repair for R17.
3. Explains how ownership is resolved when planner/selector fallback is active, using reusable intent rules rather than case phrases.
4. Proves no changes to the three frozen RA profiles, Batch16 Limited Production, Safety Gate, global ranking, `top_k`, selector capacity, or Guide-Wiki edges.
5. Separately proposes the R05 fixture correction after the ownership design is accepted.

Existing dirty files, including `data/lanternbox.db`, were observed but not modified, cleaned, staged, or otherwise handled by Batch17-J.

**BLOCKED — SYSTEMIC OWNERSHIP ISSUE**
