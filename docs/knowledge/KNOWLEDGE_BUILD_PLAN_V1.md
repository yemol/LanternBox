# LanternBox Knowledge Build Plan V1

本计划用于把 LanternBox 从“应急指南集合”推进为“长期生存能力库”。建设顺序遵循：先行动卡，再精选知识，再广域大库，再接入检索，最后用场景测试验收。

## 第一批：补 P0 Guide 缺口

目标：

- 按 `KNOWLEDGE_COVERAGE_V1.md` 补齐 P0 知识域的行动卡。
- 优先覆盖水、食物、医疗急救、能源、维修、卫生、安全、通讯、避难转移、火源通风、衣物体温、空间分区、污染控制、风险决策、虫害动物、库存纪律。
- 每张 Guide 都按 `GUIDE_SCHEMA_V1.md` 写成可执行行动卡。

验收标准：

- 每个 P0 域至少有 5 张 Guide。
- 每张 Guide 包含 `scenario`、`goal`、`keywords`、`steps`、`check`、`common_mistakes`、`fallback`、`stop_or_escalate`。
- 高风险 Guide 必须写明停止或升级信号。
- 维修和野外食物获取内容符合安全边界。

测试方式：

- 运行现有 Guide 构建脚本，确认索引生成成功。
- 用 Retrieval v2 compact 测试检查每个 P0 域至少 2 题能命中本地 Guide。
- 人工抽查 20 张 Guide，确认不是百科段落，而是行动卡。

## 第二批：补 P0 Wiki 精选条目

目标：

- 为 P0 Guide 背后的判断标准建立 Wiki 条目。
- 优先解释高风险边界，例如水质、漏电、一氧化碳、发热、感染、食物腐败、撤离决策。
- Wiki 按 `WIKI_SCHEMA_V1.md` 结构化。

验收标准：

- 每个 P0 域至少有 3 条 Wiki。
- 每条 Wiki 至少关联 1 张 Guide。
- 高风险 Wiki 必须包含判断边界和不适用条件。
- Wiki 不重复 Guide 步骤，而是解释原因和判断。

测试方式：

- 构建 Wiki 索引或同步 PocketBase Wiki。
- 用 30 个“为什么、怎么判断、还能不能”的问题测试 Wiki 召回。
- 前端调试输出能区分 Guide 行动卡和 Wiki 精选知识。

## 第三批：建立 Kiwix/ZIM 清单

目标：

- 建立可离线保存的 Kiwix/ZIM 主题清单。
- 覆盖 P0/P1/P2 的广域背景知识。
- 为每个 ZIM 记录用途、体积、语言、更新频率、优先级和安全限制。

验收标准：

- 至少列出 P0 相关 ZIM 候选：医学急救、公共卫生、食品安全、电气安全、灾害、维修、地图或地理。
- 至少列出 P1/P2 相关 ZIM 候选：农业、园艺、材料、基础制造、教育、工程。
- 每个候选都有接入优先级和不应覆盖本地 Guide 的说明。

测试方式：

- 用样例查询人工检查 ZIM 主题是否能补充背景。
- 验证 LanternBox 在无网络条件下能读取 ZIM 元数据或预生成索引。
- 建立 Kiwix/ZIM 清单文档或数据文件，供后续 fetcher 使用。

## 第四批：接入 Retrieval v2 Kiwix fetcher

目标：

- 在 Retrieval v2 中接入 Kiwix/ZIM fetcher。
- 保持 Guide 和 Wiki 优先级，不让 Kiwix/ZIM 覆盖本地更具体的安全边界。
- 在 debug 输出中稳定展示 Kiwix 候选数量和选中情况。

验收标准：

- Retrieval v2 的 `source_plan` 可以规划 `kiwix`。
- debug 中能看到 `candidate_types.kiwix`。
- 高风险问题仍优先选择 Guide。
- Kiwix 只在背景解释、学习和低风险扩展中进入最终证据。

测试方式：

- 对同一问题分别测试应急问法、判断问法和学习问法。
- 检查燃气泄漏、进水电器、严重伤病等问题不会只依赖 Kiwix。
- 加入 Kiwix fetcher 的单元测试和端到端测试。

## 第五批：建立 50 题知识验收测试集

目标：

- 建立覆盖 P0/P1/P2 的 50 题测试集。
- 测试集不硬编码具体 DG 编号作为唯一断言，但要检查领域、证据类型和安全边界。
- 支持普通输出和 compact 输出。

验收标准：

- P0 至少 32 题，覆盖全部 P0 域。
- P1 至少 12 题，覆盖持续生活和团队协作。
- P2 至少 6 题，覆盖地图、环境、材料、信息保存和重建。
- 每题输出 question、scenario_summary、core_terms、source_plan、candidate count、candidate types、selected evidence、debug。
- 高风险问题必须能看到 Guide 证据和停止线相关内容。

测试方式：

- 扩展 `scripts/test_retrieval_v2.py` 或新增知识验收脚本。
- 每次知识库构建后运行 compact 测试。
- 每次接入新数据源后运行完整测试并保存人工验收记录。

## 长期维护节奏

- 每次新增 Guide/Wiki 后，补至少 1 个测试问题。
- 每次发现 Retrieval v2 命中差，优先检查数据字段、关键词、别名和负面词，不先改业务逻辑。
- 每月复核 P0 高风险内容的停止线。
- 每季度复核 Kiwix/ZIM 清单和版本。

