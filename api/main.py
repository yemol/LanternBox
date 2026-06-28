"""后端启动入口。向 ASGI 服务器暴露 FastAPI app。"""

from .app import app

if __name__ == '__main__':
    import uvicorn

    uvicorn.run('api.main:app', host='127.0.0.1', port=8787, reload=True)
