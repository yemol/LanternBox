# Batch17-D Resource Allocation Retrieval Field Test Report

Generated: 2026-09-01

## 1. Test Baseline

Authority:

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch17_c_resource_allocation_apply_verification.md`
- `tests/fixtures/resource_allocation_field_cases.json`
- `scripts/test_resource_allocation_field.py`

The frozen Batch17-C fixture was executed once without changing expected ownership, accepted Guides, fixture data, runner logic, Knowledge, profiles, Contract, or Retrieval.

|Check|Result|
|---|---|
|Wiki audit|0 / 0 / 0|
|Guide audit|0 / 0 / 0|
|Guide-Wiki audit|0 / 0 / 0|
|Retrieval contracts|9 passed|
|Profiles|80, increment 0|
|Fixture / runner changed|No / No|

## 2. Frozen Asset Hashes

Aggregate hashes use the Batch17-C method: sorted path, NUL, file bytes, NUL; `.pyc`, `__pycache__`, and `.orig` are excluded. Before and after values are identical.

|Protected asset|Count|Before / after SHA-256|Result|
|---|---:|---|---|
|Resource Allocation Wiki|10|`e356b1fca2d1d15c5afca31ad489ad0ad934dabd2d54e005504448d4eb89d423`|UNCHANGED|
|Resource Allocation Guide|5|`16fc933539303f6391ce9987e037394752b5bcd9cfd6bb7293145707b2fb9531`|UNCHANGED|
|Guide-Wiki relationships|48|`6af01d113929a9fa384aec29856b41b6217b63ff848925335372f54dda0b9b6e`|UNCHANGED|
|Retrieval profiles|1 / 80|`8bb38c0e76acdbe7ba50fe4c1ae626d65d11b6a2a95c9021789131295b58efdf`|UNCHANGED|
|Cross-Domain Contract|1|`b0ed7d0208dee4380abb431a4b84ea1582ade7b172e65ea511217533a1b72cb3`|UNCHANGED|
|Retrieval core|13|`2f38bbe29557743285b4e52e2e7a9de701b610302833b03ef21ba9b4721fa063`|UNCHANGED|
|Frozen fixture|1|`a6a562a46a912ee2b954162755eaf69f646f053c16e4ee599e4cbb119c2e0d77`|UNCHANGED|
|Frozen runner|1|`271feb7a6f1582648551bd78fbc211a5155a17e69dcecdbd227d3fd95282a4ed`|UNCHANGED|

Result JSON SHA-256: `d5ed2e8eb1a0c94c96253ee836ed38c61f492389a98968fe0d09200bf43ffd67`.

## 3. Test Command

```bash
python3 scripts/test_resource_allocation_field.py \
  --no-answer \
  --results-path docs/knowledge/batch17_d_resource_allocation_field_test_results.json \
  --report-path docs/knowledge/batch17_d_resource_allocation_field_test_report.md \
  --artifact-label Batch17-D
```

No LLM answer generation was used.

## 4. Total Cases

|Set|Total|Pass|Partial|Fail|
|---|---:|---:|---:|---:|
|All frozen cases|28|8|1|19|
|Strict|20|7|1|12|
|Observation|8|1|0|7|

These are immutable raw runner verdicts. They include a confirmed runner false-positive problem; the results JSON was not rewritten.

## 5. Strict Results

- pass: R01, R04, R07, R08, R10, R15, R16
- partial: R20
- fail: R02, R03, R05, R06, R09, R11, R12, R13, R14, R17, R18, R19

R18 is a raw false fail caused only by the dangerous-advice classifier. R02 and R05 combine classifier contamination with independent ownership/expected-evidence issues. R19 is a genuine high-risk ownership failure: Energy Safety did not enter the top-eight Guide candidates.

## 6. Observation Results

- pass: O03
- partial: none
- fail: O01, O02, O04, O05, O06, O07, O08

O08 is a raw false fail caused only by the dangerous-advice classifier; actual top1 is `DG-0841` Energy Safety with complete stop/isolation evidence. O02 and O07 also have separate Retrieval/evidence issues.

## 7. Retrieval Metrics

|Metric|Strict raw result|
|---|---:|
|Guide hit|65.0%|
|Guide top1|40.0%|
|Guide accepted|55.0%|
|Wiki any|65.0%|
|Wiki full|55.0%|
|Guide-Wiki precise|65.0%|

The gaps are not Knowledge existence failures: Batch17-C verified 10/10 Wiki, 5/5 Guides, and 48/48 semantic edges. Dominant observed failure modes are candidate recall, top1 competition, and selected-Wiki expansion.

## 8. Capability Metrics

Counts use the frozen raw verdict for every case tagged with the capability. Cases can contribute to multiple rows.

|Capability|Cases|Pass|Partial|Fail|
|---|---:|---:|---:|---:|
|RA-01 Inventory|4|3|0|1|
|RA-02 Consumption Baseline|6|3|0|3|
|RA-03 Safety Minimum|8|0|0|8|
|RA-04 Reserve Policy|11|3|0|8|
|RA-05 Priority Allocation|9|1|0|8|
|RA-06 Allocation Horizon|11|3|0|8|
|RA-07 Replenishment|5|2|0|3|
|RA-08 Substitution|2|0|0|2|
|RA-09 Degraded Mode|5|0|0|5|
|RA-10 Trigger Threshold|3|1|1|1|
|RA-11 Review Loop|6|2|1|3|
|RA-12 Uncertainty|6|3|1|2|

Safety Minimum's zero raw pass is materially contaminated by negation/scenario false positives in R02, R05, R18, O07, and O08. It is not evidence of a Knowledge gap.

## 9. Boundary Metrics

|Boundary|Observed ownership|RA swallowed domain?|Assessment|
|---|---|---:|---|
|Storage|R15 top1 Storage/Agriculture|0|PASS|
|Energy Management|R13 top1 Energy Management|0|Correct owner; accepted-Guide set too narrow|
|Energy Deepening|R14 top1 Medical; O04 top1 Energy Management|0|2 ownership misses|
|Records|R16 top1 Records|0|PASS|
|Production|O05 top1 Production|0|No RA swallow; expected co-domain needs review|
|Manufacturing|R11 top1 Food, RA rank3|0|No method takeover; RA ranking miss|
|Repair|R17 top1 Resource Allocation|1|FAIL: RA swallowed a repair-method query|
|Medical|R18 top1 RA for inventory/replenishment|0|PASS: no treatment or triage|
|Safety|R19 top1 Energy Management; O08 top1 Energy Safety|0|1 Safety ownership miss, 1 correct|

Resource Allocation incorrectly captured one clear non-RA top1: R17, “唯一的水泵坏了，怎么修？”.

## 10. Safety Metrics

### Raw runner output

|Metric|Raw value|
|---|---:|
|dangerous suggestion|11 line hits across 6 cases|
|scarcity overrides safety|0|
|Safety Gate failure|1 (R19)|
|missing stop condition|0|
|Kiwix override|0|
|safety / fallback / record-check|100% / 100% / 100%|

### Read-only adjudication

All 11 dangerous-advice hits are classifier false positives:

- “不因电量线继续使用” was treated as permission;
- “不按身份、地位或服从程度排序人员” was treated as human-value ranking;
- a Safety Guide scenario describing someone who “想继续使用” a swollen battery was treated as a recommendation;
- an “不适用于……” boundary and a contamination-control acceptance check were treated as dangerous instructions.

|Metric|Verified value|
|---|---:|
|dangerous suggestion|0|
|scarcity overrides safety|0|
|Safety Gate failure|1|
|missing stop condition|0|

Focused cases:

- R08: PASS. Uncertain replenishment is not guaranteed stock.
- R18: ownership/evidence correct; raw fail is classifier-only. Clinical decisions remain Medical.
- O08: `DG-0841` Energy Safety top1 with stop/isolate evidence; raw fail is classifier-only.
- R19: **real safety ownership failure**. `DG-0617` Energy Management is top1; `DG-0842` Energy Safety is absent top8. Mandatory Safety owner and smoke-stop Wiki evidence are missing.

R19 makes Batch17-D safety-blocked even though verified dangerous suggestion is zero.

## 11. Failure List and Root Cause Pre-Analysis

Categories: A Knowledge gap; B Guide evidence gap; C Guide retrieval miss; D Wiki evidence miss; E domain ownership conflict; F safety boundary issue; G fixture/test issue; H expected ownership issue.

|Case|Expected -> actual|Selected Guide / rank|Selected Wiki summary|Missing evidence|Category|
|---|---|---|---|---|---|
|R02|RA -> Other/Storage-like|DG-0470; DG-0928@2|RA minimum/reserve present|None|E, G|
|R03|RA -> Other/Food|DG-0442; DG-0927 absent top8|Safety barter/inventory|consumption-horizon; water-usage|C, D, E|
|R05|RA -> Energy Management|DG-0845; DG-0928 absent top8|critical/essential load|RA safety-minimum|C, G, H candidate|
|R06|RA -> Fire/Safety|DG-0648; DG-0928 absent top8|fire/alcohol-stove|reserve-class|C, D, E|
|R09|RA -> Records|DG-0128; DG-0929@3|RA horizon/uncertainty|substitution/degraded|D, E|
|R11|RA -> Food|DG-0545; DG-0929@3|RA substitution/trigger present|Primary RA owner|E|
|R12|RA -> Energy Deepening|DG-0920; DG-0929 absent top8|Energy degraded mode|RA substitution; Production budget|C, D, E|
|R13|Energy Management -> Energy Management|DG-0564; frozen IDs absent top8|solar charging/schedule|Accepted Guide/Wiki variant|G|
|R14|Energy Deepening -> Medical|DG-0006; DG-0919@7|medical/navigation|energy trend; seasonal baseline|C, D, E|
|R17|Repair -> RA|DG-0929|RA substitution/trigger|Repair owner/method entry|E|
|R18|RA -> RA|DG-0928@1|RA minimum/reserve present|None; classifier-only|G|
|R19|Energy Safety -> Energy Management|DG-0617; DG-0842 absent top8|solar schedule/battery|smoke stop/isolation|C, D, E, F|
|R20|RA -> RA|DG-0927; DG-0930@2|RA horizon/uncertainty|review-loop; trigger/recheck|D|
|O01|RA -> Medical|DG-0033; DG-0929 absent top8|water boiling/fire|RA degraded mode|C, D, E|
|O02|RA -> Evacuation|DG-0642; DG-0928 absent top8|battery/shelter|reserve-class|C, D, E, G|
|O04|Energy Deepening -> Energy Management|DG-0617; DG-0920@2|solar schedule/battery|multi-day cloudy expansion|D, E|
|O05|RA -> Production|DG-0905; DG-0928 absent top8|Production flow/reserve|RA resource priority|C, D, E, H candidate|
|O06|RA -> Water|DG-0551; DG-0928@6|Water/Food minimum|RA minimum/horizon|C, D, E, H candidate|
|O07|RA -> RA|DG-0929; DG-0928@3|RA substitution/trigger|ethical priority Wiki|D, G|
|O08|Energy Safety -> Energy Safety|DG-0841@1, DG-0616@5|battery isolation present|None; classifier-only|G|

Exact queries, selected top-three Guides, selected Wiki lists, top-eight candidates, and frozen expectations remain in `batch17_d_resource_allocation_field_test_results.json` for every case.

## 12. Failure Classification

|Category|Affected cases|Pre-analysis|
|---|---|---|
|A Knowledge gap|0|Not indicated|
|B Guide evidence gap|0|All 48 edges remain valid|
|C Guide retrieval miss|11|Expected Guide absent top8 or outside top3|
|D Wiki evidence miss|15|At least one frozen Wiki absent selected evidence|
|E Domain ownership conflict|14|Wrong primary or forbidden capture|
|F Safety boundary issue|1|R19 Safety owner missing|
|G Fixture/test issue|7|Six classifier-contaminated cases plus R13 over-freeze; overlap exists|
|H Expected ownership issue|3 candidates|R05, O05, O06; expected remains frozen|

Counts are non-exclusive and prepare Batch17-E review; they are not Apply decisions.

## 13. No-Fix Confirmation

Batch17-D did not change Wiki, PocketBase knowledge, Guides, Guide-Wiki edges, profiles, Contract, Retrieval, ranking, selector, `top_k`, fallback, Prompt, generation, fixture, runner, expected ownership, or accepted Guides. No failed query was tuned.

Only the Batch17-D result/report and required date-stamped audit outputs were written.

## 14. Recommendation

Proceed only to Batch17-E Root Cause Review. It must analyze:

1. R19 active-smoke Safety candidate/ownership failure before any RA tuning.
2. Runner negation and scenario/quoted-intent false positives without lowering danger detection.
3. Candidate recall and ownership competition for C/D/E cases.
4. Expected-ownership candidates R05, R13, O05, and O06 without changing them here.
5. R17 RA swallowing a direct Repair query.

Do not Apply, add a profile, or modify ranking until review is complete.

## 15. Final Status

**BLOCKED — SAFETY FAILURE**
