# LanternBox / 壳中灯

LanternBox 的目标不是帮助使用者度过一天的危机，而是在长期失去外部支持的环境下，保存知识、积累经验、辅助决策，并帮助个体和社区维持生存能力与文明延续能力。

真正的核心目标：

> 让知识不会消失，让经验能够积累，让人遇到问题时始终还有一个可靠的参谋。📚🧭🧠🌱

---

## 目录结构

```text
LanternBox/
├─ app/                # 前端页面，壳中灯的操作界面
├─ api/                # Core 后端服务
├─ data/               # SQLite 数据库、配置、日志
├─ knowledge/          # 离线知识库，Markdown / txt
├─ manuals/            # PDF 手册、图片资料、维修资料
├─ maps/               # 离线地图资料，后续接入
├─ ollama_models/      # 本地模型配置或说明
├─ pocketbase/         # PocketBase 本地 Wiki 数据库
│  ├─ pocketbase       # PocketBase 可执行文件
│  └─ pb_data/         # PocketBase 数据目录，迁移和备份时必须保留
├─ backups/            # 备份
├─ scripts/            # 启动、备份、检查脚本
├─ voice_service/      # 独立语音模块，当前用于 TTS，后续可替换为硬件语音服务
└─ README.md           # 项目说明
```

---

## 启动服务

建议按下面顺序启动：

### 1. 启动 PocketBase

```bash
cd pocketbase
./pocketbase serve --http=127.0.0.1:8090
```

PocketBase 管理后台：

```text
http://127.0.0.1:8090/_/
```

PocketBase API 地址：

```text
http://127.0.0.1:8090/api/
```

### 2. 启动 Voice Service

```bash
source venv/bin/activate
uvicorn voice_service.main:app --reload --host 127.0.0.1 --port 8790
```

### 3. 启动 Core 后端

```bash
source venv/bin/activate
uvicorn api.main:app --reload --host 127.0.0.1 --port 8787
```

---

## PocketBase 本地 Wiki 数据库

LanternBox 使用 PocketBase 作为本地 Wiki / 精选知识库的数据底座之一。Core 后端会读取 PocketBase 中的 Wiki 条目，用于本地知识检索、AI 引用卡片和问答增强。

### 默认服务地址

Core 默认连接本地 PocketBase：

```text
http://127.0.0.1:8090
```

如需迁移到其他地址，可在后续通过环境变量或配置项调整。

### 数据目录

PocketBase 的核心数据目录是：

```text
pocketbase/pb_data/
```

这个目录包含本地 Wiki 数据和 PocketBase 数据库文件。迁移项目、备份项目或换机器时，必须一起复制该目录。

如果只复制代码而不复制 `pocketbase/pb_data/`，Core 后端仍然可以启动，但 Wiki 数据会丢失。

### 使用提醒

- 不要随意删除 `pocketbase/pb_data/`。
- 做大规模 Wiki 导入前，建议先备份 `pocketbase/pb_data/`。
- 后续 Kiwix / ZIM 大库接入后，PocketBase 仍作为精选知识库和高价值条目索引使用。
- PocketBase 更适合“小而准”的精选知识；Kiwix / ZIM 更适合“大而全”的离线知识底座。

---

## Voice Service 语音模块

v0.7.2 起，主系统不再直接绑定 Piper / MeloTTS。`/api/tts/speak` 会代理到独立的 `voice_service`。

当前 `voice_service` 使用 Piper 作为 TTS 引擎。后续如迁移到外部语音硬件，只需调整语音服务地址，例如：

```text
LANTERNBOX_VOICE_SERVICE_URL=http://<voice-service-host>:8790
```

主系统依赖只保留代理所需模块。Piper 模型与 `piper-tts` 依赖应放在 `voice_service` 中维护。

---

## 本地知识体系规划

当前知识体系分为三层：

```text
1. 应急指南库
   小、快、结构化，适合紧急状态下直接给出处置步骤。

2. PocketBase 精选 Wiki
   人工筛选后的高价值条目，适合 AI 快速引用、首页展示和引用卡片。

3. Kiwix / ZIM 大型离线知识库
   最大容量知识底座，后续覆盖百科、医学、农业、维修、工具、食品、自然科学等广域资料。
```

后续还会接入离线地图能力，用于本地地图浏览、地点标注、轨迹记录、资源点和风险点管理。

---

## 数据备份提醒

迁移或备份 LanternBox 时，至少需要关注：

```text
pocketbase/pb_data/      # PocketBase Wiki 数据
voice_service/models/    # Piper 等语音模型
voice_service/output/    # 语音输出，可按需清理
data/                    # Core 本地数据、配置、日志
knowledge/               # Markdown / txt 离线知识库
manuals/                 # PDF、图片、维修资料
maps/                    # 离线地图资料
backups/                 # 历史备份
```

其中 `pocketbase/pb_data/` 和 `data/` 属于重点数据资产，清理时要格外小心。
