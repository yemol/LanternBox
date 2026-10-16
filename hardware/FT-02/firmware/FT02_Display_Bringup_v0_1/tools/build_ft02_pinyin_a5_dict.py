#!/usr/bin/env python3
"""Build FT-02 Pinyin IME A5 SD dictionary.

The runtime format is deliberately tiny and dependency-free.  Source inputs are
normal text/JSON dictionaries; this script converts them on a PC/Mac into:

  /lanternbox/input/ime/ft02_pinyin_words.idx
  /lanternbox/input/ime/ft02_pinyin_words.dat

Supported text rows (TAB preferred):
  pinyin<TAB>词语<TAB>frequency
  词语<TAB>pinyin<TAB>frequency

Pinyin may contain spaces/apostrophes/tone marks/tone digits; it is normalized
to continuous lowercase tone-less Pinyin. ü is normalized to v.

JSON support intentionally accepts common shapes instead of tying FT-02 to one
upstream library:
  {"pinyin": ["候选1", "候选2"]}
  {"词语": "pin yin"}
  [{"word":"词语", "pinyin":"ci yu", "frequency":123}, ...]

FT-02's existing domain phrase table is injected at very high priority by
default, so upgrading the general dictionary does not discard field/emergency
vocabulary already tuned for the device.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import unicodedata
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

FORMAT_VERSION = 1
INDEX_MAGIC = b"FT2PYI1\0"
DATA_MAGIC = b"FT2PYD1\0"
HEADER_FMT = "<8sHHIIIII"       # 32 bytes
INDEX_RECORD_FMT = "<25sBHII"   # 36 bytes
HEADER_SIZE = struct.calcsize(HEADER_FMT)
INDEX_RECORD_SIZE = struct.calcsize(INDEX_RECORD_FMT)
MAX_KEY = 24
MAX_UTF8 = 48
DEFAULT_MAX_PER_KEY = 64
DOMAIN_BASE_FREQ = 0xF0000000

PINYIN_RE = re.compile(r"^[a-zv]+$")
CPP_PHRASE_RE = re.compile(r'\{"([a-z]+)",\s*u8"([^"]*)"\}')

TONE_REPLACEMENTS = str.maketrans({
    "ü": "v", "Ü": "v",
    "ǖ": "v", "ǘ": "v", "ǚ": "v", "ǜ": "v",
})


def normalize_pinyin(value: str) -> str:
    value = value.strip().translate(TONE_REPLACEMENTS).lower()
    # Strip combining tone marks after canonical decomposition.
    value = "".join(
        c for c in unicodedata.normalize("NFD", value)
        if unicodedata.category(c) != "Mn"
    )
    value = value.replace("u:", "v")
    value = re.sub(r"[1-5]", "", value)
    value = re.sub(r"[\s'’\-_/.,]+", "", value)
    return value


def has_han(value: str) -> bool:
    return any("\u3400" <= c <= "\u9fff" for c in value)


def valid_candidate(word: str) -> bool:
    if not word or not has_han(word):
        return False
    if "\t" in word or "\r" in word or "\n" in word:
        return False
    return len(word.encode("utf-8")) <= MAX_UTF8


def parse_freq(value, default: int = 1) -> int:
    try:
        n = int(float(value))
        return max(1, min(n, 0xEFFFFFFF))
    except Exception:
        return default


@dataclass
class Stats:
    seen: int = 0
    accepted: int = 0
    rejected: int = 0
    domain_injected: int = 0


class Collector:
    def __init__(self) -> None:
        self.groups: dict[str, dict[str, int]] = defaultdict(dict)
        self.stats = Stats()

    def add(self, pinyin: str, word: str, frequency: int = 1, *, domain=False) -> None:
        self.stats.seen += 1
        key = normalize_pinyin(str(pinyin))
        word = str(word).strip()
        if not PINYIN_RE.fullmatch(key) or not (1 <= len(key) <= MAX_KEY) or not valid_candidate(word):
            self.stats.rejected += 1
            return
        freq = max(1, min(int(frequency), 0xFFFFFFFF))
        old = self.groups[key].get(word)
        if old is None or freq > old:
            self.groups[key][word] = freq
        self.stats.accepted += 1
        if domain:
            self.stats.domain_injected += 1


def parse_text(path: Path, collector: Collector, default_frequency: int) -> None:
    with path.open("r", encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t") if "\t" in line else re.split(r"\s+", line)
            if len(fields) < 2:
                collector.stats.rejected += 1
                continue

            a, b = fields[0].strip(), fields[1].strip()
            freq = parse_freq(fields[2], default_frequency) if len(fields) >= 3 else default_frequency
            a_py = normalize_pinyin(a)
            b_py = normalize_pinyin(b)
            if PINYIN_RE.fullmatch(a_py) and has_han(b):
                collector.add(a_py, b, freq)
            elif has_han(a) and PINYIN_RE.fullmatch(b_py):
                collector.add(b_py, a, freq)
            else:
                collector.stats.rejected += 1


def _json_walk(obj, collector: Collector, default_frequency: int) -> None:
    if isinstance(obj, list):
        for item in obj:
            _json_walk(item, collector, default_frequency)
        return

    if not isinstance(obj, dict):
        return

    # Common record-object shape.
    word = next((obj[k] for k in ("word", "phrase", "text", "hanzi", "candidate") if k in obj), None)
    py = next((obj[k] for k in ("pinyin", "py", "code") if k in obj), None)
    if isinstance(word, str) and isinstance(py, str):
        freq = next((obj[k] for k in ("frequency", "freq", "count", "weight") if k in obj), default_frequency)
        collector.add(py, word, parse_freq(freq, default_frequency))

    for key, value in obj.items():
        key_py = normalize_pinyin(str(key))
        if PINYIN_RE.fullmatch(key_py):
            if isinstance(value, str) and has_han(value):
                # Also accepts "词1|词2" compact candidate strings.
                for rank, candidate in enumerate(value.split("|")):
                    collector.add(key_py, candidate, max(1, default_frequency - rank))
                continue
            if isinstance(value, list):
                handled = False
                for rank, item in enumerate(value):
                    if isinstance(item, str) and has_han(item):
                        collector.add(key_py, item, max(1, default_frequency - rank))
                        handled = True
                if handled:
                    continue
        if has_han(str(key)) and isinstance(value, str):
            val_py = normalize_pinyin(value)
            if PINYIN_RE.fullmatch(val_py):
                collector.add(val_py, str(key), default_frequency)
                continue
        _json_walk(value, collector, default_frequency)


def parse_json(path: Path, collector: Collector, default_frequency: int) -> None:
    with path.open("r", encoding="utf-8-sig", errors="strict") as f:
        obj = json.load(f)
    _json_walk(obj, collector, default_frequency)


def inject_ft02_phrases(path: Path, collector: Collector) -> None:
    text = path.read_text(encoding="utf-8")
    rank = 0
    for match in CPP_PHRASE_RE.finditer(text):
        key, candidates = match.groups()
        for candidate in candidates.split("|"):
            collector.add(key, candidate, DOMAIN_BASE_FREQ - rank, domain=True)
            rank += 1


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def build(collector: Collector, out_dir: Path, max_per_key: int, source_labels: list[str]) -> dict:
    out_dir.mkdir(parents=True, exist_ok=True)
    idx_path = out_dir / "ft02_pinyin_words.idx"
    dat_path = out_dir / "ft02_pinyin_words.dat"

    keys = sorted(k for k, values in collector.groups.items() if values)
    groups: list[tuple[str, list[tuple[str, int]]]] = []
    for key in keys:
        candidates = sorted(collector.groups[key].items(), key=lambda kv: (-kv[1], kv[0]))
        groups.append((key, candidates[:max_per_key]))

    total_candidates = sum(len(c) for _, c in groups)
    data_offsets: list[int] = []

    with dat_path.open("wb") as dat:
        dat.write(struct.pack(
            HEADER_FMT, DATA_MAGIC, FORMAT_VERSION, HEADER_SIZE,
            len(groups), total_candidates, 0, 0, 0
        ))
        for _, candidates in groups:
            data_offsets.append(dat.tell())
            for word, frequency in candidates:
                raw = word.encode("utf-8")
                dat.write(struct.pack("<IB", frequency, len(raw)))
                dat.write(raw)

    with idx_path.open("wb") as idx:
        idx.write(struct.pack(
            HEADER_FMT, INDEX_MAGIC, FORMAT_VERSION, INDEX_RECORD_SIZE,
            len(groups), total_candidates, 0, 0, 0
        ))
        for (key, candidates), offset in zip(groups, data_offsets):
            key_raw = key.encode("ascii")
            key_field = key_raw + b"\0" * (25 - len(key_raw))
            idx.write(struct.pack(
                INDEX_RECORD_FMT, key_field, len(key_raw), len(candidates), offset, 0
            ))

    manifest = {
        "format": "FT02_PINYIN_A5_V1",
        "version": FORMAT_VERSION,
        "groups": len(groups),
        "candidates": total_candidates,
        "max_candidates_per_key": max_per_key,
        "source_labels": source_labels,
        "collector": collector.stats.__dict__,
        "files": {
            idx_path.name: {"bytes": idx_path.stat().st_size, "sha256": sha256(idx_path)},
            dat_path.name: {"bytes": dat_path.stat().st_size, "sha256": sha256(dat_path)},
        },
    }
    (out_dir / "ft02_pinyin_words.manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    return manifest


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--text", action="append", default=[], help="text/TSV dictionary source; repeatable")
    ap.add_argument("--json", action="append", default=[], help="JSON dictionary source; repeatable")
    ap.add_argument("--out-dir", required=True, help="output directory corresponding to /lanternbox/input/ime")
    ap.add_argument("--max-per-key", type=int, default=DEFAULT_MAX_PER_KEY)
    ap.add_argument("--default-frequency", type=int, default=1000)
    ap.add_argument("--ft02-phrases", default=None, help="FT02_PinyinPhraseData.h to inject at top priority")
    ap.add_argument("--no-ft02-phrases", action="store_true")
    args = ap.parse_args()

    if not (1 <= args.max_per_key <= DEFAULT_MAX_PER_KEY):
        ap.error(f"--max-per-key must be 1..{DEFAULT_MAX_PER_KEY}")

    collector = Collector()
    labels: list[str] = []
    for raw in args.text:
        path = Path(raw)
        parse_text(path, collector, args.default_frequency)
        labels.append(f"text:{path.name}")
    for raw in args.json:
        path = Path(raw)
        parse_json(path, collector, args.default_frequency)
        labels.append(f"json:{path.name}")

    if not args.no_ft02_phrases:
        phrase_path = Path(args.ft02_phrases) if args.ft02_phrases else Path(__file__).resolve().parents[1] / "src" / "FT02_PinyinPhraseData.h"
        if phrase_path.exists():
            inject_ft02_phrases(phrase_path, collector)
            labels.append(f"ft02-domain:{phrase_path.name}")
        else:
            print(f"warning: FT-02 phrase table not found: {phrase_path}", file=sys.stderr)

    manifest = build(collector, Path(args.out_dir), args.max_per_key, labels)
    print(json.dumps(manifest, ensure_ascii=False, indent=2))
    if not args.text and not args.json:
        print("\nWARNING: this build contains only FT-02's existing domain phrases. "
              "Use --text/--json with a real general dictionary for the A5 smart upgrade.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
