#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def req(cond, msg):
    if not cond:
        raise SystemExit("FAIL: " + msg)

readme = (ROOT / "README.md").read_text(encoding="utf-8")
arch = (ROOT / "ARCHITECTURE.md").read_text(encoding="utf-8")
proto = (ROOT / "src/FT02_MeshVoiceProtocol.h").read_text(encoding="utf-8")
build = (ROOT / "src/FT02_BuildInfo.h").read_text(encoding="utf-8")

req((ROOT / ".gitignore").exists(), ".gitignore missing")
req("Mesh Messaging A1.5.15e" in readme, "README baseline stale")
req("Opus VOIP CBR：**6.0 kbit/s**" in readme, "README current Opus rate missing")
req("安全分片数据：**192 B**" in readme, "README current chunk size missing")
req('FT02_MESH_MESSAGING_VERSION "A1.5.15e"' in build, "BuildInfo mesh branch label stale")
req("FT02_VOICE_TX_CODEC = FT02_VOICE_CODEC_OPUS_6000" in proto, "current TX codec changed")
req("FT02_VOICE_CHUNK_BYTES = 192u" in proto, "current voice chunk changed")
req("Current RF/UI boundary: voice CHUNK data is 192 B" in arch, "architecture voice boundary stale")
req("current Opus-4800 TX" not in (ROOT / "src/FT02_MeshVoiceTransport.cpp").read_text(encoding="utf-8"), "stale Opus-4800 current-TX comment")

for stale in [
    "New voice messages: Opus VOIP 4.8 kbit/s",
    "current voice fragments carry 204 payload bytes",
    "A1.5.11 default TX: Opus VOIP 4.8 kbps",
]:
    req(stale not in readme and stale not in arch and stale not in proto, "stale current-state wording: " + stale)

# Common non-source residues are not allowed in a submission archive.
prohibited_names = {".DS_Store"}
prohibited_suffixes = {".bak", ".orig", ".rej", ".tmp", ".pyc"}
for p in ROOT.rglob("*"):
    if p.is_dir():
        if p.name in {".pio", "__pycache__", ".pytest_cache"}:
            raise SystemExit("FAIL: generated directory present: " + str(p.relative_to(ROOT)))
        continue
    if p.name in prohibited_names or p.suffix in prohibited_suffixes or p.name.endswith("~"):
        raise SystemExit("FAIL: temporary file present: " + str(p.relative_to(ROOT)))

print("PASS submission hygiene: current docs/comments aligned; no generated/backup residue")
