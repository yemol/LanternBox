# 录音、任务、存储与同步

## SD 目录

```text
/lanternbox/
├── base.json
├── audio/
│   ├── audio_###.wav
│   └── index.jsonl
├── logs/
│   ├── boot.jsonl
│   └── field_events.jsonl
├── sessions/
├── tasks/
│   ├── tasks.jsonl
│   └── task_reports.jsonl
└── tracks/
    └── path_points.jsonl
```

## 录音与日志

稳定能力：

- 录制 WAV。
- 保存录音并更新 `/lanternbox/audio/index.jsonl`。
- 在日志页选择、播放和停止录音。
- 调整回放增益。
- 删除选中录音，并同步重写索引。
- 重建列表时过滤不存在的 WAV 索引。

`AudioLogger.*` 负责采集和 WAV 写入，`UiLog.*` 负责列表与回放交互，`FtAudioStore.*` 负责安全删除和索引重写。

音频删除只接受安全的 `audio_*.wav` 文件名，拒绝路径穿越、目录路径和非录音文件。

## 任务

- Core 下发任务保存到 `/lanternbox/tasks/tasks.jsonl`。
- FT01 读取任务列表与详情。
- 现场状态/完成报告写入 `task_reports.jsonl`。
- 任务保存完成后才向 Core 返回成功 ACK。

## USB 同步

任务下行已经从突发式发送演进为可恢复协议：

```text
PUT_TASKS_BEGIN <count>
FT01_SYNC_TASKS_BEGIN_ACK ...
<task json line>
FT01_SYNC_TASK_LINE_ACK index=...
...
PUT_TASKS_END
FT01_SYNC_TASKS_ACK ... stage=saved
```

核心原则：

- 每行任务等待 FT01 ACK 后再发送下一行。
- 接收期间不刷新屏幕、不进行 SD 写入，避免 USB CDC 丢行。
- `PUT_TASKS_END` 后保存、重新加载，再返回 `stage=saved`。
- 超时、过长行或新命令可中止残留接收状态，无需重启。
- USB 同步时串口保持协议干净。

## 同步后的本地清理

Core 明确确认上传完成后，FT01 可清理：

- `path_points.jsonl`
- `field_events.jsonl`
- `task_reports.jsonl`
- boot 日志仅保留最近 20 行

不会自动清除：

- `tasks.jsonl`
- 未确认上传的 WAV
- `base.json`
- `sessions/`

音频采用单独的 `DELETE_UPLOADED_AUDIO <filename>` 流程。只有 Core 确认对应 WAV 已上传后才删除文件和索引，避免仅清空 `audio/index.jsonl` 导致孤儿文件或数据丢失。

## 共享 SPI 注意事项

SD 与 LoRa 共享硬件资源管理。后台无线接收虽然非阻塞，但录音写 SD 时仍须实机验证：

- WAV 是否连续。
- 收包是否丢失。
- ACK 是否正常。
- 是否出现 SPI 冲突或音频断裂。
