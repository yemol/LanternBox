# LanternBox FT-01 固件说明

当前固件版本：`v0.4.4c-audio-cleanup-guard`

本文档是 FT-01 固件的主说明。原先分散的音频、导航、同步、任务和本地清理补丁笔记已压缩合并到这里；FT-01 目录不再保留零散的 `*_NOTES.md` 过程文件。

## 当前能力

- 录音日志：录制 WAV、保存、播放、停止、音量增益、列表刷新、删除录音，并维护 `/lanternbox/audio/index.jsonl`。
- 路径与导航：路径记录、轨迹列表、轨迹点浏览、地图页、指南针导航、基地指向和帮助页。
- 设备页：显示版本、运行时间、时间、电源、SD、GNSS、坐标和主要模块状态。
- 同步页：USB 同步入口、同步清单输出、JSONL 只读导出、WAV 只读导出、任务下发、任务回报上传和 Core 确认后的本地清理。
- 任务页：本地任务收件箱、任务详情、状态更新、任务回报写入和任务列表刷新。

暂未实现：Wi-Fi 同步、BLE 同步、端侧主动 HTTP 上传、端侧加密同步。

## 稳定边界

音频模块以 `v0.3.0-audio-log-final` 为稳定基线：

- `AudioLogger.*` 负责麦克风录音、WAV 写入、播放、增益、音频列表、索引维护、删除和僵尸索引清理。
- `UiLog.*` 负责日志页绘制、按键、删除确认、帮助入口和加载/刷新界面。
- 不要恢复整段音频进 RAM 的实现。
- 不要在启动时扫描 `audio_001.wav` 到 `audio_999.wav`。
- WAV 缺失时不能继续显示旧索引。
- 不要把 WAV 读写细节塞回 UI 代码。
- 不要给所有 SD 调用套激进的全局 SD Guard。

导航模块以 `v0.3.1` 系列为稳定 UI 基线：

- 轨迹列表和轨迹点列表采用 3 行卡片布局。
- 顶部状态栏显示标题、SD、GNSS 和时间。
- `H` 可从列表、总览、地图和指南针页面打开帮助。
- 不改动轨迹文件格式、GNSS 解析、基地点逻辑、地图绘制算法和方位角计算。

同步和任务模块的安全边界：

- `GET_MANIFEST`、`GET_RECORDS`、`GET_AUDIO_FILE` 都是只读导出。
- `CLEAR_SYNCED_RECORDS` 只清理 Core 已确认上传的 JSONL 缓冲。
- `DELETE_UPLOADED_AUDIO` 只在 Core 确认音频上传成功后删除指定 WAV，并同步移除索引行。
- 不通过清理命令删除 `/lanternbox/tasks/tasks.jsonl`。
- 不通过 `CLEAR_SYNCED_RECORDS` 清理 `/lanternbox/audio/index.jsonl`。
- 不删除未指定的 WAV 文件。

## 本地数据路径

```text
/lanternbox/audio/audio_###.wav
/lanternbox/audio/index.jsonl
/lanternbox/tracks/path_points.jsonl
/lanternbox/logs/field_events.jsonl
/lanternbox/logs/boot.jsonl
/lanternbox/tasks/tasks.jsonl
/lanternbox/tasks/task_reports.jsonl
/lanternbox/base.json
/lanternbox/sessions/
```

`tasks.jsonl` 是 Core 下发给 FT-01 的任务快照。`task_reports.jsonl` 保存 FT-01 本地任务状态变更，下一次同步时上传给 Core。

## 首页布局

首页第 1 页：

```text
路径 / LOG
日志 / LOGS
导航 / NAV
任务 / TASK
```

首页第 2 页：

```text
设置 / SET
同步 / SYNC
设备 / DEV
关于 / INFO
```

`TASK` 打开任务收件箱；`SYNC` 进入同步方式选择页；旧的 `STAT` 不再作为首页入口展示。

## 日志页按键

```text
< / >      选择录音
P / Enter  播放或停止播放
R          开始录音
S          停止并保存
A          刷新列表
B / DEL    删除选中录音，二次确认
U / D      调整回听音量
H          帮助
ESC        返回首页
```

日志列表显示日期、时间和时长。删除录音时会同时处理 WAV 文件和 `/lanternbox/audio/index.jsonl`；刷新列表时会过滤 WAV 已不存在的旧索引。

## 导航页按键

```text
< / >      选择
Enter      打开地图或进入导航
R          刷新
L          返回轨迹列表
M          地图
N          指南针导航
B          指向基地
H          帮助
ESC        返回首页
```

## 任务页按键

```text
< / > 或 Up / Down  选择任务
Enter               打开或关闭详情
S                   标记为 in_progress
D                   标记为 completed
B                   标记为 blocked
R                   重新读取本地任务文件
ESC / DEL           返回
```

任务详情页支持中文 UTF-8 换行。长描述使用 `< / >` 翻页；描述不超过一页时隐藏页码提示。任务详情隐藏 `task_id`，优先显示状态、优先级、版本号、标题和描述。

Core 下发任务建议包含：

```text
task_id
title
description
status
priority
revision
```

第一阶段不要下发过重字段，例如 `assigned_to`、`target`、`tags`，除非下发协议已经确认能稳定承载更大的结构化内容。

## USB 同步协议

### 握手与同步清单

同步页按 `U` 输出握手信息；按 `M` 刷新统计并输出同步清单。同步清单位于下面两行之间：

```text
FT01_SYNC_MANIFEST_BEGIN
{...json...}
FT01_SYNC_MANIFEST_END
```

同步清单中包含设备信息、固件版本、待同步 JSONL 数量、录音索引数量、录音文件大小和任务回报数量。同步清单输出期间抑制 GNSS/SPI 调试串口噪声，避免 Core 解析失败。

### JSONL 只读导出

支持命令：

```text
GET_RECORDS path_points
GET_RECORDS field_events
GET_RECORDS boot_logs
GET_RECORDS audio_index
GET_RECORDS task_reports
```

输出格式：

```text
FT01_SYNC_RECORDS_BEGIN <record_type>
{...json line...}
FT01_SYNC_RECORDS_END <record_type>
```

这些命令只读 SD，不清理、不删除、不截断、不旋转文件。

### WAV 只读导出

支持命令：

```text
GET_AUDIO_FILE audio_001.wav
```

输出格式：

```text
FT01_SYNC_AUDIO_BEGIN audio_001.wav <size>
<base64 chunks>
FT01_SYNC_AUDIO_END audio_001.wav <size>
```

实现要点：

- WAV base64 原始块大小为 192 字节。
- 每个块发送后调用 `Serial.flush()` 并保持轻微节奏。
- 按已知文件大小流式传输，不依赖 `File.available()` 判断结束。
- 如果 SD 无法读完整文件，输出 `FT01_SYNC_AUDIO_ERROR short_read`。
- 该协议只读，不删除 WAV，也不修改音频索引。

### 任务下发

当前任务下发使用逐行 ACK，Core 必须等待每行 ACK 后再发送下一行 JSONL：

```text
PUT_TASKS_BEGIN <count>
FT01_SYNC_TASKS_BEGIN_ACK expected=<count> ok=true protocol=line_ack

{task json line 1}
FT01_SYNC_TASK_LINE_ACK index=1 ok=true

{task json line 2}
FT01_SYNC_TASK_LINE_ACK index=2 ok=true

PUT_TASKS_END
FT01_SYNC_TASKS_ACK received=<n> stored=<n> ok=true stage=saved
FT01_SYNC_TASKS_SAVE_DONE received=<n> stored=<n> ok=true
```

任务接收期间不逐行写 SD、不刷新电子纸，避免 USB CDC 收包被阻塞。收到 `PUT_TASKS_END` 后，FT-01 写入并重新读取 `/lanternbox/tasks/tasks.jsonl`，成功持久化后才返回 `stage=saved`。

异常恢复：

- 任务接收有看门狗超时。
- 新的顶层同步命令会中止过期任务接收。
- Core 可发送 `PUT_TASKS_ABORT` 主动清理半包状态。
- 明确错误原因包括 `timeout`、`new_command`、`host_abort`、`line_too_long`。

### Core 确认后的本地清理

支持命令：

```text
CLEAR_SYNCED_RECORDS path_points field_events task_reports boot_logs
```

行为：

- 清空 `/lanternbox/tracks/path_points.jsonl`
- 清空 `/lanternbox/logs/field_events.jsonl`
- 清空 `/lanternbox/tasks/task_reports.jsonl`
- `/lanternbox/logs/boot.jsonl` 只保留最新 20 行

不会清理：

- `/lanternbox/tasks/tasks.jsonl`
- `/lanternbox/audio/index.jsonl`
- `/lanternbox/audio/*.wav`
- `/lanternbox/base.json`
- `/lanternbox/sessions/`

如果把 `audio_index` 放进 `CLEAR_SYNCED_RECORDS`，FT-01 返回 `error=audio_index_cleanup_disabled`。

### Core 确认后的音频删除

支持命令：

```text
DELETE_UPLOADED_AUDIO audio_001.wav
```

行为：

- 只接受类似 `audio_001.wav` 的基础文件名。
- 拒绝斜杠、反斜杠、`..`、非 WAV、隐藏文件和资源叉文件。
- 删除匹配的 `/lanternbox/audio/audio_001.wav`。
- 重写 `/lanternbox/audio/index.jsonl`，移除对应索引行。

响应示例：

```text
FT01_SYNC_AUDIO_DELETE_ACK file=audio_001.wav wav_deleted=true index_removed=true index_removed_count=1 ok=true
```

## 压缩版版本历史

- `v0.2.5` 到 `v0.2.5f`：关闭原始 NMEA 刷屏，恢复 SD 启动稳定，收敛 AudioLogger 内部 SD 总线保护，修正日志/导航底部栏位置。
- `v0.2.6` 到 `v0.2.6g`：日志列表改为缓存读取，减少 SD 访问卡顿；优化日期、时间、时长、GNSS 状态、页码和可读性。
- `v0.2.7` 到 `v0.2.9l`：补齐日志删除、回车播放、帮助页、索引同步、僵尸索引过滤和仪表盘布局。
- `v0.3.0`：冻结音频日志模块，明确 `AudioLogger.*` 与 `UiLog.*` 职责边界。
- `v0.3.1` 到 `v0.3.1f`：导航页对齐日志仪表盘风格，补齐帮助页，调整地图/指南针入口，并交换首页导航与状态卡片。
- `v0.3.2` 到 `v0.3.2a`：新增设备页，补齐版本、运行时间、存储、GNSS、模块状态和编译前置声明。
- `v0.3.3`：完成一次低风险清理审计，移除少量无用导航状态，确认后续优先拆分设备页和状态服务。
- `v0.4.0` 到 `v0.4.0f`：新增同步 UI 和 `SyncManager` 骨架；同步清单输出；同步方式选择；抑制同步页 GNSS/SPI 串口噪声。
- `v0.4.1` 到 `v0.4.1d`：新增 JSONL 只读导出；同步入口延迟扫描 SD；稳定音频 ID；清理同步清单中不可靠或干扰解析的输出。
- `v0.4.2` 到 `v0.4.2a`：新增 WAV 只读导出，并通过更小块、flush、节奏控制和按文件大小读取提升真实设备稳定性。
- `v0.4.2b`：首页加入任务入口，为任务同步做布局准备。
- `v0.4.3` 到 `v0.4.3l`：实现本地任务收件箱、任务详情和任务状态回报；任务下发从突发发送改为逐行 ACK；任务保存改为成功持久化后 ACK；增加超时/中止恢复；优化中文详情显示和任务列表空行。
- `v0.4.4` 到 `v0.4.4c`：新增 Core 确认后的本地清理；撤回通过清理命令清空音频索引的设计；新增 `DELETE_UPLOADED_AUDIO`，确保 WAV 和音频索引配对删除。

## 后续维护建议

- 保持 FT-01 目录只放主 README 和源码文件；临时排障记录优先合并进本文档的压缩版历史。
- 下一轮安全拆分优先考虑 `UiDevice.*`，把设备页 UI 从 `LanternBox_FT01.ino` 移出。
- 再下一步可考虑 `DeviceStatus.*`，集中 SD、GNSS、电源和运行状态辅助函数。
- 拆分前必须避免触碰已冻结的音频录放链路。
