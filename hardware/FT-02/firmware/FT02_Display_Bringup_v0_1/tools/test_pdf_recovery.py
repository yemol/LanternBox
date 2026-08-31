#!/usr/bin/env python3
from __future__ import annotations
import argparse, importlib.util, json, re, shutil, tempfile
from pathlib import Path


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--builder', type=Path, required=True)
    args=ap.parse_args()
    import fitz
    spec=importlib.util.spec_from_file_location('ft02_builder', args.builder)
    mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    with tempfile.TemporaryDirectory(prefix='ft02_pdf_recovery_') as td:
        root=Path(td); src=root/'src'/'P0-TEST'; out=root/'out'; src.mkdir(parents=True)
        pdf=src/'book.pdf'
        d=fitz.open()
        for i in range(3):
            p=d.new_page(); p.insert_text((72,72), ('FT02 PDF recovery page %d seed electrical building safety ' % (i+1))*12)
        d.save(pdf); d.close()
        b=pdf.read_bytes(); ms=list(re.finditer(rb'startxref\s+(\d+)\s+%%EOF',b))
        if not ms: raise RuntimeError('test PDF has no startxref')
        m=ms[-1]; repl=b'0'*len(m.group(1)); pdf.write_bytes(b[:m.start(1)]+repl+b[m.end(1):])
        (src/'metadata.json').write_text(json.dumps({'title':'PDF恢复自检','author':'FT02','category':'测试','priority':'P0'},ensure_ascii=False),encoding='utf-8')
        # Tier 1: malformed xref should be recovered quietly.
        mod.build_pdf(pdf, out/'tier1')
        # Tier 2: force tier 1 failure and prove pypdf fallback.
        original=mod._extract_pdf_with_pymupdf
        mod._extract_pdf_with_pymupdf=lambda _: (_ for _ in ()).throw(RuntimeError('forced'))
        mod.build_pdf(pdf, out/'tier2')
        mod._extract_pdf_with_pymupdf=original
        txt=(out/'tier2'/'IMPORT.txt').read_text(encoding='utf-8')
        assert 'pypdf-recovery' in txt
    print('PASS: malformed-xref recovery + pypdf fallback')
    return 0
if __name__=='__main__': raise SystemExit(main())
