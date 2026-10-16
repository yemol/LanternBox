#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
audio = (root / "src/FT02_AudioLog.cpp").read_text(encoding="utf-8")
mesh = (root / "src/FT02_MeshVoiceAudio.cpp").read_text(encoding="utf-8")

checks = []

def check(name, ok):
    checks.append((name, bool(ok)))

# Producer/consumer mailbox must be protected by one cross-core mux.
check("transient_mux_declared", "g_ft02AudioTransientMux = portMUX_INITIALIZER_UNLOCKED" in audio)
check("transient_snapshot_locked",
      re.search(r"FT02_AudioLogTransientResultCurrent\(\).*?portENTER_CRITICAL\(&g_ft02AudioTransientMux\).*?snapshot\s*=\s*g_ft02AudioTransientResult.*?portEXIT_CRITICAL\(&g_ft02AudioTransientMux\)",
                audio, re.S) is not None)
check("transient_clear_locked",
      re.search(r"FT02_AudioClearTransientResultLocked\(\).*?portENTER_CRITICAL\(&g_ft02AudioTransientMux\).*?memset\(&g_ft02AudioTransientResult.*?portEXIT_CRITICAL\(&g_ft02AudioTransientMux\)",
                audio, re.S) is not None)

# In the transient finish branch, session/state cleanup must precede publication.
finish = re.search(r"if\(g_ft02AudioRecording\.transient\)\s*\{(.*?)\n\s*return;\n\s*\}", audio, re.S)
if finish:
    block = finish.group(1)
    pos_reset = block.find("g_ft02AudioRecording = FT02AudioRecordingSession{}")
    pos_idle = block.find("g_ft02AudioState = FT02_AUDIO_LOG_IDLE")
    pos_publish = block.find("FT02_AudioPublishTransientResult")
    check("cleanup_before_publish", min(pos_reset, pos_idle, pos_publish) >= 0 and pos_reset < pos_publish and pos_idle < pos_publish)
else:
    check("cleanup_before_publish", False)

# Mesh side must defer Codec2 until a later poll turn after owner release.
check("encode_defer_state", "g_encodeNotBeforeMs = millis() + 300u" in mesh)
check("encode_defer_return",
      re.search(r"g_encodeNotBeforeMs\s*=\s*millis\(\)\s*\+\s*300u;.*?return;", mesh, re.S) is not None)
check("encode_later_branch",
      re.search(r"if\(g_status\.state == FT02_MESH_VOICE_AUDIO_ENCODING\).*?millis\(\) - g_encodeNotBeforeMs.*?FT02_MeshVoiceEncodeWavToLbv",
                mesh, re.S) is not None)

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(("PASS " if ok else "FAIL ") + name)
if failed:
    raise SystemExit("Mesh voice stop handoff guard failed: " + ", ".join(failed))
print("PASS Mesh voice stop handoff atomic/deferred-encode guard")
