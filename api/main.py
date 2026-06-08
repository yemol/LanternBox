# -----------------------------
# main.py：精简入口，导入 api.app
# app.py：FastAPI 应用实例、静态挂载、启动事件
# config.py：路径、常量、资源缓存、TTS 设置
# models.py：Pydantic 请求/响应模型
# utils.py：通用辅助函数
# resources.py：本地资源加载、触发规则与指南匹配
# ai.py：Ollama 调用、消息构建与回退策略
# db.py：SQLite 连接与初始化
# tts.py：TTS 生成与输出清理
# routes.py：所有 HTTP 路由定义
# -----------------------------


from .app import app

if __name__ == '__main__':
    import uvicorn

    uvicorn.run('api.main:app', host='127.0.0.1', port=8000, reload=True)
