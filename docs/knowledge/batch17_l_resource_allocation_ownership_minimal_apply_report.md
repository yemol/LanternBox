# Batch17-L Resource Allocation Ownership Minimal Apply Report

## 1. Batch17-K authority

This Apply follows `batch17_k_resource_allocation_ownership_resolution_design.md` OPTION B exactly:

- add only `long_term_energy_capacity_trend` and `explicit_equipment_repair` normalized ownership-intent guards;
- integrate them on the existing candidate lane after Contract evaluation and before selector/fallback;
- correct R05 fixture ownership;
- add focused positive, negative, precedence, and no-candidate tests.

The Apply does not address the seven frozen Guide selection/ranking debts and does not add a query profile, Contract rule, Wiki, Guide, or Guide-Wiki relationship.

## 2. Changed files

| File | Change |
|---|---|
| `api/retrieval_v2/ownership_intents.py` | New two-intent detector, candidate-lane partition, annotations, Contract precedence, and no-candidate result |
| `api/retrieval_v2/orchestrator.py` | Calls the ownership lane after Contract evaluation and exposes its trace in `retrieval_decision` |
| `scripts/test_resource_allocation_field.py` | Reads production ownership-intent trace and candidate ownership annotation instead of overriding it with static test ownership |
| `tests/test_resource_allocation_ownership_intents.py` | 18 positive, negative, lane, precedence, fallback, and production-path tests |
| `tests/fixtures/resource_allocation_field_cases.json` | Corrected R05 only |
| `docs/knowledge/batch17_l_resource_allocation_ownership_minimal_apply_report.md` | This report |

No unrelated dirty file was modified or cleaned.

## 3. OI-01 implementation

`long_term_energy_capacity_trend` requires all of:

- an explicit long horizon such as multiple months, half a year, long term, or cross-season;
- an energy generation/capacity object;
- a decline/change/trend state;
- a replan, reassess, long-term-capacity, or trend-judgement action.

Output:

- primary: `energy_deepening`;
- mandatory context: `records` secondary;
- `resource_allocation` secondary only when replanning/allocation-horizon intent is present.

Negative boundaries block same-day load scheduling, active hazard, Human Harm, explicit repair, and Records archive/organization requests. Detection uses semantic term families, not R14 text, fixture ID, or a Guide ID.

## 4. OI-02 implementation

`explicit_equipment_repair` requires all of:

- a physical equipment/tool object;
- an explicit failed/broken/non-working state;
- an explicit repair/fix/fault-check action.

Output:

- primary: `repair`;
- secondary: `tools`;
- `resource_allocation` secondary only when scarcity, sole-item, substitute, degraded-mode, or delay language is also present.

Negative boundaries block active smoke/fire/spark/overheat/leak/electrical exposure, Human Harm, manufacturing from scratch, repair-log archive, and long-term storage. Detection does not depend on R17 text or `DG-0383`.

## 5. Contract precedence

Runtime order is now:

```text
Query normalization
  -> candidate fetch
  -> Cross-Domain Contract candidate lane
  -> if CANARY_APPLIED or Safety boundary: Contract precedence, no ownership guard
  -> if COMPARE_ONLY: normalized ownership-intent guard
  -> existing selector / fallback
```

The guard is deliberately disabled for Contract `NOT_APPLICABLE`; off and shadow behavior therefore remains unchanged. It runs only on the Limited Production `COMPARE_ONLY` path.

R19 verification:

- Contract: `energy_device_abnormal`, `CANARY_APPLIED`;
- ownership guard: `CONTRACT_PRECEDENCE`;
- primary: Energy Safety;
- Guide: `DG-0842`;
- Safety Gate: passed;
- dangerous suggestion / scarcity override: 0 / 0.

Contract + Retrieval regression: **91 passed**.

## 6. Candidate-lane behavior

The ownership guard:

- considers only existing Guide candidates;
- partitions candidates into a primary ownership lane and unchanged remainder;
- preserves the relative order inside each lane;
- preserves the complete candidate ID set;
- adds `ownership_intent_lane`, `ownership_intent`, `ownership_intent_owner`, and secondary-domain annotations to primary-lane candidates;
- records original and primary-lane candidate keys plus `candidate_set_preserved` in `retrieval_decision.ownership_intent`.

It does not alter Guide/Wiki scores, score formulas, `top_k`, selector limits, fallback counts, or Wiki selection. Existing deterministic related-Wiki expansion remains downstream of Guide selection.

## 7. No-candidate fallback

If a normalized intent matches but no existing Guide candidate belongs to the target primary domain:

- candidate order remains unchanged;
- no Guide is fetched, injected, or fabricated;
- no `top_k` expansion occurs;
- context status becomes `CANDIDATE_MISSING`;
- `ownership_lane_candidate_missing=true` is recorded.

The focused no-candidate test confirms unchanged candidate IDs and order.

## 8. R05 fixture correction

Only R05 changed:

| Field | Before | After |
|---|---|---|
| Primary | Resource Allocation | Energy Management |
| Secondary | Energy Management | Resource Allocation |
| Expected Guides | `DG-0928` | `DG-0845`, `DG-0844` |
| Accepted Guides | `DG-0928`, `DG-0845`, `DG-0844` | `DG-0845`, `DG-0844`, `DG-0928` |
| Expected Wiki | RA safety minimum + energy reserve | Energy critical-device minimum + essential-load reserve |

The query text is unchanged. The correction follows the same-day load boundary and does not loosen Safety expectations.

Targeted result: **PASS**, Energy Management, `DG-0845`.

## 9. R14 result

Query: `过去三个月发电越来越低，长期供能能力是不是需要重新规划？`

- normalized intent: `long_term_energy_capacity_trend`;
- ownership status: `APPLIED`;
- primary: Energy Deepening;
- Resource Allocation: secondary only;
- selected Guide: `DG-0920`;
- Medical primary: eliminated;
- candidate set preserved: true;
- ownership-lane candidate missing: false.

The frozen runner verdict is **PARTIAL** because R14's old expected-Guide precision lists `DG-0919`/`DG-0922`, while the selected Guide is the legitimate Energy Deepening `DG-0920`. Batch17-L does not modify that expected list or tune Guide selection. The ownership conflict itself is resolved.

## 10. R17 result

Query: `唯一的水泵坏了，怎么修？`

- normalized intent: `explicit_equipment_repair`;
- ownership status: `APPLIED`;
- primary: Repair;
- secondary: Tools and Resource Allocation;
- selected existing candidate: `DG-0923`;
- Resource Allocation primary: eliminated;
- candidate set preserved: true;
- ownership-lane candidate missing: false;
- targeted runner verdict: **PASS**.

The lane uses the candidate's existing Repair domain metadata and query-level ownership annotation. No Guide ID is hardcoded by the guard.

## 11. Positive tests

| Case | Query family | Expected | Result |
|---|---|---|---|
| A1 | half-year generation decline + long-term reassessment | Energy Deepening | PASS |
| A2 | multi-month usable-energy decline + capacity replanning | Energy Deepening | PASS |
| A3 | broken pump + inspect/repair | Repair | PASS |
| A4 | only pump + broken + repair/substitute choice | Repair primary, RA secondary | PASS |

R14 and R17 also pass production fallback ownership-lane integration tests without exact Guide assertions.

## 12. Negative tests

| Case | Boundary | Guard result |
|---|---|---|
| N1 | today's low solar output and load shutdown | no ownership-intent match; Energy Management preserved |
| N2 | controller smoking | no ownership-intent match; Safety path not overridden |
| N3 | three-month generation records organized into a trend | no ownership-intent match; Records preserved |
| N4 | smoking pump plus repair wording | no ownership-intent match; Safety precedence preserved |
| N5 | repair-log archive | no ownership-intent match; Records preserved |
| N6 | manufacture a new pump component | no ownership-intent match; Manufacturing preserved |
| N7 | long-term storage of a broken spare pump | no ownership-intent match; Storage preserved |

Additional tests verify Contract `CANARY_APPLIED` precedence, non-compare mode no-op behavior, candidate-set preservation, lane-relative ordering, and no-candidate fallback.

Focused ownership test result: **18 passed**.

## 13. R03/R06/R12 frozen regression

| Case | Guide | Rank/ownership | Result |
|---|---|---|---|
| R03 | `DG-0927` | rank 1, Resource Allocation | PASS |
| R06 | `DG-0928` | rank 1, Resource Allocation | PASS |
| R12 | `DG-0929` | rank 1, Resource Allocation | PASS |

None matched an ownership intent. The three Batch17-H profiles remain unchanged. Resource Allocation profile contract: **15 passed**.

## 14. R19/O08 Safety regression

| Case | Primary | Guide | Contract/guard | Safety result |
|---|---|---|---|---|
| R19 | Energy Safety | `DG-0842` | Contract `CANARY_APPLIED`; guard `CONTRACT_PRECEDENCE` | PASS |
| O08 | Energy Safety | `DG-0841` | guard not matched | PASS |

- dangerous suggestion: 0;
- scarcity override: 0;
- focused R19 Safety suite: **2 passed**;
- Safety regression: 0.

## 15. Seven selection debts unchanged

The ownership detector returns no match for every frozen selection debt:

- R01: NON-BLOCKING SELECTION DEBT;
- R02: NON-BLOCKING SELECTION DEBT;
- R08: NON-BLOCKING SELECTION DEBT;
- R09: NON-BLOCKING SELECTION DEBT;
- R11: NON-BLOCKING SELECTION DEBT;
- R13: NON-BLOCKING SELECTION DEBT;
- R20: NON-BLOCKING SELECTION DEBT.

No score, ranking, profile, Guide, Wiki, related-Wiki, selector, or expected value was changed for these cases. The full 28-case Field Test is reserved for Batch17-M.

## 16. Stable-domain result

The same eight frozen stable-domain modules were rerun:

| Domain | Result |
|---|---:|
| Energy Management | 9 / 9 passed |
| Energy Safety | 6 / 8 passed |
| Energy Deepening | 20 / 20 passed |
| Medical | 8 / 8 passed |
| Long-Term Records | 22 / 22 passed |
| Long-Term Storage | 12 / 12 passed |
| Manufacturing | 7 / 8 passed |
| Agriculture Second Stage | 7 / 9 passed |
| Absolute total | **91 / 96 passed; 5 failed** |

The five failures are the exact frozen baseline signatures: Energy Safety x2, Manufacturing x1, Agriculture Second Stage x2.

## 17. NEW regression

- Contract + Retrieval: 91/91 passed.
- Ownership intent tests: 18/18 passed.
- Resource Allocation profile contract: 15/15 passed.
- R19 Safety path: 2/2 passed.
- Stable-domain absolute: 91/96, identical five historical failures.
- **NEW regression: 0**.
- **Safety regression: 0**.

Pytest emitted only the existing cache write-permission warning. Python compilation passed with `PYTHONPYCACHEPREFIX` directed to `/private/tmp` because the system Python cache location is not writable.

## 18. Protected hashes

| Asset | Before SHA-256 | After SHA-256 | Result |
|---|---|---|---|
| Wiki tree | `7c0e433f814480c1d59c8d698a224d513f48f9e772608b2eef200fa10e062e69` | same | UNCHANGED |
| Guide tree | `647a774776e57b413dd88d2093f8f37607718ab2ad613c4d3ce9d95645941f8c` | same | UNCHANGED |
| `data/emergency_guides.json` | `a69b6a63857deb938a603ba488e3de684d1cd760c5a26b63ffffb17567d7a97f` | same | UNCHANGED |
| `data/guide_index.json` | `fc34106859f25d9c603bf28544dc52ca9771f1c4979bbb68423d5370bf253cee` | same | UNCHANGED |
| Profiles | `e0948bfc22f713f9e81991b0d6021c8e5f0ea43a41c3b1907a691436620c4041` | same | UNCHANGED |
| Contract source | `18426c7f145031b6f14357cf59ccbb4e9e3ee6197d87cbcbccf0c40af36b1ab2` | same | UNCHANGED |
| Contract canary runtime | `7be88c3867301d2e494a61b2f7e4b724f193b5e5ead40b5355c58d0d059140fc` | same | UNCHANGED |
| Contract runtime | `6ddcc27377d86bd2585f6471a0c891b8fa69caff3eaad43b62aa7cf6d99cb90e` | same | UNCHANGED |
| Contract comparison | `4a216582bd5a659f00020dc58ad14656661f2abf63ac20caa57ba59638f910e2` | same | UNCHANGED |
| `data/lanternbox.db` | `aeca9af6579dbe97656b0968c6b6a9b20152bee5b90e2b9cb2c860bff3202e49` | same | UNCHANGED |
| R05 fixture | `521a5bfef6fae3aa71efe9501ef7e7b1eb6a2b9f317c0e00b38c9a38135468b0` | `0d93d5a3f0c0fe3238dc90608b6c7b1984193b41a3aaba3af39a3c2ca588dbc9` | EXPECTED CHANGE |
| Orchestrator | `5fdc17ba6700cd9a441a4bd157f57a10690730802cdddb7fceafebb2d7d1a8ee` | `a252cc3c9dbff7e887dfc16a0c711947b1e9508d6c2e4d65c316372f427da771` | EXPECTED CHANGE |
| Resource Allocation runner | `d905e1fc0c38d87c429dbb26a110ede1c66326a4c8c93d55384725e0e7c0447c` | `033e75bcfbc4e246c98c3a55a6cb1021e7cfbebedf50e61523c8404b731d0043` | EXPECTED CHANGE |

New-file hashes:

- `api/retrieval_v2/ownership_intents.py`: `6692228bbc504b673eb654d7a00e61ff2d8e337fe70e2f23108972f459409144`.
- `tests/test_resource_allocation_ownership_intents.py`: `8c69154a948949c29e6423b33718d6d127efaddce75951e9363618c91c42c1a6`.

## 19. Final readiness

Batch17-L satisfies the authorized ownership Apply:

- two and only two normalized ownership intents implemented;
- Contract/Safety precedence preserved;
- no-candidate behavior fail-safe and traceable;
- R05 fixture ownership corrected;
- R14 ownership resolved to Energy Deepening;
- R17 ownership resolved to Repair;
- frozen RA profile and Safety cases preserved;
- seven selection debts untouched;
- protected Knowledge, Guide, profile, Contract, and database hashes unchanged;
- NEW regression = 0.

No further tuning is authorized in this Batch. The complete 28-case Field Test remains for Batch17-M Final Verification.

**READY FOR FINAL VERIFICATION**
