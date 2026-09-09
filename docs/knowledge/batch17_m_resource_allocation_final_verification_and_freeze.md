# Batch17-M Resource Allocation v0.1 Final Verification + Freeze

Date: 2026-09-02

Authority: `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`

Final status: **RESOURCE ALLOCATION FINAL VERIFICATION BLOCKED**

## 1. Final baseline

- Verification commit: `92ae7b2d3a51ff4085c4790cabd3d189e2c99479` (`知识库阶段性成果提交`).
- Runtime mode: `contract_mode=production`, limited canary enabled.
- Field Test path: production `run_retrieval_v2()` through normalization, Contract evaluation, ownership decision, ownership-intent lane, Retrieval, and Safety Gate.
- Fixture validation: 28 formal cases, 20 strict and 8 observation.
- Answer generation: disabled with `--no-answer`; planner and selector remained production-equivalent.
- Model environment: configured LAN Ollama service and `qwen3.8:27b`; intermittent HTTP 500 and 120-second selector timeouts correctly entered production fallback.
- Apply/tuning performed in Batch17-M: **none**.

Field Test command:

```bash
python3 scripts/test_resource_allocation_field.py --no-answer \
  --results-path docs/knowledge/.batch17_m_resource_allocation_field_test_results.tmp.json \
  --report-path docs/knowledge/.batch17_m_resource_allocation_field_test_runner.tmp.md \
  --artifact-label Batch17-M
```

## 2. Frozen asset inventory

|Asset|Freeze candidate|
|---|---:|
|Resource Allocation Wiki|10|
|Resource Allocation Guide|5 (`DG-0926` through `DG-0930`)|
|New bidirectional Guide-Wiki relationships|48|
|Narrow Resource Allocation profiles|3|
|Ownership intents|2|
|Fixture correction|R05 only|

The inventory is complete. Pre-test and post-test hashes are identical. Batch17-M did not alter Knowledge, Guide, Guide-Wiki, profile, ownership-intent, Retrieval, Contract, Safety, fixture, database, or PocketBase state.

## 3. 28-case Field Test

Candidate rank is the best rank among the frozen expected or accepted Guides. `absent` means none entered the production candidate set.

|Case|Type|Verdict|Selected Guide(s)|Primary owner|Candidate rank|Classification|
|---|---|---|---|---|---:|---|
|R01|strict|fail|`DG-0860`|Records|3|Known selection debt|
|R02|strict|fail|`DG-0551/DG-0462/DG-0563`|Water|absent|**New recall regression indicator**|
|R03|strict|pass|`DG-0927/DG-0930/DG-0929`|Resource Allocation|1|Stable profile target|
|R04|strict|pass|`DG-0927`|Resource Allocation|1|Stable|
|R05|strict|fail|`DG-0563/DG-0337/DG-0561`|Energy Management|absent|**R05 hard-gate failure**|
|R06|strict|pass|`DG-0928`|Resource Allocation|1|Stable profile target|
|R07|strict|fail|`DG-0551`|Water|2|**New selection/ownership regression indicator**|
|R08|strict|fail|`DG-0599/DG-0596/DG-0927`|Food|3|Known selection debt|
|R09|strict|partial|`DG-0927`|Resource Allocation|1|Known precision debt|
|R10|strict|pass|`DG-0930`|Resource Allocation|1|Stable|
|R11|strict|fail|`DG-0545`|Food|3|Known selection debt|
|R12|strict|pass|`DG-0929`|Resource Allocation|1|Stable profile target|
|R13|strict|fail|`DG-0617/DG-0564/DG-0922`|Energy Management|7|Known selection debt|
|R14|strict|partial|`DG-0920`|Energy Deepening|2|Frozen Guide precision debt|
|R15|strict|pass|`DG-0897`|Agriculture / Storage|1|Stable boundary|
|R16|strict|pass|`DG-0914`|Records|1|Stable boundary|
|R17|strict|pass|`DG-0923`|Repair|absent|Stable ownership-intent boundary|
|R18|strict|pass|`DG-0928/DG-0929/DG-0930`|Resource Allocation|1|Stable|
|R19|strict|pass|`DG-0546/DG-0111/DG-0842`|Energy Safety|3|Safety first; Contract applied|
|R20|strict|partial|`DG-0927`|Resource Allocation|2|Known precision debt|
|O01|observation|fail|`DG-0599/DG-0551/DG-0563`|Food|absent|Known cross-domain debt|
|O02|observation|fail|`DG-0916`|Records|absent|Known reserve/Records debt|
|O03|observation|pass|`DG-0926/DG-0126/DG-0388`|Resource Allocation|1|Acceptable|
|O04|observation|fail|`DG-0617`|Energy Management|2|Acceptable Energy ambiguity|
|O05|observation|fail|`DG-0905`|Production|absent|Known fixture debt; actual owner correct|
|O06|observation|fail|`DG-0551/DG-0599/DG-0930`|Water|absent|Known WASH/Medical/RA debt|
|O07|observation|partial|`DG-0929`|Resource Allocation|3|Acceptable RA/Organization ambiguity|
|O08|observation|pass|`DG-0841`|Energy Safety|1|Safety first|

## 4. Strict results

|Metric|Result|
|---|---:|
|Strict total|20|
|Pass|10|
|Partial|3|
|Fail|7|

Pass: R03, R04, R06, R10, R12, R15, R16, R17, R18, R19.

Partial: R09, R14, R20.

Fail: R01, R02, R05, R07, R08, R11, R13.

The aggregate count is unchanged from Batch17-I, but case composition is not stable. R14 and R17 improved as designed, while R02 lost its expected/accepted candidate, R07 changed from pass to fail, and R05 did not reproduce the Batch17-L targeted pass.

## 5. Observation results

|Metric|Result|
|---|---:|
|Observation total|8|
|Pass|2|
|Partial|1|
|Fail|5|

|Case|Classification|Freeze impact|
|---|---|---|
|O01|KNOWN DEBT|Non-blocking; Food/fuel scarcity remains cross-domain|
|O02|KNOWN DEBT|Non-blocking; reserve release competes with Records|
|O03|PASS|None|
|O04|ACCEPTABLE AMBIGUITY|Non-blocking; two-week solar planning spans Management and Deepening|
|O05|KNOWN FIXTURE/OWNERSHIP DEBT|Non-blocking; Production primary is semantically correct|
|O06|KNOWN DEBT|Non-blocking; hygiene consumables span RA, WASH, and Medical|
|O07|ACCEPTABLE AMBIGUITY|Non-blocking; RA and Organization/Training overlap|
|O08|PASS|None|

No Observation case produced a Safety issue. These outcomes do not justify expanding Resource Allocation.

## 6. R03 / R06 / R12

|Case|Profile|Required Guide|Result|
|---|---|---|---|
|R03|`resource_allocation_consumption_horizon`|`DG-0927` rank 1 / accepted|PASS|
|R06|`resource_allocation_emergency_reserve_release`|`DG-0928` rank 1 / accepted|PASS|
|R12|`resource_allocation_high_energy_task_decision`|`DG-0929` rank 1 / accepted|PASS|

Candidate recall for all three narrow profiles is stable.

## 7. R05

- Expected primary: Energy Management.
- Actual primary: Energy Management.
- Selected top Guide: `DG-0563`.
- Frozen accepted Guides: `DG-0845`, `DG-0844`, `DG-0928`.
- Accepted Guide candidate: absent.
- Frozen expected Wiki evidence: absent.
- Verdict: **FAIL**.

The ownership boundary is correct and Resource Allocation did not take primary. However, Batch17-M explicitly requires R05 to pass the frozen Guide/Wiki evidence contract. The semantically adjacent `DG-0563` is outside that contract, so this cannot be reclassified as PASS.

## 8. R14

- Ownership intent: `long_term_energy_capacity_trend`, status `APPLIED`.
- Primary: Energy Deepening.
- Secondary: Records and Resource Allocation.
- Medical primary: eliminated.
- Selected Guide: `DG-0920`, a valid Energy Deepening Guide.
- Evidence covers long-term generation shortage, degraded operation, generation-component failure, and essential-load reserve.
- Candidate set preserved: true.
- Verdict: PARTIAL.

The remaining partial is the authorized precision mismatch between frozen `DG-0919/DG-0922` and legitimate `DG-0920`. There is no new Knowledge or ownership gap.

Disposition: **FROZEN GUIDE PRECISION DEBT**.

## 9. R17

- Ownership intent: `explicit_equipment_repair`, status `APPLIED`.
- Primary: Repair.
- Secondary: Tools and Resource Allocation.
- Selected Guide: `DG-0923`, annotated into the Repair ownership lane.
- Candidate set preserved: true.
- Verdict: PASS.

Resource Allocation does not take primary for a direct repair request.

## 10. R19 / O08 Safety

|Check|R19|O08|
|---|---|---|
|Primary|Energy Safety|Energy Safety|
|Guide evidence|`DG-0842` present at rank 3; Safety Guide selected|`DG-0841` rank 1|
|Contract|`active_hazard_to_energy_safety`, `CANARY_APPLIED`|Original Retrieval already Safety first|
|Safety Gate|PASS|No intervention required|
|Dangerous suggestion|0|0|
|Scarcity override|0|0|

Safety remains primary despite the resource being unique or irreplaceable.

## 11. Seven selection debts

|Case|Frozen state|Current rank|Current selection|Result|
|---|---|---:|---|---|
|R01|Candidate-present debt|3|`DG-0860` / Records|UNCHANGED DEBT|
|R02|Candidate-present debt|absent|Water Guides|**REGRESSED**|
|R08|Candidate-present debt|3|Food Guides|UNCHANGED DEBT|
|R09|Precision debt|1|`DG-0927` / RA|UNCHANGED DEBT|
|R11|Candidate-present debt|3|`DG-0545` / Food|UNCHANGED DEBT|
|R13|Candidate-present debt|7|Energy Management Guides|UNCHANGED DEBT|
|R20|Precision debt|2|`DG-0927` / RA|UNCHANGED DEBT|

Six remain selection/precision debt. R02 violates the condition that the accepted Guide remain in candidates and therefore blocks freeze.

R07 is outside the frozen seven. It changed from pass in Batch17-I to fail here while its accepted Guide remained rank 2. This new production-selection/ownership indicator also blocks freeze.

## 12. Capability matrix

|Capability|Pass|Partial|Fail|Status|
|---|---:|---:|---:|---|
|RA-01 Resource Inventory|3|0|1|STABLE WITH DEBT|
|RA-02 Consumption Baseline|4|1|1|STABLE WITH DEBT|
|RA-03 Safety Minimum|3|1|4|STABLE WITH DEBT|
|RA-04 Reserve Policy|3|1|7|STABLE WITH DEBT|
|RA-05 Priority Allocation|2|1|6|STABLE WITH DEBT|
|RA-06 Allocation Horizon|3|2|6|STABLE WITH DEBT|
|RA-07 Replenishment|1|1|3|STABLE WITH DEBT|
|RA-08 Substitution|1|0|1|STABLE WITH DEBT|
|RA-09 Degraded Mode|1|1|3|STABLE WITH DEBT|
|RA-10 Trigger Threshold|2|1|0|STABLE WITH DEBT|
|RA-11 Review Loop|4|2|0|STABLE WITH DEBT|
|RA-12 Uncertainty|1|1|4|STABLE WITH DEBT|

Capability `BLOCKED` count is 0: every capability has positive evidence and no systematic Knowledge absence. Overall freeze is still blocked by explicit case-level hard gates.

## 13. Domain boundary matrix

|Domain|Verification|Judgement|
|---|---|---|
|Storage|R15 remains physical seed storage|PASS|
|Energy Management|R05 and R13 remain Energy Management primary|PASS WITH R05 GUIDE DEBT|
|Energy Deepening|R14 is Energy Deepening primary; O04 is accepted ambiguity|PASS|
|Records|R16 remains Records; O02 is registered debt|PASS WITH DEBT|
|Production|O05 selects Production primary; RA does not swallow production choice|PASS|
|Manufacturing|No new RA-primary capture; historical suite failure unchanged|PASS|
|Repair|R17 is Repair primary with Tools/RA secondary|PASS|
|Medical|R14 no longer selects Medical; RA does not enter treatment decisions|PASS|
|Safety|R19 and O08 remain Safety first|PASS|

## 14. Ownership conflict count

- Systemic RA ownership conflicts in mandatory anchors R05/R14/R17/R19/O08: **0**.
- Case-level unexpected ownership selection outside those anchors: **1 (R07)**.
- R05 ownership is correct; its failure is Guide/Wiki recall and precision.
- R02 combines candidate absence with Water primary and is not a reason to widen RA ownership.

## 15. Safety metrics

|Metric|Result|
|---|---:|
|Safety blocker|0|
|Safety Gate failure|0|
|Dangerous suggestion|0|
|Scarcity overrides safety|0|
|Human-value ranking|0|
|Kiwix override|0|
|Stop condition|100.0%|
|Safety / fallback / record-check|100.0% / 100.0% / 100.0%|

The aggregate safety-priority evidence rate is 66.7%, but no Active Hazard or scarcity case violated Safety ownership. The hard Safety gate is satisfied.

## 16. Regression tests

|Suite|Result|
|---|---:|
|Ownership intents|18 passed|
|RA profile contracts|15 passed|
|R19 Safety path|2 passed|
|Contract + Retrieval contracts|91 passed|
|Batch16 Limited Production canary|20 passed|
|Stable-domain suite|91 passed / 5 failed|

## 17. Stable-domain absolute result

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
|Absolute total|**91 / 96 passed; 5 failed**|

The five failures exactly match the frozen pre-Batch16 baseline: Energy Safety x2, Manufacturing x1, and Agriculture Second Stage x2.

## 18. NEW regression

Stable-domain baseline-relative result:

- frozen failures: 5;
- current failures: 5;
- same nodes and signatures: 5/5;
- stable-domain NEW regression: **0**.

Resource Allocation final-verification indicators:

1. R02 changed from candidate-present to candidate-absent.
2. R07 changed from pass to fail.
3. R05's Batch17-L targeted PASS did not reproduce in the complete production run.

These three indicators do not authorize an Apply. Because the overall hard gate requires `NEW regression = 0`, freeze is not permitted.

## 19. Knowledge audits

|Audit|Result|
|---|---|
|Wiki|1167 Markdown / 1167 PocketBase; errors 0, warnings 0, advisories 0|
|Guide|856 Guides; errors 0, warnings 0, advisories 0|
|Guide-Wiki|All slugs valid; forward/reverse graph fully symmetric|
|RA inventory|10 Wiki, 5 Guide, 48 relationships|

Knowledge integrity is not a blocker.

## 20. Hash / freeze manifest

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

### Guides

|Guide|SHA-256|
|---|---|
|`DG-0926`|`72be9382b002359a9c0d928dd5477a9d4600b2bb9a316fc0ad6cb6a00a00b4f5`|
|`DG-0927`|`eccc905cda11e5e5c5772cdd64e9ac1057be658649a3a9db64e0170da03dfc3d`|
|`DG-0928`|`2c43fc3b75ce96a35db375978566641e1f0801584d78709344ef04f32ad2419d`|
|`DG-0929`|`7384f7f88855b353424ab234c513f4eb4b7e6abbeb8395c935641f7b0e197a05`|
|`DG-0930`|`db022df375d3aa79a1c9383598dfcc09a9aa9737f9f4f09761ffdabb8e66f36d`|

### Relationships, profiles, and ownership intents

|Asset|Count|SHA-256|
|---|---:|---|
|Canonical sorted `Guide|slug` representation|48|`919672924fad1903034d046d079d4f3384a578ae4485a015586d5d07ea6c5588`|
|`resource_allocation_consumption_horizon`|1|`085d488fd0640511db521da6d9b1353bcfc06ff9206ed98ced52e432dfb29bed`|
|`resource_allocation_emergency_reserve_release`|1|`bc827ae298eaa149116b047bc2bbf6e25f356eb08bd4f7e51ff455f57c25e45e`|
|`resource_allocation_high_energy_task_decision`|1|`68452a9475f0aeeb5f9a4c15815a4233597ea6ca4ff48ccfaa3cfd74d8d52c2b`|
|OI-01 branch fingerprint|1|`b7b317916be9013697f22608049fd993791276bf6d1cbccacb948dbef8111a67`|
|OI-02 branch fingerprint|1|`eaaf29a1a13796aa307635c863e2a9b43d1679ea9cf4928189d9cc54c8cf3c5d`|
|Complete ownership-intent module|2 intents|`6692228bbc504b673eb654d7a00e61ff2d8e337fe70e2f23108972f459409144`|

### Fixture, Contract, and Retrieval

|Asset|SHA-256|
|---|---|
|RA fixture|`0d93d5a3f0c0fe3238dc90608b6c7b1984193b41a3aaba3af39a3c2ca588dbc9`|
|Cross-domain Contract|`18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2`|
|Query profile source|`e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041`|
|Orchestrator|`a252cc3c9dbff7e887dfc16a0c711947b1e9508d6c2e4d65c316372f427da771`|
|Fetchers|`b8cf5a113f49dbb390ac90159ea65e10333a4bf695b9df0b5d03540892c9c145`|
|Contract canary|`7be88c3867301d2e494a61b2f7e4b724f193b5e5ead40b5355c58d0d059140fc`|
|Contract runtime|`6ddcc27377d86bd2585f6471a0c891b8fa69caff3eaad43b62aa7cf6d99cb90e`|
|Contract compare|`4a216582bd5a659f00020dc58ad14656661f2abf63ac20caa57ba59638f910e2`|
|Runtime settings|`313a5ef5d7a1c634ab88d54cf9429606654052a38320c46339d31f52ad19eb02`|
|RA runner|`033e75bcfbc4e246c98c3a55a6cb1021e7cfbebedf50e61523c8404b731d0043`|

Pre-run and post-run manifests are byte-for-byte equal.

## 21. Final debt register

### A. Non-blocking selection debts

- R01, R08, R09, R11, R13, and R20 remain candidate-present selection/precision debt.
- R02 cannot remain only non-blocking because its accepted candidate is absent in this run.

### B. Frozen Guide precision debt

- R14: Energy Deepening ownership and evidence are correct; `DG-0920` differs from the frozen precision list.

### C. Observation debt / acceptable ambiguity

- Known debt: O01, O02, O05, O06.
- Acceptable ambiguity: O04, O07.
- Passing anchors: O03, O08.

### D. Pre-existing stable-domain debt

- Energy Safety x2.
- Manufacturing x1.
- Agriculture Second Stage x2.

### E. Final-verification blockers

- R02 accepted Guide candidate absent.
- R07 changed from pass to fail with unexpected Water primary.
- R05 did not meet the mandatory PASS contract.

No blocker is fixed or tuned in Batch17-M.

## 22. Freeze policy

The v0.1 asset set remains a coherent freeze candidate, but final freeze is **not granted** by this run.

Until a separately authorized Root Cause Review determines whether R02/R05/R07 represent model-service variance, planner instability, selector instability, or a reproducible Retrieval defect:

- do not add or widen RA profiles;
- do not tune ranking, selector, `top_k`, or fallback;
- do not change ownership intents or Contract behavior;
- do not modify Wiki, Guide, or Guide-Wiki relationships;
- do not change fixture expected values to manufacture passes;
- keep verified Safety and stable-domain boundaries frozen.

## 23. Final judgement

|Gate|Result|
|---|---|
|Safety blocker = 0|PASS|
|Safety Gate failure = 0|PASS|
|Dangerous suggestion = 0|PASS|
|Scarcity override = 0|PASS|
|Stable-domain NEW regression = 0|PASS|
|Overall verification NEW regression = 0|**FAIL**|
|Systemic RA ownership conflict = 0|PASS|
|R03/R06/R12 stable|PASS|
|R05 correct and PASS|**FAIL**|
|R14 ownership correct|PASS|
|R17 correct|PASS|
|R19/O08 Safety first|PASS|
|Capability BLOCKED = 0|PASS|
|Wiki/Guide/Relation audits = 0/0/0|PASS|

Resource Allocation v0.1 cannot be marked stable and frozen from this run. The next valid step is a read-only Root Cause Review of R02, R05, and R07, with no Apply in Batch17-M.

**RESOURCE ALLOCATION FINAL VERIFICATION BLOCKED**
