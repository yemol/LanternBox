# Batch5-E Energy Management Expansion Apply Report

生成时间：2026-07-14

## 1. 新增 Wiki 清单

本批新增 23 篇能源 P1/P2 管理类 Wiki，未新增 P0 电气危险边界 Wiki。

| slug | title | category | priority | risk_level | Guide |
| --- | --- | --- | --- | --- | --- |
| `energy-daily-energy-budget-001` | 每日能源预算表 | 能源 | P1 | caution | DG-0844 |
| `energy-critical-device-minimum-line-001` | 关键设备最低电量线 | 能源 | P1 | caution | DG-0845 |
| `energy-device-power-priority-tier-001` | 设备供电优先级分层 | 能源 | P1 | caution | DG-0844 |
| `energy-charging-queue-schedule-001` | 充电队列和排班 | 能源 | P1 | caution | DG-0846 |
| `energy-low-battery-power-window-001` | 低电量设备开机窗口 | 能源 | P1 | caution | DG-0845 |
| `energy-solar-daytime-charge-schedule-001` | 太阳能白天补电排程 | 能源 | P1 | caution | DG-0617 |
| `energy-cloudy-solar-downgrade-001` | 阴天太阳能降级安排 | 能源 | P1 | caution | DG-0617 |
| `energy-night-lighting-runtime-plan-001` | 夜间照明续航安排 | 能源 | P1 | caution | DG-0845 |
| `energy-backup-battery-rotation-ledger-001` | 备用电池轮换台账 | 能源 | P1 | normal | DG-0336, DG-0618 |
| `energy-charge-discharge-handover-log-001` | 充放电交接记录 | 信息保存与长期重建 | P1 | normal | DG-0846 |
| `energy-low-battery-load-disconnect-order-001` | 低电量断开负载顺序 | 能源 | P1 | caution | DG-0619 |
| `energy-shared-power-bank-borrow-return-001` | 共享移动电源借用归还 | 信息保存与长期重建 | P1 | normal | DG-0846 |
| `energy-device-runtime-estimate-card-001` | 设备续航估算卡 | 能源 | P1 | normal | DG-0844 |
| `energy-essential-load-reserve-001` | 关键负载预留量 | 能源 | P1 | caution | DG-0845 |
| `energy-nonessential-device-suspend-list-001` | 非必要设备停用清单 | 能源 | P1 | normal | DG-0619 |
| `energy-day-end-power-review-001` | 每日用电复盘 | 信息保存与长期重建 | P1 | normal | DG-0844 |
| `energy-solar-charge-target-selection-001` | 日照不足时充电目标选择 | 能源 | P1 | caution | DG-0617 |
| `energy-charging-port-rotation-rule-001` | 充电口轮换规则 | 能源 | P1 | normal | DG-0846 |
| `energy-wh-mah-local-estimate-001` | Wh 和 mAh 的本地估算口径 | 能源 | P2 | normal | 无 |
| `energy-device-consumption-baseline-001` | 设备耗电基线记录 | 信息保存与长期重建 | P2 | normal | 无 |
| `energy-solar-weather-pattern-log-001` | 太阳能天气产出记录 | 信息保存与长期重建 | P2 | normal | DG-0617 |
| `energy-battery-aging-capacity-note-001` | 电池老化和可用容量记录 | 能源 | P2 | normal | DG-0336, DG-0618 |
| `general-energy-handover-summary-001` | 能源交班摘要 | 信息保存与长期重建 | P2 | normal | DG-0844 |

分布：

- P1：18
- P2：5
- P0：0

## 2. 新增 Guide 清单

本批新增 3 个核心 Guide，未新增 DG-0847 / DG-0848。

### DG-0844 每日能源预算

用途：长期断电或离网供电不稳定时，把可用能源、关键负载、预留量和暂停任务写成可交接的一日预算。

related_wiki：

- `energy-daily-energy-budget-001`
- `energy-device-power-priority-tier-001`
- `energy-device-runtime-estimate-card-001`
- `energy-day-end-power-review-001`
- `general-energy-handover-summary-001`

### DG-0845 关键设备最低电量线

用途：为通信、照明、记录或医疗照护设备设定低于即收紧的电量线，保留最低关键能力。

related_wiki：

- `energy-critical-device-minimum-line-001`
- `energy-essential-load-reserve-001`
- `energy-low-battery-power-window-001`
- `energy-night-lighting-runtime-plan-001`

### DG-0846 充电队列和排班

用途：移动电源、太阳能板或充电口有限，多台设备同时要求补电时，按关键等级和时间窗口安排充电。

related_wiki：

- `energy-charging-queue-schedule-001`
- `energy-charge-discharge-handover-log-001`
- `energy-shared-power-bank-borrow-return-001`
- `energy-charging-port-rotation-rule-001`

### 未新增的可选 Guide

- 未新增 DG-0847：现有 DG-0617 `太阳能充电白天排程` 已覆盖太阳能白天排程行动入口。本批将太阳能管理 Wiki 精准关联到 DG-0617。
- 未新增 DG-0848：现有 DG-0619 `负载过大时断开排序` 已覆盖断开负载行动入口。本批将低电量断开/非必要停用 Wiki 精准关联到 DG-0619。

## 3. 是否新增 Category

未新增 category。

本批只使用现有正式分类：

- 能源
- 信息保存与长期重建

未使用新分类，未修改 PocketBase schema。

## 4. PocketBase 同步数量

同步方式：本地 PocketBase SQLite upsert 到 `pocketbase/pb_data/data.db` 的 `wiki_articles` 表。

结果：

- 新增 `wiki_articles`：23
- 更新 `wiki_articles`：3
- 同步后 Markdown Wiki 总数：804
- 同步后 PocketBase `wiki_articles` 总数：804
- PocketBase category 数量：24

3 条更新是新增后修正 summary 长度并同步 content/summary：

- `energy-day-end-power-review-001`
- `energy-charging-port-rotation-rule-001`
- `energy-device-runtime-estimate-card-001`

## 5. Guide-Wiki 关系变化

新增 Guide-Wiki 双向关系：

- DG-0844：5 条
- DG-0845：4 条
- DG-0846：4 条

复用现有 Guide 的新增关系：

- DG-0617：4 条太阳能管理 Wiki
- DG-0619：2 条低电量断开/非必要停用 Wiki
- DG-0336：2 条电池轮换/老化记录 Wiki
- DG-0618：2 条电池轮换/老化记录 Wiki

校验结果：

- 单边关系：0
- 无效 Guide ID：0
- 无效 Wiki slug：0
- Guide-Wiki 前后向关系完全对称。

## 6. Audit 结果

已运行：

```bash
python3 tools/audit_wiki.py
```

结果：

- Articles：markdown=804 pocketbase=804 categories=24
- Issues：errors=0 warnings=0 advisories=0

已运行：

```bash
python3 tools/build_guides.py
```

结果：

- Generated 776 Guides
- Generated 776 Guide Index Items

已运行：

```bash
python3 scripts/audit_guides.py
```

结果：

- Guides：776
- Issues：errors=0 warnings=0 advisories=0
- Guide-Wiki 关联检查通过。

## 7. 未处理内容

本批未处理：

- P0 电气危险边界重复扩展。
- Energy Safety Retrieval v0.1 stable 的 profile。
- Retrieval Pipeline。
- Prompt。
- query profile。
- top_k。
- selector limit。
- fallback 架构。
- 终端同步代码。
- Core 任务系统。
- 通信协议细节。
- USB PD / 快充协议百科。
- 发电机维护。
- 逆变器改装。
- 高压电、市电维修教程。
- 暂缓的 4 篇规划 Wiki：
  - `energy-power-source-status-board-001`
  - `energy-low-power-core-terminal-window-001`
  - `energy-cold-weather-runtime-adjustment-001`
  - `energy-load-category-vocabulary-001`

## 8. 是否进入下一阶段 Field Test

应进入下一阶段 Field Test。

本批只完成 Energy Management 知识扩容和 Guide-Wiki 关系接入，不能宣布 stable。

下一阶段必须进入：

**Batch5-F Energy Management Retrieval Field Test**

建议 Field Test 覆盖：

- 停电三天优先给什么设备充电。
- 手机快没电但还要保持联系。
- 对讲机和照明谁优先。
- 阴天太阳能还要不要展开。
- 云很多时先充移动电源还是手机。
- 晚上只留一盏灯是否安全。
- 共享移动电源借走后剩余电量不清。
- 备用电池轮换记录丢失。
- 低电量时按什么顺序断开负载。

目标应先验证新增能源管理 Wiki/Guide 是否进入 evidence，不应直接追求 stable。
