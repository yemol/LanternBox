#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

def text(name):
    return (SRC / name).read_text(encoding="utf-8", errors="strict")

def require(cond, msg):
    if not cond:
        raise SystemExit(f"FAIL: {msg}")

# A5 smart engine must exist and be wired into the public IME path.
require((SRC / "FT02_PinyinEngineAdapter.cpp").exists(), "A5 engine adapter cpp missing")
require((SRC / "FT02_PinyinEngineAdapter.h").exists(), "A5 engine adapter header missing")
ime = text("FT02_PinyinIme.cpp")
require('#include "FT02_PinyinEngineAdapter.h"' in ime, "PinyinIme not wired to A5 adapter")
for token in ["smartPrefixCandidateCount", "FT02_PINYIN_A5_MAX_CANDIDATES", "rankedSmartCandidate"]:
    require(token in ime, f"A5 smart IME feature missing: {token}")
adapter = text("FT02_PinyinEngineAdapter.h")
require('/lanternbox/input/ime/ft02_pinyin_words.idx' in adapter, "A5 index path drift")
require('/lanternbox/input/ime/ft02_pinyin_words.dat' in adapter, "A5 data path drift")
require('FT02_PINYIN_A5_MAX_CANDIDATES = 64' in adapter, "A5 candidate cache size drift")

# Direct toggle semantic key must be frozen at 0xF1.
ih = text("FT02_InputManager.h")
ic = text("FT02_InputManager.cpp")
require("FT02_KEY_IME_TOGGLE" in ih, "IME semantic key missing")
require("code == 0xF1" in ic and "FT02_KEY_IME_TOGGLE" in ic, "0xF1 direct toggle mapping missing")
require('case FT02_KEY_IME_TOGGLE: return "IME_TOGGLE";' in ic, "IME toggle diagnostics missing")

# All text-entry surfaces must consume the semantic key, never printable backtick.
pages = [
    "FT02_CommunicationNodeUI.cpp",
    "FT02_FieldLibrary.cpp",
    "FT02_KnowledgeUI.cpp",
    "FT02_MapSearch.cpp",
]
for name in pages:
    s = text(name)
    require("FT02_KEY_IME_TOGGLE" in s, f"{name} does not handle semantic IME toggle")
    require("static_cast<uint8_t>('`')" not in s and "raw == '`'" not in s,
            f"{name} still steals printable backtick for IME switching")

# User learning must remain part of the engine path.
require((SRC / "FT02_PinyinLearning.cpp").exists(), "Pinyin learning implementation missing")
require("FT02_PinyinLearningHasData" in ime, "Pinyin learning rank path missing")
for name in pages:
    require("FT02_PinyinLearningRecord" in text(name), f"{name} no longer records user selections")

print("PASS: Pinyin A5 smart engine + direct long-SYM toggle frozen")
