LanternBox/
├─ app/                # 前端页面，壳中灯的操作界面
├─ api/                # 后端服务
├─ data/               # SQLite 数据库、配置、日志
├─ knowledge/          # 离线知识库，Markdown / txt
├─ manuals/            # PDF 手册、图片资料、维修资料
├─ maps/               # 离线地图资料，后面再加
├─ ollama_models/      # 本地模型配置或说明
├─ backups/            # 备份
├─ scripts/            # 启动、备份、检查脚本
├─ voice_service/      # 单独分离出来的语音模块，用来实现语音识别和TTS语音发声（之后考虑使用硬件替代）
└─ README.md           # 项目说明

启动服务的命令：
core服务
source venv/bin/activate    （进入虚拟环境）
uvicorn api.main:app --reload --host 127.0.0.1 --port 8787
pocketbase
./pocketbase serve --http=127.0.0.1:8090

voice service
source venv/bin/activate
uvicorn voice_service.main:app --reload --host 127.0.0.1 --port 8790


基本方案：
LanternBox 的目标不是帮助使用者度过一天的危机，而是在长期失去外部支持的环境下，保存知识、积累经验、辅助决策，并帮助个体和社区维持生存能力与文明延续能力。

真正的核心目标其实是：
让知识不会消失，让经验能够积累，让人遇到问题时始终还有一个可靠的参谋。 📚🧭🧠🌱

语音模块说明：
v0.7.2 起，主系统不再直接绑定 Piper / MeloTTS。`/api/tts/speak` 会代理到独立的 `voice_service`。
当前 voice_service 使用 Piper 作为 TTS 引擎；后续可迁移到外部语音硬件，只需调整环境变量：
`LANTERNBOX_VOICE_SERVICE_URL=http://<voice-service-host>:8790`

主系统依赖只保留代理所需模块。Piper 模型与 piper-tts 依赖应放在 voice_service 中维护。
