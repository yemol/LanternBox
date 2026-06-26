# Deployment Guide

## Core

运行环境：

-   macOS
-   Python
-   FastAPI
-   PocketBase

启动：

``` bash
source venv/bin/activate
uvicorn api.main:app --reload --port 8787
```

## AI Node

推荐：

-   Windows 11
-   NVIDIA RTX GPU
-   Ollama

环境变量：

``` text
OLLAMA_HOST=0.0.0.0:11434
```

启动：

``` bash
ollama serve
```

## Core 配置

``` env
OLLAMA_URL=http://AI_NODE_IP:11434
```

## 推荐启动顺序

1.  AI Node
2.  PocketBase
3.  Voice Service
4.  Core

## 后续扩展

-   Whisper
-   Piper
-   OCR
-   Vision
-   Embedding
-   Reranker
