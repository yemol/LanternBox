# Wiki 迁移计划 Batch1-B

生成日期：2026-07-11

## 范围

本计划基于最新 `python3 tools/audit_wiki.py` 结果，只筛选同时缺少以下四个正文最低结构的 Wiki：

- `## 用途`
- `## 操作步骤`
- `## 判断标准`
- `## 风险提示`

排序优先级为 `P0 > P1 > P2`。已完成 Batch1-A 的 10 个文件已排除；P3 背景项不纳入本批次。

## 审计基线

- 审计命令：`python3 tools/audit_wiki.py`
- 最新审计结果：`markdown=662 pocketbase=662 categories=24 errors=425 warnings=9 advisories=35`
- 当前计划不修改任何 Wiki 内容。

## 统计

- 总数量：59

### P0/P1/P2 分布

- P0: 10
- P1: 8
- P2: 41

### A/B/D 分布

- A 直接补全结构: 32
- B 重新生成正文: 17
- D 降级背景资料: 10

### Wiki 类型分布

- 操作型: 16
- 数据记录型: 16
- 制造型: 16
- 索引/背景型: 10
- 边界风险型: 1

## 条目清单

### 1. 水位变化

- 文件路径：`wiki_import/maps/navigation-water-level-change-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 2. 常见传染病基础索引

- 文件路径：`wiki_import/medical/medical-common-infectious-disease-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`caution`
- category：`医疗急救`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 3. 常见外伤并发症索引

- 文件路径：`wiki_import/medical/medical-trauma-complication-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`caution`
- category：`医疗急救`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 4. 基础电学安全索引

- 文件路径：`wiki_import/power/energy-basic-electrical-safety-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`high`
- category：`能源`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 5. 锂电池安全索引

- 文件路径：`wiki_import/power/energy-lithium-battery-safety-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`high`
- category：`能源`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 6. 太阳能系统基础索引

- 文件路径：`wiki_import/power/energy-solar-system-basics-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`caution`
- category：`能源`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 7. 饮用水微生物风险索引

- 文件路径：`wiki_import/water/water-drinking-water-microbial-risk-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`high`
- category：`水`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 8. 简易过滤材料索引

- 文件路径：`wiki_import/water/water-simple-filter-material-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`caution`
- category：`水`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 9. 蘑菇高风险索引

- 文件路径：`wiki_import/wild_food/wildlife-mushroom-high-risk-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`high`
- category：`野外食物获取 / 狩猎捕捞 / 动物蛋白补充`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 10. 有毒植物风险索引

- 文件路径：`wiki_import/wild_food/wildlife-toxic-plant-risk-index-001.md`
- 迁移优先级：P0: 核心生存能力
- Wiki 类型：索引/背景型
- risk_level：`high`
- category：`野外食物获取 / 狩猎捕捞 / 动物蛋白补充`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 11. 无线电通信基础索引

- 文件路径：`wiki_import/comms/communication-radio-communication-basics-index-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：索引/背景型
- risk_level：`normal`
- category：`通讯`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询、使用方式
- 推荐策略：D 降级背景资料

### 12. 库存消耗记录

- 文件路径：`wiki_import/data/general-inventory-consumption-log-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 13. 离线设备数据导出

- 文件路径：`wiki_import/data/general-offline-device-export-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 14. 维修记录

- 文件路径：`wiki_import/data/general-repair-log-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 15. 危险区域标记

- 文件路径：`wiki_import/maps/navigation-danger-zone-marking-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 16. 返回路线标记

- 文件路径：`wiki_import/maps/navigation-return-route-marking-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 17. 路线难度记录

- 文件路径：`wiki_import/maps/navigation-route-difficulty-record-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 18. 避难点记录

- 文件路径：`wiki_import/maps/navigation-shelter-point-record-001.md`
- 迁移优先级：P1: 高频使用能力
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 19. 失败经验记录

- 文件路径：`wiki_import/data/general-failure-lesson-record-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 20. 重要证件备份

- 文件路径：`wiki_import/data/general-important-documents-copy-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 21. 知识卡片归档

- 文件路径：`wiki_import/data/general-knowledge-card-archive-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 22. 长期档案轮检

- 文件路径：`wiki_import/data/general-long-term-archive-check-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 23. 地图标记规范

- 文件路径：`wiki_import/data/general-map-marking-standard-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 24. 成员信息备份

- 文件路径：`wiki_import/data/general-member-info-backup-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 25. 防潮保存

- 文件路径：`wiki_import/data/general-moisture-proof-storage-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 26. 多份备份

- 文件路径：`wiki_import/data/general-multiple-copies-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 27. 纸质备份价值

- 文件路径：`wiki_import/data/general-paper-backup-value-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 28. 纸电双备份

- 文件路径：`wiki_import/data/general-paper-digital-dual-backup-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 29. 记录可信度标注

- 文件路径：`wiki_import/data/general-record-trust-label-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 30. 种子批次记录

- 文件路径：`wiki_import/data/general-seed-batch-record-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 31. 小团队知识交接

- 文件路径：`wiki_import/data/general-small-team-handoff-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：数据记录型
- risk_level：`normal`
- category：`信息保存与长期重建`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 32. 容器修补材料

- 文件路径：`wiki_import/manufacturing/repair-container-repair-material-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 33. 布料磨损

- 文件路径：`wiki_import/manufacturing/repair-fabric-wear-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 34. 固定与支撑

- 文件路径：`wiki_import/manufacturing/repair-fixing-and-support-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 35. 材料降级使用

- 文件路径：`wiki_import/manufacturing/repair-material-downgrade-use-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 36. 金属锈蚀

- 文件路径：`wiki_import/manufacturing/repair-metal-corrosion-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 37. 塑料老化

- 文件路径：`wiki_import/manufacturing/repair-plastic-aging-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 38. 绳索承重

- 文件路径：`wiki_import/manufacturing/repair-rope-load-limit-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 39. 拆解回收零件

- 文件路径：`wiki_import/manufacturing/repair-salvaged-parts-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 40. 螺丝和螺母

- 文件路径：`wiki_import/manufacturing/repair-screws-and-nuts-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 41. 简单杠杆原理

- 文件路径：`wiki_import/manufacturing/repair-simple-lever-principle-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 42. 简易密封

- 文件路径：`wiki_import/manufacturing/repair-simple-sealing-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 43. 胶带类型

- 文件路径：`wiki_import/manufacturing/repair-tape-types-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 44. 临时替代件

- 文件路径：`wiki_import/manufacturing/repair-temporary-substitute-part-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 45. 磨损判断

- 文件路径：`wiki_import/manufacturing/repair-wear-judgement-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 46. 木材基础性质

- 文件路径：`wiki_import/manufacturing/repair-wood-basic-properties-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 47. 扎带用途边界

- 文件路径：`wiki_import/manufacturing/repair-zip-tie-boundary-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：制造型
- risk_level：`normal`
- category：`基础制造与材料维修`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 48. 环境变化复盘

- 文件路径：`wiki_import/maps/navigation-environment-change-review-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 49. 地标选择

- 文件路径：`wiki_import/maps/navigation-landmark-selection-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 50. 滑坡风险

- 文件路径：`wiki_import/maps/navigation-landslide-risk-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：边界风险型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：B 重新生成正文

### 51. 地图比例尺

- 文件路径：`wiki_import/maps/navigation-map-scale-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 52. 观察点选择

- 文件路径：`wiki_import/maps/navigation-observation-point-selection-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 53. 离线地图维护

- 文件路径：`wiki_import/maps/navigation-offline-map-maintenance-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 54. 气压变化

- 文件路径：`wiki_import/maps/navigation-pressure-change-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 55. 降雨记录

- 文件路径：`wiki_import/maps/navigation-rainfall-record-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 56. 土壤潮湿判断

- 文件路径：`wiki_import/maps/navigation-soil-moisture-judgement-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 57. 温湿度记录

- 文件路径：`wiki_import/maps/navigation-temperature-humidity-record-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 58. 地形高低判断

- 文件路径：`wiki_import/maps/navigation-terrain-height-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

### 59. 风向观察

- 文件路径：`wiki_import/maps/navigation-wind-direction-observation-001.md`
- 迁移优先级：P2: 辅助知识
- Wiki 类型：操作型
- risk_level：`normal`
- category：`地图地形与环境监测`
- 当前缺失结构：
  - 缺少 `## 用途`
  - 缺少 `## 操作步骤`
  - 缺少 `## 判断标准`
  - 缺少 `## 风险提示`
- 当前 H2：是什么、为什么重要、如何判断、适用边界、常见误区、对应 Guide、Kiwix/ZIM 可继续查询
- 推荐策略：A 直接补全结构

