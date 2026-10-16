from pathlib import Path
root=Path(__file__).resolve().parents[1]
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
bh=(root/'src/FT02_BottomBar.h').read_text(errors='ignore')
bc=(root/'src/FT02_BottomBar.cpp').read_text(errors='ignore')

def req(cond,msg):
    if not cond:
        raise SystemExit('FAIL: '+msg)

req('↑↓浏览 · F筛选' in ui, 'browse/filter footer missing')
req('ENTER回复/播放 · T新建' in ui, 'reply/new-message footer grouping missing')
req('{nullptr, "S诊断"}' in ui, 'standalone diagnostics footer missing')
req('FT02_DrawBottomBarWithFontSplits(display, BOTTOM_INBOX, ft02_cjk_20r, 25, 75);' in ui, 'Inbox 25/50/25 layout missing')
req('FT02_DrawBottomBarWithFontSplits' in bh and 'FT02_DrawBottomBarWithFontSplits' in bc, 'weighted bottom-bar API missing')
req('V广播' not in ui, 'global V broadcast footer remains')
req('V 广播语音' not in ui, 'global V broadcast empty-state hint remains')
req("g_mode == COMM_INBOX && cmd == 'v'" not in ui, 'global inbox V handler remains')
req('"广播语音"' in ui and 'case 1u:' in ui and 'return beginVoice(FT02_MESH_BROADCAST_NODE, COMM_INBOX);' in ui, 'T New broadcast voice path missing')
print('PASS A1.5.14b single-line grouped footer cleanup')
