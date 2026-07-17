# LanternBox Batch9-C：Manufacturing & Production Foundation Apply Report

日期：2026-07-17

本阶段执行 Manufacturing & Production Foundation v0.1 第一阶段知识扩展。遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`。

未修改 Retrieval Pipeline、Prompt、query profile、top_k、selector limit、ranking、fallback 或 schema。

## 1. 新增 Wiki 清单

本批新增 38 篇 Wiki，落在 `wiki_import/manufacturing/`。由于当前 Wiki slug domain 白名单尚无 `manufacturing`，本批未修改 schema，采用合法的 `repair-manufacturing-*` slug，并使用正式分类 `基础制造与材料维修`。

|slug|title|priority|risk_level|Guide|
|---|---|---|---|---|
|`repair-manufacturing-workbench-start-check-001`|工坊工作台开工前安全检查|P0|high|DG-0871|
|`repair-manufacturing-zone-layout-001`|低资源工坊最小分区|P0|high|DG-0871, DG-0878|
|`repair-manufacturing-cut-drill-clear-zone-001`|切割钻孔打磨前清场|P0|high|DG-0871, DG-0873, DG-0876|
|`repair-manufacturing-clamp-before-processing-001`|切割和钻孔前固定夹持|P0|high|DG-0873, DG-0876|
|`repair-manufacturing-dust-spark-stop-line-001`|粉尘碎屑火花停止线|P0|high|DG-0871|
|`repair-manufacturing-bystander-exclusion-001`|儿童和旁人远离加工区|P0|high|DG-0871, DG-0878|
|`repair-manufacturing-ppe-minimum-001`|制造作业最低 PPE|P0|high|DG-0871|
|`repair-manufacturing-damaged-tool-stop-001`|工具损坏后的生产停用|P0|high|DG-0871, DG-0878|
|`repair-manufacturing-wood-selection-001`|木材制作前选择边界|P0|caution|DG-0872, DG-0873, DG-0874|
|`repair-manufacturing-scrap-material-check-001`|废旧材料制作前总检查|P0|high|DG-0872|
|`repair-manufacturing-metal-scrap-check-001`|金属废料再利用安全判断|P0|high|DG-0872, DG-0876|
|`repair-manufacturing-plastic-reuse-boundary-001`|塑料容器再利用边界|P0|high|DG-0872|
|`repair-manufacturing-fabric-rope-leather-check-001`|布料绳索皮革替代材料选择|P0|caution|DG-0872, DG-0875|
|`repair-manufacturing-connection-choice-001`|胶粘捆扎螺丝连接选择|P0|caution|DG-0875|
|`repair-manufacturing-fastener-reuse-boundary-001`|旧螺丝铆钉扎带再利用边界|P0|caution|DG-0872, DG-0875|
|`repair-manufacturing-salvaged-parts-sort-001`|拆解零件清洁分类标记|P0|caution|DG-0872, DG-0878|
|`repair-manufacturing-material-prep-001`|材料预处理：干燥清洁去毛刺|P0|high|DG-0872, DG-0876|
|`repair-manufacturing-downgrade-label-001`|材料降级使用标签|P0|caution|DG-0872|
|`repair-manufacturing-load-test-001`|成品承重前检查|P0|high|DG-0877|
|`repair-manufacturing-wood-measure-mark-001`|木材测量划线和切割线确认|P1|caution|DG-0873|
|`repair-manufacturing-wood-saw-support-001`|木材重复锯切支撑|P1|high|DG-0873|
|`repair-manufacturing-wood-drill-position-001`|木材打孔定位和边距检查|P1|caution|DG-0873, DG-0874|
|`repair-manufacturing-wood-screw-join-001`|木材螺丝连接基础|P1|caution|DG-0874, DG-0875|
|`repair-manufacturing-frame-square-check-001`|简易框架方正检查|P1|caution|DG-0874|
|`repair-manufacturing-shelf-support-check-001`|简易架子和支撑检查|P1|high|DG-0874, DG-0877|
|`repair-manufacturing-box-crate-check-001`|简易箱体和周转箱制作检查|P1|caution|DG-0874|
|`repair-manufacturing-thin-metal-drill-001`|薄金属片打孔支撑|P1|high|DG-0876|
|`repair-manufacturing-thin-metal-bend-001`|薄金属简易弯折边界|P1|high|DG-0876|
|`repair-manufacturing-metal-edge-safe-001`|金属锐边去毛刺和包边|P1|high|DG-0876|
|`repair-manufacturing-metal-bracket-check-001`|简易金属支架制作检查|P1|high|DG-0876, DG-0877|
|`repair-manufacturing-metal-join-choice-001`|金属螺丝铆接铁丝连接边界|P1|caution|DG-0875, DG-0876|
|`repair-manufacturing-corroded-metal-reuse-001`|腐蚀金属再利用前检查|P1|caution|DG-0872, DG-0876|
|`repair-manufacturing-raw-finished-zones-001`|原料半成品成品分区|P1|caution|DG-0871, DG-0878|
|`repair-manufacturing-start-record-001`|每日开工工具材料检查记录|P1|caution|DG-0871|
|`repair-manufacturing-end-clean-count-001`|收工清点与危险工具封存|P1|high|DG-0878|
|`repair-manufacturing-batch-record-001`|小批量制作记录|P1|normal|DG-0878|
|`repair-manufacturing-quality-recheck-001`|成品使用前质量复查|P1|high|DG-0877|
|`repair-manufacturing-defect-isolation-001`|缺陷品隔离和返工记录|P1|caution|DG-0877, DG-0878|

## 2. 新增 Guide 清单

新增 8 个 Guide，落在 `data/guides/manufacturing/`。

|Guide|title|priority|risk_level|related_wiki 数|
|---|---|---|---|---:|
|DG-0871|工坊开工前安全检查|P0|high|9|
|DG-0872|材料再利用前安全判断|P0|high|10|
|DG-0873|木材切割前固定与支撑|P0|high|6|
|DG-0874|简易结构件制作检查|P1|high|6|
|DG-0875|连接方式选择：胶粘/螺丝/捆扎|P0|caution|5|
|DG-0876|废旧金属材料安全处理|P0|high|10|
|DG-0877|制作完成后的承重检查|P0|high|5|
|DG-0878|工坊收工与工具封存|P0|high|8|

Guide 均为行动入口，包含 `scenario`、`steps`、`check`、`stop_or_escalate`、`fallback` 和 `related_wiki`。没有新增只做百科解释的 Guide。

## 3. Guide-Wiki 关系

本批新增 Guide-Wiki evidence chain：

- Guide -> Wiki：59 条 `related_wiki`
- Wiki -> Guide：38 篇 Wiki frontmatter 均写入 `guide_links`
- 双向关系：通过 `scripts/audit_guides.py`
- 无效 Guide ID：0
- 无效 Wiki slug：0
- 单边关系：0

关系设计重点：

- DG-0871 承接工坊开工、分区、PPE、旁人隔离、工具异常停用。
- DG-0872 承接材料来源、污染、塑料/木材/金属/软材料再利用边界。
- DG-0873 承接木材测量、划线、切割、打孔和固定。
- DG-0874 承接框架、箱体、架子等低负荷结构件。
- DG-0875 承接胶粘、螺丝、捆扎、铁丝、铆接等连接选择。
- DG-0876 承接废旧金属、锐边、打孔、弯折和腐蚀处理。
- DG-0877 承接成品承重、质量复查和缺陷隔离。
- DG-0878 承接收工清点、工具封存、成品/缺陷品分区和批次记录。

## 4. PocketBase 同步

同步方式：本地 SQLite upsert 到 `pocketbase/pb_data/data.db` 的 `wiki_articles` 表，仅写入本批新增 Wiki 的 metadata 与 content，未修改 PocketBase schema。

|项目|结果|
|---|---:|
|新增 PocketBase `wiki_articles`|38|
|更新 PocketBase `wiki_articles`|0|
|同步后 Markdown Wiki 总数|950|
|同步后 PocketBase `wiki_articles` 总数|950|
|同步后 PocketBase `repair-manufacturing-*` 数|38|
|PocketBase `wiki_categories` 总数|24|

说明：PocketBase 当前没有 Guide 关系表，Guide-Wiki 关系由 Guide JSON、Wiki frontmatter 和生成索引维护。

## 5. 验证结果

已运行：

```bash
python3 tools/audit_wiki.py
python3 tools/build_guides.py
python3 scripts/audit_guides.py
```

结果：

|检查|结果|
|---|---|
|Wiki audit|errors=0, warnings=0, advisories=0|
|Guide build|Generated 804 Guides / 804 Guide Index Items|
|Guide audit|errors=0, warnings=0, advisories=0|
|Markdown / PocketBase 数量|950 / 950|
|Guide-Wiki 单边关系|0|
|无效 Guide ID|0|
|无效 Wiki slug|0|

生成文件：

- `data/emergency_guides.json`
- `data/guide_index.json`
- `docs/knowledge/wiki_audit_2026-07-17.md`
- `docs/knowledge/guide_audit_2026-07-17.md`

## 6. 未覆盖内容

本批刻意没有进入 P2 深制造：

- 治具系统和复杂定位模板。
- 滑轮、杠杆、绳轮和简单传动 Guide。
- 专业木工结构、榫卯、家具级加工。
- 金属冶炼、热处理、焊接、机床加工。
- 电池自制、发动机深维修、高压容器或武器制造。
- 大型工坊生产排程、库存批号和商业质量管理。

这些内容应放到 Manufacturing v0.2 或更后阶段，并且需要 Field Test 后再判断是否值得扩展。

## 7. 下一步 Field Test 建议

建议进入 Batch9-D Manufacturing Field Test。本阶段不要提前修改 Retrieval/profile。

建议 strict cases：

1. “用废木板做一个架子，开工前先检查什么？” -> DG-0871 / DG-0874
2. “切木板时没有夹具，能不能一只手扶着锯？” -> DG-0873
3. “旧塑料桶能不能改成装饮水的桶？” -> DG-0872
4. “金属片边很锋利，能不能直接做支架？” -> DG-0876
5. “胶水、螺丝、扎带应该怎么选？” -> DG-0875
6. “自制架子放水桶前怎么测试？” -> DG-0877
7. “收工后刀具和金属碎片怎么处理？” -> DG-0878
8. “木材有裂纹还能不能做支撑？” -> DG-0872 / DG-0873
9. “薄金属片打孔能不能手扶？” -> DG-0876
10. “做完的箱子有钉尖和毛刺怎么办？” -> DG-0877

建议 observation cases：

- “修一个坏架子”观察 repair 是否合理主导。
- “做一个新架子”观察 manufacturing 是否进入 primary evidence。
- “废料怎么分类”观察 recycling / repair / manufacturing 边界。
- “割到手了怎么办”观察 medical 是否应主导。
- “做育苗箱”观察 planting 与 manufacturing 协同。
- “临时门板怎么做”观察 shelter 与 manufacturing 协同。

验收目标建议：

- fail = 0
- dangerous suggestion = 0
- Kiwix 越权 = 0
- safety / fallback / record-check = 100%
- manufacturing Guide 能稳定进入制作、材料、工坊、承重、金属锐边等 evidence。

## 8. 结论

Batch9-C 已建立 Manufacturing & Production Foundation v0.1 的第一阶段行动知识链：

个人/小团队工坊安全 -> 材料再利用判断 -> 木材加工 -> 连接方式选择 -> 废旧金属处理 -> 成品承重检查 -> 收工清点与记录。

当前状态适合进入 Batch9-D Field Test。后续如出现 repair/tools/shelter/recycling 抢主位，应先做 Root Cause Review，再决定是否需要最小 profile 或 evidence priority 修复。
