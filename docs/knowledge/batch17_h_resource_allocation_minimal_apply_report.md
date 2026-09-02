# Batch17-H Resource Allocation Retrieval Minimal Apply Report

Date: 2026-09-02

Authority:

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch17_g_resource_allocation_retrieval_root_cause_review.md`
- `docs/knowledge/batch17_f_resource_allocation_field_test_rerun_report.md`

Scope: apply only the three candidate-recall fixes confirmed by Batch17-G. The full Resource Allocation Field Test was not run.

## 1. Root Cause Reference

Batch17-G established that Resource Allocation Knowledge is complete and that `related_wiki` order is not the root cause. The only Apply-approved defects are three strict queries whose preferred Resource Allocation Guide did not enter the candidate set:

|Case|Intent|Preferred Guide|Batch17-G finding|
|---|---|---|---|
|`R03`|Consumption baseline and remaining support time|`DG-0927`|preferred Guide absent from eight candidates|
|`R06`|Emergency reserve release|`DG-0928`|preferred Guide absent from eight candidates|
|`R12`|High-energy task start/delay decision|`DG-0929`|preferred Guide absent from eight candidates|

No other fail or partial was included in the Apply scope.

## 2. Changed Files

|File|Change|
|---|---|
|`data/retrieval_query_profiles.json`|Appended exactly three narrow Resource Allocation profiles|
|`tests/test_resource_allocation_retrieval_profiles.py`|Added positive recall, profile-count gate, and cross-domain negative-boundary tests|
|`docs/knowledge/batch17_h_resource_allocation_minimal_apply_report.md`|This Apply and verification report|

No existing profile was edited or reordered.

## 3. Three Profiles

|Profile|Target case|Guide evidence|
|---|---|---|
|`resource_allocation_consumption_horizon`|`R03`|`DG-0927` primary; `DG-0930` review evidence|
|`resource_allocation_emergency_reserve_release`|`R06`|`DG-0928` primary; `DG-0930` recheck evidence|
|`resource_allocation_high_energy_task_decision`|`R12`|`DG-0929` primary; `DG-0927` horizon evidence|

All three use object + state conjunctions. None uses the broad English trigger words `resource`, `planning`, `management`, or `allocation`.

## 4. Intent Scope

### Consumption Horizon

Requires both a historical/current consumption object and a change/support-time state. Examples include “过去每天 + 增加到” and “库存还能撑 + 能撑多久”.

Boundary:

- same-day electrical load shutdown remains Energy Management;
- long-term generation capacity remains Energy Deepening;
- Records still owns trend trust and archive objects.

### Emergency Reserve Release

Requires both a named reserve object and a release/minimum-line state. “备用” alone cannot trigger it.

Boundary:

- Storage owns how a reserve item is preserved;
- Resource Allocation owns when a reserve class may be released;
- damaged, smoking, hot, leaking, or swollen equipment remains unavailable under Safety.

### High-Energy Task Decision

Requires both a high-energy/task object and a start, delay, reorder, or defer state.

Boundary:

- Production owns how to produce;
- Energy Management owns same-day electrical scheduling;
- Resource Allocation owns whether a constrained task should start, wait, or enter a degraded plan.

## 5. Candidate Recall Target

|Case|Before profile|Before candidates|After profile|After candidates|Result|
|---|---|---|---|---|---|
|`R03`|none|`DG-0442, DG-0696, DG-0704, DG-0596, DG-0126, DG-0128, DG-0241, DG-0124`|`resource_allocation_consumption_horizon`|`DG-0927, DG-0930, DG-0929, ...`|`DG-0927` absent -> rank 1|
|`R06`|none|`DG-0648, DG-0900, DG-0642, DG-0470, DG-0033, DG-0220, DG-0522, DG-0217`|`resource_allocation_emergency_reserve_release`|`DG-0928, DG-0930, DG-0927, DG-0929`|`DG-0928` absent -> rank 1|
|`R12`|none|`DG-0920, DG-0606, DG-0910, DG-0884, DG-0596, DG-0927, DG-0545, DG-0886`|`resource_allocation_high_energy_task_decision`|`DG-0929, DG-0927, DG-0394`|`DG-0929` absent -> rank 1|

This verification checks candidate recall only. It does not claim a full Field Test verdict and does not alter selector or ranking code.

## 6. Safety Boundary Validation

`R19` query:

> 唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，能不能继续用？

Results:

- matched new Resource Allocation profiles: none
- direct candidate list before/after: unchanged
- production Safety Path focused tests: `2 passed`
- expected production owner: `energy_safety`
- expected Guide: `DG-0842`
- Active Hazard Contract / Safety Gate: preserved
- Resource Allocation scarcity override: not introduced

The Battery abnormal boundary query `O08` also matches none of the three new profiles.

## 7. Cross-Domain Boundary Validation

The profile contract includes negative cases for:

- Energy Management: today’s load shutdown
- Energy Deepening: three-month generation decline and capacity replanning
- Storage: long-term preservation of a spare battery
- Production / Manufacturing: concrete shelf cutting and production method
- Repair: direct broken-pump repair
- Records: trusted long-term resource trend
- Medical / WASH: treatment during an active injury
- Energy Safety / Fire: controller smoke and swollen battery

Final profile contract result: `15 passed`.

Two positive ownership checks also remain unchanged:

- same-day Energy Management query -> `DG-0845` rank 1
- concrete production-method query -> `DG-0873` rank 1

The pre-existing `R17` Repair ownership debt remains: its query matches no new profile, but the old candidate path can still place `DG-0929` first. Batch17-H did not repair or hide that unrelated debt.

## 8. Regression Results

### Profile Contract

|Run|Result|
|---|---:|
|Initial focused run|14 passed / 1 failed|
|Single R06 natural-word-order correction|added exact state phrase “什么时候才应该动用”|
|Final focused run|15 passed / 0 failed|

The correction remained inside the R06 profile and did not broaden its reserve object triggers.

### Batch16 Limited Production Contract / Retrieval

The frozen eight-module Contract + Retrieval suite was rerun:

```text
91 passed
```

Active Hazard and Human Harm contract tests remain passing. Batch16 Limited Production new regression: `0`.

### R19 Safety Path

```text
tests/test_resource_allocation_safety_path_alignment.py
2 passed
```

### Stable-Domain Absolute Baseline

The same eight stable-domain profile modules used by Batch16-R were rerun with the Energy Deepening runner re-exec guard set:

|Domain|Result|
|---|---:|
|Energy Management|9 / 9 passed|
|Energy Safety|6 / 8 passed|
|Energy Deepening|20 / 20 passed|
|Medical|8 / 8 passed|
|Long-Term Records|22 / 22 passed|
|Long-Term Storage|12 / 12 passed|
|Manufacturing|7 / 8 passed|
|Agriculture Second Stage|7 / 9 passed|
|Absolute total|91 / 96 passed; 5 failed|

The five failures are exactly the Batch16-R registered baseline debt: Energy Safety x2, Manufacturing x1, Agriculture x2. They are not new and were not modified in this Batch.

Baseline-relative result:

- Batch17-H new stable-domain regression: `0`
- unexpected Contract/Retrieval failure: `0`
- profile boundary regression: `0`

Pytest emitted only the existing cache write-permission warning.

## 9. Profile Count Before / After

|Metric|Before|After|Delta|
|---|---:|---:|---:|
|All query profiles|80|83|+3|
|Resource Allocation profiles|0|3|+3|
|Other new profiles|0|0|0|

Final Resource Allocation profile names:

1. `resource_allocation_consumption_horizon`
2. `resource_allocation_emergency_reserve_release`
3. `resource_allocation_high_energy_task_decision`

The exactly-three count gate is enforced by the new contract test.

## 10. No Knowledge Change Confirmation

No Wiki, Guide, Guide-Wiki relationship, fixture expected value, Retrieval implementation, ranking, selector, top_k, fallback, Prompt, generation, Contract, or Safety Gate was modified.

Protected direct hashes before and after:

|Asset|Before SHA-256|After SHA-256|Result|
|---|---|---|---|
|`data/cross_domain_contracts.json`|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|same|UNCHANGED|
|`api/retrieval_v2/fetchers.py`|`b8cf5a113f49dbb390ac90159ea65e10333a4bf695b9df0b5d03540892c9c145`|same|UNCHANGED|
|`api/retrieval_v2/orchestrator.py`|`5fdc17ba6700cd9a441a4bd157f57a10690730802cdddb7fceafebb2d7d1a8ee`|same|UNCHANGED|
|`tests/fixtures/resource_allocation_field_cases.json`|`521a5bfef6fae3aa71efe9501ef7e7b1eb6a2b9f317c0e00b38c9a38135468b0`|same|UNCHANGED|
|`data/emergency_guides.json`|`a69b6a63857deb938a603ba488e3de684d1cd760c5a26b63ffffb17567d7a97f`|same|UNCHANGED|
|`data/guide_index.json`|`fc34106859f25d9c603bf28544dc52ca9771f1c4979bbb68423d5370bf253cee`|same|UNCHANGED|

The profile source changed as intended from `b6c7f7877114f8c0520d2835054508b4e81ce80c36b7e2492534aceea97e06bc` to `e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041`.

The full Resource Allocation Field Test was not run. Batch17-I remains the required Field Test Revalidation stage.

**READY FOR FIELD TEST REVALIDATION**
