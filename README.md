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
└─ README.md           # 项目说明

启动服务的命令：
uvicorn api.main:app --reload --host 127.0.0.1 --port 8787