# Batch5-H Energy Management Retrieval Minimal Apply Report

生成日期：2026-07-15

## 1. 修改文件

- `data/retrieval_query_profiles.json`
- `data/guides/power/DG-0617.json`
- `data/guides/power/DG-0619.json`
- `data/emergency_guides.json`
- `data/guide_index.json`
- `tests/test_energy_management_retrieval_profiles.py`
- `docs/knowledge/batch5_f_energy_management_field_test_results.json`
- `docs/knowledge/batch5_f_energy_management_field_test_report.md`
- `docs/knowledge/batch5_h_energy_management_apply_report.md`

未修改 Wiki 正文、Guide 正文、Retrieval Pipeline、Prompt、top_k、selector limit、fallback 架构、PocketBase schema，也未新增 Wiki 或 Guide。

## 2. 新增 Profile

### `energy_daily_budget_priority`

覆盖：

- 停电多日后多设备电量不足
- 每日能源预算和低资源估算
- 用电复盘、晚间复盘、明天调整
- 夜间省电和最低照明
- 非必要设备停用、断开负载
- 低电量设备短时间开机窗口

优先拉入：

- DG-0844 每日能源预算
- DG-0845 关键设备最低电量线
- DG-0619 负载过大时断开排序

### `energy_charging_queue_management`

覆盖：

- 一个充电口、多设备排队
- 充电口占用
- 充电口轮换
- 共享移动电源借还
- 充放电交接

优先拉入：

- DG-0846 充电队列和排班

### `energy_solar_low_output_management`

覆盖：

- 阴天、云多、输出很弱
- 连续低产出
- 有限日照窗口
- 太阳能充电目标选择
- 白天补电排程

优先拉入：

- DG-0617 太阳能充电白天排程
- DG-0564 作为太阳能基础补充

## 3. Related Wiki 顺序调整

### DG-0617

将 Batch5-E 新增太阳能管理 Wiki 前移到 `related_wiki` 前部：

- `energy-solar-daytime-charge-schedule-001`
- `energy-cloudy-solar-downgrade-001`
- `energy-solar-charge-target-selection-001`
- `energy-solar-weather-pattern-log-001`

旧太阳能基础 Wiki 保留，未删除。

### DG-0619

将低电量断负载 Wiki 前移：

- `energy-low-battery-load-disconnect-order-001`
- `energy-nonessential-device-suspend-list-001`

未新增 DG-0848，未新增大批关联。

## 4. Contract Test

新增 `tests/test_energy_management_retrieval_profiles.py`，覆盖 9 个入口：

1. 停电三天多设备优先充电
2. 无准确电量百分比时做能源预算
3. 关键设备最低电量线
4. 太阳能板一个口多设备排队
5. 唯一充电口占用
6. 阴天太阳能低输出目标选择
7. 连续两天太阳能低输出后的照明与通信调整
8. 夜间只留一盏灯的最低照明 evidence
9. 用电混乱后的每日复盘

结果：`9 passed`。

## 5. Batch5-F 前后变化

| 指标 | Apply 前 | Apply 后 |
| --- | ---: | ---: |
| pass / partial / fail | 5 / 8 / 2 | 14 / 1 / 0 |
| Guide 命中率 | 83.3% | 100.0% |
| 主 Guide 命中率 | 33.3% | 91.7% |
| Wiki 命中率 | 33.3% | 91.7% |
| Guide-Wiki 精准组合率 | 50.0% | 100.0% |
| safety boundary | 100.0% | 100.0% |
| fallback | 100.0% | 100.0% |
| record/check | 100.0% | 100.0% |
| dangerous suggestion | 0 | 0 |
| P0 电气安全误触发 | 0 | 0 |
| 跨域竞争 | 2 | 0 |

## 6. 剩余 Partial

### `energy_one_light_at_night`

状态：partial。

当前 selected Guide：

- DG-0844
- DG-0619
- DG-0845

当前问题：

- 能源管理 Guide 组合已经正确进入。
- safety / fallback / record/check 均满足。
- 未出现通信、夜间安全或 P0 电气安全抢主位。
- 但 `energy-night-lighting-runtime-plan-001` 未进入 selected Wiki 前 6。

分类：

- selector / evidence priority。

处理建议：

- 不继续扩 profile。
- 不扩大 top_k。
- 不改 selector limit。
- 后续若必须追求 15/15，应先评审是否需要调整 DG-0844 / DG-0845 的 related_wiki evidence priority，而不是再增加 profile。

## 7. 是否达到 Energy Management Retrieval v0.1 Stable

建议可以标记为 **Energy Management Retrieval v0.1 stable candidate**，但不建议宣称“完全无缺口”。

理由：

- fail=0。
- strict 用例全部无危险建议、无 P0 误触发、无跨域抢位。
- safety / fallback / record/check 均为 100%。
- Guide-Wiki 精准组合率达到 100%。
- 剩余 1 个 partial 不影响安全边界，属于 selected Wiki evidence priority。

若项目稳定标准必须要求 `partial=0`，则不应 stable，应进入下一轮 evidence priority review；但按 Batch5-H 的目标“fail=0，不强求 15/15”，本批已经达到最小验收线。

## 8. 不建议继续修复的内容

- 不扩大 top_k。
- 不扩大 selector limit。
- 不修改 Prompt。
- 不修改 Retrieval Pipeline。
- 不新增 DG-0847 / DG-0848。
- 不新增 Wiki。
- 不把通信窗口 Guide 降权。
- 不把 P0 电气安全 profile 扩到能源管理问题。
- 不为了 `energy_core_terminal_low_power_window` 写硬编码规则或新增专门 Guide。

## 9. 验证结果

- `python3 tools/audit_wiki.py`：errors=0 warnings=0 advisories=0；markdown=804，pocketbase=804，categories=24。
- `python3 tools/build_guides.py`：Generated 776 Guides / 776 Guide Index Items。
- `python3 scripts/audit_guides.py`：errors=0 warnings=0 advisories=0；Guides=776。
- `env PYTEST_DISABLE_PLUGIN_AUTOLOAD=1 venv/bin/python -m pytest -q tests/test_retrieval_traceability.py tests/test_retrieval_root_contract.py tests/test_energy_management_retrieval_profiles.py`：18 passed。
- `python3 -m py_compile scripts/test_energy_management_field.py`：通过。
