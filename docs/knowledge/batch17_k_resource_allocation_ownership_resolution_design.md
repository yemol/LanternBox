# Batch17-K Resource Allocation Ownership Conflict Resolution Design

## 1. Batch17-J baseline

Batch17-J froze the following findings:

- Full Field Test: **12 pass / 4 partial / 12 fail**.
- Strict: **10 pass / 3 partial / 7 fail**.
- Observation: **2 pass / 1 partial / 5 fail**.
- Ownership conflicts: **R14 and R17**.
- Fixture ownership issue: **R05**.
- Seven ordinary Guide selection/ranking gaps: R01, R02, R08, R09, R11, R13, R20.
- `additional_profile_required: NO`.
- `related_wiki: NO CHANGE`.
- Safety blocker: **0**.
- R03/R06/R12, their three profiles, R19/O08 Safety-first behavior, and Batch16 Limited Production v1.0 remain frozen.

This Batch is design-only. It does not Apply any ownership rule, candidate-lane change, fixture correction, or production behavior.

## 2. Two ownership conflicts

| Case | Expected primary | Actual primary | Resource Allocation role | Competing domain | Boundary judgement |
|---|---|---|---|---|---|
| R14 | Energy Deepening | Medical | Secondary: allocation horizon/replanning | Energy Deepening | Energy Deepening primary + Records/RA secondary |
| R17 | Repair | Resource Allocation | Secondary: substitution/degraded continuity | Repair | Repair primary + Tools/RA secondary |

Neither conflict supports Resource Allocation as primary. They are ownership conflicts because nominal scarcity or generic semantic matches displaced the query's controlling action/time horizon.

## 3. Full trace for each conflict

### R14 - long-term generation decline

- `case_id`: R14.
- Query: `过去三个月发电越来越低，长期供能能力是不是需要重新规划？`
- Expected primary: Energy Deepening.
- Accepted secondary: Records, Resource Allocation.
- Expected capabilities: baseline, allocation horizon, review loop.
- Expected Guides: `DG-0919`, `DG-0922`; accepted additionally `DG-0914`.
- Actual primary: Medical.
- Selected Guide: `DG-0006` 伤口感染：红肿热痛处理.
- Candidate Guides, in order: `DG-0006, DG-0863, DG-0920, DG-0110, DG-0531, DG-0930, DG-0919, DG-0726`.
- Relevant candidate state: `DG-0919` exists at #7; `DG-0920` at #3 is also Energy Deepening/degraded-energy evidence. No Guide or knowledge gap is present.
- Selected Wiki: three Medical wound/infection Wiki linked from `DG-0006`.
- Triggered profiles: none.
- Contract decision: production mode, `COMPARE_ONLY`; `matched_rule` empty, original primary Medical, contract primary unassigned, no ownership change.
- Safety Gate: not applicable.
- Resource Allocation capability: allocation horizon and future replanning may supplement the decision, but the object being assessed is long-term energy capacity/trend.
- Competing domain: Energy Deepening, with Records as evidence/trend support.
- Why systemic: fallback ownership follows the first Guide candidate. The normalized query contains a clear multi-month time horizon and energy-capacity action, yet no ownership lane protects Energy Deepening from an unrelated Medical semantic collision.

### R17 - explicit repair action under single-resource pressure

- `case_id`: R17.
- Query: `唯一的水泵坏了，怎么修？`
- Expected primary: Repair.
- Accepted secondary: Tools, Resource Allocation.
- Expected capability: substitution.
- Frozen preferred Guide: none; accepted generic Repair boundary Guide: `DG-0383`.
- Actual primary: Resource Allocation.
- Selected Guide: `DG-0929` 补给中断后的替代与降级分配.
- Candidate Guides, in order: `DG-0929, DG-0923, DG-0148, DG-0066, DG-0113, DG-0117, DG-0123, DG-0153`.
- Relevant candidate state: accepted `DG-0383` is absent, although it exists and its scenario explicitly covers pumps and repair isolation.
- Selected Wiki: six RA/degraded-mode Wiki linked from `DG-0929`.
- Triggered profiles: none.
- Contract decision: production mode, `COMPARE_ONLY`; `matched_rule` empty, original inferred primary Records, contract primary unassigned, no ownership change.
- Safety Gate: not applicable for this query; future active-hazard or human-harm evidence must still override Repair.
- Resource Allocation capability: the word “唯一” creates a valid secondary continuity/substitution concern, but it does not change the core action “怎么修”.
- Competing domain: Repair, with Tools as co-domain.
- Why systemic: the existing candidate path elevates scarcity/degraded-mode evidence while failing to establish an explicit Repair action lane. A resource noun plus scarcity marker can therefore capture a stable-domain action that RA must not own.

## 4. Domain-boundary judgement

### R14

1. Resource Allocation primary? **No.** The query does not ask which resource to allocate, reserve, delay, or release.
2. Energy Deepening primary? **Yes.** It asks whether a three-month generation decline changes long-term energy capacity planning.
3. Correct relationship: **Energy Deepening primary + Records and Resource Allocation secondary.** Records supports the trend; RA may consume the resulting capacity estimate for later allocation.
4. Medical has no legitimate primary or secondary claim. No patient, symptom, injury, or treatment object appears in the query.

### R17

1. Resource Allocation primary? **No.** “唯一” establishes scarcity, but the requested action is repair.
2. Repair primary? **Yes.** “坏了，怎么修” is an explicit repair request.
3. Correct relationship: **Repair primary + Tools and Resource Allocation secondary.** RA may describe temporary substitution/degraded continuity after repair safety and feasibility are established.
4. Production, Manufacturing, Storage, Records, Medical, Energy Management, and Energy Deepening must not take primary ownership.

### R05 fixture boundary

R05 asks which loads to keep under low power now. By the frozen boundary, same-day load allocation belongs to Energy Management, with RA supplemental. The actual `DG-0845` ownership is semantically correct.

## 5. Shared semantic pattern analysis

R14 and R17 share one architectural symptom but not one domain rule:

- Shared symptom: fallback selection treats first-candidate semantic relevance as de facto ownership when Contract produces `COMPARE_ONLY`.
- R14 controlling signal: **long time horizon + energy generation/capacity trend + replanning action**.
- R17 controlling signal: **explicit repair action + failed physical equipment**, with scarcity only secondary.

There is no defensible generalized RA ownership rule covering both. R14 must keep RA secondary to Energy Deepening; R17 must keep RA secondary to Repair. Combining them into a broad “resource planning” or “scarce equipment” Contract would be over-general and could swallow Energy, Repair, Storage, and Production.

The reusable abstraction is an ownership-intent lane, not a new Resource Allocation Contract:

- `long_term_energy_capacity_trend` -> Energy Deepening primary.
- `explicit_equipment_repair` -> Repair primary.

These are domain-boundary intents, not case IDs, Guide IDs, or exact query strings.

## 6. Contract-extension necessity

`cross_domain_contract_extension_required: NO`

Reasons:

1. The two conflicts do not share one generalized ownership rule, so Batch17-K's Contract-extension gate is not met.
2. Batch16 Limited Production v1.0 is frozen and only permits three validated transitions.
3. Its existing long-term-energy transition is intentionally narrower: rule `solar_seasonal_baseline` requires Solar plus seasonal terms and only accepts original ownership from Energy Management, Energy Other, or Repair. R14 is generic generation/capacity trend and currently appears as Medical, so widening this transition would change two validated boundaries at once.
4. R17 is an ordinary stable-domain action boundary. Routing every explicit repair request through Cross-Domain Contract would turn Contract into a general intent classifier, outside its Active Hazard and Information Object role.
5. No new RA Contract is justified. RA is secondary in both conflicts.

Contract remains authoritative when it produces `CANARY_APPLIED`, especially for Active Hazard and Human Harm. The proposed ownership lane must never override that result.

## 7. Retrieval ownership integration analysis

Recommended path: **B. existing candidate-lane ownership adjustment**.

### Minimal integration point

Use a narrow ownership-intent guard after production normalization/candidate fetch and Contract evaluation, but before AI or fallback selection:

```text
Query normalization
  -> candidate fetch
  -> Cross-Domain Contract evaluation
  -> if CANARY_APPLIED: preserve Contract lanes unchanged
  -> if COMPARE_ONLY and no Safety/Human Harm ownership:
       apply normalized-intent ownership lane
  -> selector / fallback
```

This point is deliberately after Contract evaluation so Safety, Fire, Medical Human Harm, and the three frozen Limited Production transitions retain precedence.

### Proposed ownership hints

`long_term_energy_capacity_trend`:

- Positive structure: long horizon (`多月/三个月/长期/跨季`) + energy generation/capacity change + compare/replan/reassess action.
- Primary lane: Energy Deepening.
- Secondary lanes: Records, then Resource Allocation where allocation-horizon language is present.
- Negative boundaries: current-day load scheduling, active battery/device hazard, fire/smoke, repair action, archive-only request.

`explicit_equipment_repair`:

- Positive structure: failed/broken equipment + explicit repair/fix/check action.
- Primary lane: Repair; Tools may be co-primary metadata.
- Secondary lane: Resource Allocation only when scarcity, sole-item, substitute, delay, or degraded-mode intent is also present.
- Negative boundaries: active smoke/fire/spark/overheat/leak/electrical exposure, human injury, manufacturing from scratch, repair-log archive, storage request.

### Candidate behavior

- Use existing Guide ownership metadata and reusable category/domain/scenario/keyword evidence; do not target `DG-0919`, `DG-0383`, or any fixture ID in production logic.
- Preserve the full original candidate set for traceability.
- Preserve relative score order inside each ownership lane.
- If no primary-lane Guide is available, retain existing candidates and emit a traceable no-lane-candidate result; do not fabricate ownership.
- Do not change Guide score, Wiki score, global ranking formula, `top_k`, selector limit, or fallback count.
- Do not select Wiki directly. Once the correct Guide is selected, existing deterministic `related_wiki` loading remains authoritative.

### Required future tests

- Positive paraphrases for multi-month energy decline/replanning, without requiring “太阳能” or exact R14 wording.
- Negative boundaries for today's load scheduling, active energy hazard, current repair, and Records-only archiving.
- Positive paraphrases for failed pump/tool/device plus explicit repair action, including and excluding scarcity language.
- Negative boundaries for active hazard, human injury, manufacture, storage, and repair-log archive.
- Candidate preservation, within-lane order preservation, Contract `CANARY_APPLIED` precedence, and Batch16 Limited Production regression.

## 8. Seven ranking-gap disposition

No new evidence changes Batch17-J's classification:

| Case | Correct/accepted candidate | Rank | Disposition |
|---|---|---:|---|
| R01 | `DG-0926` | 3 | NON-BLOCKING SELECTION DEBT |
| R02 | `DG-0928` | 2 | NON-BLOCKING SELECTION DEBT |
| R08 | `DG-0927` | 2 | NON-BLOCKING SELECTION DEBT |
| R09 | `DG-0929` | 3 | NON-BLOCKING SELECTION DEBT |
| R11 | `DG-0929` | 3 | NON-BLOCKING SELECTION DEBT |
| R13 | accepted `DG-0619` | 7 | NON-BLOCKING SELECTION DEBT |
| R20 | `DG-0930` | 2 | NON-BLOCKING SELECTION DEBT |

All seven have an expected or accepted Guide in candidates. They remain selection/ranking debt and are out of scope for the ownership Apply. No profile, rank formula, selector, evidence, or fixture change is recommended for them in Batch17-K.

## 9. Fixture ownership correction

- `case_id`: R05.
- Query: `电量很少，照明、通信和娱乐设备应该怎么留？`
- Original expected primary: Resource Allocation.
- Actual ownership: Energy Management, selected `DG-0845` rank 1.
- Domain reasoning: this is a same-day critical-load retention decision, not a future cross-resource budget or reserve-release decision. Existing Energy Management evidence covers minimum lines, essential-load reserve, low-battery windows, and night lighting.
- Recommended corrected expected: Energy Management primary; Resource Allocation secondary.
- Recommended accepted Guides: retain `DG-0845`, `DG-0844`, and RA `DG-0928` as acceptable supplemental evidence.
- Recommended evidence expectation: Energy critical-device minimum and essential-load reserve are sufficient. Do not require an RA Wiki solely to make RA appear primary.

This correction follows the existing domain boundary and is not a pass-rate adjustment.

## 10. Minimal Apply recommendation

Selected combination: **OPTION B - ownership metadata / candidate-lane adjustment + fixture correction**.

Future Apply scope must be exactly:

1. Add the two normalized ownership-intent guards described above.
2. Integrate them only on the `COMPARE_ONLY`/non-safety path before selector fallback.
3. Reuse existing Guide ownership evidence and preserve candidate IDs and within-lane order.
4. Correct R05 fixture ownership to Energy Management primary and RA secondary.
5. Add focused positive/negative ownership tests and rerun Batch16 Limited Production plus stable-domain retrieval contracts.

Explicit exclusions:

- No Contract data/rule/allowlist extension.
- No RA profile addition or modification.
- No Wiki, Guide, Guide-Wiki, or `related_wiki` change.
- No score, ranking formula, `top_k`, selector, fallback, Prompt, generation, schema, PocketBase, or database change.
- No work on the seven selection/ranking debts.

## 11. Stable candidate criteria

Resource Allocation v0.1 does not require 28/28 green. It may proceed to final verification when:

- Safety blocker = 0.
- NEW regression = 0.
- R03/R06/R12 approved recall intents remain stable.
- R19/O08 remain Safety first.
- R14 resolves to Energy Deepening primary with RA supplemental at most.
- R17 resolves to Repair primary with RA supplemental at most.
- R05 fixture ownership reflects Energy Management primary.
- Systemic ownership conflict count = 0.
- The seven frozen selection/ranking debts do not regress into new ownership or Safety errors.
- Observation ambiguity remains documented rather than tuned to 100% pass.

Strict all-green is not a freeze requirement. Ownership correctness, Safety, and baseline parity are the gates.

## 12. Scope protection

Batch17-K changed only this design report. It did not modify:

- Wiki or Guide content.
- Guide-Wiki relationships.
- Retrieval profiles.
- Retrieval, ranking, selector, `top_k`, or fallback.
- Cross-Domain Contract source, allowlist, runtime, or Safety Gate.
- Field Test fixture or expected values.
- PocketBase or `data/lanternbox.db`.
- Any unrelated dirty file.

The report does not reopen R03/R06/R12, R19/O08, Batch16 Limited Production v1.0, or the seven non-blocking selection debts.

## 13. Next-step plan

Authorize one ownership minimal Apply using OPTION B:

1. Capture pre-change hashes for Contract source/runtime settings, profiles, fixture, Retrieval core, and stable-domain tests.
2. Implement the two normalized ownership-intent guards at the candidate-lane integration point, with Contract/Safety precedence.
3. Correct only R05 fixture ownership.
4. Add positive and negative contract-boundary-style tests without modifying Contract data.
5. Verify R14 and R17 ownership, R05 fixture semantics, R03/R06/R12 recall, R19/O08 Safety, Batch16 Limited Production, and stable-domain baseline parity.
6. Do not run a tuning loop for the seven ranking debts; report their unchanged status.

**READY FOR OWNERSHIP MINIMAL APPLY**
