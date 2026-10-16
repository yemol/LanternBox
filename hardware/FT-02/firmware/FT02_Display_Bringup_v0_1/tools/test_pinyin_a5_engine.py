#!/usr/bin/env python3
"""Host-side format and lookup tests for FT-02 Pinyin IME A5."""
from __future__ import annotations

import json
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "tools" / "build_ft02_pinyin_a5_dict.py"
HEADER_FMT = "<8sHHIIIII"
INDEX_RECORD_FMT = "<25sBHII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
INDEX_RECORD_SIZE = struct.calcsize(INDEX_RECORD_FMT)


def lookup(out: Path, key: str):
    idxp = out / "ft02_pinyin_words.idx"
    datp = out / "ft02_pinyin_words.dat"
    with idxp.open("rb") as f:
        magic, ver, recsz, groups, total, *_ = struct.unpack(HEADER_FMT, f.read(HEADER_SIZE))
        assert magic == b"FT2PYI1\0" and ver == 1 and recsz == INDEX_RECORD_SIZE
        lo, hi = 0, groups
        rec = None
        while lo < hi:
            mid = lo + (hi-lo)//2
            f.seek(HEADER_SIZE + mid * INDEX_RECORD_SIZE)
            keyb, keylen, count, off, _ = struct.unpack(INDEX_RECORD_FMT, f.read(INDEX_RECORD_SIZE))
            k = keyb[:keylen].decode("ascii")
            if k == key:
                rec = (count, off)
                break
            if key < k: hi = mid
            else: lo = mid + 1
    if rec is None:
        return []
    count, off = rec
    ans=[]
    with datp.open("rb") as f:
        magic, ver, hsz, *_ = struct.unpack(HEADER_FMT, f.read(HEADER_SIZE))
        assert magic == b"FT2PYD1\0" and ver == 1 and hsz == HEADER_SIZE
        f.seek(off)
        for _ in range(count):
            freq, n = struct.unpack("<IB", f.read(5))
            ans.append((f.read(n).decode("utf-8"), freq))
    return ans


def main():
    with tempfile.TemporaryDirectory() as td:
        td=Path(td)
        src=td/"words.tsv"
        src.write_text(
            "woxihuan\t我喜欢\t90000\n"
            "woxihuan\t我稀罕\t1000\n"
            "xihuan\t喜欢\t80000\n"
            "zhongwen\t中文\t70000\n"
            "nihao\t你好\t100000\n",
            encoding="utf-8"
        )
        out=td/"ime"
        subprocess.run([
            "python3", str(BUILDER), "--text", str(src), "--out-dir", str(out),
            "--ft02-phrases", str(ROOT/"src"/"FT02_PinyinPhraseData.h")
        ], check=True, stdout=subprocess.DEVNULL)

        wx=lookup(out,"woxihuan")
        assert wx[0][0] == "我喜欢", wx[:3]
        assert any(w == "我稀罕" for w,_ in wx)
        assert lookup(out,"xihuan")[0][0] == "喜欢"
        assert lookup(out,"zhongwen")[0][0] == "中文"
        # Domain phrases are injected even when absent from the external source.
        assert lookup(out,"anquan")[0][0] == "安全"
        assert lookup(out,"notakey") == []

        manifest=json.loads((out/"ft02_pinyin_words.manifest.json").read_text(encoding="utf-8"))
        assert manifest["groups"] > 5
        assert manifest["collector"]["domain_injected"] > 100
        assert manifest["files"]["ft02_pinyin_words.idx"]["bytes"] > HEADER_SIZE
        print("PASS: FT-02 Pinyin IME A5 dictionary format / lookup / domain injection")


if __name__ == "__main__":
    main()
