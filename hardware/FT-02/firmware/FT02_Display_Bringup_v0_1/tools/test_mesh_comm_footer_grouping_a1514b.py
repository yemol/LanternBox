from pathlib import Path
root=Path(__file__).resolve().parents[1]
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
bh=(root/'src/FT02_BottomBar.h').read_text(errors='ignore')
bc=(root/'src/FT02_BottomBar.cpp').read_text(errors='ignore')
checks = [
    ('left group', '↑↓浏览 · F筛选' in ui),
    ('center message group', 'ENTER回复/播放 · T新建' in ui),
    ('right diagnostics group', '{nullptr, "S诊断"}' in ui),
    ('25/50/25 inbox call', 'FT02_DrawBottomBarWithFontSplits(display, BOTTOM_INBOX, ft02_cjk_20r, 25, 75);' in ui),
    ('split API declared', 'FT02_DrawBottomBarWithFontSplits' in bh),
    ('split API implemented', 'FT02_DrawBottomBarWithFontSplits' in bc),
]
failed=[]
for name,ok in checks:
    print(('PASS' if ok else 'FAIL'), name)
    if not ok: failed.append(name)
if failed:
    raise SystemExit('FAIL: '+', '.join(failed))
print('PASS A1.5.14b inbox footer grouping')
