# LanternBox Architecture

## 设计目标

Core 专注业务逻辑，AI Node 专注模型推理，两者通过 HTTP 通讯。

## 模块

### Core

-   FastAPI
-   PocketBase
-   RAG
-   Web UI
-   数据管理

### AI Node

-   Ollama
-   Whisper（规划）
-   Piper（规划）
-   OCR（规划）
-   Vision（规划）

### 终端

-   Field Terminal
-   Study Terminal
-   Sensor Node

## 数据流

用户 → Core → AI Node → Core → 用户

知识检索：

PocketBase → RAG → AI → 回答

## 演进路线

v0.5：Core 内置 AI

v0.8：Core + AI Node

v1.0：多终端协同
