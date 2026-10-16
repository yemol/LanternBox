#!/usr/bin/env python3
from pathlib import Path
import hashlib, zlib, struct, sys, re

ROOT = Path(__file__).resolve().parents[1]

def fail(msg):
    print("FAIL", msg)
    raise SystemExit(1)

# Firmware identity / refresh path
build=(ROOT/"src/FT02_BuildInfo.h").read_text(encoding="utf-8")
cpp=(ROOT/"src/FT02_FieldLibrary.cpp").read_text(encoding="utf-8")
version=(ROOT/"VERSION.txt").read_text(encoding="utf-8").strip()
if not version == "v2.84-sleep-power-a5c-stable-r1.5": fail(f"unexpected firmware version: {version}")
if '"v2.84-sleep-power-a5c-stable-r1.5"' not in build: fail("BuildInfo/version mismatch")
if 'FT02_LibraryRedrawSelectionRows' not in cpp: fail("row-level partial selection refresh missing")
if 'FT02_LibraryRedrawFull(display, "field-library-full")' in cpp: fail("legacy full redraw route still present")
if cpp.count("setPartialWindow") < 2: fail("partial redraw paths missing")

# SD update catalog / ELE runtime
upd=ROOT/"SD_UPDATE_v0.8.2"
cat=(upd/"library/catalog.tsv").read_text(encoding="utf-8").splitlines()
rows=[l for l in cat if l and not l.startswith("#")]
if len(rows) != 4: fail(f"merged catalog has {len(rows)} books, expected 4")
ids=[]
for row in rows:
    f=row.split("\t")
    if len(f) != 8: fail("catalog row does not have 8 columns")
    if f[4] != "P0": fail("non-P0 row in catalog")
    ids.append(f[0])
if ids != ["BLD-P0-01","AGR-P0-01","SUR-P0-01","ELE-P0-01"]:
    fail(f"unexpected catalog IDs: {ids}")

book=upd/"library/books/ELE-P0-01"
toc=[l for l in (book/"toc.tsv").read_text(encoding="utf-8").splitlines() if l and not l.startswith("#")]
media=[l for l in (book/"media.tsv").read_text(encoding="utf-8").splitlines() if l and not l.startswith("#")]
if len(toc) != 45: fail(f"ELE TOC={len(toc)}, expected 45")
if len(media) != 139: fail(f"ELE media={len(media)}, expected 139")
for line in media:
    f=line.split("\t")
    if len(f) != 6: fail("bad ELE media row")
    p=ROOT / "SD_UPDATE_v0.8.2" / f[3].lstrip("/")
    # runtime paths start /library, while update root contains library
    if not p.is_file(): fail(f"missing PBM {p}")
    if p.read_bytes()[:2] != b"P4": fail(f"not PBM P4 {p}")

# FieldManual hard integrity gate: must remain unchanged and valid.
fm=ROOT/"SD_ROOT/knowledge/field_manual"
raw=(fm/"manifest.bin").read_bytes()
if len(raw) != 52: fail("manifest.bin size")
vals=struct.unpack("<8sHHHH16sIIIII", raw)
if vals[0] != b"FT02FM1\0": fail("manifest magic")
for name,saved in zip(["categories.bin","cards.idx","cards.dat","quick.idx"], vals[6:10]):
    actual=zlib.crc32((fm/name).read_bytes()) & 0xffffffff
    if actual != saved: fail(f"manifest CRC mismatch {name}: {saved:08X}!={actual:08X}")

mi=(fm/"media.idx").read_bytes()
magic,version,count,data_size=struct.unpack("<8sHHI", mi[:16])
md=(fm/"media.dat").read_bytes()
if magic != b"FT02MD1\0" or data_size != len(md): fail("FieldManual media header")
entry_fmt="<HBBHHIII64s"
esz=struct.calcsize(entry_fmt)
for i in range(count):
    ent=struct.unpack(entry_fmt, mi[16+i*esz:16+(i+1)*esz])
    off,length,crc=ent[5],ent[6],ent[7]
    if off+length > len(md): fail(f"media entry range {i}")
    if (zlib.crc32(md[off:off+length]) & 0xffffffff) != crc: fail(f"media CRC {i}")

print(f"PASS firmware=v2.84-sleep-power-a5c-stable-r1.5 catalog=4 ELE_toc={len(toc)} ELE_media={len(media)} FieldManual_manifest_crc=OK media_crc={count}/{count}")
