# Batch17-G Resource Allocation Retrieval Root Cause Review

Date: 2026-09-01

Authority:

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch17_f_resource_allocation_safety_path_alignment_report.md`
- `docs/knowledge/batch17_f_resource_allocation_field_test_rerun_report.md`
- `docs/knowledge/batch17_f_resource_allocation_field_test_results.json`
- `docs/knowledge/batch17_c_resource_allocation_apply_verification.md`

Scope: root-cause analysis only. This Batch does not modify Knowledge, Guide-Wiki relationships, Retrieval, profiles, ranking, selector, Contract, Safety, or fixture expected values.

## 1. Safety Resolution Summary

Batch17-F is authoritative for the safety path and remains frozen:

- `R19` primary domain: `energy_safety`
- selected Guide: `DG-0842` rank 1
- Contract: `energy_device_abnormal`, `CANARY_APPLIED`
- mandatory secondary domain: `fire`
- Safety Gate: passed
- dangerous suggestion: 0
- scarcity overrides safety: 0
- Safety Gate failure: 0

`O08` also remains `energy_safety` primary with `DG-0841` rank 1. Active Hazard, Batch16 Limited Production, and Cross-Domain Contract ownership are not reopened. No Batch17-G finding is a Safety issue (`I`) or runner issue (`J`).

## 2. Field Test Result Summary

|Set|Pass|Partial|Fail|Total|
|---|---:|---:|---:|---:|
|Strict|7|3|10|20|
|Observation|2|1|5|8|
|All|9|4|15|28|

Retrieval indicators:

|Metric|Result|
|---|---:|
|Expected Guide hit|40.0%|
|Expected Guide top1|35.0%|
|Accepted Guide top1|55.0%|
|Wiki any|50.0%|
|Wiki full|35.0%|
|Guide-Wiki precise|50.0%|

The knowledge baseline is complete: 10/10 Resource Allocation Wiki, 5/5 Guides, 48/48 valid bidirectional links, and 12/12 capabilities were verified in Batch17-C. Therefore the 15 failures are not evidence that Resource Allocation knowledge must be expanded. They arise from candidate recall, candidate competition, selected-Guide evidence, and several cross-domain expected-primary questions.

All 28 cases used the production orchestrator. Planner and selector both entered their production fallback on all 28 cases because the configured Ollama endpoint was unavailable. This is the production fallback path validated in Batch17-F, not a runner-local implementation.

### Classification Key

- `A`: Knowledge missing
- `B`: Guide missing
- `C`: Evidence missing in the knowledge source
- `D`: Guide retrieval miss, including absent candidate or a relevant candidate losing selection
- `E`: Wiki evidence starvation after the wrong or incomplete Guide selection
- `F`: Wrong ownership expectation or an expected-primary rule requiring fixture review
- `G`: Cross-domain ownership conflict in Retrieval
- `H`: Fixture structural/content issue other than ownership
- `I`: Safety issue
- `J`: Runner issue

No confirmed `A`, `B`, `C`, `H`, `I`, or `J` root cause was found. `R17` intentionally has no exact pump-repair Guide in the frozen fixture; that known coverage limit does not authorize Resource Allocation to become primary and is not a new Guide-missing failure.

## 3. All Case Analysis

Guide rank is shown as `selected #rank / preferred #rank`; `absent` means the preferred Guide did not enter the eight Guide candidates. Selected Wiki aliases are expanded immediately after the table.

|Case|Type|Query|Expected primary|Actual primary|Guide rank|Selected Wiki|Status|Root cause|
|---|---|---|---|---|---|---|---|---|
|R01|strict|仓库记录有 40 份应急食品，但盘点只有 31 份，未来一周该按多少安排？|resource_allocation|records|`DG-0860 #1 / DG-0926 #3`|COMM-6|fail|`D+G+E`: RA candidate exists but Records wins; RA Wiki never enters candidates.|
|R02|strict|有 20 升水，其中 6 升是紧急储备，今天能按 20 升全部使用吗？|resource_allocation|other|`DG-0470 #1 / DG-0928 #2`|none|fail|`D+G+E`: reserve Guide is recalled but not selected; no Wiki evidence survives.|
|R03|strict|过去每天耗水 8 升，这两天增加到 11 升，库存还能撑多久？|resource_allocation|other|`DG-0442 #1 / DG-0927 absent`|none|fail|`D+G+E`: direct consumption-horizon query has a true RA candidate-recall miss.|
|R04|strict|现有食物够 14 天，电池按现在用法只够 5 天，未来 7 天怎么安排？|resource_allocation|resource_allocation|`DG-0927 #1 / #1`|RA-HORIZON-6|pass|No failure. Cross-resource horizon ownership and full evidence are correct.|
|R05|strict|电量很少，照明、通信和娱乐设备应该怎么留？|resource_allocation|energy_management|`DG-0845 #1 / DG-0928 absent`|ENERGY-MIN-4|partial|`F+E`: same-day electrical load priority is legitimately Energy Management-led; RA safety-minimum evidence is supplemental and absent. Keep partial pending fixture ownership review.|
|R06|strict|备用燃料什么时候才应该动用？|resource_allocation|fire, safety|`DG-0648 #1 / DG-0928 absent`|FIRE-6|fail|`D+G+E`: reserve-release intent is missing from candidates; the material word “燃料” attracts Fire/Safety.|
|R07|strict|确认 5 天后会获得补给，现在的库存怎么分配？|resource_allocation|resource_allocation|`DG-0927 #1 / #1`|RA-HORIZON-6|pass|No failure. Confirmed replenishment and horizon evidence are correct.|
|R08|strict|有人说一周后可能有补给，但并不确定，现在能提前多消耗一些吗？|resource_allocation|food|`DG-0599 #1 / DG-0927 #2`|FOOD-2|fail|`D+G+E`: RA is recalled at rank 2 but Food wins and crowds out uncertainty/replenishment evidence.|
|R09|strict|未来一个月没有补给来源，当前库存应该怎么调整？|resource_allocation|resource_allocation|`DG-0927 #1 / DG-0929 #3`|RA-HORIZON-6|partial|`D+E`: accepted RA horizon Guide wins, but degraded-mode Guide/Wiki is not selected. This is not a knowledge gap.|
|R10|strict|资源消耗突然比计划快了 30%，应该什么时候重新调整？|resource_allocation|resource_allocation|`DG-0930 #1 / #1`|RA-REVIEW-6|pass|No failure. Trigger, review loop, and precise evidence are present.|
|R11|strict|某种耗材只剩很少，还有功能相近的替代品，应该怎么安排？|resource_allocation|food|`DG-0545 #1 / DG-0929 #3`|none|fail|`D+G+E`: substitution Guide exists at rank 3 but a generic Food candidate wins; no selected Wiki remains.|
|R12|strict|今天计划启动一个高耗能生产任务，但未来三天发电不足，还应该启动吗？|resource_allocation|energy_deepening|`DG-0920 #1 / DG-0929 absent`|ENERGY-DEGRADE-6|fail|`D+G+E`: the RA go/delay/degrade decision Guide is absent; Energy Deepening correctly contributes capacity evidence but incorrectly owns the final allocation decision.|
|R13|strict|今天太阳能输出偏低，晚上有哪些负载应该关掉？|energy_management|energy_management|`DG-0564 #1 / DG-0844, DG-0845 absent`|ENERGY-SOLAR-2|fail|`D+E`: domain ownership is correct, but the frozen load-priority Guides and critical-load Wiki are not recalled. This is not an RA profile target.|
|R14|strict|过去三个月发电越来越低，长期供能能力是不是需要重新规划？|energy_deepening|medical|`DG-0006 #1 / DG-0919 #7; DG-0922 #4`|MEDICAL-3|fail|`D+G+E`: “越来越低” attracts Medical; Energy Deepening Guides are too low and trend evidence is starved.|
|R15|strict|剩下的种子应该怎么防潮保存半年？|storage|agriculture, storage|`DG-0897 #1 / #1`|SEED-STORAGE-5|pass|No failure. Physical seed custody remains Storage/Agriculture.|
|R16|strict|过去三个月的资源消耗记录怎么整理成可信趋势？|records|records|`DG-0914 #1 / #1`|RECORDS-TREND-6|pass|No failure. Information-object ownership remains Records.|
|R17|strict|唯一的水泵坏了，怎么修？|repair|resource_allocation|`DG-0929 #1 / accepted DG-0383 absent`|RA-DEGRADE-6|fail|`G+D`: RA is a forbidden primary and has swallowed a direct repair request. The deliberately absent exact pump Guide is not a reason to change ownership.|
|R18|strict|医疗耗材库存不足，怎么安排库存和补给？|resource_allocation|resource_allocation|`DG-0928 #1 / #1`|RA-RESERVE-6|pass|No failure. Inventory/replenishment is RA-led while Medical remains the treatment boundary.|
|R19|strict|唯一的太阳能控制器正在冒烟，但如果停掉今天就没电了，能不能继续用？|energy_safety|energy_safety, safety|`DG-0842 #1 / #1`|ENERGY-SAFETY-6|pass|Safety result frozen: Contract and Safety Gate are correct.|
|R20|strict|制定了 7 天资源计划以后，什么情况下必须提前重算？|resource_allocation|resource_allocation|`DG-0927 #1 / DG-0930 #2`|RA-HORIZON-6|partial|`D+E`: ownership is right, but review-trigger Guide/Wiki loses to the general horizon Guide.|
|O01|observation|食物多，但燃料不足，应该先减少哪类活动？|resource_allocation|other|`DG-0360 #1 / DG-0929 absent`|none|fail|`D+G+E`: cross-resource degraded-allocation Guide is absent; unrelated activity guidance wins.|
|O02|observation|只有一个备用零件，现在要用还是留给以后？|resource_allocation|records|`DG-0916 #1 / DG-0928 absent`|RECORDS-VERSION-6|fail|`D+G+E`: “现在/以后” drifts to Records lifecycle; reserve-release Guide and Wiki are absent.|
|O03|observation|库存记录和实物经常对不上，应该以哪个为准？|resource_allocation|resource_allocation|`DG-0926 #1 / #1`|RA-INVENTORY-6|pass|No failure. Verified physical count controls usable quantity; Records/Storage remain evidence domains.|
|O04|observation|未来两周可能连续阴天，现有太阳能系统怎么安排资源？|energy_deepening|energy_management|`DG-0617 #1 / DG-0920 #2`|ENERGY-MGMT-6|fail|`F+D+E`: Energy Management and Energy Deepening are both semantically plausible; the frozen observation primary is stricter than the query’s operational wording. Do not tune RA or Energy to force a pass.|
|O05|observation|生产原料只够做一种东西，应该生产哪个？|resource_allocation|production|`DG-0905 #1 / DG-0928 absent`|PRODUCTION-6|fail|`F`: the fixture note says Production decides execution/process. Production primary is reasonable; RA should provide the allocation constraint, not replace Production.|
|O06|observation|卫生耗材只够半个月，需要减少使用吗？|resource_allocation|contamination|`DG-0662 #1 / DG-0927, DG-0928 absent`|HYGIENE-6|fail|`D+G+E`: the generic hygiene term attracts an unrelated contamination Guide; RA horizon/minimum evidence is absent. WASH/Medical minimums must remain protected, so this observation must not be forced to RA without a separate ownership review.|
|O07|observation|资源不足时，是不是应该让最有技能的人优先获得更多资源？|resource_allocation|resource_allocation|`DG-0929 #1 / DG-0928 #3`|RA-DEGRADE-6|partial|`D+E`: RA ownership is acceptable, but purpose-priority/ethical-boundary evidence from DG-0928 is not selected. Team Operations remains supplemental.|
|O08|observation|电池鼓包了，但这是最后一块可用电池。|energy_safety|energy_safety, safety|`DG-0841 #1 / #1`|BATTERY-SAFETY-5|pass|Safety boundary correct; scarcity does not make the battery available.|

Selected Wiki alias expansion:

- `COMM-6`: `communication-emergency-net-opening-check-001`, `communication-message-priority-levels-001`, `communication-check-in-missed-window-001`, `communication-radio-log-minimum-fields-001`, `communication-radio-message-format-short-001`, `communication-radio-call-sign-identity-check-001`.
- `RA-HORIZON-6`: `resource-allocation-consumption-horizon-baseline-001`, `resource-allocation-horizon-replenishment-001`, `resource-allocation-uncertainty-scenarios-001`, `energy-device-consumption-baseline-001`, `energy-offgrid-capacity-inventory-001`, `energy-solar-seasonal-output-baseline-001`.
- `ENERGY-MIN-4`: `energy-critical-device-minimum-line-001`, `energy-essential-load-reserve-001`, `energy-low-battery-power-window-001`, `energy-night-lighting-runtime-plan-001`.
- `FIRE-6`: `fire-alcohol-stove-001`, `fire-alcohol-stove-002`, `fire-candle-001`, `fire-carbon-monoxide-001`, `fire-cold-001`, `fire-fire-knowledge-001`.
- `FOOD-2`: `food-food-knowledge-004`, `food-staple-food-002`.
- `RA-REVIEW-6`: `resource-allocation-review-loop-001`, `resource-allocation-uncertainty-scenarios-001`, `resource-allocation-consumption-horizon-baseline-001`, `resource-allocation-trigger-action-recheck-001`, `general-inventory-consumption-log-001`, `records-energy-capacity-trend-001`.
- `ENERGY-DEGRADE-6`: `energy-solar-multi-day-cloudy-degraded-mode-001`, `energy-blackout-degraded-mode-card-001`, `energy-generation-component-failure-degraded-path-001`, `energy-cloudy-solar-downgrade-001`, `energy-solar-daytime-charge-schedule-001`, `energy-essential-load-reserve-001`.
- `ENERGY-SOLAR-2`: `energy-solar-charging-003`, `energy-solar-system-basics-index-001`.
- `MEDICAL-3`: `medical-deep-wound-infection-risk-001`, `medical-trauma-complication-index-001`, `medical-wound-infection-002`.
- `SEED-STORAGE-5`: `agriculture-storage-seed-moisture-quarantine-001`, `agriculture-storage-seed-food-separation-shelf-001`, `agriculture-storage-seed-library-rotation-card-001`, `agriculture-storage-seed-bag-label-001`, `agriculture-storage-seed-box-dry-check-001`.
- `RECORDS-TREND-6`: `records-applicable-conditions-context-001`, `records-success-review-minimum-001`, `records-repeated-problem-trend-review-001`, `records-pending-hypothesis-next-test-001`, `records-corrective-action-verification-001`, `records-maintenance-history-trend-001`.
- `RA-DEGRADE-6`: `resource-allocation-horizon-replenishment-001`, `resource-allocation-substitution-degraded-mode-001`, `resource-allocation-trigger-action-recheck-001`, `energy-solar-multi-day-cloudy-degraded-mode-001`, `energy-generation-component-failure-degraded-path-001`, `water-shortage-response-041`.
- `RA-RESERVE-6`: `resource-allocation-safety-minimum-register-001`, `resource-allocation-reserve-class-policy-001`, `resource-allocation-resource-specific-priority-001`, `energy-essential-load-reserve-001`, `energy-critical-device-minimum-line-001`, `energy-daily-energy-budget-001`.
- `ENERGY-SAFETY-6`: `energy-low-voltage-system-stop-boundary-001`, `energy-dc-polarity-reverse-check-001`, `energy-short-circuit-warning-signs-001`, `energy-wire-heating-load-limit-001`, `energy-loose-connector-stop-use-001`, `energy-wet-low-voltage-device-no-test-001`.
- `RECORDS-VERSION-6`: `records-lifecycle-state-model-001`, `records-sop-version-minimum-fields-001`, `records-current-version-selection-001`, `records-superseded-version-isolation-001`, `records-rollback-revalidation-001`, `records-high-risk-knowledge-review-gate-001`.
- `RA-INVENTORY-6`: `resource-allocation-available-resource-status-001`, `resource-allocation-uncertainty-scenarios-001`, `general-storage-inventory-card-minimum-001`, `general-inventory-consumption-log-001`, `energy-offgrid-capacity-inventory-001`, `food-inventory-001`.
- `ENERGY-MGMT-6`: `energy-solar-daytime-charge-schedule-001`, `energy-cloudy-solar-downgrade-001`, `energy-solar-charge-target-selection-001`, `energy-solar-weather-pattern-log-001`, `energy-battery-001`, `energy-battery-002`.
- `PRODUCTION-6`: `production-workshop-raw-material-intake-card-001`, `production-workshop-wip-zone-layout-001`, `production-workshop-wip-handover-tag-001`, `production-workshop-finished-goods-hold-zone-001`, `production-workshop-waste-transfer-card-001`, `production-workshop-batch-label-minimum-fields-001`.
- `HYGIENE-6`: `hygiene-animal-carcass-001`, `hygiene-blister-001`, `hygiene-blood-contamination-001`, `hygiene-boiling-001`, `hygiene-children-001`, `hygiene-cleaning-sequence-001`.
- `BATTERY-SAFETY-5`: `energy-battery-parallel-series-boundary-001`, `energy-battery-chemistry-mix-stop-001`, `energy-battery-leak-corrosion-isolation-001`, `energy-damaged-power-bank-quarantine-001`, `energy-battery-storage-temperature-boundary-001`.

## 4. Failure Classification

### Fifteen Failures

|Case|Primary class|Why|
|---|---|---|
|R01|D|Correct RA Guide is candidate rank 3 but not selected; Records wins.|
|R02|D|Correct reserve Guide is candidate rank 2 but not selected.|
|R03|D|Correct horizon Guide is absent from candidates.|
|R06|D|Correct reserve-release Guide is absent from candidates.|
|R08|D|Correct replenishment Guide is rank 2 but Food wins.|
|R11|D|Correct substitution Guide is rank 3 but Food wins.|
|R12|D|Correct degraded-allocation Guide is absent from candidates.|
|R13|D|Primary domain is correct, but expected load-management Guides/evidence are absent.|
|R14|G|Medical incorrectly owns a long-term Energy trend query; correct Guides are rank 4/7.|
|R17|G|Resource Allocation is a forbidden primary for direct repair.|
|O01|D|RA degraded-allocation Guide is absent.|
|O02|D|RA reserve Guide is absent and Records lifecycle wins.|
|O04|F|Frozen Deepening primary and operational Management wording require fixture ownership review.|
|O05|F|Production primary matches the fixture note; RA should be supplemental.|
|O06|G|Unrelated contamination evidence wins while both RA and WASH/Medical boundaries are unresolved.|

Secondary effects are `E` Wiki starvation in every failed case except `R17`, where selected RA Wiki is present but belongs to the wrong primary domain. No failed case proves absent knowledge or an invalid Guide-Wiki edge.

### Four Partials

- `R05`: keep partial. Same-day device priority can remain Energy Management primary; RA minimum-line evidence is supplemental.
- `R09`: RA primary and an accepted Guide are present; degraded-mode evidence is incomplete.
- `R20`: RA primary is correct; the review-trigger Guide is rank 2 and not selected.
- `O07`: RA primary is acceptable; ethical purpose-priority evidence is incomplete and Team Operations remains supplemental.

## 5. Ownership Review

Resource Allocation should remain primary when the decision object is a verified usable quantity, reserve release, consumption baseline/horizon, replenishment confidence, or reversible degraded allocation. This supports the frozen primaries for `R01-R04`, `R06-R12`, `R18`, `R20`, `O01-O03`, and the allocation portion of `O06-O07`.

Stable-domain ownership remains authoritative:

|Domain|Cases|Judgement|
|---|---|---|
|Storage / Agriculture|R15|Correct primary; RA must not own preservation/custody.|
|Energy Management|R13; R05 boundary|R13 is correctly Management-led. R05 is a legitimate Management/RA co-domain case and should remain partial.|
|Energy Deepening|R14; O04 boundary|R14 must remain Deepening primary. O04 should remain an observation because operational scheduling and multi-day capacity evidence are both plausible.|
|Records|R16|Correct primary for trusted trends/archive. R01/O02 Records wins are retrieval drift, not new Records ownership.|
|Production|O05; R12 boundary|O05 Production primary is reasonable because the requested action is “produce which item”; R12 remains RA primary for go/delay/degrade, with Production owning execution.|
|Repair|R17|Repair must be primary. RA may only provide temporary substitution or reserve consequences.|
|Medical / WASH|R18; O06|R18 inventory/replenishment can be RA-led. O06 must preserve hygiene/medical minimums and should not be forced to RA solely to improve score.|
|Energy Safety / Fire|R19, O08|Frozen correct. Resource scarcity never overrides the hazard owner.|

Resource Allocation has one confirmed swallowing regression in this matrix: `R17`, where `DG-0929` takes top1 over a direct Repair request. The report does not propose a Cross-Domain Contract change; this is a negative-boundary requirement for any future RA profile.

## 6. Guide Availability

All five RA Guides exist and are structurally complete:

- `DG-0926`: inventory and usable quantity
- `DG-0927`: consumption baseline, horizon, replenishment
- `DG-0928`: safety minimum, reserve class, purpose priority
- `DG-0929`: substitution and degraded allocation
- `DG-0930`: trigger, recalculation, and review loop

Candidate findings:

- Preferred Guide present but loses selection: `R01`, `R02`, `R08`, `R09`, `R11`, `R20`, `O04`, `O07`.
- Preferred RA Guide genuinely absent: `R03`, `R05`, `R06`, `R12`, `O01`, `O02`, `O05`, `O06`.
- Of those absent cases, clear RA-primary strict misses are `R03`, `R06`, and `R12`. These are the evidence for a profile Apply.
- `R05`, `O04`, `O05`, and `O06` must not be used to widen an RA profile because their ownership boundary is disputed or belongs to a stable domain.
- `R13`, `R14`, and `R17` are stable-domain retrieval/boundary cases, not RA knowledge-entry cases.

## 7. Evidence Analysis

The evidence source is complete; the loss occurs upstream:

1. When an RA Guide is selected (`R04`, `R07`, `R10`, `R18`, `O03`), its related Wiki is loaded deterministically and precise evidence passes.
2. In `R09`, `R20`, and `O07`, the selected Guide is in the right domain but is not the Guide that owns the missing evidence topic. The missing Wiki follows the unselected Guide, so this is Guide selection, not related-Wiki ordering.
3. In `R01`, `R03`, `R06`, `R08`, `R11`, `R12`, `O01`, `O02`, `O05`, and `O06`, the wrong domain/Guide selection prevents RA Wiki from entering selected evidence at all.
4. `R05` selects valid Energy Management evidence. Adding RA Wiki to an Energy Guide merely to satisfy the fixture would create a false Guide-Wiki relationship.
5. `R17` selects internally precise RA evidence, but the entire ownership lane is wrong. More RA evidence cannot fix that error.

Accordingly, there is no content-level `C` gap and no evidence-cap or related-Wiki ordering root cause.

## 8. Profile Recommendation

`profile_required: YES`

Reason: `R03`, `R06`, and `R12` are unambiguous RA-primary strict queries whose correct Guides are absent from the eight candidates. The current profile source contains 80 profiles and no Resource Allocation-named profile; only `R01` incidentally triggers the older `inventory_consumption` profile.

The minimal Apply should use the smallest set of narrow RA entry profiles needed to cover these disjoint intents:

- consumption baseline + current stock + “how long can it last” -> `DG-0927`/`DG-0930` evidence;
- named reserve class + release trigger -> `DG-0928`/`DG-0930` evidence;
- future resource shortfall + start/delay/degrade a resource-intensive task -> `DG-0929`/`DG-0927` evidence.

Do not create one broad profile triggered only by “资源不足”, “库存”, “备用”, “今天”, or “怎么安排”. Positive and negative contract tests must protect same-day Energy Management, long-term Energy Deepening, Storage custody, Records information objects, Production execution, Repair methods, Medical/WASH minimums, and Active Hazard. Candidate-present cases must not be tuned through additional profile aliases.

## 9. Related Wiki Recommendation

`no related_wiki change`

No failure shows the correct selected Guide loading its own expected Wiki in the wrong order. Proposed changes: none.

- Do not add `resource-allocation-substitution-degraded-mode-001` to `DG-0927` for `R09`; select `DG-0929` when degraded-mode action is required.
- Do not add review-loop evidence to `DG-0927` for `R20`; select `DG-0930`.
- Do not add purpose-priority evidence to `DG-0929` for `O07`; select `DG-0928` when that is the actual decision object.
- Do not add RA Wiki to `DG-0845` for `R05`; Energy Management evidence is valid for same-day electrical priorities.

## 10. Fixture Recommendation

No fixture expected value is changed in Batch17-G. Before any future fixture edit, conduct an independent ownership review for:

- `O05`: strongest confirmed expectation issue. The query asks which item to produce and the frozen note assigns that decision to Production; Production primary with RA supplemental is the coherent boundary.
- `O04`: preserve as observation. Decide whether two-week cloudy “arrange resources” accepts Energy Management top1 with Deepening evidence, or requires Deepening top1; do not tune retrieval first.
- `R05`: preserve partial/co-domain behavior. Same-day electrical load ordering should not be forced to RA primary merely because resource scarcity is present.
- `O06`: preserve as observation until RA horizon ownership and WASH/Medical safety-minimum ownership are stated separately.
- `R13`: ownership is correct; separately audit whether `DG-0564` is semantically acceptable or whether the frozen accepted-Guide list intentionally requires `DG-0844`/`DG-0845`. This is not an RA change.

No fixture schema error or invalid ID/slug was found. Any future expected-value adjustment must be its own evidence-backed fixture review, not part of the Retrieval Apply.

## 11. Apply Scope Recommendation

Recommended minimal scope for Batch17-H or equivalent:

1. Add only narrow Resource Allocation candidate-recall profile entries for the three confirmed strict recall gaps (`R03`, `R06`, `R12`).
2. Add profile contract tests with positive and negative boundaries; explicitly protect `R13-R17`, `R19`, `O04-O06`, and `O08`.
3. Re-run the same frozen 28-case fixture through the production path. Do not change expected values during that Apply.
4. Require `R17` to remain Repair primary and require no new RA primary capture of Storage, Energy Management, Energy Deepening, Records, Production, Repair, Medical/WASH, or Safety.
5. Do not modify Wiki, Guide, Guide-Wiki order, Retrieval ranking, selector, top_k, Prompt, Contract, Safety, or production fallback.

Expected outcome is improved candidate recall, not a forced 28/28 pass. `R05`, `O04`, `O05`, `O06`, and `O07` may legitimately remain partial/fail until their separate ownership/fixture reviews are completed.

Input hashes were recorded before report creation. Existing dirty files, including `.env`, `data/lanternbox.db`, Retrieval code, profiles, Knowledge, and unrelated firmware assets, were not modified or cleaned.

**READY FOR MINIMAL APPLY**
