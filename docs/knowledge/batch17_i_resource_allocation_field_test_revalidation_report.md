# Batch17-I Resource Allocation Field Test Revalidation Report

Date: 2026-09-02

Authority:

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch17_h_resource_allocation_minimal_apply_report.md`
- `docs/knowledge/batch17_f_resource_allocation_field_test_rerun_report.md`
- `docs/knowledge/batch17_g_resource_allocation_retrieval_root_cause_review.md`
- `tests/fixtures/resource_allocation_field_cases.json`
- `scripts/test_resource_allocation_field.py`

Scope: verification only. No Apply was performed.

## 1. Frozen Baseline

|Item|Frozen value|
|---|---|
|Field Test fixture|20 strict + 8 observation = 28|
|Query profiles|83 total|
|Resource Allocation profiles|exactly 3|
|Fixture SHA-256|`521a5bfef6fae3aa71efe9501ef7e7b1eb6a2b9f317c0e00b38c9a38135468b0`|
|Profile SHA-256|`e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041`|
|Runner SHA-256|`d905e1fc0c38d87c429dbb26a110ede1c66326a4c8c93d55384725e0e7c0447c`|
|Contract SHA-256|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|

Batch17-F baseline:

- full: 9 pass / 4 partial / 15 fail
- strict: 7 pass / 3 partial / 10 fail
- observation: 2 pass / 1 partial / 5 fail

## 2. Batch17-H Delta

The frozen Minimal Apply added only:

1. `resource_allocation_consumption_horizon`
2. `resource_allocation_emergency_reserve_release`
3. `resource_allocation_high_energy_task_decision`

Profile count remains `80 -> 83`; no fourth Resource Allocation profile exists.

Before/after case comparison proves that only the three approved targets changed Guide, owner, or verdict:

|Case|Before|After|
|---|---|---|
|R03|`DG-0442`, other, fail|`DG-0927`, Resource Allocation, pass|
|R06|`DG-0648`, Fire/Safety, fail|`DG-0928`, Resource Allocation, pass|
|R12|`DG-0920`, Energy Deepening, fail|`DG-0929`, Resource Allocation, pass|

All other 25 cases retained their prior selected Guide and verdict.

## 3. Test Execution Path

Command:

```bash
env OLLAMA_URL=http://127.0.0.1:1 \
python3 scripts/test_resource_allocation_field.py \
  --no-answer \
  --results-path docs/knowledge/batch17_i_resource_allocation_field_test_results.json \
  --report-path docs/knowledge/batch17_i_resource_allocation_field_test_runner_report.md \
  --artifact-label Batch17-I
```

Execution path:

```text
Query
  -> production normalization
  -> Cross-Domain Contract
  -> OwnershipDecision
  -> run_retrieval_v2
  -> production selector/fallback
  -> Safety Gate
  -> selected Guide and deterministic related Wiki
```

Path counters:

- production path: 28 / 28
- planner fallback: 28 / 28
- selector fallback: 28 / 28
- test-only retrieval shortcut: 0
- test-only Safety Gate: 0
- Kiwix override: 0

The local Ollama endpoint was intentionally made immediately unreachable, matching Batch17-F's production offline-fallback condition. The runner still called `run_retrieval_v2()` for every case.

## 4. Strict Result

|Result|Batch17-F|Batch17-I|Delta|
|---|---:|---:|---:|
|Pass|7|10|+3|
|Partial|3|3|0|
|Fail|10|7|-3|
|Total|20|20|0|

All three strict improvements are the approved Batch17-H targets. No strict case regressed.

## 5. Observation Result

|Result|Batch17-F|Batch17-I|Delta|
|---|---:|---:|---:|
|Pass|2|2|0|
|Partial|1|1|0|
|Fail|5|5|0|
|Total|8|8|0|

No new Resource Allocation profile triggered in any Observation case. Existing non-RA profiles still trigger only O04 and O08. Observation ownership behavior is unchanged.

## 6. Full Result

|Result|Batch17-F|Batch17-I|Delta|
|---|---:|---:|---:|
|Pass|9|12|+3|
|Partial|4|4|0|
|Fail|15|12|-3|
|Total|28|28|0|

The Minimal Apply improved exactly three cases and did not turn unrelated partial/fail cases green.

## 7. R03 / R06 / R12 Verification

|Check|R03|R06|R12|
|---|---|---|---|
|Profile|`resource_allocation_consumption_horizon`|`resource_allocation_emergency_reserve_release`|`resource_allocation_high_energy_task_decision`|
|Preferred Guide|`DG-0927`|`DG-0928`|`DG-0929`|
|Candidate hit|yes|yes|yes|
|Candidate rank|1|1|1|
|Accepted top1|yes|yes|yes|
|Primary owner|Resource Allocation|Resource Allocation|Resource Allocation|
|Wiki any|yes|yes|yes|
|Wiki full|no|yes|no|
|Guide-Wiki precise|yes|yes|yes|
|Topic evidence|yes|yes|yes|
|Verdict|pass|pass|pass|

Capability evidence:

- R03: baseline, allocation horizon, review loop = all true
- R06: reserve, trigger threshold, review loop = all true
- R12: allocation horizon, degraded mode, priority allocation = all true

The target gate is fully satisfied. Wiki full is not required where the selected Guide deterministically supplies the required subset and precise evidence chain.

## 8. Retrieval Metrics

|Metric|Batch17-F|Batch17-I|Delta|
|---|---:|---:|---:|
|Expected Guide hit|40.0%|55.0%|+15.0 pp|
|Expected Guide top1|35.0%|50.0%|+15.0 pp|
|Accepted Guide top1|55.0%|70.0%|+15.0 pp|
|Wiki any|50.0%|65.0%|+15.0 pp|
|Wiki full|35.0%|40.0%|+5.0 pp|
|Guide-Wiki precise|50.0%|65.0%|+15.0 pp|

The delta is attributable only to R03, R06, and R12. No non-target selected Guide changed.

## 9. Capability Metrics

Status rule:

- PASS: every applicable case passes;
- PARTIAL: at least one applicable case passes or is partial, but not all pass;
- FAIL: all applicable cases fail.

|Capability|Applicable|Pass|Partial|Fail|Evidence hit|Status|
|---|---:|---:|---:|---:|---:|---|
|RA-01 Inventory|4|3|0|1|3|PARTIAL|
|RA-02 Consumption Baseline|6|4|0|2|4|PARTIAL|
|RA-03 Safety Minimum|8|3|2|3|3|PARTIAL|
|RA-04 Reserve Policy|11|4|2|5|4|PARTIAL|
|RA-05 Priority Allocation|9|2|2|5|2|PARTIAL|
|RA-06 Allocation Horizon|11|4|1|6|4|PARTIAL|
|RA-07 Replenishment|5|2|1|2|2|PARTIAL|
|RA-08 Substitution|2|0|0|2|1|FAIL|
|RA-09 Degraded Mode|5|1|1|3|1|PARTIAL|
|RA-10 Trigger Threshold|3|2|1|0|2|PARTIAL|
|RA-11 Review Loop|6|4|1|1|4|PARTIAL|
|RA-12 Uncertainty|6|1|1|4|1|PARTIAL|

Improvements versus Batch17-F are concentrated in baseline, allocation horizon, degraded mode, priority allocation, reserve, trigger threshold, and review loop, matching the three approved intents. Substitution remains a retrieval/ownership problem and was not included in Batch17-H.

## 10. Boundary Metrics

|Boundary|Batch17-F|Batch17-I|Absolute judgement|
|---|---:|---:|---|
|Storage|45.5%|54.5%|R15 remains Storage/Agriculture pass|
|Energy Management|55.6%|66.7%|R13 owner remains Energy Management; evidence still fails|
|Energy Deepening|62.5%|75.0%|R14 still incorrectly selects Medical|
|Records|60.0%|70.0%|R16 remains Records pass|
|Production|0.0%|33.3%|R12 now has correct RA decision with Production evidence; O05 remains Production-led|
|Manufacturing|0.0%|0.0%|No new RA capture; existing coverage remains unresolved|
|Repair|0.0%|0.0%|R17 still incorrectly selects RA `DG-0929`|
|Medical|50.0%|50.0%|No new RA capture|
|Team Operations|100.0%|100.0%|Unchanged|
|Energy Safety|100.0%|100.0%|R19/O08 remain Safety first|

Required case review:

- R13: Energy Management owner preserved; fail is expected Guide/Wiki evidence, not RA swallowing.
- R14: Energy Deepening expected, Medical actual; known ownership conflict remains.
- R15: Storage/Agriculture primary, pass.
- R16: Records primary, pass.
- R17: Repair expected, Resource Allocation actual; known forbidden-primary failure remains.
- R19: Energy Safety primary, pass.

The three new profiles do not swallow Storage, Energy Management, Energy Deepening, Records, Production methods, Repair methods, Medical, or Safety. However, the absolute matrix still contains concentrated pre-existing selection/ownership failures, especially R01/R02/R08/R11 and R14/R17.

## 11. Safety Metrics

|Metric|Result|
|---|---:|
|R19 primary|Energy Safety|
|R19 Guide|`DG-0842` rank 1|
|R19 Contract|`energy_device_abnormal`, `CANARY_APPLIED`|
|R19 mandatory secondary|Fire|
|R19 Safety Gate|PASS|
|O08 primary|Energy Safety|
|O08 Guide|`DG-0841` rank 1|
|Dangerous suggestion|0|
|Scarcity override|0|
|Safety Gate failure|0|
|Stop condition|100%|
|Safety rate|100%|
|Kiwix override|0|

Neither R19 nor O08 matched a new Resource Allocation profile. Safety is not a blocker.

## 12. Regression Results

|Suite|Expected baseline|Batch17-I result|Judgement|
|---|---:|---:|---|
|Resource Allocation profile contract|15 passed|15 passed|PASS|
|Contract / Retrieval contracts|91 passed|91 passed|PASS|
|R19 Safety Path|2 passed|2 passed|PASS|
|Stable domains absolute|91 / 96|91 / 96|BASELINE PARITY|

Stable-domain historical failures remain exactly:

- Energy Safety x2
- Manufacturing x1
- Agriculture Second Stage x2

No new stable-domain test failed. Pytest emitted only the existing cache write-permission warning.

## 13. Remaining Fail / Partial List

### Strict

|Case|Status|Actual Guide / owner|Remaining issue|
|---|---|---|---|
|R01|fail|`DG-0860` / Records|RA Guide candidate loses selection; RA Wiki starved|
|R02|fail|`DG-0470` / other|RA reserve Guide candidate loses selection|
|R05|partial|`DG-0845` / Energy Management|acceptable same-day Energy/RA co-domain; RA minimum Wiki absent|
|R08|fail|`DG-0599` / Food|RA replenishment Guide candidate loses selection|
|R09|partial|`DG-0927` / RA|degraded-mode Guide/Wiki incomplete|
|R11|fail|`DG-0545` / Food|RA substitution Guide candidate loses selection|
|R13|fail|`DG-0564` / Energy Management|owner correct; frozen Guide/Wiki evidence miss|
|R14|fail|`DG-0006` / Medical|Energy Deepening ownership conflict|
|R17|fail|`DG-0929` / RA|forbidden RA capture of direct Repair query|
|R20|partial|`DG-0927` / RA|review-trigger Guide/Wiki incomplete|

### Observation

|Case|Status|Actual Guide / owner|Remaining issue|
|---|---|---|---|
|O01|fail|`DG-0360` / other|RA degraded-allocation candidate absent|
|O02|fail|`DG-0916` / Records|reserve decision drifts to Records lifecycle|
|O04|fail|`DG-0617` / Energy Management|Energy Management / Deepening ambiguity|
|O05|fail|`DG-0905` / Production|Production primary is semantically reasonable; expected-primary issue|
|O06|fail|`DG-0662` / contamination|RA versus WASH/Medical boundary plus unrelated hygiene retrieval|
|O07|partial|`DG-0929` / RA|ethical purpose-priority evidence incomplete|

## 14. Failure Classification

|Case|Classification|Reason|
|---|---|---|
|R01|C + D + E|Guide retrieval miss, Wiki starvation, ownership conflict|
|R02|C + D + E|Guide selection miss and no selected RA evidence|
|R05|H|Acceptable Energy Management / RA coexistence|
|R08|C + D + E|Candidate present but Food wins|
|R09|C + D|Correct domain, incomplete Guide/Wiki selection|
|R11|C + D + E|Candidate present but Food wins|
|R13|C + D|Correct owner, expected evidence miss|
|R14|E|Cross-domain ownership conflict|
|R17|E|Forbidden RA ownership of Repair|
|R20|C + D|Correct domain, review evidence starvation|
|O01|C + D + E|Candidate recall and ownership conflict|
|O02|C + D + E|Reserve intent drifts to Records|
|O04|H|Acceptable observation ambiguity, but frozen primary not met|
|O05|G|Expected ownership issue; Production primary matches the query action|
|O06|H + E|Observation ambiguity and unrelated contamination owner|
|O07|H + D|Acceptable observation ownership with incomplete ethical evidence|

Classification summary:

- Knowledge gap (A): 0 confirmed
- Guide evidence gap (B): 0 confirmed
- Safety issue (I): 0
- Other/new runner issue (J): 0
- Fixture schema issue (F): 0
- concentrated remaining defects: Guide selection/Wiki starvation and cross-domain ownership

No expected value was changed.

## 15. New Regression Count

- target case regression: 0
- non-target case verdict change: 0
- non-target selected-Guide change: 0
- Observation change: 0
- Contract/Retrieval new regression: 0
- stable-domain new regression: 0
- Safety regression: 0

**NEW regression count: 0**

## 16. Changed File / Hash Confirmation

Batch17-I created only validation artifacts:

- `docs/knowledge/batch17_i_resource_allocation_field_test_results.json`
- `docs/knowledge/batch17_i_resource_allocation_field_test_runner_report.md`
- `docs/knowledge/batch17_i_resource_allocation_field_test_revalidation_report.md`

Protected hashes before and after are identical:

|Asset|SHA-256|Result|
|---|---|---|
|Profiles|`e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041`|UNCHANGED|
|Contract|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|UNCHANGED|
|Retrieval fetcher|`b8cf5a113f49dbb390ac90159ea65e10333a4bf695b9df0b5d03540892c9c145`|UNCHANGED|
|Retrieval orchestrator|`5fdc17ba6700cd9a441a4bd157f57a10690730802cdddb7fceafebb2d7d1a8ee`|UNCHANGED|
|Field runner|`d905e1fc0c38d87c429dbb26a110ede1c66326a4c8c93d55384725e0e7c0447c`|UNCHANGED|
|Fixture|`521a5bfef6fae3aa71efe9501ef7e7b1eb6a2b9f317c0e00b38c9a38135468b0`|UNCHANGED|
|Emergency Guides|`a69b6a63857deb938a603ba488e3de684d1cd760c5a26b63ffffb17567d7a97f`|UNCHANGED|
|Guide index|`fc34106859f25d9c603bf28544dc52ca9771f1c4979bbb68423d5370bf253cee`|UNCHANGED|

No Wiki, Guide, Guide-Wiki, profile, Retrieval, ranking, selector, top_k, fallback, Prompt, generation, Contract, Safety Gate, fixture, expected ownership, PocketBase, or database file was modified.

## 17. Stable-Candidate Judgement

The Minimal Apply is correct and tightly scoped:

- all three approved recall gaps are fixed;
- only the three target cases changed;
- Safety and Regression gates pass;
- Observation behavior is unchanged.

It is not yet sufficient to freeze Resource Allocation Retrieval v0.1. Seven strict failures remain, including a concentrated group of candidate-present Guide selection/Wiki starvation failures (R01, R02, R08, R11), plus explicit ownership failures in R14 and R17. These are not Batch17-H regressions, but they remain system-level Retrieval/ownership debt and prevent final verification.

No Apply is authorized in this Batch. The next stage should review the remaining strict selection and ownership paths without widening Resource Allocation profiles.

**READY FOR ROOT CAUSE REVIEW**
