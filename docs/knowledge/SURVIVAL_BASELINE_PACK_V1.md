# LanternBox Survival Knowledge Baseline Pack V1

本文件定义 LanternBox 在任何离线、断供、灾害或长期自持环境下必须携带的最小知识集合。它是静态 baseline，不是运行时下载计划，也不是外部资源发现策略。

“Kiwix enabled” 的含义是：该领域允许通过 Kiwix 查询本地 ZIM 内容，用作解释增强和背景补充。

“ZIM coverage” 的含义是：该领域应被离线 ZIM 知识底座覆盖。本文列出的所有 baseline domain 都应为 Y，因为越完整的离线基础知识越能帮助团队处理困难和异常问题。

“ZIM required” 的含义是：该领域是否属于最低强制随包携带的 ZIM 范围。它不表示运行时必须联网获取，也不表示系统可以在灾害环境中动态新增资源。

## 1. 核心生存领域（必须覆盖）

以下为不可缺失知识域。每个领域必须至少有本地 Guide 行动边界和 Wiki 判断解释；Kiwix 是访问层，ZIM 是离线内容层，只提供背景、术语、教材和广域补充，不覆盖本地 Guide/Wiki 的安全边界。

### water（饮水获取/净化/污染判断）

- Baseline scope：饮水获取、储水、配水、过滤、煮沸、消毒、化学污染停止线、脱水观察。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充水质、公共卫生、化学污染、基础水文）
- ZIM required: Y

### food（口粮管理/可食性判断/基础保存）

- Baseline scope：库存盘点、配给、变质判断、停电后食物处置、儿童老人优先级、低燃料烹饪。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充营养、保存、食品安全、常见食材知识）
- ZIM required: Y

### medical（外伤/感染/基础急救/发热）

- Baseline scope：出血、伤口清洁、感染信号、发热、腹泻脱水、烧烫伤、扭伤固定、药品边界。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充基础医学、急救教材、公共卫生和常见疾病背景）
- ZIM required: Y

### shelter（避难/空间保护/温控）

- Baseline scope：室内分区、入口缓冲、睡眠区、病人隔离、临时避难点、保温、通风、防潮。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充临时庇护、建筑通风、营地管理）
- ZIM required: N

### evacuation（撤离/路径/风险判断）

- Baseline scope：撤离包、路线记录、集合点、负重限制、洪水撤高、返回风险、探路超时规则。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充地图阅读、地形、灾害避难和户外行走背景）
- ZIM required: N

### power（基础能源/断电应对/电池管理）

- Baseline scope：断电后优先级、照明、手机低电量、移动电源、电池轮换、进水电器停用线。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充基础电学、太阳能、蓄电池、低功耗系统）
- ZIM required: Y

### hygiene（卫生/感染控制/垃圾处理）

- Baseline scope：少水洗手、厕所替代、垃圾密封、污染区/清洁区、呕吐物和粪便处理、虫害预防。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充公共卫生、感染控制、环境卫生、消毒边界）
- ZIM required: Y

### security（基础风险识别/夜间安全/异常情况）

- Baseline scope：夜间异响、陌生人接触、低暴露照明、双人外出、路线和返回时间、冲突降温。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充风险管理、态势观察、非暴力沟通）
- ZIM required: N

### tools（基础工具使用/替代/维修）

- Baseline scope：常用工具、临时固定、替代材料、拆解前停用检查、胶带/绳索/容器边界。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充基础维修、材料、手工制作、低技术方案）
- ZIM required: N

### communication（断网通讯/信号规则/备用方式）

- Baseline scope：断网计划、联系人表、集合点、固定开机窗口、纸条留言、近距离信号、无线电基础。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充无线电、通信协议、离线通信案例）
- ZIM required: Y

## 2. 扩展生存领域（重要但非绝对）

扩展领域支持长期自持、恢复生产和团队韧性。它们不应覆盖核心领域，也不应把系统推向运行时外部依赖。

### planting（基础种植）

- Baseline scope：容器种植、快菜、芽苗菜、种子保存、少水灌溉、土壤污染边界。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充园艺、农学、土壤、种子保存）
- ZIM required: N

### wild_food（野外食物获取/识别/处理）

- Baseline scope：未知植物停止线、低风险采集、野外食物获取、捕鱼/贝类/小型动物获取边界、被动陷阱和简易狩猎工具的安全/合法/误伤控制、动物性食物处理和彻底加热、替代蛋白。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充生态、食品安全、野外食物获取、基础陷阱/工具风险和猎获物处理背景）
- ZIM required: N

### repair（基础维修）

- Baseline scope：门窗、容器、线缆、背包、衣物、基础工具、材料替代和安全停用线。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充材料学、机械基础、维修手册）
- ZIM required: N

### psychology（压力/恐慌/冲突控制）

- Baseline scope：恐慌降温、坏消息沟通、疲劳保护、任务暂停、儿童参与、封闭空间边界。
- Guide: Y
- Wiki: Y
- Kiwix enabled: Y（允许查询本地 ZIM）
- ZIM coverage: Y（补充心理急救、危机干预、团队沟通）
- ZIM required: N

## 3. ZIM 映射规则（关键）

ZIM 映射只用于预置包和本地 selection 的静态依据，不允许在运行时触发外部扩展。

| Domain | Guide | Wiki | Kiwix enabled | ZIM coverage | ZIM required | Mapping note |
| --- | --- | --- | --- | --- | --- | --- |
| water | Y | Y | Y | Y | Y | 优先中文基础百科和公共卫生/水质背景。 |
| food | Y | Y | Y | Y | Y | 补充营养、保存、食品安全，不替代本地禁食边界。 |
| medical | Y | Y | Y | Y | Y | 医疗背景必须本地可用；高风险行动仍以 Guide 为准。 |
| shelter | Y | Y | Y | Y | N | 以 Guide/Wiki 为主，ZIM 补临时庇护和通风背景。 |
| evacuation | Y | Y | Y | Y | N | 以路线和风险 Guide 为主，ZIM 补地图/户外背景。 |
| power | Y | Y | Y | Y | Y | 电学和电池背景应随预置包携带。 |
| hygiene | Y | Y | Y | Y | Y | 公共卫生和感染控制背景应随预置包携带。 |
| security | Y | Y | Y | Y | N | 风险处理必须本地化，ZIM 补概念。 |
| tools | Y | Y | Y | Y | N | 工具和维修以本地材料为准，ZIM 补手册背景。 |
| communication | Y | Y | Y | Y | Y | 断网通信、无线电和信号知识应随预置包携带。 |
| planting | Y | Y | Y | Y | N | 长期恢复领域，ZIM 可补园艺和土壤知识。 |
| wild_food | Y | Y | Y | Y | N | 覆盖获取、识别、停止线、合法性、误伤控制和食品安全；不提供攻击性用途或针对人的武器化步骤。 |
| repair | Y | Y | Y | Y | N | 与 tools 互补，优先安全停用和低风险修复。 |
| psychology | Y | Y | Y | Y | N | 以团队稳定和冲突控制为主，ZIM 补心理急救背景。 |

## 4. ZIM 最小必备清单（核心）

以下清单是人工预置 baseline，不是运行时获取策略。文件名是目标命名约定；实际部署可以用等价本地 ZIM 替代，但必须在离线前准备好。

### PRIMARY（中文优先）

- `wikipedia_zh_simple.zim`
- `wikipedia_zh_medical`（如果存在）

### SECONDARY（补充）

- `wiktionary_en.zim`（术语）
- `wikipedia_en_simple.zim`（缺失补全）

当前仓库中的 mini ZIM 可用于 smoke test 和页面验证，但不能视为完整 Survival Baseline Pack。

## 5. 禁止规则（非常重要）

- 不允许运行时新增 ZIM。
- 不允许动态规划下载策略。
- 不允许外部知识源扩展。
- 不允许依赖未来数据。
- 不允许把 Kiwix/ZIM 结果作为绕过本地 Guide/Wiki 安全边界的依据。
- 不允许在灾害回答中默认互联网、外部搜索、在线地图、在线购买或远端服务可用。

## 6. 输出目标

该文件必须成为：

- Gap Detection 的依据。
- Kiwix selection 的依据。
- 系统生存能力 baseline。

具体落地约束：

- Gap Detection 应以本文固定 domain、ZIM coverage 和 ZIM required 字段判断 baseline 缺口。
- Kiwix selection 应优先读取本文的 domain-to-ZIM 映射；Kiwix 只负责访问本地 ZIM，不在运行时发现外部资源。
- Retrieval 和回答链路必须保持 Guide 优先、Wiki 解释、Kiwix 背景补充的层级。
- 当 ZIM 缺失时，系统必须降级为本地 Guide/Wiki，不得阻断核心生存回答。
