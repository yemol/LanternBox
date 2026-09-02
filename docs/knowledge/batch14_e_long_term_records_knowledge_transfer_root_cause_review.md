# LanternBox Batch14-E: Long-Term Records / Knowledge Transfer Retrieval Root Cause Review

生成日期：2026-08-24

依据：

- `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`
- `docs/knowledge/batch14_b_long_term_records_knowledge_transfer_plan.md`
- `docs/knowledge/batch14_c_long_term_records_knowledge_transfer_apply_report.md`
- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_report.md`
- `docs/knowledge/batch14_d_long_term_records_knowledge_transfer_field_test_results.json`

本阶段只做只读根因分析，没有 Apply。

## 0. 结论摘要

Batch14-D 的主要问题是 Records 系统入口缺少窄而可区分的检索 anchor，不是 Knowledge 内容缺失，也不是危险语义或生命周期规则写错。

关键结论：

1. 32 篇新 Wiki、13 篇复用 Wiki 和 8 个新 Guide 已覆盖本阶段主题，没有 P 类 Knowledge / Guide 真缺口。
2. `lifecycle correctness=0%` 实际表示 4 个专项 case 都没有检索到足以证明边界的 evidence，不表示系统主动给出了错误生命周期建议；所有自动晋升违规计数仍为 0。
3. Batch14-D 自动分类中的 3 个 G 类 related_wiki 顺序问题均不成立。建议 Batch14-F 调整 `related_wiki` 顺序的数量为 0。
4. S06、S11、S19 存在合理的旧 Guide 或 Records 内部共主，fixture 有条件地过窄；S10 应允许 Energy top1 + Records trend secondary。
5. Observation 为 `3 pass / 3 partial / 0 fail`，当前 Medical、Manufacturing、Repair、Storage、Production、Energy 均未被 Records 吞掉。O03、O04、O05 不修。
6. 建议 Batch14-F 只新增 5 个复合 Records profiles、少量 fixture / test ownership 修正和对应 contract tests；不新增 Wiki / Guide，不改旧 stable domains，不改 Pipeline 或参数。

## 1. Batch14-D 基线总结

|指标|结果|
|---|---:|
|total|26|
|strict / observation|20 / 6|
|全量 pass / partial / fail|7 / 5 / 14|
|strict pass / partial / fail|4 / 2 / 14|
|observation pass / partial / fail|3 / 3 / 0|
|Records primary|35.0%|
|expected Guide hit / top1|50.0% / 20.0%|
|Wiki any / full|50.0% / 25.0%|
|Guide-Wiki precise|35.0%|
|version correctness|100.0%|
|knowledge promotion gate|100.0%|
|skill authorization boundary|100.0%|
|dangerous suggestion / Kiwix override|0 / 0|
|safety / fallback / record-check|100% / 100% / 100%|

RAW 自动晋升、unverified -> CURRENT、single log -> VERIFIED、single experience -> CURRENT、SUPERSEDED / ARCHIVED 默认执行、无重新验证回滚、自动技能授权的违规计数全部为 0。

## 2. Lifecycle 0% 指标解释

Batch14-D 的 `lifecycle_correctness` 不是对 26 条 case 的内容正确率，也不是自动晋升违规率。脚本只把 fixture 中带 `lifecycle_rule` 的 4 条纳入分母：

|case|专项规则|结果|实际含义|
|---|---|---|---|
|S03|device_not_verified|false|DG-0912 与目标 Wiki 均未进入，selected evidence 无法证明设备记录不得自动 VERIFIED。|
|S04|conflict_preserved|false|DG-0912 与冲突记录 Wiki 均未进入，selected evidence 无法证明两份冲突记录应并列保留。|
|S09|correlation_not_cause|false|DG-0914 与跨季趋势 Wiki 均未进入，selected evidence 无法证明相关性不得写成确定原因。|
|S12|single_log_not_verified|false|DG-0912 / DG-0915 与 lifecycle Wiki 均未进入，selected evidence 无法证明单条日志不得直接 VERIFIED。|

因此分子为 0、分母为 4，得到 0%。其他 case 中大量 `lifecycle_boundary_correct=true` 是因为没有配置 lifecycle 专项规则，函数按 N/A 返回 true，不能用来扩大分母。

应把两个概念拆开：

- `lifecycle evidence retrieval correctness`：目标 lifecycle evidence 是否实际进入 selected evidence。本轮为 0/4。
- `lifecycle semantic violation count`：selected evidence 是否主动建议自动晋升。本轮为 0。

结论是 B：正确 lifecycle evidence 没有被检索出来，不是 A：Knowledge 给出错误建议。未来应重命名该 rate，或同时输出 applicable case count，避免把 evidence retrieval miss 写成 Knowledge semantic failure。

## 3. 16 个 Strict 非 Pass 逐条根因

说明：candidate rank 按 Batch14-D `guide_candidates` 前 8 排名；“饱和”只表示 top1 的 `related_wiki` 已达到或超过 selected Wiki 上限 8。“因果 F”还要求正确 Guide 已在后续 selected 位，且 top1 evidence 实际阻断它。

### S01 原始现场记录最小字段

- query：我刚记录了一件现场异常，最少应该写哪些信息？
- expected Guide：`DG-0911` / `DG-0912`
- actual top1 / top3：`DG-0640` / `DG-0640, DG-0860, DG-0829`
- correct Guide candidate rank：均 `>8`
- actual selected Wiki：`communication-contact-window-001`, `communication-offline-communication-001`, `communication-sms-001`, `communication-emergency-net-opening-check-001`, `communication-message-priority-levels-001`, `communication-check-in-missed-window-001`, `communication-radio-log-minimum-fields-001`, `communication-radio-message-format-short-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：3；不饱和。
- 合理旧 Records Guide：无。`DG-0860` 是通信日志，不是系统级 Record Capture。
- fixture 是否过窄：否。
- root cause：A + B + E；另有 K，Batch14-D 曾因 secondary `records` domain 容易把 `DG-0860` 看成 Records。
- 最小修复候选：窄 `records_capture_trust_summary` profile，使“现场记录 + 最小字段/来源/复核”锚定 `DG-0912`，`DG-0911` 可 secondary。

### S02 亲历、转述与设备来源

- query：这条记录是别人告诉我的，不是我亲眼看到的，应该怎么标？
- expected Guide：`DG-0912`
- actual top1 / top3：`DG-0829` / `DG-0829, DG-0912, DG-0894`
- correct Guide candidate rank：`DG-0912@2`
- actual selected Wiki：`water-clarity-judgement-013`, `water-odor-judgement-014`, `water-unknown-source-010`, `records-lifecycle-state-model-001`, `records-raw-record-minimum-fields-001`, `records-review-verification-checklist-001`, `records-source-direct-reported-boundary-001`, `records-applicable-conditions-context-001`
- correct Wiki 是否存在：是；`records-source-direct-reported-boundary-001` 已进入。
- profile triggered：无
- top1 related_wiki：3；不饱和。
- 合理旧 Records Guide：无。
- fixture 是否过窄：否。
- root cause：D。Knowledge 和 evidence 链有效，只是 source/trust 语义没有取得 top1。
- 最小修复候选：同一 capture/trust profile 提升 `DG-0912`；不新增知识。

### S03 设备记录不等于事实

- query：设备自动记录了一个异常值，可以直接当成已经确认的事实吗？
- expected Guide：`DG-0912`
- actual top1 / top3：`DG-0829` / `DG-0829, DG-0563, DG-0845`
- correct Guide candidate rank：`>8`
- actual selected Wiki：`water-clarity-judgement-013`, `water-odor-judgement-014`, `water-unknown-source-010`, `energy-bank-battery-priority-001`, `energy-lithium-battery-safety-index-001`, `energy-power-outage-002`, `energy-swollen-lithium-battery-stop-use-001`, `energy-critical-device-minimum-line-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：3；不饱和。
- 合理旧 Records Guide：无。
- fixture 是否过窄：否。
- root cause：A + B + E。设备、source、verification 没有形成 Records trust anchor。
- 最小修复候选：capture/trust profile；必须要求“设备记录/自动记录 + 确认事实/verification”，不能用泛化“异常”触发。

### S04 冲突记录

- query：两份记录对同一件事说法不一样，应该删掉哪一份？
- expected Guide：`DG-0912`
- actual top1 / top3：`DG-0157` / `DG-0157, DG-0585, DG-0894`
- correct Guide candidate rank：`>8`
- actual selected Wiki：`communication-separated-movement-002`, `general-recycling-material-pool-ledger-001`, `general-waste-reuse-failure-record-001`, `general-waste-disposal-handover-card-001`, `general-waste-source-hazard-log-001`, `general-waste-source-label-minimum-001`, `hygiene-waste-organic-pest-odor-daily-check-001`, `repair-recycling-fasteners-small-parts-sort-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：0；不饱和。
- 合理旧 Records Guide：无。
- fixture 是否过窄：否。
- root cause：A + B + E。
- 最小修复候选：capture/trust profile，使用“两份记录/冲突/说法不一致/覆盖或删除”组合 anchor；不要提高单词“记录”的全局权重。

### S05 每日跨域摘要

- query：今天发生了很多事，我想整理一份跨领域的日终摘要，应该保留什么？
- expected Guide：`DG-0911`
- actual top1 / top3：`DG-0605` / `DG-0605, DG-0886, DG-0911`
- correct Guide candidate rank：`DG-0911@3`
- actual selected Wiki：`water-flood-water-001`, `water-priority-outage-001`, `water-rainwater-001`, `agriculture-staple-crop-small-plot-priority-001`, `agriculture-seed-reserve-use-line-001`, `agriculture-quick-leaf-long-cycle-balance-001`, `agriculture-production-failure-backup-plan-001`, `agriculture-yield-record-minimum-fields-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：3；不饱和。top2 `DG-0886` 有 10 条 related_wiki，top1 + top2 累计先占满 8。
- 合理旧 Records Guide：无。
- fixture 是否过窄：否。
- root cause：D + E。不是 G；`DG-0911` 为 top3，调整它自己的 Wiki 顺序不会让 evidence 获得位置。严格定义下也不是 top1 单独饱和的 F。
- 最小修复候选：将“跨领域/日终摘要/未结项/明日第一步”作为 capture/trust 复合 profile 的 summary 分支，提升 `DG-0911`；不单独建第六个 profile。

### S06 失败复盘

- query：一个任务失败了，现场已经安全，之后应该怎么做失败复盘？
- expected Guide：`DG-0913`
- actual top1 / top3：`DG-0295` / `DG-0295, DG-0880, DG-0729`
- correct Guide candidate rank：`DG-0913@4`
- actual selected Wiki：`general-failure-lesson-record-001`, `agriculture-continuous-nursery-schedule-001`, `agriculture-seedling-damping-off-isolation-001`, `agriculture-transplant-hardening-off-001`, `agriculture-nursery-capacity-limit-001`, `agriculture-seedling-001`, `agriculture-seedling-transplant-001`, `agriculture-seedling-thinning-001`
- correct Wiki 是否存在：是；`general-failure-lesson-record-001` 已进入。
- profile triggered：无
- top1 related_wiki：1；不饱和。
- 合理旧 Records Guide：有，`DG-0295` 是真正的系统级“任务失败复盘”。它有事实记录、原因分类、具体改进、后续复查、fallback 和重大事件升级边界。
- fixture 是否过窄：是。一般任务失败场景应允许 `DG-0295`；`DG-0913` 在 incident closure、纠正措施验证和 lifecycle 上更完整，适合正式事故复盘。
- root cause：H + I + J；从新 Guide 排名看也有 C，但不应机械判 fail。
- 最小修复候选：fixture 接受 `DG-0295` 或 `DG-0913`。post-event profile 仍可服务 S07，但不必为了 S06 强制替换旧 Guide。

### S07 Near Miss 上层复盘

- query：刚才发生了一次险情，但已经处理完并恢复安全，怎么记录原因和整改？
- expected Guide：`DG-0913`
- actual top1 / top3：`DG-0909` / `DG-0909, DG-0881, DG-0913`
- correct Guide candidate rank：`DG-0913@3`
- actual selected Wiki：`production-workshop-risk-source-scan-001`, `production-workshop-ppe-check-before-task-001`, `production-workshop-dust-spark-near-miss-log-001`, `production-workshop-low-light-stop-line-001`, `production-workshop-fatigue-stop-line-001`, `production-workshop-bystander-exclusion-log-001`, `production-workshop-incident-review-card-001`, `agriculture-soil-poor-fertility-signs-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：7；未单独占满 8，top2 的 9 条 evidence 随后占去最后位置。
- 合理旧 Records Guide：无；top1 是 Production near miss Guide，在正在发生的工坊险情中合理，但 query 已明确恢复安全并询问事后复盘。
- fixture 是否过窄：否。
- root cause：D + E + M。不是 G；调整 top3 `DG-0913` 的 related_wiki 顺序无效。严格定义下 top1 只有 7 条，也不单独构成 F。
- 最小修复候选：窄 `records_post_event_review` profile，必须同时要求“已经安全/处理完/事后”与“复盘/原因/整改/关闭验证”；当前事故、仍未安全场景继续由 Production / Safety 主导。

### S08 重复设备故障趋势

- query：同一个设备三个月里坏了四次，我想从历史记录判断是不是重复故障，怎么整理？
- expected Guide：`DG-0914`
- actual top1 / top3：`DG-0846` / `DG-0846, DG-0845, DG-0641`
- correct Guide candidate rank：`DG-0914@8`
- actual selected Wiki：`energy-charging-queue-schedule-001`, `energy-charge-discharge-handover-log-001`, `energy-shared-power-bank-borrow-return-001`, `energy-charging-port-rotation-rule-001`, `energy-critical-device-minimum-line-001`, `energy-essential-load-reserve-001`, `energy-low-battery-power-window-001`, `energy-night-lighting-runtime-plan-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：4；不饱和。
- 合理旧 Records Guide：无；Energy top Guides 是误召回，不是历史故障分析。
- fixture 是否过窄：否。
- root cause：A + C + E。
- 最小修复候选：`records_long_term_trend` profile，要求时间跨度 + 重复次数/趋势；Repair 可 secondary。

### S09 农业跨季失败

- query：过去三季种同一种作物都失败了，我想比较天气、地点和批次记录找规律。
- expected Guide：`DG-0914`
- actual top1 / top3：`DG-0902` / `DG-0902, DG-0884`
- correct Guide candidate rank：`>8`
- actual selected Wiki：`general-storage-handover-card-001`, `general-storage-inventory-card-minimum-001`, `general-storage-check-calendar-001`, `general-storage-issue-log-001`, `general-storage-fifo-shelf-rule-001`, `general-storage-label-minimum-fields-001`, `agriculture-storage-seed-library-rotation-card-001`, `agriculture-storage-seed-bag-label-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：`storage_inventory_handover`
- top1 related_wiki：13；饱和，但不是正确 Guide 缺席候选的根因。
- 合理旧 Records Guide：无。`DG-0902` 是 stable Storage Guide，尽管 category 为“信息保存与长期重建”。
- fixture 是否过窄：否；fixture 已允许 Agriculture 共域。
- root cause：A + B + E + M。Storage profile 因“批次记录”单点触发造成 semantic drift；不是 G，也不应通过扩大 evidence cap 解决。
- 最小修复候选：窄 long-term trend profile 以“过去三季 + 多次失败 + 比较/规律”取得主位，Agriculture 保持 secondary；不修改 Storage stable profile。

### S10 能源容量趋势

- query：我想比较过去几个月电池可用容量和太阳能产出的变化趋势。
- expected Guide：`DG-0914`
- actual top1 / top3：`DG-0617` / `DG-0617, DG-0564, DG-0109`
- correct Guide candidate rank：`>8`
- actual selected Wiki：`energy-solar-daytime-charge-schedule-001`, `energy-cloudy-solar-downgrade-001`, `energy-solar-charge-target-selection-001`, `energy-solar-weather-pattern-log-001`, `energy-battery-001`, `energy-battery-002`, `energy-battery-003`, `energy-battery-capacity-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：24；饱和，但 `DG-0914` 不在候选，饱和不是入口缺失根因。
- 合理旧 Records Guide：无；Energy 主导电池/太阳能语义合理。
- fixture 是否过窄：有条件地是。Energy top1 可以接受，但 long-term trend profile 应让 `DG-0914` 进入 top3 并带入 Records 趋势 evidence。
- root cause：L + A + B + E，不是 Energy 错误抢位。
- 最小修复候选：long-term trend profile；future fixture 允许 Energy top1 + `DG-0914` secondary + expected Records Wiki hit。

### S11 Promotion 与 Version 内部竞争

- query：这个办法今天第一次成功了，可以直接把它写进正式 SOP 吗？
- expected Guide：`DG-0915`
- actual top1 / top3：`DG-0916` / `DG-0916, DG-0915, DG-0115`
- correct Guide candidate rank：`DG-0915@2`
- actual selected Wiki：`records-lifecycle-state-model-001`, `records-sop-version-minimum-fields-001`, `records-current-version-selection-001`, `records-superseded-version-isolation-001`, `records-rollback-revalidation-001`, `records-high-risk-knowledge-review-gate-001`, `records-version-change-log-001`, `general-knowledge-card-archive-001`
- correct Wiki 是否存在：是；`records-lifecycle-state-model-001` 与 `records-high-risk-knowledge-review-gate-001` 已进入。
- profile triggered：无
- top1 related_wiki：9；前 8 截断最后一条，但本 case 所需 evidence 完整，饱和不构成失败根因。
- 合理旧 Records Guide：不涉及；这是 `DG-0915` / `DG-0916` 新 Records 内部职责相邻。
- fixture 是否过窄：是。`DG-0916` 能明确只有受审查的新 CURRENT 可发布，且 `DG-0915` 已 top2、promotion gate 和 version semantics 均为 100%。
- root cause：I + J。
- 最小修复候选：fixture 允许 `DG-0916` top1，但要求 `DG-0915` 或 promotion Wiki 同时进入且 promotion boundary 正确；不新增 promotion/version profile。

### S12 Single Log -> VERIFIED

- query：我刚写完一条日志，能不能直接把状态设成 VERIFIED？
- expected Guide：`DG-0912` / `DG-0915`
- actual top1 / top3：`DG-0860` / `DG-0860, DG-0428, DG-0639`
- correct Guide candidate rank：均 `>8`
- actual selected Wiki：`communication-emergency-net-opening-check-001`, `communication-message-priority-levels-001`, `communication-check-in-missed-window-001`, `communication-radio-log-minimum-fields-001`, `communication-radio-message-format-short-001`, `communication-radio-call-sign-identity-check-001`, `communication-lora-message-size-priority-001`, `communication-shortwave-receive-log-verification-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：8；饱和，但正确 Guide 未进入 selected，仍不是主入口缺失的根因。
- 合理旧 Records Guide：无。`DG-0860` 是通信信息记录与交接；`DG-0428` 是外部观察日志。
- fixture 是否过窄：否。
- root cause：A + B + E + K。`records_primary=true` 在此为 domain metadata 误分类。
- 最小修复候选：capture/trust profile，以 `VERIFIED + 日志/状态 + 直接设置/自动晋升` 为强 anchor；不要让 generic “日志”提升所有 Records。

### S17 ARCHIVED 历史版本

- query：归档的 ARCHIVED 知识卡还能作为当前操作依据吗？
- expected Guide：`DG-0916` / `DG-0917`
- actual top1 / top3：`DG-0412` / `DG-0412, DG-0915, DG-0526`
- correct Guide candidate rank：`DG-0917@7`, `DG-0916@8`
- actual selected Wiki：`general-knowledge-card-archive-001`, `records-lifecycle-state-model-001`, `records-applicable-conditions-context-001`, `records-knowledge-promotion-gate-001`, `records-high-risk-knowledge-review-gate-001`
- correct Wiki 是否存在：是；`general-knowledge-card-archive-001` 已进入。
- profile triggered：无
- top1 related_wiki：1；不饱和。
- 合理旧 Records Guide：`DG-0412` 与 `DG-0526` 是 knowledge/task 和分类管理的相邻入口，但都不定义 ARCHIVED / CURRENT 默认行动边界，不能单独替代 `DG-0916` / `DG-0917`。
- fixture 是否过窄：否。
- root cause：C + I。Version semantic 本轮正确来自 `DG-0915` 和 Wiki，但严格 version/archive Guide 没进入 top3。
- 最小修复候选：窄 archive/index profile；不扩大 version profile，不接受 `DG-0526` 单独作为 strict pass。

### S18 长期记录归档

- query：SD、纸本和 Core 上都有记录，我怎么做长期索引和备份？
- expected Guide：`DG-0917`
- actual top1 / top3：`DG-0688` / `DG-0688, DG-0917, DG-0416`
- correct Guide candidate rank：`DG-0917@2`
- actual selected Wiki：`organization-children-001`, `organization-conflict-001`, `organization-fatigue-001`, `organization-handover-001`, `organization-incident-report-001`, `organization-inventory-001`, `organization-morale-001`, `organization-night-safety-001`
- correct Wiki 是否存在：是；本轮均未进入。
- profile triggered：无
- top1 related_wiki：20；饱和并直接占满前 8。
- 合理旧 Records Guide：无；`DG-0688` 是 Team 双人备份，不是数据归档。
- fixture 是否过窄：否。
- root cause：D + E + F。这是 S05/S07/S18 中唯一符合严格 F 定义的 case；不是 G，因为正确 Guide 不是 top1。
- 最小修复候选：archive/index profile 把 `DG-0917` 提到 top1；不调整 `DG-0917` 自身 related_wiki 顺序。

### S19 技能授权等级

- query：一个成员只跟着做过几次，还没有独立完成过，现在应该算什么技能等级？
- expected Guide：`DG-0918`
- actual top1 / top3：`DG-0400` / `DG-0400, DG-0918, DG-0401`
- correct Guide candidate rank：`DG-0918@2`
- actual selected Wiki：`records-skill-authorization-minimum-001`, `records-skill-level-boundaries-001`, `records-supervised-practice-evidence-001`, `records-skill-expiry-refresher-001`, `records-new-member-learning-path-001`, `records-knowledge-transfer-handover-summary-001`, `general-small-team-handoff-001`, `organization-onboarding-001`
- correct Wiki 是否存在：是；三篇 expected Wiki 全部进入。
- profile triggered：无
- top1 related_wiki：0；不饱和，evidence 来自 top2 `DG-0918`。
- 合理旧 Records Guide：有条件地有。`DG-0400` 是 Organization / skill map 合理主入口，能区分“独立/需要协助”，但没有完整的 evidence、到期和人工授权模型。
- fixture 是否过窄：是。当前 `DG-0918` top2、Wiki full、skill boundary=100%，不应只因新 Guide 不是 top1 判 partial。
- root cause：H + I + J。
- 最小修复候选：fixture 允许 `DG-0400` top1，但必须同时满足 `DG-0918` top3、skill evidence Wiki 命中和 authorization boundary 正确；不为 S19 单独加 profile。

### S20 技能过期与复训

- query：一个人很久没做这个高风险任务了，还能继续按 INDEPENDENT 独立操作吗？
- expected Guide：`DG-0918`
- actual top1 / top3：`DG-0689` / `DG-0689, DG-0394, DG-0136`
- correct Guide candidate rank：`>8`
- actual selected Wiki：`organization-night-safety-001`, `organization-outside-movement-001`, `organization-return-check-001`, `records-skill-level-boundaries-001`
- correct Wiki 是否存在：是；`records-skill-level-boundaries-001` 进入，但 expiry/refresher evidence 未进入。
- profile triggered：无
- top1 related_wiki：3；不饱和。
- 合理旧 Records Guide：无。Team fatigue / high-risk task Guide 不能替代技能到期与复训。
- fixture 是否过窄：否。
- root cause：A + B。
- 最小修复候选：`records_skill_evidence` profile，强 anchor 为“很久没做/技能过期/复训/SUPERVISED/INDEPENDENT/授权 evidence”；不能仅凭“高风险任务”触发。

## 4. Observation 边界审查

|case|当前结果|主域判断|Root cause|Batch14-F 处理|
|---|---|---|---|---|
|O01 当前医疗急救|pass|Medical top1 正确，Records 不应进入主位。|R|不修，作为 capture/trust profile 的 Medical negative boundary。|
|O02 当前制造操作|pass|Manufacturing top1 正确。|R|不修，作为 trend profile 的 current-action negative boundary。|
|O03 当前维修|partial|Repair / Tools top1 正确；Records 历史补充未进入不是当前动作缺陷。|R|不修，不为当前坏锯触发 Records。|
|O04 当前 Storage 交接|partial|Storage top1 `DG-0902` 正确；Records 长期归档未进入可接受。|R|不修，不修改 Storage profiles。|
|O05 当前 Production WIP 交接|partial|Production top1 `DG-0910` 正确；Records 历史补充不应抢班次入口。|R|不修，不修改 Production profiles。|
|O06 当前 Energy 调度|pass|Energy top1 正确。|R|不修，作为 trend profile 的 current-budget negative boundary。|

Observation `0 fail` 是重要保护结果。O03、O04、O05 必须进入未来 5 个 profile 的 boundary regression tests，不应为了补充 evidence 从 partial 调成 pass。

## 5. 额外边界 X-A / X-B / X-C

### X-A Medical Observation Handover

- 当前 top3：`DG-0695, DG-0615, DG-0478`；Records Guide / Wiki 均未进入。
- Query 是已存在观察记录的照护交接，不是当前急救，理论上可由 Medical + Records 共域。
- 但“病人 + 交接”过于接近当前医疗行动。若在 Batch14-F 加入通用 capture 或 handover trigger，可能吞掉 O01 与其他当前医疗 case。
- 结论：不纳入 v0.1 Minimal Apply。记录为后续 Medical / Records cross-domain review；未来只有“观察记录 + 下一照护者 + 时间趋势/访问边界”多重 anchor 成熟后再考虑独立 profile。

### X-B Repair History

- 当前 top1 `DG-0667` Repair 合理，但 `DG-0914` 完全未进候选，Records 历史趋势缺失。
- 结论：由 `records_long_term_trend` 覆盖“坏三次 + 历史记录”；Repair 仍可 top1，Records 只需进入 top3 和 evidence，不要求抢主位。

### X-C Agriculture Multi-Season Failure

- 当前 top1 `DG-0667` Repair，`DG-0880` Agriculture 仅 top3，`DG-0914` 不在 top8，是明显 semantic drift。
- 结论：由同一个 long-term trend profile 修复，Agriculture 保持合理共域；不修改 Agriculture stable profile，不直接输出确定种植原因。

## 6. DG-0911 至 DG-0918 稳定性审查

“candidate / top3 / top1”按 20 个 strict case 全量计数；target cases 是 fixture 明确期待该 Guide 的 case。

|Guide|Intended scope|Target cases|candidate / top3 / top1|related_wiki|top1 evidence|Profile|重排|旧 Guide 重叠 / adjacent risk|
|---|---|---|---:|---:|---|---|---|---|
|`DG-0911`|每日跨域摘要、未结项和次日第一步|S01, S05|1 / 1 / 0|4|未观察到 top1|需要，合并入 capture/trust summary|否|与通用事件日志相邻；避免吞当前班次/能源/医疗摘要。|
|`DG-0912`|原始记录来源、可信度、冲突和 verification|S01-S04, S12|2 / 1 / 0|8|未观察到 top1|需要|否|与通信日志、外部观察日志争词；必须用 trust/source 强 anchor。|
|`DG-0913`|现场安全后的失败/事故复盘和整改验证|S06, S07|3 / 1 / 0|5|未观察到 top1|需要|否|与 `DG-0295` 重叠；风险是吞正在发生的 Production / Safety incident。|
|`DG-0914`|跨周期重复问题和长期趋势|S08-S10|1 / 0 / 0|9|未观察到 top1|需要|否|跨 Repair/Agriculture/Energy；必须要求时间跨度或重复次数。|
|`DG-0915`|Lesson / candidate 的人工晋升门禁|S11-S13|7 / 6 / 1|5|S13 target evidence 完整|不需要|否|与 `DG-0916` 合理内部相邻；当前表现稳定，不调。|
|`DG-0916`|CURRENT、SUPERSEDED、回滚和版本发布|S14-S17|5 / 4 / 4|9|S11、S14-S16 所需 evidence 完整；第 9 条通用索引被 cap 截断但不影响目标|不需要新 version profile|否|与 `DG-0915` 相邻；避免为了 S11/S17 扩大后破坏 S14-S16。|
|`DG-0917`|长期档案、索引、访问与保留|S17, S18|3 / 1 / 0|8|未观察到 top1|需要|否|与旧 archive/index Guide 相邻；S18 是前序 top1 saturation，不是自身顺序问题。|
|`DG-0918`|技能 evidence、授权边界、到期与交接|S19, S20|2 / 1 / 0|9|未观察到 top1；S19 前 8 已覆盖全部 target skill evidence|需要，主要解决 S20|否|与 `DG-0400/0401` 和 Organization 相邻；避免用“高风险任务”泛触发。|

## 7. 旧 Records Guide 重叠审查

|Guide|真实职责|是否系统级 Records|合理 top1|与新 Guide 关系|建议|
|---|---|---|---|---|---|
|`DG-0295` 任务失败复盘|跨域一般任务失败、原因和改进复查|是|S06 合理|与 `DG-0913` 部分重叠；新 Guide 增加 incident closure、纠正验证和 lifecycle|保留；fixture 允许一般任务失败时作为 top1。|
|`DG-0400` 基础技能清单|Organization 技能地图与备份学习|不是纯 Records，属于 Organization / skill 共域|S19 可条件接受|与 `DG-0918` 的技能等级相邻，但缺 evidence、到期和正式授权|保留 top1 可能性，必须由 `DG-0918` evidence 补全。|
|`DG-0401` 五分钟技能教学|短时教学、示范和一次练习|Organization / Training，Records secondary|不应单独回答长期授权/到期|与 `DG-0918` 的 supervised practice 相邻|保持 secondary，不修改。|
|`DG-0412` 知识卡转任务卡|把知识拆成当天任务|Organization / knowledge workflow|不适合 S17|不等同版本或归档状态|保持 secondary；不加入 ARCHIVED strict accepted Guide。|
|`DG-0526` 分类归档规则|分类稳定、避免重复、分类变更|Comms / Security / information taxonomy|不适合单独回答 CURRENT/ARCHIVED|与 `DG-0917` 仅在索引治理上相邻|保持 secondary；不作为 S17 strict 等价入口。|
|`DG-0860` 通信信息记录与交接|无线电、LoRa、短波消息日志与交接|否，主域是 Communication|通信日志 query 可 top1；S01/S12 不合理|与 `DG-0912` 只共享“来源/待核”字段|保持通信主域；Field Test 不得因 secondary records label 把它算 Records v0.1 主位。|

## 8. Field Test Domain Classification 审查

Batch14-D 最终脚本已优先使用部分稳定 Guide ID 范围，但对不在这些范围内的旧 Guide 仍会按 category / domains 逐项推断。该方法仍有偏差：

|Guide|Metadata 中的 records|实际 primary|Batch14-D 风险|
|---|---|---|---|
|`DG-0860`|domains 含 records|Communication|S12 被计入 `records_primary=true`，高估 Records 系统主位。|
|`DG-0428`|domains 含 records|External Observation / Safety context|不应算系统 lifecycle Records。|
|`DG-0845`|domains 含 records|Energy|应以 category/业务动作判 Energy。|
|`DG-0859`|domains 含 records|Communication + Energy|应以低功耗通信动作判主域。|
|`DG-0401`|domains 含 records|Organization / Training|不能因教学记录当成 Records authorization 主位。|
|`DG-0902`|category 为信息保存与长期重建|Long-Term Storage|必须以 stable Guide ownership 判 Storage；O04 当前 top1 正确。|
|`DG-0295`|category 含“库存资料”|System-level Records Failure Review|若先按“库存”词分类会低估 Records。|

因此 `records_primary=35%` 只能作方向性参考：`DG-0860` 造成 false positive，`DG-0295` 可能造成 false negative，两者甚至可能在总数上互相抵消。Batch14-F 应先在 Field Test 侧建立显式 Guide ownership / accepted co-primary 规则，再比较修复前后 primary rate；不得用目录名或是否含 `records` domain 直接判主位。

这属于 K 类测试指标分类偏差，不是 Retrieval Pipeline 问题。修正测试 ownership 不改变候选或 selected evidence。

## 9. Profile 候选比较

|候选|覆盖|收益|主要误伤风险|结论|
|---|---|---|---|---|
|`records_capture_trust`|S01-S04, S12|解决最大的一组无候选 / 非 top1；恢复 lifecycle evidence|泛化“记录/日志”会吞 Communication、Medical、业务日志|采用，但与 daily summary 合并为一个复合 profile，使用强 trust/capture anchors。|
|`records_daily_summary`|S05|DG-0911 已 rank3，只需窄提升|“今天/摘要”可能吞 Energy、Production、Storage、Medical 当前交接|不单独建立；作为复合 profile 的严格 summary 分支。|
|`records_post_event_review`|S06, S07|区分事后复盘与当前 incident|若没有“已经安全/事后”状态锚点会吞 Production / Safety|采用；S06 同时允许旧 `DG-0295`。|
|`records_long_term_trend`|S08-S10, X-B, X-C|修复最重要的跨域长期比较入口|可能抢当前 Repair、Agriculture、Energy 行动|采用；必须要求时间跨度/重复次数 + 比较/趋势，允许业务域 top1。|
|`records_archive_index`|S17, S18|提升 DG-0917，补长期索引和 ARCHIVED 查询|过宽会扰动已全绿的 S14-S16 version retrieval|采用；只锚定长期索引、多介质备份、ARCHIVED 历史查询，不覆盖 CURRENT/rollback 一般 query。|
|`records_skill_evidence`|S19, S20|主要修复 S20 的 expiry/refresher 缺口|“高风险任务”会吞 Team Safety|采用；要求技能到期/很久未练/复训/INDEPENDENT 等组合 anchor。|
|Medical observation handover|X-A|可补 Medical + Records 交接|吞当前急救的安全风险最高|本批不采用，后续 Medical cross-domain review。|

最少数量为 5，不是 6：

1. `records_capture_trust_summary`，合并 capture/trust 与严格 daily-summary 分支。
2. `records_post_event_review`。
3. `records_long_term_trend`。
4. `records_archive_index`。
5. `records_skill_evidence`。

不建议压缩为 4。post-event 的“现场已安全”门禁、archive 对 S14-S16 的保护、skill 对 Team Safety 的保护都需要独立 negative boundaries；强行合并会放大跨域风险。

不为 `DG-0915` / `DG-0916` 新增 promotion/version profile。它们当前是 8 个新 Guide 中表现最稳定的一组。

## 10. related_wiki 顺序结论

Batch14-D 自动 G=3 的复核结果：

|case|正确 Guide 排名|前序 evidence|是否符合 G|结论|
|---|---:|---|---|---|
|S05|`DG-0911@3`|top1 3 条 + top2 10 条，累计先占满 8|否|正确 Guide 不是 top1；调 DG-0911 顺序无效。|
|S07|`DG-0913@3`|top1 7 条 + top2 9 条，累计先占满 8|否|正确 Guide 不是 top1；调 DG-0913 顺序无效。|
|S18|`DG-0917@2`|top1 `DG-0688` 有 20 条，单独占满 8|否，属于 F|先让 DG-0917 获得正确主位；调它自身顺序无效。|

其他检查：

- `DG-0916` 有 9 条 related_wiki，但它 top1 的 S11、S14-S16 所需 evidence 均进入，未观察到目标 Wiki 因第 9 位失败。
- `DG-0914` / `DG-0918` 各有 9 条，但没有“正确 Guide 已 top1 且自身目标 Wiki 被第 9 位截断”的 case。
- S09/S10/S12 的 top1 确实各有 13/24/8 条 evidence，但正确 Records Guide 根本未进入 selected；这是入口缺失或稳定域共主问题，不是新 Guide 顺序问题。

Batch14-F related_wiki 调整建议：**0 个**。

## 11. Fixture 修正建议

|case|当前 expected|实际|是否过窄|建议|理由|
|---|---|---|---|---|---|
|S06|`DG-0913`|`DG-0295` top1，`DG-0913@4`|是|允许 `DG-0295` 或 `DG-0913`|旧 Guide 已能完成一般任务失败复盘；新 Guide 对正式 incident closure 更完整。|
|S10|`DG-0914` top1 / 共主|Energy top1，DG-0914 缺席|有条件地是|profile 修复后允许 Energy top1 + `DG-0914` top3 + Records trend Wiki hit|当前对象本来就是电池和太阳能，破坏 Energy stable 不合理。|
|S11|`DG-0915`|`DG-0916` top1，`DG-0915` top2，promotion/version 皆正确|是|允许 DG-0916 top1，但要求 DG-0915 或 promotion evidence 和 boundary 正确|属于 Records 内部 promotion/version 合理共主。|
|S17|`DG-0916` / `DG-0917`|`DG-0412`, `DG-0915`, `DG-0526`|否|保持 expected，不把 DG-0412/0526 作为等价 strict Guide|旧 Guide 不定义 ARCHIVED 的默认行动边界。|
|S19|`DG-0918`|`DG-0400` top1，`DG-0918` top2，Wiki full|是|允许 DG-0400 top1，但要求 DG-0918 top3 + skill evidence + boundary correct|技能地图可先回答当前级别，新 Guide evidence 保证不自动 INDEPENDENT。|

Fixture 调整必须表达“共主 + evidence 条件”，不能简单把所有旧 Guide 加入 accepted list。测试脚本同时需要显式 ownership map，修正 K 类 `records_primary` 指标偏差。

## 12. 是否新增 Wiki / Guide

不新增。

- 所有 16 个 strict 非 pass 都能映射到现有新 Guide、合理旧 Guide或相邻 stable domain。
- 所有 expected Wiki 均真实存在；miss 来自 Guide candidate / rank 或前序 evidence budget。
- Version、promotion、skill semantic 和所有违规计数证明内容边界已经存在。
- 未发现没有行动入口的 P 类主题。

Batch14-F 不新增 Wiki，不新增 Guide，不补旧 Guide evidence，不调整 Guide-Wiki 顺序。

## 13. Batch14-F Minimal Apply 唯一建议

### A. 新增 5 个复合 Records Profiles

1. `records_capture_trust_summary`
   - Guide：`DG-0912` primary；`DG-0911` 仅在明确日终/跨域摘要 branch 主导。
   - 覆盖：S01-S05、S12。
   - 禁止用单独“记录/日志/今天”触发；要求最小字段、亲历/转述、设备来源、冲突、VERIFIED，或跨领域日终摘要等强组合。
2. `records_post_event_review`
   - Guide：`DG-0913` primary，`DG-0295` accepted secondary/legacy。
   - 覆盖：S06、S07。
   - 必须要求现场已安全、处理完或明确事后复盘；正在发生的 incident 保持 Production / Safety / Medical 主位。
3. `records_long_term_trend`
   - Guide：`DG-0914` system-level primary/secondary，具体对象域可保持 top1。
   - 覆盖：S08-S10、X-B、X-C。
   - 要求跨月/跨季/多次/历史 + 比较/趋势/重复；禁止触发当前修法、当前种植处置、当前电量调度。
4. `records_archive_index`
   - Guide：`DG-0917` primary，`DG-0916` secondary。
   - 覆盖：S17、S18。
   - 只覆盖 ARCHIVED 历史依据、长期索引和纸本/SD/Core 多介质备份；保护 S14-S16 当前版本、旧版和回滚现有表现。
5. `records_skill_evidence`
   - Guide：`DG-0918` primary。
   - 主要覆盖：S20；S19 由 fixture 共主修正。
   - 要求技能过期、长期未练、复训、SUPERVISED、INDEPENDENT 或授权 evidence；“高风险任务”单独出现不得触发。

### B. related_wiki 调整

- 0 个。
- 不调整 `DG-0911`、`DG-0913`、`DG-0917` 或其他新 Guide 的 related_wiki 顺序。

### C. Fixture / Field Test 最小修正

- S06：接受 `DG-0295` 或 `DG-0913`。
- S10：允许 Energy top1，但要求 `DG-0914` top3 与 Records trend evidence。
- S11：允许 `DG-0916` top1，但要求 promotion evidence / boundary。
- S19：允许 `DG-0400` top1，但要求 `DG-0918` top3、skill evidence 和 authorization boundary。
- 修正 Field Test Guide ownership：不要把 secondary `records` domain 当作 Records v0.1 primary；显式区分 `DG-0860`, `DG-0428`, `DG-0845`, `DG-0859`, `DG-0401`, `DG-0902`, `DG-0295`。
- 将 lifecycle rate 命名为 evidence retrieval correctness，并单列 semantic violation count 与 applicable denominator。

### D. Profile Contract Tests

每个新 profile 增加正向 contract 与至少一个负向边界：

- capture/trust：S01-S05/S12 正向；通信日志、当前医疗观察负向。
- post-event：已恢复安全的复盘正向；正在流血、正在起火、工坊仍未安全负向。
- long-term trend：跨月/跨季/多次正向；O02、O03、O06 当前动作负向。
- archive/index：S17/S18 正向；S14-S16 回归不得下降。
- skill evidence：S20 正向；普通 Team fatigue/high-risk task 负向。
- O01-O06 全部作为 domain ownership regression；O03-O05 允许保持 partial。

### E. Knowledge / Guide

- 不新增 Wiki / Guide。
- 不修改旧 Guide。
- 不补 evidence，不改 Guide-Wiki 顺序。

### F. Stable Domain 保护

- 不修改 Production、Storage、Medical、Repair / Manufacturing、Agriculture、Energy 既有 profiles。
- S09 的 Storage semantic drift 由更窄的 Records long-term profile竞争解决，不直接修改 stable Storage profile。
- X-A 暂不修，留给 Medical / Records cross-domain review。

### G. Retrieval 架构冻结

- 不修改 Retrieval Pipeline、Prompt、`top_k`、selector limit、ranking、fallback、schema 或 PocketBase。
- 不用增大 evidence cap 掩盖 Guide 主位问题。
- 不为单个 case 建 profile。

## 14. 明确不建议修改项

- 不修改任何 Wiki、Guide 或 Guide-Wiki。
- 不调整 `DG-0911` 至 `DG-0918` 的 related_wiki 顺序。
- 不为 S11 增加 promotion/version profile。
- 不为了 O03、O04、O05 补 Records evidence 或让 Records 抢 top1。
- 不为 X-A 建宽泛 Medical handover profile。
- 不修改 Storage、Production、Agriculture、Energy 等 stable profile。
- 不增加 `top_k`、selector limit 或 selected Wiki 上限。
- 不修改 ranking、fallback、Prompt 或 Pipeline。
- 不把 domain metadata 中的 `records` 当成 system-level ownership 的充分条件。
- 不以 26 条全绿为目标；稳定目标仍是 strict 系统能力、生命周期/版本/晋升边界、跨域不吞域、安全完整性和零危险建议。

## 15. 阶段状态

建议进入 Batch14-F Minimal Apply，严格按第 13 节唯一方案执行。本报告没有实施任何修改。
