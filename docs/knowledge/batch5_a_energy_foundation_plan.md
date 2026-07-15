# Batch5-A Energy Foundation Expansion Plan

生成时间：2026-07-14

## 1. 范围与原则

本阶段只做能源基础知识扩容规划，不修改 Wiki、Guide、Retrieval Pipeline、Prompt、query profile、schema、PocketBase 或测试。

规划遵守 `docs/engineering/ROOT_CAUSE_FIX_POLICY.md`：不通过扩大召回、Prompt 修饰或强行关联掩盖知识缺口；后续 Apply 应先补可执行知识，再按真实场景做 Retrieval field test。

本批规划围绕离线环境、资源有限、长期维护和安全判断，不写百科型知识。

## 2. 当前能源知识覆盖

当前 `wiki_import/power/` 约 39 篇，已覆盖以下主题：

### 电池与移动电源

已有条目包括：

- `energy-battery-001`：锂电池进水和短路风险
- `energy-battery-002`：电池混用和新旧混放问题
- `energy-battery-003`：备用电池封存和轮换原则
- `energy-battery-capacity-001` / `energy-battery-capacity-002`：容量、功耗和可用时间
- `energy-battery-swelling-001`、`energy-battery-swelling-overheat-001`、`energy-swollen-lithium-battery-stop-use-001`：鼓包、发热和停用
- `energy-power-bank-001`、`energy-bank-battery-priority-001`：移动电源安全与低电量优先级
- `energy-charge-log-001`：充放电记录

覆盖状态：已有基础，但重复较多，且“鼓包/发热/停用/隔离”主题分散在多篇中。

### 太阳能

已有条目包括：

- `energy-solar-panel-001`：太阳能板基础使用
- `energy-solar-charging-001`：光照和温度影响
- `energy-solar-charging-002`：防水和线头保护
- `energy-solar-charging-003`：光照因素
- `energy-solar-system-basics-index-001`：太阳能系统基础索引

覆盖状态：已有入门和风险边界，但缺少“日内排程、阴天降级、面板遮挡/角度检查、充电优先级、临时固定和防风”这些长期离线使用动作单元。

### 电气安全与低压边界

已有条目包括：

- `energy-basic-electrical-safety-index-001`：基础电学安全索引
- `energy-power-strip-001`：插线板进水后不能通电测试
- `energy-water-damaged-electrical-no-power-001`：电器进水后禁止通电
- `energy-water-damaged-strip-001`：插线板进水后不能用通电测试确认安全
- `energy-charging-cable-001`：充电线破皮和接触不良风险
- `energy-device-port-001`：设备接口潮湿和腐蚀风险

覆盖状态：高风险边界已有，但偏向进水/插线板/线缆。缺少低压直流系统、临时接线、极性错误、保险丝/保护件、线径发热、短路前兆和“不可修”边界。

### 断电、照明和任务优先级

已有条目包括：

- `energy-power-outage-001`：断电后第一小时能源盘点
- `energy-power-outage-002`：停电后的关键设备分级
- `energy-blackout-lighting-priority-001`：停电照明优先级
- `energy-lighting-001` 到 `energy-lighting-005`：低亮照明、夜间跌倒、暴露风险
- `energy-task-scheduling-001`：低电量任务排程
- `energy-core-shutdown-loss-001`：Core 设备断电前处理
- `energy-radio-power-001`、`energy-phone-002`：通信和手机低电量

覆盖状态：已有行动入口和相关 Guide，但照明、通信、省电、任务排程之间可能发生 Retrieval 竞争。能源记录和长期维护仍偏少。

### 跨目录相关内容

能源相关内容分布在其他目录：

- 通信：`communication-power-saving-001`、`communication-contact-window-001`、`communication-communication-power-001`
- 记录：`general-energy-log-001`
- 风险决策：`safety-energy-budget-001`、`safety-risk-levels-001`
- 维修：`repair-power-outage-001`、`repair-electric-shock-001`、`repair-temporary-insulation-001`、`repair-wire-jacket-damage-stop-001`

覆盖状态：这些条目有价值，但后续 Retrieval 需要避免“能源问题被通信窗口抢占”或“电气维修问题被泛维修抢占”。

## 3. 缺口分析

### 已覆盖主题

- 锂电池鼓包、发热、进水、短路风险
- 移动电源低电量优先级
- 电池容量和功耗基础
- 备用电池封存和轮换
- 太阳能板基础使用、光照因素、防水线头保护
- 断电后第一小时盘点
- 停电照明优先级
- 插线板进水、电器进水、接口潮湿、充电线破皮
- 手机和通信设备低电量策略

### 缺失主题

P0 缺口：

- 低压直流系统停用边界
- 正负极接反判断
- 短路前兆和立即断开顺序
- 线缆发热、线径不足、接头松动判断
- 保险丝/保护件缺失时不可替代边界
- 临时接线前检查
- 充电时无人值守边界
- 多电池并联/串联禁止边界
- 混用不同化学体系和不同容量电池的停用判断
- 电池漏液、异味、腐蚀的隔离处理
- 潮湿环境下低压设备也不可盲目通电的边界
- 室内集中充电区防火隔离

P1 缺口：

- 低电量时设备开机窗口
- 设备耗电分级和优先级表
- 每日能源预算
- 充电队列和排班
- 太阳能日内排程和阴天降级
- 太阳能板遮挡、角度、温升检查
- 备用电池轮换台账
- 关键设备最低电量线

P2 缺口：

- Wh、mAh、V、A 的本地估算口径
- 直流和交流的现场差异
- 常见接口和充电规格识别
- 太阳能板开路电压/标称功率的低资源解释
- 电池老化对容量的影响
- 负载、线损和发热的关系

### 重复主题

可能重复或需后续合并评审的主题：

- 鼓包/发热/停用：
  - `energy-battery-swelling-001`
  - `energy-battery-swelling-overheat-001`
  - `energy-fever-001`
  - `energy-fever-003`
  - `energy-swollen-lithium-battery-stop-use-001`
- 容量/功耗：
  - `energy-battery-capacity-001`
  - `energy-battery-capacity-002`
- 太阳能光照因素：
  - `energy-solar-charging-001`
  - `energy-solar-charging-003`
- 插线板/电器进水：
  - `energy-power-strip-001`
  - `energy-water-damaged-electrical-no-power-001`
  - `energy-water-damaged-strip-001`

本阶段只记录，不合并、不废弃。

### 应废弃或合并内容

暂不建议直接废弃。建议下一阶段 Apply 前先做内容质量复核：

- `energy-battery-swelling-overheat-001`、`energy-fever-001`、`energy-fever-003` 的摘要出现“体温感受、精神状态、尿量”等医疗发热语境，疑似旧批量生成污染。后续应作为单独数据质量修复批处理，不混入 Batch5-A Apply。
- 鼓包/发热主题应保留一个“行动边界”核心条目，其余条目转为更细的场景单元，避免 Retrieval 选中多篇同义内容。
- 太阳能光照因素可保留一篇基础知识，新增条目应偏向动作：排程、固定、防水、阴天降级、记录。

## 4. Batch5-A 新增 Wiki 清单

目标约 40 篇。建议分类使用现有正式分类：

- 能源
- 维修 / 制作 / 替代 / 拆解再利用
- 风险决策
- 信息保存与长期重建

不新增重复 category。

### P0：电池安全、储能管理、低压系统和危险边界

1. `energy-low-voltage-system-stop-boundary-001`  
   title：低压直流系统停用边界  
   category：能源  
   risk_level：high  
   重点：发热、异味、冒烟、火花、接头变色、线皮软化时断开，不继续试接。

2. `energy-dc-polarity-reverse-check-001`  
   title：直流正负极接反检查  
   category：能源  
   risk_level：high  
   重点：红黑线、标记、接口方向、接反后的发热/无响应/保护断开。

3. `energy-short-circuit-warning-signs-001`  
   title：短路前兆和立即断开顺序  
   category：能源  
   risk_level：high  
   重点：火花、焦味、线缆快速升温、电压骤降、保护跳断。

4. `energy-wire-heating-load-limit-001`  
   title：线缆发热和负载过大判断  
   category：能源  
   risk_level：high  
   重点：接头温热、线皮变软、反复掉电、插头发烫时减载。

5. `energy-loose-connector-stop-use-001`  
   title：电源接头松动停用边界  
   category：能源  
   risk_level：high  
   重点：晃动断续、插口火花、黑点、接触不良，不用手压着继续充。

6. `energy-fuse-protection-no-bypass-001`  
   title：保险丝和保护件不可绕过  
   category：能源  
   risk_level：high  
   重点：不用铁丝、锡纸、导线替代保险丝，不反复强启。

7. `energy-temporary-wiring-precheck-001`  
   title：临时接线前检查  
   category：能源  
   risk_level：high  
   重点：断电、极性、裸露铜线、绝缘、固定、拉扯余量。

8. `energy-unattended-charging-stop-boundary-001`  
   title：无人值守充电停止边界  
   category：能源  
   risk_level：high  
   重点：睡眠区、布料旁、密闭箱内、鼓包设备、潮湿设备不无人充。

9. `energy-battery-parallel-series-boundary-001`  
   title：电池串并联禁用边界  
   category：能源  
   risk_level：high  
   重点：不同容量、不同新旧、不同品牌、不同电压不临时串并联。

10. `energy-battery-chemistry-mix-stop-001`  
    title：不同电池类型混用停用边界  
    category：能源  
    risk_level：high  
    重点：碱性、镍氢、锂电、一次性电池不混充不混组。

11. `energy-battery-leak-corrosion-isolation-001`  
    title：电池漏液和腐蚀隔离  
    category：能源  
    risk_level：high  
    重点：白粉、液体、刺鼻味、腐蚀弹片时戴防护、隔离、标记。

12. `energy-wet-low-voltage-device-no-test-001`  
    title：潮湿低压设备也不盲目通电  
    category：能源  
    risk_level：high  
    重点：USB、移动电源、小灯、对讲机潮湿后也先干燥复查。

13. `energy-charging-zone-fire-isolation-001`  
    title：集中充电区防火隔离  
    category：能源  
    risk_level：high  
    重点：离床铺、纸张、布料、燃料和出口，留散热和观察位置。

14. `energy-damaged-power-bank-quarantine-001`  
    title：受摔移动电源隔离观察  
    category：能源  
    risk_level：high  
    重点：外壳裂、进水、鼓包、按压异响、异常掉电时停用。

15. `energy-device-smell-heat-no-restart-001`  
    title：设备异味发热后不反复重启  
    category：能源  
    risk_level：high  
    重点：焦味、塑料味、局部烫手后断电，不靠重启验证。

16. `repair-power-cable-temporary-insulation-boundary-001`  
    title：电源线临时绝缘边界  
    category：维修 / 制作 / 替代 / 拆解再利用  
    risk_level：high  
    重点：只允许低压、干燥、轻载、短时标记；露铜/发热/潮湿停用。

17. `energy-unknown-adapter-stop-use-001`  
    title：未知电源适配器停用判断  
    category：能源  
    risk_level：high  
    重点：电压不明、接口不合、发热、啸叫、无标记时不试插关键设备。

18. `energy-battery-storage-temperature-boundary-001`  
    title：电池存放温度边界  
    category：能源  
    risk_level：high  
    重点：暴晒、火源、被褥、密闭高温、冰冻后立即充电都停止。

### P1：低电量策略、能源记录和设备优先级

19. `energy-daily-energy-budget-001`  
    title：每日能源预算  
    category：能源  
    risk_level：caution  
    重点：按通信、照明、记录、医疗/安全设备分配可用电量。

20. `energy-critical-device-minimum-charge-line-001`  
    title：关键设备最低电量线  
    category：能源  
    risk_level：caution  
    重点：为手机、收音机、Core、照明设红线，低于红线禁娱乐用途。

21. `energy-device-power-priority-table-001`  
    title：设备供电优先级表  
    category：能源  
    risk_level：caution  
    重点：保命、通信、照明、记录、舒适设备分层。

22. `energy-charging-queue-schedule-001`  
    title：充电队列和排班  
    category：能源  
    risk_level：caution  
    重点：谁先充、充到多少、何时拔、谁记录。

23. `energy-low-power-device-window-001`  
    title：低电量设备开机窗口  
    category：能源  
    risk_level：caution  
    重点：固定开机、固定同步、固定通信，不让后台耗电。

24. `energy-solar-day-schedule-001`  
    title：太阳能白天充电排程  
    category：能源  
    risk_level：caution  
    重点：上午试角度、中午防热、下午补关键设备。

25. `energy-solar-cloudy-day-downgrade-001`  
    title：阴天太阳能降级策略  
    category：能源  
    risk_level：caution  
    重点：弱光只补低功耗设备，暂停大电池和非必要负载。

26. `energy-solar-panel-shading-angle-check-001`  
    title：太阳能板遮挡和角度检查  
    category：能源  
    risk_level：caution  
    重点：阴影、灰尘、角度、温升、线头固定。

27. `energy-solar-panel-wind-tie-down-001`  
    title：太阳能板防风临时固定  
    category：能源  
    risk_level：caution  
    重点：不用拉扯线缆固定，风大收板，固定点复查。

28. `energy-spare-battery-rotation-log-001`  
    title：备用电池轮换台账  
    category：信息保存与长期重建  
    risk_level：normal  
    重点：编号、日期、电量、用途、异常、下次复查。

29. `energy-charge-discharge-handoff-log-001`  
    title：充放电交接记录  
    category：信息保存与长期重建  
    risk_level：normal  
    重点：设备、时间、起止电量、发热、负责人、异常。

30. `energy-night-lighting-runtime-plan-001`  
    title：夜间照明续航安排  
    category：能源  
    risk_level：caution  
    重点：固定灯位、巡视灯、厕所路线灯、备用灯分配。

31. `energy-shared-power-bank-checkout-001`  
    title：共享移动电源借用归还  
    category：信息保存与长期重建  
    risk_level：normal  
    重点：编号、借用人、用途、归还电量、异常标签。

32. `energy-load-shedding-order-001`  
    title：低电量断开负载顺序  
    category：能源  
    risk_level：caution  
    重点：先断舒适、娱乐、重复照明，再限通信窗口，最后保安全设备。

### P2：背景知识和长期维护

33. `energy-wh-mah-estimation-001`  
    title：Wh 和 mAh 的本地估算  
    category：能源  
    risk_level：normal  
    重点：用标称电压、容量和实际折损做粗略估算，不追求精密计算。

34. `energy-voltage-current-power-basics-001`  
    title：电压电流功率基础判断  
    category：能源  
    risk_level：normal  
    重点：V、A、W 在设备标签上的含义和常见误读。

35. `energy-dc-ac-field-difference-001`  
    title：直流和交流的现场差异  
    category：能源  
    risk_level：normal  
    重点：电池/USB/太阳能多为低压直流，市电/逆变为高风险交流。

36. `energy-usb-port-type-identification-001`  
    title：常见 USB 接口识别  
    category：能源  
    risk_level：normal  
    重点：接口形状、松动、潮湿、线缆损坏，不讨论快充协议深细节。

37. `energy-battery-aging-capacity-loss-001`  
    title：电池老化和容量下降  
    category：能源  
    risk_level：normal  
    重点：充得快掉得也快、低温明显衰减、鼓包发热不再计入可用。

38. `energy-line-loss-heat-basics-001`  
    title：线损和发热基础  
    category：能源  
    risk_level：normal  
    重点：线越细越长越容易掉压发热，出现发热时按 P0 边界处理。

39. `energy-inverter-risk-basics-001`  
    title：逆变器使用风险基础  
    category：能源  
    risk_level：caution  
    重点：逆变输出按高风险电源处理，不接潮湿设备和未知负载。

40. `energy-device-label-reading-001`  
    title：设备电源标签读取  
    category：能源  
    risk_level：normal  
    重点：输入电压、电流、极性、功率、适配器匹配和未知标签停用。

## 5. Guide 候选

最多建议 8 条，不为每篇 Wiki 创建 Guide。ID 仅为建议，实际 Apply 前需检查当前 `data/emergency_guides.json` 最大 ID 和命名连续性。

### DG-0841 低压设备发热或异味时停用

scenario：USB 灯、移动电源、对讲机、充电线或小型电源设备使用时发热、异味、冒烟、火花或反复掉电。

steps：

1. 先断开负载，再断开电源输入。
2. 不用手压插头、不反复重启、不换设备继续试。
3. 把设备移到不靠近布料、纸张、燃料和床铺的位置。
4. 贴停用标签，记录设备编号、现象、时间和责任人。
5. 至少冷却观察后再决定是否降级为非关键用途；鼓包、焦味或冒烟直接禁用。

check：

- 接头是否发烫、变色、松动。
- 线皮是否变软、变黑、有焦味。
- 设备是否鼓包、异响、异常掉电。

stop_or_escalate：

- 冒烟、火花、焦味、鼓包、烫手、液体渗出时立即隔离禁用。

fallback：

- 改用已知安全的低功耗设备。
- 暂停非必要照明或充电任务。

related_wiki：

- `energy-low-voltage-system-stop-boundary-001`
- `energy-short-circuit-warning-signs-001`
- `energy-wire-heating-load-limit-001`
- `energy-device-smell-heat-no-restart-001`

### DG-0842 临时接线前检查

scenario：需要临时连接低压电源、太阳能板、USB 线、灯具或小型设备。

steps：

1. 先断开所有电源。
2. 确认正负极、接口方向和标记。
3. 检查裸露铜线、潮湿、腐蚀、断股和线皮破损。
4. 固定线缆，避免拉扯接头。
5. 首次通电只接低价值、低功耗负载并有人观察。

check：

- 红黑线是否一致。
- 接头是否松动。
- 线缆是否发热。

stop_or_escalate：

- 极性不明、保护件缺失、接头发热或潮湿时停止。

fallback：

- 不临时接线，改用完整成品线缆或纸面记录待修。

related_wiki：

- `energy-temporary-wiring-precheck-001`
- `energy-dc-polarity-reverse-check-001`
- `energy-fuse-protection-no-bypass-001`
- `repair-power-cable-temporary-insulation-boundary-001`

### DG-0843 集中充电区布置

scenario：多个手机、移动电源、收音机、照明设备需要集中充电。

steps：

1. 选择远离床铺、纸张、布料、燃料和出口的位置。
2. 每个设备留散热空隙。
3. 鼓包、摔裂、进水或异味设备不进入充电区。
4. 充电设备编号，记录开始时间和负责人。
5. 夜间或无人时只保留必要设备，并降低充电数量。

check：

- 是否有设备堆叠。
- 是否有线缆绊倒风险。
- 是否有人负责巡查温度和异味。

stop_or_escalate：

- 发热、焦味、鼓包、冒烟或线缆变软时断电隔离。

fallback：

- 分批充电，优先通信、照明和记录设备。

related_wiki：

- `energy-charging-zone-fire-isolation-001`
- `energy-unattended-charging-stop-boundary-001`
- `energy-charge-discharge-handoff-log-001`
- `energy-device-power-priority-table-001`

### DG-0844 每日能源预算

scenario：长期断供下需要安排当天照明、通信、记录和关键设备电量。

steps：

1. 盘点所有可用电源和大致剩余电量。
2. 标出关键设备最低电量线。
3. 按保命、安全、通信、记录、舒适用途排序。
4. 安排开机窗口和充电队列。
5. 晚上复盘实际耗电和异常设备。

check：

- 是否有设备低于红线。
- 是否有重复照明或后台耗电。
- 是否有未记录的借用移动电源。

stop_or_escalate：

- 关键通信、照明或医疗/安全设备低于红线时停止非必要用电。

fallback：

- 改为纸质记录、固定短期开机、低亮灯位和轮流使用设备。

related_wiki：

- `energy-daily-energy-budget-001`
- `energy-critical-device-minimum-charge-line-001`
- `energy-device-power-priority-table-001`
- `energy-low-power-device-window-001`
- `general-energy-log-001`

### DG-0845 太阳能白天补电排程

scenario：有便携太阳能板，但光照、天气和设备电量有限。

steps：

1. 上午检查面板、线头、防水和固定点。
2. 先给关键设备或中转移动电源补电。
3. 中午检查温升和遮挡，不把设备暴晒。
4. 阴天只保低功耗任务。
5. 收板前记录天气、时段、设备和充入估计。

check：

- 是否有阴影、灰尘、线头受拉。
- 面板是否过热或被风吹动。
- 设备是否放在防晒防潮位置。

stop_or_escalate：

- 线头进水、设备发热、风大无法固定时停止充电。

fallback：

- 转入节电模式，推迟大电池充电，保留通信窗口。

related_wiki：

- `energy-solar-day-schedule-001`
- `energy-solar-cloudy-day-downgrade-001`
- `energy-solar-panel-shading-angle-check-001`
- `energy-solar-panel-wind-tie-down-001`

### DG-0846 电池混用和串并联禁用

scenario：想把多个电池、移动电源或电池盒临时组合使用。

steps：

1. 检查电池类型、电压、容量、新旧和外观。
2. 不混用不同化学体系、不同容量、不同新旧电池。
3. 不临时串联或并联未知电池组。
4. 漏液、腐蚀、鼓包、摔裂电池直接隔离。
5. 可用电池单独编号、单独记录、低风险使用。

check：

- 是否有混装、漏液、腐蚀、发热。
- 是否有人尝试用导线临时并联。

stop_or_escalate：

- 类型不明、电压不明、漏液、鼓包、接线发热时停止。

fallback：

- 分开轮换使用，不组合；优先低功耗设备。

related_wiki：

- `energy-battery-parallel-series-boundary-001`
- `energy-battery-chemistry-mix-stop-001`
- `energy-battery-leak-corrosion-isolation-001`
- `energy-spare-battery-rotation-log-001`

### DG-0847 关键设备最低电量线

scenario：手机、收音机、Core、照明和终端设备都需要用电，但总电量不足。

steps：

1. 列出关键设备和最低保留电量。
2. 低于红线的设备只允许短期开机或充电。
3. 暂停娱乐、重复照明、长时间后台运行。
4. 设备使用后立即记录剩余电量和下次开机时间。
5. 每天固定时间复核红线是否需要调整。

check：

- 是否有设备越过红线仍被使用。
- 是否有未登记的借用和充电。
- 是否有后台耗电。

stop_or_escalate：

- 通信或照明设备低于红线时停止非关键任务。

fallback：

- 纸质记录、短信优先、固定开机窗口、共享低亮灯位。

related_wiki：

- `energy-critical-device-minimum-charge-line-001`
- `energy-device-power-priority-table-001`
- `energy-low-power-device-window-001`
- `energy-load-shedding-order-001`

### DG-0848 未知适配器和未知接口停用

scenario：找到旧电源适配器、未知充电器、接口不确定的线缆或设备。

steps：

1. 读标签，确认输入/输出电压、电流、极性和接口。
2. 标签不清或不匹配时不接关键设备。
3. 检查线皮、插头、接口松动、腐蚀、异味。
4. 如必须测试，只在低价值设备上短时观察，不无人值守。
5. 记录适配器编号和可用/禁用状态。

check：

- 电压是否匹配。
- 极性是否明确。
- 接头是否松、烫、有焦味。

stop_or_escalate：

- 标签缺失、电压不明、接口不合、发热、啸叫时停止。

fallback：

- 改用原配或已登记适配器；未知设备贴禁用标签。

related_wiki：

- `energy-unknown-adapter-stop-use-001`
- `energy-device-label-reading-001`
- `energy-usb-port-type-identification-001`
- `energy-loose-connector-stop-use-001`

## 6. Retrieval 风险预测

### power / repair 混淆

高风险 query：

- “电线破了还能不能胶带缠一下”
- “临时接线前怎么处理”
- “插线板进水能不能试一下”
- “低压设备发热还能用吗”

风险：可能在 `repair_wire_damage`、维修绝缘、能源低压、电气安全之间竞争。

建议：后续 field test 分开测“电源系统判断”和“维修临时绝缘”，不要用扩大 top_k 解决。若 Apply 需要 profile，应以对象词区分：电池/充电/适配器/太阳能偏 energy，外皮破损/胶带/维修偏 repair。

### power / computer 混淆

高风险 query：

- “Core 快没电了先关什么”
- “离线设备怎么省电”
- “USB 口潮湿还能同步吗”
- “终端设备低电量怎么办”

风险：可能误召回终端同步、Core 设备、计算机维护或任务系统内容。

建议：能源 Wiki 应强调供电判断、关机前记录和最低电量线；不要引入软件同步、文件传输或设备协议细节。

### power / communication 混淆

高风险 query：

- “手机低电量怎么联系”
- “对讲机和收音机谁优先供电”
- “固定开机窗口怎么安排”
- “LoRa 节点没电怎么办”

风险：通信 Guide 抢占能源预算问题，或能源 Guide 抢占通信纪律问题。

建议：通信类 query 可保留通信 Guide 主导；能源 Batch5-A 应补“供电优先级”和“最低电量线”，不要重写通信窗口策略。

### power / food / shelter 混淆

高风险 query：

- “停电后冰箱怎么办”
- “高温停电怎么安排”
- “夜间照明怎么布置”

风险：食物保存、避暑、庇护、照明都有正当 evidence。规划时应接受多领域组合，但 P0 电气危险 query 必须优先停用/隔离/不可通电。

## 7. 不建议加入内容

本批不建议加入：

- 高压电、市电配电箱维修、屋内重新布线教程。
- 发电机深度维护、燃油系统、排烟工程。
- 复杂逆变器改装、BMS 改装、电芯点焊、拆电池包。
- 快充协议百科、USB PD 细节表、太阳能控制器参数大全。
- 需要网购、专业仪器、专业电工资质的流程。
- 终端同步、LoRa / USB / Wi-Fi / BLE 传输细节。
- Core 软件任务系统或数据同步逻辑。
- 把医疗发热语境混入电池发热条目。

## 8. 下一阶段 Batch5-A Apply 范围

建议 Apply 分两步，不要一次性改 Retrieval：

1. 新增约 40 篇 Wiki。
   - 优先 P0 18 篇，确保危险边界明确。
   - P1 14 篇补能源预算、排班和记录。
   - P2 8 篇做低资源背景知识。

2. 新增最多 8 个 Guide。
   - 只创建真实行动入口，不为每篇 Wiki 配 Guide。
   - high Guide 必须包含停止条件、禁用边界、降级方案和记录建议。

3. 不修改 Retrieval Pipeline / Prompt / top_k / selector limit。

4. Apply 后运行：
   - `python3 tools/audit_wiki.py`
   - `python3 tools/build_guides.py`
   - `python3 scripts/audit_guides.py`

5. 再进入 Batch5-B Field Test。
   - 测试低压短路、未知适配器、无人充电、太阳能阴天、关键设备红线、临时接线、电池混用、能源日志等真实问题。
   - 若失败，只按 root cause 分类，不临时补丁。

## 9. 验收建议

Batch5-A Apply 的验收目标：

- 新增 Wiki 约 40 篇。
- 不引入 audit 问题。
- 不新增重复 category。
- Guide 数量不超过 8。
- 不修改 Retrieval Pipeline。
- 不修改 Prompt。
- 不修改 query profile，除非后续 Field Test 证明是 profile 根因。
- 所有 P0 high 条目必须具备明确停止条件、禁用边界、降级用途和记录建议。
