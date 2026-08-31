#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
import sys


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('library',type=Path)
    args=ap.parse_args()
    root=args.library
    catalog=root/'catalog.tsv'
    if not catalog.is_file():
        print('FAIL missing catalog.tsv'); return 2
    errors=[]; books=0; chapters=0; media_items=0; inline_markers=0
    for raw in catalog.read_text(encoding='utf-8').splitlines():
        line=raw.strip()
        if not line or line.startswith('#'): continue
        f=line.split('\t')
        if len(f)!=8:
            errors.append(f'catalog columns != 8: {line[:80]}'); continue
        bid,title,author,category,priority,fmt,bookroot,toc=f
        books+=1
        book_inline_refs=[]
        if priority!='P0': errors.append(f'{bid}: non-P0 priority {priority}')
        if fmt not in {'epub','pdf','scanpdf','txt'}: errors.append(f'{bid}: unsupported format {fmt}')
        meta_fs=root/'books'/bid/'metadata.json'
        if not meta_fs.is_file():
            errors.append(f'{bid}: missing metadata.json')
        else:
            try:
                meta=json.loads(meta_fs.read_text(encoding='utf-8'))
                required={
                    'priority':'P0',
                    'source_priority':'P0',
                    'library_role':'CORE_CONTENT',
                    'source_status':'APPROVED',
                    'content_audit_status':'APPROVED',
                    'practical_value_status':'APPROVED',
                    'admission_status':'APPROVED',
                }
                for k,expected in required.items():
                    actual=str(meta.get(k,'')).upper()
                    if actual!=expected:
                        errors.append(f'{bid}: A6 gate {k}={actual or "MISSING"} expected {expected}')
                if meta.get('installable') is not True:
                    errors.append(f'{bid}: A6 gate installable is not true')
                if str(meta.get('language','')).lower() not in {'zh','zh-cn','zh-hans'}:
                    errors.append(f'{bid}: A6 gate language is not Chinese')
            except Exception as exc:
                errors.append(f'{bid}: invalid metadata.json: {exc}')
        toc_fs=root.parent / toc.lstrip('/') if toc.startswith('/library/') else Path(toc)
        # root.parent is SD_ROOT; /library/... maps from there.
        if not toc_fs.is_file():
            errors.append(f'{bid}: missing toc {toc_fs}'); continue
        for tline in toc_fs.read_text(encoding='utf-8').splitlines():
            t=tline.strip()
            if not t or t.startswith('#'): continue
            p=t.split('\t')
            if len(p)!=3:
                errors.append(f'{bid}: bad toc row {t[:80]}'); continue
            _,ctitle,cpath=p
            if cpath.startswith('@media:'):
                try:
                    int(cpath.split(':',1)[1])
                    chapters+=1
                except ValueError:
                    errors.append(f'{bid}: invalid media TOC target {cpath}')
                continue
            ch_fs=root.parent / cpath.lstrip('/') if cpath.startswith('/library/') else Path(cpath)
            if not ch_fs.is_file(): errors.append(f'{bid}: missing chapter {ch_fs}')
            else:
                try:
                    chapter_text=ch_fs.read_text(encoding='utf-8')
                    try: chapter_index=int(p[0])
                    except ValueError: chapter_index=-1
                    for marker in re.findall(r'\[\[IMG:(\d+)\]\]', chapter_text):
                        book_inline_refs.append((chapter_index,int(marker),ch_fs))
                        inline_markers+=1
                except UnicodeDecodeError: errors.append(f'{bid}: chapter not UTF-8 {ch_fs}')
                chapters+=1
        if fmt=='epub':
            src=root/'books'/bid/'source.epub'
            if not src.is_file(): errors.append(f'{bid}: EPUB source.epub missing')
        elif fmt in {'pdf','scanpdf'}:
            src=root/'books'/bid/'source.pdf'
            if not src.is_file(): errors.append(f'{bid}: PDF source.pdf missing')

        media_tsv=root/'books'/bid/'media.tsv'
        media_chapters={}
        if media_tsv.is_file():
            for mline in media_tsv.read_text(encoding='utf-8').splitlines():
                m=mline.strip()
                if not m or m.startswith('#'): continue
                mf=m.split('\t')
                if len(mf)!=6:
                    errors.append(f'{bid}: bad media row {m[:80]}'); continue
                media_index_s,chapter,caption,mpath,width,height=mf
                try:
                    media_index=int(media_index_s); chapter_index=int(chapter); w=int(width); h=int(height)
                    media_chapters[media_index]=chapter_index
                    if w<=0 or h<=0: raise ValueError()
                except ValueError:
                    errors.append(f'{bid}: invalid media dimensions/chapter {m[:80]}'); continue
                media_fs=root.parent / mpath.lstrip('/') if mpath.startswith('/library/') else Path(mpath)
                if not media_fs.is_file():
                    errors.append(f'{bid}: missing media {media_fs}'); continue
                try:
                    with media_fs.open('rb') as fh:
                        magic=fh.read(2)
                    if magic!=b'P4': errors.append(f'{bid}: media is not PBM P4 {media_fs}')
                except OSError as exc:
                    errors.append(f'{bid}: cannot read media {media_fs}: {exc}')
                media_items+=1
        elif fmt=='scanpdf':
            errors.append(f'{bid}: scanpdf missing media.tsv')
        for chapter_index,marker_index,ch_fs in book_inline_refs:
            if marker_index not in media_chapters:
                errors.append(f'{bid}: inline image marker {marker_index} has no media.tsv row ({ch_fs.name})')
            elif media_chapters[marker_index] != chapter_index:
                errors.append(f'{bid}: inline image marker {marker_index} chapter mismatch {chapter_index}!={media_chapters[marker_index]}')
    if errors:
        print(f'FAIL books={books} chapters={chapters} media={media_items} inline={inline_markers} errors={len(errors)}')
        for e in errors: print(' -',e)
        return 1
    suffix=' CLEAN-EMPTY=YES' if books==0 else ''
    print(f'PASS books={books} chapters={chapters} media={media_items} inline={inline_markers} P0-only=YES A6-gate=YES{suffix}')
    return 0

if __name__=='__main__': raise SystemExit(main())
