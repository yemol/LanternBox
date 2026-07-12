# LanternBox 根因修复原则

## 1. 项目原则

LanternBox 不接受补丁式处理。所有问题优先定位数据源、接口契约、处理链路和架构边界，再修改实现。

## 2. 禁止的修复方式

- 前端兜底掩盖后端数据错误
- Prompt 表面修饰掩盖 evidence 缺失
- 针对测试用例硬编码
- 单题关键词特殊处理
- 用 top_k 增大掩盖召回策略错误
- 让 AI 自行猜测缺失字段
- 忽略 slug / ID / schema 一致性
- 只修最终回答，不修数据流
- 只在最终回答阶段强塞 Wiki
- 让 Kiwix 覆盖 Guide/Wiki 的行动建议

## 3. 正确修复顺序

1. 复现问题
2. 定位数据源
3. 检查 schema / contract
4. 检查 pipeline 节点
5. 修正根因
6. 增加回归测试
7. 运行审计
8. 输出变更报告

## 4. 对 Retrieval Pipeline 的特别要求

- Guide 被选中后必须确定性加载 related_wiki
- Wiki evidence 必须保留 slug
- Guide evidence 必须包含 risk_level、check、fallback、stop_or_escalate
- high / critical Guide 必须优先输出停止、隔离、撤离、停用、不可入口、不可通电、不可继续等边界
- Kiwix 只能做背景补充，不能覆盖 Guide/Wiki 行动建议
- source priority 必须体现 Guide > related Wiki > independent Wiki > Kiwix
- selected_sources / excluded_sources 必须可追踪来源和排除原因

## 5. 对知识库的特别要求

- 不用结构合格掩盖内容空洞
- 不用泛化词掩盖判断标准缺失
- 不强行关联不匹配的 Guide/Wiki
- 不用百科背景替代行动卡
- 不用错误的关联制造“看起来有来源”的回答

## 6. 验收标准

修复后必须通过：

- 对应单元测试
- Guide audit
- Wiki audit
- Guide-Wiki 双向校验
- Batch3-F 的真实场景回归测试
