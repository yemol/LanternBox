#!/usr/bin/env python3
from __future__ import annotations
import argparse
import shutil
from pathlib import Path

CATALOG_HEADER = (
    "# FT-02 Field Library catalog v0.2\n"
    "# id<TAB>title<TAB>author<TAB>category<TAB>priority<TAB>format<TAB>root<TAB>toc\n"
)

def rm(path: Path) -> None:
    if path.is_dir():
        shutil.rmtree(path)
        print(f"REMOVE {path}")
    elif path.exists():
        path.unlink()
        print(f"REMOVE {path}")

def main() -> int:
    ap = argparse.ArgumentParser(description="Reset FT-02 Field Library to an empty A6 P0 baseline.")
    ap.add_argument("--project-root", type=Path, default=Path.cwd())
    ap.add_argument("--keep-user", action="store_true", help="Keep /library/user reading state")
    args = ap.parse_args()

    root = args.project_root.resolve()
    for name in ("P0_SOURCE_STAGING", "P0_REFERENCE_ARCHIVE", "P0_SOURCE_DOWNLOADS"):
        rm(root / name)

    library = root / "SD_ROOT" / "library"
    books = library / "books"
    user = library / "user"
    rm(books)
    if not args.keep_user:
        rm(user)
    rm(library / "catalog.tsv")
    rm(library / "CLEAN_BASELINE.txt")

    books.mkdir(parents=True, exist_ok=True)
    user.mkdir(parents=True, exist_ok=True)
    (library / "catalog.tsv").write_text(CATALOG_HEADER, encoding="utf-8")
    (library / "CLEAN_BASELINE.txt").write_text(
        "FT-02 Field Library P0 Clean Baseline A6\n"
        "No books installed. A6 P0 Admission Gate is zero-default-deny.\n",
        encoding="utf-8",
    )
    print("RESET COMPLETE: Field Library is empty and ready for a new P0-only curation pass.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
