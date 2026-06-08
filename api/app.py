from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

from .config import APP_DIR, TTS_OUTPUT_DIR
from .db import init_db
from .resources import load_local_resources
from .routes import router

app = FastAPI(title="LanternBox", version="0.4.0")
app.mount("/static", StaticFiles(directory=APP_DIR), name="static")
app.mount("/tts_output", StaticFiles(directory=TTS_OUTPUT_DIR), name="tts_output")
app.include_router(router)


@app.on_event("startup")
def on_startup():
    init_db()
    load_local_resources()
