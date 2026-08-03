# 系统架构

## 顶层结构

FT-01 采用单主循环、非阻塞服务与页面状态机。硬件服务负责采集和通信，页面只读取状态并绘制，不应拥有底层驱动生命周期。

```text
setup()
├── 初始化显示、键盘、SD、GNSS、音频和任务
├── 初始化 LoRa / PKI
└── 启动非阻塞无线接收

loop()
├── serviceLoRaBackground()
├── GNSS / 记录 / 同步等服务
├── 当前页面 tick()
└── 键盘事件分发
```

## 核心模块

| 模块 | 文件 | 职责 |
|---|---|---|
| 应用壳与页面切换 | `LanternBox_FT01.ino` | 启动、主循环、首页、全局服务、页面调度 |
| 硬件与共享 SPI | `FtHardware.*` | SD、LoRa、GNSS 引脚和共享 SPI 状态 |
| 通用 UI | `FtUiCommon.*` | 按键判断、标题、底栏和公共绘制 |
| 文本工具 | `FtTextUtil.*` | JSON、路径、文件名和容量格式化 |
| 时间工具 | `FtTimeUtil.*` | 编译时间、Epoch 与日期时间转换 |
| GNSS 消息上下文 | `FtGnssContext.*` | FIX 新鲜度、坐标合法性和位置尾部构建 |
| LoRa/Meshtastic | `LoRaManager.*` | RF、加解密、ACK、去重、节点目录和收件箱 |
| PKI | `FtMeshPki.*` | X25519 身份、对端公钥、CCM 加解密与 NVS |
| 通信 UI | `UiLoRaProbe.*` | 收件箱、终端目录、编辑器和诊断页 |
| 录音 | `AudioLogger.*`, `UiRecorder.*`, `UiLog.*` | WAV 采集、保存、回放和日志页面 |
| 音频存储 | `FtAudioStore.*` | 安全删除 WAV 与重写 `index.jsonl` |
| 任务 | `TaskManager.*`, `UiTasks.*` | 任务存储、列表、详情和状态报告 |
| 同步 | `SyncManager.*`, `UiSync.*` | USB CDC 协议、上传清单、任务下行和清理 |
| 导航 | `UiNavigation.*` | GNSS 状态和路径记录页面 |
| 帮助 | `HelpManager.*` | 各页面按键帮助 |

## 通信数据流

```text
SX1262 DIO1 IRQ
→ LoRaManager::pollReceive()
→ Mesh 头解析与去重
→ 频道 AES 解密或 PKI CCM 解密
→ NodeInfo / ACK / 正文分类
→ 50 条环形收件箱
→ messageRevision + unreadMessageCounter
→ 当前页面按需刷新
```

## 消息与节点缓存

- 消息容量：50 条，固定环形缓冲区，第 51 条覆盖最旧记录。
- 节点目录：最多 64 个节点。
- 名称优先级：`short_name → long_name → 8 位节点编号`。
- 节点名称与公钥保存到 NVS。
- 消息正文当前不持久化，重启后清空。

## 共享资源原则

- LoRa 与 SD 使用共享 SPI 管理，不允许页面直接抢占总线。
- 正常收包采用 IRQ + 非阻塞轮询，不使用秒级 `receive()` 等待。
- USB 同步阶段保持串口协议安静，避免 GNSS、按键和 SPI 调试输出污染协议。
- 大型词库和中文字库未来应放 microSD，固件仅保存输入法引擎和小缓存。

## 结构性约束

- 不恢复固定测试消息或旧 LBX1 兼容发送接口。
- 不在多个页面复制 GNSS 判断、JSON 解析、时间转换或音频删除逻辑。
- 新增全局能力时优先放入后台服务，页面只负责交互和显示。
- 对新的跨模块函数必须显式声明，并通过完整对象链接检查。
