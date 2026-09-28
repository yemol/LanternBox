#!/usr/bin/env python3
from pathlib import Path
import argparse, shutil, sys, time, hashlib

BOOK_ID = "ELE-P0-01"
BOOK_LINE = "ELE-P0-01\t灾后与离线场景零基础电工手册\tLanternBox 编辑组（综合整理）\t电工与电力\tP0\tepub\t/library/books/ELE-P0-01\t/library/books/ELE-P0-01/toc.tsv"

def main():
    ap=argparse.ArgumentParser(description="Install FT-02 Field Library v0.8.2 ELE-P0-01 SD update")
    ap.add_argument("sd_root", type=Path, help="Mounted FT-02 SD root, e.g. /Volumes/FT02")
    args=ap.parse_args()
    root=args.sd_root
    lib=root/"library"
    catalog=lib/"catalog.tsv"
    if not root.exists():
        raise SystemExit(f"SD root does not exist: {root}")
    if not catalog.is_file():
        raise SystemExit(f"Missing existing catalog: {catalog}")
    payload=Path(__file__).resolve().parent/"library"
    src_book=payload/"books"/BOOK_ID
    if not src_book.is_dir():
        raise SystemExit("Package is incomplete: ELE-P0-01 runtime missing")

    # Preserve the user's current catalog before merging.
    backup=lib/"catalog.tsv.pre_v0.8.2.bak"
    shutil.copy2(catalog, backup)

    old=catalog.read_text(encoding="utf-8").splitlines()
    kept=[line for line in old if not line.startswith(BOOK_ID+"\t")]
    catalog.write_text("\n".join(kept+[BOOK_LINE])+"\n", encoding="utf-8")

    dst_book=lib/"books"/BOOK_ID
    if dst_book.exists():
        shutil.rmtree(dst_book)
    dst_book.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(src_book, dst_book)

    # Hard checks required by the current runtime.
    required=[
        dst_book/"metadata.json",
        dst_book/"toc.tsv",
        dst_book/"source.epub",
        dst_book/"media.tsv",
    ]
    missing=[str(p) for p in required if not p.is_file()]
    if missing:
        raise SystemExit("Install failed, missing: " + ", ".join(missing))
    toc_rows=[l for l in (dst_book/"toc.tsv").read_text(encoding="utf-8").splitlines() if l and not l.startswith("#")]
    media_rows=[l for l in (dst_book/"media.tsv").read_text(encoding="utf-8").splitlines() if l and not l.startswith("#")]
    catalog_rows=[l for l in catalog.read_text(encoding="utf-8").splitlines() if l and not l.startswith("#")]
    ids=[l.split("\t",1)[0] for l in catalog_rows]
    if BOOK_ID not in ids:
        raise SystemExit("Install failed: ELE-P0-01 not present in catalog after merge")
    # Do not touch /lanternbox/library. That path holds persistent reading progress.
    print(f"PASS catalog_books={len(catalog_rows)} ELE_toc={len(toc_rows)} ELE_media={len(media_rows)}")
    print(f"Installed: {dst_book}")
    print(f"Catalog backup: {backup}")
    print("Reading progress untouched: /lanternbox/library")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
