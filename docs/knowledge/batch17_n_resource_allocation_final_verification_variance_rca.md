# Batch17-N Resource Allocation Final Verification Variance RCA

Date: 2026-09-02

Authority: `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

Scope: read-only Root Cause Review of R02, R05, and R07. No Apply was performed.

## 1. Batch17-M blocker summary

Batch17-M ended as `RESOURCE ALLOCATION FINAL VERIFICATION BLOCKED` for three
case-level indicators:

- R02: accepted Guide `DG-0928` changed from candidate-present to candidate-absent.
- R05: Batch17-L targeted PASS did not reproduce in the full run.
- R07: Batch17-I PASS changed to Batch17-M FAIL while `DG-0927` remained candidate rank 2.

The following gates remain frozen and were not reopened: Safety blocker 0,
dangerous suggestion 0, scarcity override 0, systemic RA ownership conflict 0,
stable-domain NEW regression 0, Knowledge audits PASS, and protected asset
hashes unchanged.

## 2. Historical result timeline

### Frozen expectations

|Case|Query|Expected primary|Accepted secondary|Expected / accepted Guide|Expected Wiki/evidence|
|---|---|---|---|---|---|
|R02|有 20 升水，其中 6 升是紧急储备，今天能按 20 升全部使用吗？|Resource Allocation|Water, Storage|`DG-0928` / `DG-0928`|`resource-allocation-reserve-class-policy-001`, `resource-allocation-safety-minimum-register-001`; reserve exclusion, release trigger, water minimum|
|R05|电量很少，照明、通信和娱乐设备应该怎么留？|Energy Management|Resource Allocation|`DG-0845`, `DG-0844` / plus `DG-0928`|`energy-critical-device-minimum-line-001`, `energy-essential-load-reserve-001`; critical load, safety minimum, optional-load stop|
|R07|确认 5 天后会获得补给，现在的库存怎么分配？|Resource Allocation|Storage, Records|`DG-0927` / `DG-0927`|`resource-allocation-horizon-replenishment-001`; confirmed replenishment, arrival horizon, reserve before arrival|

### Result history

|Stage|R02|R05|R07|
|---|---|---|---|
|Batch17-F|FAIL; `DG-0928` candidate rank 2, `DG-0470` selected|PARTIAL; `DG-0845` selected under the pre-correction fixture|PASS; `DG-0927` selected with precise Wiki|
|Batch17-I|FAIL; `DG-0928` candidate rank 2, `DG-0470` selected|PARTIAL; `DG-0845` selected|PASS; `DG-0927` selected with precise Wiki|
|Batch17-L targeted|Not rerun; frozen selection debt|PASS after the authorized ownership/fixture correction; Energy Management + `DG-0845`|Not rerun; frozen PASS|
|Batch17-M full|FAIL; `DG-0928` absent, Water selected|FAIL; ownership semantically Energy Management, accepted Guide/Wiki absent|FAIL; `DG-0927` rank 2, selector fallback selected Water|
|Batch17-N isolated|5 FAIL|5 FAIL|1 PASS / 4 PARTIAL / 0 FAIL|
|Batch17-N full order|FAIL|FAIL|PASS|

The frozen fixture values were not changed.

## 3. Repeated-run methodology

- Runtime: the same production `run_retrieval_v2()` path, `contract_mode=production`,
  limited canary enabled, configured LAN Ollama, and `qwen3.8:27b`.
- Isolated matrix: R02, R05, and R07 each ran five times in a fresh Python
  process with the same code, fixture, Contract, profiles, runtime settings, and model.
- Order matrix: one complete R01-O08 run in frozen fixture order. Target cases
  executed at positions 2, 5, and 7.
- Fixed-plan isolation: for every target execution, the exact normalized
  `source_plan` and `core_terms` were passed to the production fetcher three
  additional times.
- Fixed-selector isolation: one captured selector input per case was replayed
  five times with byte-identical user payload, model, temperature, and timeout.
- Trace captured query, normalized query, timestamps, Contract/ownership
  contexts, candidates and ranks, selector payload and raw response, selected
  Guides/Wiki, fallback state, model status, HTTP status, exception, latency,
  and cache observability.
- One sandbox-denied LAN preflight was discarded. It was an execution-policy
  failure, not an Ollama service result and is excluded from every metric.

Added test-side tools:

- `scripts/trace_resource_allocation_variance.py`
- `scripts/replay_resource_allocation_selector.py`

They monkeypatch only the test process for observation and do not change
production behavior.

## 4. Per-run trace

All timestamps below are UTC on 2026-09-02. `P/S` is planner/selector latency.
Every row had HTTP 200, no timeout, no exception, planner fallback false, and
selector fallback false. The final column is
`planner-output / selector-input / selector-output` SHA-256 prefix.

|Run|Mode/order|Time|Case|Owner from selected top Guide|P/S|Target Guide rank|Selected Guides|Selected Wiki lead|Verdict|Trace hashes|
|---|---|---|---|---|---:|---:|---|---|---|---|
|isolated-r02-1|isolated/1|13:15:50|R02|Water|18.2s/9.8s|absent|`DG-0551`|Water priority|FAIL|`9523c5c4/bf953e11/4a2b86ca`|
|isolated-r02-2|isolated/1|13:17:00|R02|Other/Storage-like|14.1s/10.4s|3|`DG-0470, DG-0551, DG-0928`|Water priority|FAIL|`5e7dbd91/bc91e072/97eef7e3`|
|isolated-r02-3|isolated/1|13:17:26|R02|Other/Storage-like|13.0s/10.1s|1|`DG-0470, DG-0551`|Water priority|FAIL|`52b8b844/17c72fca/8398eaa1`|
|isolated-r02-4|isolated/1|13:17:52|R02|Water|13.9s/9.4s|absent|`DG-0551`|Water priority|FAIL|`5a65ce23/d46087d6/9f9cd966`|
|isolated-r02-5|isolated/1|13:18:17|R02|Other/Storage-like|10.9s/5.7s|absent|`DG-0470`|none|FAIL|`bf021540/477c9f1f/60d069c1`|
|isolated-r05-1|isolated/1|13:18:36|R05|Energy semantic; runner labels Records|10.9s/12.6s|8|`DG-0107, DG-0563`|Energy reserve/battery|FAIL|`75df30b3/e0f0908d/0d6ebd7f`|
|isolated-r05-2|isolated/1|13:19:11|R05|Energy semantic; runner labels Records|10.1s/11.4s|absent|`DG-0107, DG-0563, DG-0337, DG-0859`|Energy/Communication|FAIL|`b55df2d6/c44beed7/cddcf5ba`|
|isolated-r05-3|isolated/1|13:19:44|R05|Energy semantic; runner labels Records|12.5s/10.9s|7|`DG-0107, DG-0337`|Energy priority/lighting|FAIL|`814283a5/62d505cc/78078ea9`|
|isolated-r05-4|isolated/1|13:20:13|R05|Energy semantic; runner labels Records|11.4s/14.9s|absent|`DG-0107, DG-0337, DG-0563, DG-0859`|Energy/Communication|FAIL|`154b98e0/4ca85410/78333f0e`|
|isolated-r05-5|isolated/1|13:20:41|R05|Energy semantic; runner labels Records|13.7s/14.1s|absent|`DG-0107, DG-0563, DG-0859, DG-0337`|Energy/Communication|FAIL|`226ba2fc/c0a12403/4973e885`|
|isolated-r07-1|isolated/1|13:21:21|R07|Resource Allocation|11.1s/11.0s|2|`DG-0926, DG-0927`|available status|PARTIAL|`a2c6cffe/34694a8a/f5de316b`|
|isolated-r07-2|isolated/1|13:21:45|R07|Resource Allocation|14.0s/8.4s|2|`DG-0926, DG-0927`|available status|PARTIAL|`a0d66113/cfc179a6/3e038d4c`|
|isolated-r07-3|isolated/1|13:22:09|R07|Resource Allocation|11.8s/9.3s|2|`DG-0927`|horizon/replenishment|PASS|`693eac08/9c9033d6/5733f2c1`|
|isolated-r07-4|isolated/1|13:22:32|R07|Resource Allocation|10.6s/9.7s|2|`DG-0926, DG-0927`|available status|PARTIAL|`7493de0f/ba23247e/5f92e77a`|
|isolated-r07-5|isolated/1|13:22:55|R07|Resource Allocation|14.6s/14.4s|8|`DG-0926, DG-0927, DG-0928, DG-0596`|available status|PARTIAL|`e796fc77/212d4e9d/f957f3ec`|
|full-sequence-1|full/2|13:25:09|R02|Water|9.2s/10.4s|absent|`DG-0551, DG-0462`|Water priority|FAIL|`416d2c91/4244d7be/f6e96b6d`|
|full-sequence-1|full/5|13:26:17|R05|Energy semantic; runner labels Records|14.9s/12.2s|8|`DG-0107, DG-0563, DG-0337`|Energy reserve/battery|FAIL|`43d43678/f92975ab/dc3db134`|
|full-sequence-1|full/7|13:27:19|R07|Resource Allocation|12.6s/10.6s|1|`DG-0927, DG-0926`|horizon/replenishment|PASS|`eb2eb822/61d0fb71/1e7a3d56`|

Candidate Guide lists, in production order:

|Run|Candidate Guides (first eight)|
|---|---|
|R02-i1|`0551,0462,0563,0605,0587,0886,0603,0599`|
|R02-i2|`0470,0551,0928,0587,0462,0563,0605,0603`|
|R02-i3|`0928,0470,0587,0463,0563,0886,0551,0462`|
|R02-i4|`0551,0462,0563,0605,0587,0886,0603,0599`|
|R02-i5|`0551,0605,0603,0462,0457,0470,0031,0555`|
|R02-full|`0551,0462,0563,0599,0605,0587,0886,0603`|
|R05-i1|`0855,0859,0563,0107,0337,0587,0886,0845`|
|R05-i2|`0855,0859,0563,0551,0107,0337,0886,0587`|
|R05-i3|`0563,0107,0337,0561,0551,0859,0845,0886`|
|R05-i4|`0855,0859,0563,0551,0107,0337,0886,0587`|
|R05-i5|`0859,0855,0563,0107,0337,0886,0587,0930`|
|R05-full|`0855,0859,0563,0107,0337,0587,0886,0845`|
|R07-i1|`0929,0927,0926,0551,0596,0886,0563,0587`|
|R07-i2|`0929,0927,0551,0926,0596,0930,0886,0563`|
|R07-i3|`0929,0927,0926,0551,0596,0886,0563,0587`|
|R07-i4|`0929,0927,0551,0926,0596,0930,0886,0563`|
|R07-i5|`0551,0596,0563,0587,0886,0926,0928,0927`|
|R07-full|`0927,0929,0551,0930,0599,0596,0926,0587`|

Guide IDs in this table omit the common `DG-` prefix only for compactness.
Full selector payloads and raw responses were captured under
`/tmp/batch17n/` during the review; the report records their hashes and
behavioral output without adding production artifacts.

## 5. Candidate-stage stability

|Check|R02|R05|R07|
|---|---:|---:|---:|
|Unique planner output across 5 isolated runs|5|5|5|
|Unique selector input across 5 isolated runs|5|5|5|
|Unique candidate order across 5 isolated runs|5|5|5|
|Target candidate present|2/5|2/5|5/5|
|Target candidate ranks|absent, 3, 1, absent, absent|8, absent, 7, absent, absent|2, 2, 2, 2, 8|
|Fixed-plan fetch reproducible|5/5|5/5|5/5|

Across the 15 isolated and three full-sequence target traces, the initial fetch
plus three exact fixed-plan refetches produced 72 candidate observations.
Every refetch matched its initial candidate IDs and order: **18/18 trace groups
stable**.

Candidate sets vary only when the planner output varies. Profile matching is
stable and empty for all three queries. The Contract and ownership lanes
preserved candidate IDs and order in 18/18 traces. The runner's
`expected_guide_candidate` metric agrees with the actual post-lane Guide list.

R02 candidate absence therefore occurs at stage 1: the planner-dependent
candidate fetch is already missing `DG-0928`. It is not removed by ownership
or filtering, and it is not a report-metric error. Selector behavior cannot
cause candidate absence because selection occurs after the candidate list is
materialized.

## 6. Ownership stability

- Query normalization was byte-stable for every repetition.
- Contract status was `COMPARE_ONLY` in all 18 target traces.
- Ownership-intent status was `NOT_APPLICABLE` in all 18 target traces.
- Contract and ownership-intent lanes changed no candidate IDs or order.
- R07 remained Resource Allocation primary in all five isolated runs and the
  full sequence.
- R02 never reached Resource Allocation primary. Its selected top Guide varied
  between Water and Storage-like legacy evidence.
- R05 remained semantically Energy Management: every selected Guide set was
  about device power priority, lighting, communications, or energy budgeting.

The runner labels `DG-0107` as Records because its legacy multi-domain metadata
contains `records` and `guide_owners()` has no explicit `DG-0107` Energy
mapping. This is an ancillary ownership-metric defect. It does not cause R05's
frozen Guide/Wiki failure: `DG-0845/DG-0844` remain unselected and precise
expected evidence remains incomplete even when the semantic owner is read as
Energy Management. R05 ownership is not reopened.

## 7. Selector stability

The fixed-input replay used the exact captured 16-candidate selector payload.
All 15 calls were healthy HTTP 200 responses and all 15 raw outputs had distinct
hashes.

|Case|Selector input hash|Result across five byte-identical replays|
|---|---|---|
|R02|`bc91e072973d...`|`DG-0928` first 2/5; `DG-0470` first 3/5; five distinct outputs|
|R05|`e0f0908d44ec...`|`DG-0107` first 5/5; secondary Guides/Wiki varied; accepted `DG-0845` selected 0/5|
|R07|`34694a8a4cea...`|five distinct evidence combinations; `DG-0927` selected 3/5; one output selected only Wiki|

The selector is demonstrably stochastic even with identical input. This is
allowed at the model layer, but its output can move the related-Wiki expansion
anchor. For R07, selecting `DG-0926` before `DG-0927` fills the Wiki evidence
budget with inventory/status material; selecting `DG-0927` first loads the
required horizon/replenishment Wiki and passes.

## 8. Model-service health correlation

|Population|Healthy|Timeout/500|R02|R05|R07|
|---|---:|---:|---|---|---|
|Batch17-N isolated target runs|15|0|5 FAIL|5 FAIL|1 PASS / 4 PARTIAL|
|Batch17-N full target positions|3|0|FAIL|FAIL|PASS|
|Fixed-input selector replays|15|0|variable order|stable miss|variable evidence|

The Batch17-M artifact recorded selector fallback for R07 and no fallback for
R02/R05. Its temporary raw exception artifact was intentionally not retained,
so the precise R07 event cannot be split retrospectively between HTTP 500 and
timeout. Batch17-M's global run did observe both event types.

Conclusions:

1. R02 and R05 failures reproduce under healthy service and are not caused by
   timeout/500.
2. R07's Batch17-M FAIL was amplified by selector fallback, but healthy runs
   still vary between PASS and PARTIAL. Service failure is not its sole cause.
3. No Batch17-N target result used a fallback path, so current RCA does not
   confuse healthy model variance with service availability.

## 9. Fallback-path analysis

Code-path review finds:

1. Planner exception/invalid JSON builds a local deterministic plan from the
   query and marks `plan.raw.fallback=true`.
2. Selector exception receives the existing post-Contract/post-ownership
   candidates, picks the first Guide deterministically, and records
   `selection.raw={fallback,error,stage}`.
3. Fallback does not mutate the candidate set or candidate order.
4. Ownership lanes execute before selector fallback, and Contract canary
   selection enforcement executes afterward; established Safety behavior is
   therefore retained.
5. Fallback is auditable through stage, error, debug state, and selected-source
   trace.

The deterministic selector fallback is deterministic only relative to its
input candidate order. If the upstream stochastic planner changes that order,
the end-to-end fallback result can still change. Batch17-M R07 is consistent
with this: `DG-0927` was rank 2, fallback chose the first Water Guide. R02 and
R05 had healthy selector paths in Batch17-M and Batch17-N, so fallback is not
their root cause.

## 10. Order/state-dependence analysis

|Case|Five fresh processes|Full-sequence position/result|Order/state judgement|
|---|---|---|---|
|R02|5 FAIL|2 / FAIL|Same outcome; no order-only behavior|
|R05|5 FAIL|5 / FAIL|Same outcome; no order-only behavior|
|R07|1 PASS, 4 PARTIAL|7 / PASS|Full result lies inside isolated distribution|

No target needed prior-query context, Contract/ownership state stayed identical,
and fixed-plan fetches remained stable in both isolated and sequence runs.
Production planner/selector calls are stateless request/response calls; no
conversation state is retained by these modules. Cache hit/miss is not exposed,
so no positive cache assertion is possible, but there is no observed cache or
global-state signature.

Result: no evidence of runner order dependence, previous-query contamination,
selector conversation leakage, cache-driven divergence, or mutable global
candidate state.

## 11. R02 RCA

Observed:

- End-to-end outcome: 5/5 isolated FAIL and full-sequence FAIL, all healthy.
- `DG-0928` candidate presence: 2/5 isolated; absent in full sequence.
- Whenever a planner output was fixed, fetch result and order were stable.
- Contract and ownership lanes never removed `DG-0928`.
- The candidate metric exactly matched the actual Guide candidates.
- Under one identical selector input containing `DG-0928` at payload position
  3, selector output placed it first 2/5 and behind Water Guides 3/5.

Root cause: the model planner alternates between reserve-release semantics and
Water preservation/rationing semantics, changing candidate recall. When the RA
Guide is present, selector ordering remains stochastic and may still put Water
first. The local deterministic stages behave correctly relative to their
inputs, but the frozen R02 contract fails reproducibly.

Primary classification: **G. MIXED — MUST EXPLAIN**.

The mix is planner nondeterminism plus selector nondeterminism, producing a
6/6 healthy end-to-end failure. It is not candidate-fetch instability,
ownership filtering, fallback variance, order leakage, or runner metric error.

## 12. R05 RCA

Observed:

- End-to-end outcome: 5/5 isolated FAIL and full-sequence FAIL, all healthy.
- Energy Management ownership remains semantically correct.
- `DG-0845` was candidate-present only 2/5 isolated runs, at Guide ranks 8 and
  7; it was rank 8 in the full sequence.
- Accepted `DG-0845/DG-0844/DG-0928` was selected 0/6.
- Expected Wiki any-hit was 1/5, full-hit 0/5, precise Guide-Wiki 0/5.
- A fixed selector payload containing `DG-0845` and
  `energy-essential-load-reserve-001` produced `DG-0107` first 5/5 and
  `DG-0845` 0/5.
- `DG-0107` is semantically relevant Energy evidence but is outside the
  frozen accepted Guide contract and has no `related_wiki`.

Root cause: candidate generation inconsistently recalls the frozen Energy
minimum-line Guide, and selector consistently prefers a nearby legacy power
priority Guide when both are available. This is stable under healthy service
and independent execution. The runner's Records label for `DG-0107` is a
separate metric issue, but correcting that label would not satisfy the frozen
Guide/Wiki contract.

Primary classification: **A. REPRODUCIBLE RETRIEVAL DEFECT**.

R05 ownership remains frozen as Energy Management primary. No fixture change is
recommended.

## 13. R07 RCA

Observed:

- `DG-0927` was candidate-present in 5/5 isolated runs and the full sequence.
- It was selected in 5/5 isolated runs and the full sequence.
- Primary ownership was Resource Allocation in all six executions.
- The isolated result varied between PASS 1/5 and PARTIAL 4/5; the full run
  passed.
- PASS occurs when `DG-0927` is the first selected Guide and its
  horizon/replenishment Wiki is expanded. PARTIAL occurs when `DG-0926` is
  first and inventory/status Wiki consumes the evidence budget.
- Exact-input selector replay produced five distinct evidence combinations.
- Batch17-M selector fallback selected the first Water candidate while the
  accepted Guide remained rank 2.

Primary classification: **C. SELECTOR NONDETERMINISM**.

R07 is not a candidate-recall problem, does not need a profile, and provides no
evidence for ownership or state leakage.

## 14. Deterministic vs stochastic boundary

|Layer|Expected boundary|Batch17-N result|
|---|---|---|
|Query normalization|Deterministic|Stable|
|Profile matching|Deterministic|Stable; no profile matched any target|
|Candidate fetch for fixed plan|Deterministic|Stable in 72/72 observations|
|Contract evaluation/lane|Deterministic|Stable; `COMPARE_ONLY`, no reorder|
|Ownership intents/lane|Deterministic|Stable; `NOT_APPLICABLE`, no reorder|
|Related-Wiki expansion for a fixed selected Guide order|Deterministic|Stable; evidence changes only when selected Guide order changes|
|Planner semantic interpretation|Model-variable|Five distinct outputs per query|
|Selector/reranking|Model-variable|Distinct outputs under identical payload|
|Answer generation|Model-variable|Not run (`--no-answer` semantics)|

No deterministic production layer was observed drifting under identical input.
The production concern is that stochastic planner/selector choices are not
sufficiently constrained to satisfy two frozen evidence contracts reliably.

## 15. Failure classification

|Case|Single primary classification|Excluded classifications|
|---|---|---|
|R02|**G. MIXED — MUST EXPLAIN**: planner candidate recall and selector order both vary; final failure is 6/6 healthy|Not fallback, fetcher, ownership filter, order/state, or metric|
|R05|**A. REPRODUCIBLE RETRIEVAL DEFECT**: frozen accepted Guide/Wiki path fails 6/6 healthy|Not ownership, model-service outage, fallback, order/state, or fixture change|
|R07|**C. SELECTOR NONDETERMINISM**: accepted Guide remains reachable/selected, but Guide order and Wiki precision vary|Not candidate recall, profile need, ownership, or state leakage|

## 16. Freeze-gate reassessment

Resource Allocation v0.1 should **not** require every stochastic selector run to
return byte-identical Guides. That would treat an explicitly model-driven layer
as deterministic and make verification depend on an unrealistic property.

The valid freeze gate is:

- deterministic layers stable under identical inputs;
- ownership stable and domain boundaries preserved;
- Safety stable;
- accepted Guide/Wiki evidence reachable;
- fallback preserves candidate, ownership, and Safety contracts;
- no systemic or repeatedly reproducible Retrieval defect;
- frozen fixture requirements remain satisfied without changing expected values.

Batch17-N satisfies deterministic-layer, ownership-lane, and Safety assumptions,
but it does not satisfy the final two evidence conditions:

- R02 fails every healthy end-to-end repetition through mixed planner/selector behavior.
- R05 fails every healthy repetition and is a reproducible Retrieval defect.
- R07 alone would be a verification-stability debt rather than a profile or
  ownership defect, but its frozen evidence result is not yet repeatable.

Therefore Resource Allocation Retrieval v0.1 remains **not frozen**. The gate is
not lowered and no case is reclassified as PASS merely because a semantically
adjacent Guide was selected.

## 17. Recommended next action

Proceed to a separately authorized **minimal Retrieval fix**, limited to the
generalized planner/selector evidence contract demonstrated by R02 and R05:

1. Preserve the verified deterministic fetch, Contract, ownership, Safety, and
   related-Wiki behavior.
2. Do not add a case-ID or exact-query rule.
3. Do not widen Resource Allocation profiles or change the three frozen profile
   targets.
4. Do not change R05 ownership or any fixture expected value.
5. Require a general intent-level solution that distinguishes reserve release
   from water preservation and critical-load minimum-line evidence from generic
   power-priority evidence.
6. Re-run fixed-input selector contracts before another stochastic full Field
   Test.
7. Keep R07 as a selector repeatability boundary; do not add a profile because
   `DG-0927` was candidate-present in 6/6 current executions.
8. Continue freezing R01, R08, R09, R11, R13, R14, and R20. This Batch did not
   tune or reclassify them.

No implementation is authorized by this report.

## 18. Protected hashes

Before and after values are identical.

### Resource Allocation Wiki

|Slug|SHA-256|
|---|---|
|`resource-allocation-available-resource-status-001`|`9641e4ec328df2bb843c70652334b5bba071a9dfe440d0e7aee96f12a04a8bf7`|
|`resource-allocation-consumption-horizon-baseline-001`|`080fcdaa3f1c48ddeefdc44708d9001f94aa73b176ad914d2f15e8037e22a61f`|
|`resource-allocation-horizon-replenishment-001`|`93dd6193466b1e49f5dcab046a58321ab2633ae7a0e8d37e29c28b5dc3e0ddf0`|
|`resource-allocation-reserve-class-policy-001`|`e9aa1f7317d89ffddc951f94368d6749bfe159d41a55f540e457ae525f813ecf`|
|`resource-allocation-resource-specific-priority-001`|`6dff2e4f27b34b45b9943f40bed708934480f0e58a72d5d4394d838290f7ff74`|
|`resource-allocation-review-loop-001`|`604fedf48c24ff43da8219b8ae0680a70f61bcba7af6477977ca4bb673045d4b`|
|`resource-allocation-safety-minimum-register-001`|`c7521a26b5b9e0273a67b8e4e09869be5d050a94ef7f3f32d4a7f0a25f7f7ce9`|
|`resource-allocation-substitution-degraded-mode-001`|`09e1fea9b56f9c0af2a199dd9191bc04ee659bbb77e9c747ae50900f9ba14eb3`|
|`resource-allocation-trigger-action-recheck-001`|`ba590cc8168c4943da034f8f4ac0843289a95dee4dd76a1390db36042a7667e4`|
|`resource-allocation-uncertainty-scenarios-001`|`67549e1141110a47b1152547b6a8c4d41c72351805f5d1d1932d3afbd009b931`|

### Resource Allocation Guides

|Guide|SHA-256|
|---|---|
|`DG-0926`|`72be9382b002359a9c0d928dd5477a9d4600b2bb9a316fc0ad6cb6a00a00b4f5`|
|`DG-0927`|`eccc905cda11e5e5c5772cdd64e9ac1057be658649a3a9db64e0170da03dfc3d`|
|`DG-0928`|`2c43fc3b75ce96a35db375978566641e1f0801584d78709344ef04f32ad2419d`|
|`DG-0929`|`7384f7f88855b353424ab234c513f4eb4b7e6abbeb8395c935641f7b0e197a05`|
|`DG-0930`|`db022df375d3aa79a1c9383598dfcc09a9aa9737f9f4f09761ffdabb8e66f36d`|

### Relationships, profiles, ownership, fixture, Contract, and Retrieval

|Asset|Count|Before / after SHA-256|
|---|---:|---|
|Canonical sorted `Guide|slug` relationships|48|`919672924fad1903034d046d079d4f3384a578ae4485a015586d5d07ea6c5588`|
|`resource_allocation_consumption_horizon`|1|`085d488fd0640511db521da6d9b1353bcfc06ff9206ed98ced52e432dfb29bed`|
|`resource_allocation_emergency_reserve_release`|1|`bc827ae298eaa149116f047bc2bbf6e25f356eb08bd4f7e51ff455f57c25e45e`|
|`resource_allocation_high_energy_task_decision`|1|`68452a9475f0aeeb5f9a4c15815a4233597ea6ca4ff48ccfaa3cfd74d8d52c2b`|
|OI-01 branch fingerprint|1|`b7b317916be9013697f22608049fd993791276bf6d1cbccacb948dbef8111a67`|
|OI-02 branch fingerprint|1|`eaaf29a1a13796aa307635c863e2a9b43d1679ea9cf4928189d9cc54c8cf3c5d`|
|Ownership-intent module|2 intents|`6692228bbc504b673eb654d7a00e61ff2d8e337fe70e2f23108972f459409144`|
|RA fixture|28 cases|`0d93d5a3f0c0fe3238dc90608b6c7b1984193b41a3aaba3af39a3c2ca588dbc9`|
|Cross-domain Contract|v1.1|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|
|Query-profile source|3 RA profiles|`e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041`|
|Orchestrator|Retrieval core|`a252cc3c9dbff7e887dfc16a0c711947b1e9508d6c2e4d65c316372f427da771`|
|Fetchers|Retrieval core|`b8cf5a113f49dbb390ac90159ea65e10333a4bf695b9df0b5d03540892c9c145`|
|Contract canary|Retrieval core|`7be88c3867301d2e494a61b2f7e4b724f193b5e5ead40b5355c58d0d059140fc`|
|Contract runtime|Retrieval core|`6ddcc27377d86bd2585f6471a0c891b8fa69caff3eaad43b62aa7cf6d99cb90e`|
|Contract compare|Retrieval core|`4a216582bd5a659f00020dc58ad14656661f2abf63ac20caa57ba59638f910e2`|
|Runtime settings|Production configuration|`313a5ef5d7a1c634ab88d54cf9429606654052a38320c46339d31f52ad19eb02`|
|Resource Allocation Field Test runner|Frozen runner|`033e75bcfbc4e246c98c3a55a6cb1021e7cfbebedf50e61523c8404b731d0043`|

No Wiki, Guide, Guide-Wiki edge, profile, ownership intent, Retrieval module,
Contract, Safety Gate, fixture, database, PocketBase data, or unrelated dirty
file was modified.

**READY FOR MINIMAL RETRIEVAL FIX**
