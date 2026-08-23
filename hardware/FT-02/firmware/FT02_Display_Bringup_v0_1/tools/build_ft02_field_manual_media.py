#!/usr/bin/env python3
"""Build FT-02 FieldManual Runtime A3 optional image-page pack.

A3.2 adds multi-page diagrams. flags bit0 means the reader should render the
caption below the image and use a generic 图解 n/N heading.
"""
from __future__ import annotations
import argparse, json, struct, zlib
from pathlib import Path
from PIL import Image

MAGIC=b'FT02MD1\0'
FORMAT_VERSION=1
ENTRY_STRUCT=struct.Struct('<HBBHHIII64s')
HEADER_STRUCT=struct.Struct('<8sHHI')
MAX_W=688
MAX_H=184
FLAG_CAPTION_BELOW=0x01

DEFAULT_SPEC = {
    'TOL001': [
        ('tool_01_flat_01.png', '第1步：A压过B，先做第一次交叉', FLAG_CAPTION_BELOW),
        ('tool_01_flat_02.png', '第2步：A从B下方穿回，拉成半结', FLAG_CAPTION_BELOW),
        ('tool_01_flat_03.png', '第3步：反向交叉，这次B压过A', FLAG_CAPTION_BELOW),
        ('tool_01_flat_04.png', '第4步：四端拉紧；绳尾应平行', FLAG_CAPTION_BELOW),
    ],
    'TOL002': [
        ('tool_02_bowline_01.png', '第1步：主绳做小圈，留出工作端A', FLAG_CAPTION_BELOW),
        ('tool_02_bowline_02.png', '第2步：A从小圈下方穿到上方', FLAG_CAPTION_BELOW),
        ('tool_02_bowline_03.png', '第3步：A绕到主绳S的背后', FLAG_CAPTION_BELOW),
        ('tool_02_bowline_04.png', '第4步：A再穿回最初的小圈', FLAG_CAPTION_BELOW),
        ('tool_02_bowline_05.png', '第5步：整理并拉紧，得到固定绳圈', FLAG_CAPTION_BELOW),
    ],
    'TOL003': [
        ('tool_03_clove_01.png', '第1步：工作端A绕杆一整圈', FLAG_CAPTION_BELOW),
        ('tool_03_clove_02.png', '第2步：跨过主绳，再绕杆一圈', FLAG_CAPTION_BELOW),
        ('tool_03_clove_03.png', '第3步：A从最后交叉处下方穿出', FLAG_CAPTION_BELOW),
        ('tool_03_clove_04.png', '第4步：两端拉紧，结体贴住杆件', FLAG_CAPTION_BELOW),
    ],
    'TOL004': [
        ('tool_04_trucker_01.png', '第1步：主绳中段先做一个固定小环', FLAG_CAPTION_BELOW),
        ('tool_04_trucker_02.png', '第2步：工作端A绕过远端锚点', FLAG_CAPTION_BELOW),
        ('tool_04_trucker_03.png', '第3步：A穿回主绳上的小环', FLAG_CAPTION_BELOW),
        ('tool_04_trucker_04.png', '第4步：拉A收紧，产生增力效果', FLAG_CAPTION_BELOW),
        ('tool_04_trucker_05.png', '第5步：保持张力，用两个半结锁住', FLAG_CAPTION_BELOW),
    ],
    'TOL005': [('tool_05_tripod.png', '图解：三脚架绑扎', 0)],
    'TOL006': [('tool_06_ground_anchor.png', '图解：地面锚点', 0)],
    'TOL007': [('tool_07_cloth_prefilter.png', '图解：布料预滤', 0)],
    'TOL008': [('tool_08_carry_pole.png', '图解：双人搬运杆', 0)],
    'TOL009': [('tool_09_splint_clamp.png', '图解：夹板式固定夹', 0)],
    'TOL010': [('tool_10_rain_catch.png', '图解：防水布集雨', 0)],
}

def pack_bitmap(path: Path):
    im=Image.open(path).convert('L')
    if im.width > MAX_W or im.height > MAX_H:
        raise ValueError(f'{path}: {im.size} exceeds {MAX_W}x{MAX_H}')
    pixels=im.load(); row_bytes=(im.width+7)//8; out=bytearray(row_bytes*im.height)
    for y in range(im.height):
        for x in range(im.width):
            if pixels[x,y] < 128:
                out[y*row_bytes + x//8] |= 0x80 >> (x & 7)
    return im.width, im.height, bytes(out)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--cards', required=True)
    ap.add_argument('--images', required=True)
    ap.add_argument('--out', required=True)
    args=ap.parse_args()
    cards=json.loads(Path(args.cards).read_text(encoding='utf-8'))
    card_index={c['id']:i for i,c in enumerate(cards)}
    image_dir=Path(args.images); out_dir=Path(args.out); out_dir.mkdir(parents=True,exist_ok=True)
    entries=[]; data=bytearray(); manifest=[]
    for card_id,items in DEFAULT_SPEC.items():
        if card_id not in card_index: raise KeyError(card_id)
        for order,(filename,caption,flags) in enumerate(items):
            w,h,raw=pack_bitmap(image_dir/filename)
            offset=len(data); data.extend(raw); crc=zlib.crc32(raw)&0xffffffff
            cap=caption.encode('utf-8')
            if len(cap)>63: raise ValueError(f'caption too long ({len(cap)}): {caption}')
            cap=cap+b'\0'*(64-len(cap))
            entries.append(ENTRY_STRUCT.pack(card_index[card_id],order,flags,w,h,offset,len(raw),crc,cap))
            manifest.append({'card_id':card_id,'card_index':card_index[card_id],'order':order,'file':filename,'caption':caption,'flags':flags,'width':w,'height':h,'offset':offset,'length':len(raw),'crc32':f'{crc:08x}'})
    (out_dir/'media.dat').write_bytes(data)
    header=HEADER_STRUCT.pack(MAGIC,FORMAT_VERSION,len(entries),len(data))
    (out_dir/'media.idx').write_bytes(header+b''.join(entries))
    (out_dir/'media_manifest.json').write_text(json.dumps({'format':'FT02MD1','version':FORMAT_VERSION,'media_count':len(entries),'data_size':len(data),'flags':{'1':'caption_below'},'items':manifest},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'PASS media_count={len(entries)} data={len(data)} bytes')

if __name__=='__main__': main()
