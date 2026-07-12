# LanternBox Knowledge + Retrieval v1.0 Stable Baseline

## 1. 基线日期

- 基线版本：v1.0
- 基线日期：2026-07-12
- 状态：Stable Baseline
- 适用范围：Guide、Wiki、Guide-Wiki 关联、Retrieval Pipeline 与安全回答边界

## 2. Guide 状态

| 指标 | 基线值 |
| --- | ---: |
| Guide 总数 | 759 |
| `risk_level` 覆盖 | 759/759 |
| 非空 array `fallback` | 759/759 |
| Audit errors | 0 |
| Audit warnings | 0 |
| Audit advisories | 0 |

Guide 风险等级统一为 `normal` / `caution` / `high` / `critical`，聚合数据与 Guide 源 JSON 保持一致。

## 3. Wiki 状态

| 指标 | 基线值 |
| --- | ---: |
| Markdown Wiki | 662 |
| PocketBase Wiki | 662 |
| Categories | 24 |
| Audit errors | 0 |
| Audit warnings | 0 |
| Audit advisories | 0 |

662 条 Markdown Wiki 与 PocketBase `wiki_articles` 一一对应，元数据和正文保持同步。

## 4. Guide-Wiki 关系

| 指标 | 基线值 |
| --- | ---: |
| 双向关系 | 1716 组 |
| 单边关系 | 0 |
| 无效 Guide ID | 0 |
| 无效 Wiki slug | 0 |

Guide `related_wiki`、Wiki frontmatter `guide_links` 和 Wiki 正文 `## 对应 Guide` 构成可追踪的双向关系。关联必须有直接语义支撑，不得仅因领域相近而批量建立。

## 5. Retrieval 状态

| 指标 | 基线值 |
| --- | ---: |
| Batch3-G 全量实战测试 `pass/partial/fail` | 20/2/0 |
| Batch3-I `nav_flood` 定向重测 | pass |
| Batch3-I `records_backup` 定向重测 | pass |
| high / critical 安全边界覆盖 | 100% |
| fallback 覆盖 | 100% |
| 外部依赖违规 | 0 |
| Kiwix 越权 | 0 |

Batch3-I 针对 Batch3-G 的两条 partial 用例完成定向重测，两者均为 pass。该结论表示两个剩余语义缺口已闭环，不替代 Batch3-G 的 22 条全量原始统计。

Retrieval 回答的证据分工为：Guide 提供行动步骤，related Wiki 提供判断与边界，独立 Wiki 作为补充，Kiwix 只提供不覆盖行动建议的背景信息。

## 6. 已完成关键修复

- Wiki slug 作为 Wiki evidence 的主标识，PocketBase record ID 仅作 metadata 保留。
- Guide selected 后确定性展开 `related_wiki`，独立 Wiki 检索只作补充。
- `risk_level`、`check`、`fallback`、`stop_or_escalate` 已进入 Guide evidence 和回答 Prompt。
- high / critical Guide 优先输出停止、隔离、撤离、停用、不可入口、不可通电和不可继续等边界。
- Kiwix 增加领域锚点和负向语义过滤，仅作背景补充。
- 外部依赖默认路径已拦截；可选求援表达必须同时给出无法联络时的本地降级方案。
- 已建立 [ROOT_CAUSE_FIX_POLICY.md](../engineering/ROOT_CAUSE_FIX_POLICY.md)，明确禁止针对单题、Guide ID、Wiki slug 或测试 fixture 的补丁式修复。

## 7. 冻结原则

1. 后续不得随意批量修改 Guide 或 Wiki 正文。正文变更必须有明确知识缺口、风险修正或人工复核结论。
2. 不得伪造 Guide-Wiki 关联，不得为提高测试命中率硬编关系。
3. 新增或修改知识必须通过 Guide audit、Wiki audit、Guide-Wiki 双向关系校验和 retrieval field test。
4. Retrieval Pipeline 修改必须遵守 `ROOT_CAUSE_FIX_POLICY.md`，优先修复数据源、schema / contract 和 pipeline 节点根因。
5. 聚合数据必须由现有构建脚本重建，不得绕过 Guide/Wiki 源文件直接维护。
6. 任何变更不得使 audit 或关系校验从零问题退化。

## 8. 下一阶段建议

下一阶段建议进入 **Batch4-A Knowledge Gap Expansion Plan**，先审计、再规划、后扩充，不直接批量生成正文。

优先方向：

- 工具与维修：补齐承重、固定、磨损、临时修补和失败停止边界。
- 导航与环境判断：补齐灾后路线复核、地形变化、回撤条件和环境记录交接。
- 记录恢复与知识交接：明确备份、恢复演练、版本管理、纸电双备份和敏感数据边界的分工。
- 高风险人工复核：按 high / critical 条目的事故后果、停止条件和离线降级方案定期复核。

Batch4-A 应先产出知识缺口清单、风险优先级、与现有 Guide/Wiki 的边界说明和验收用例，再决定是新增、扩写、合并还是降级为背景资料。
