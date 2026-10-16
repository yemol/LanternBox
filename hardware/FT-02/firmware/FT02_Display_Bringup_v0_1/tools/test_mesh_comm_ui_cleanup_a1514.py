from pathlib import Path
root = Path(__file__).resolve().parents[1]
ui = (root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')

def req(cond, msg):
    if not cond:
        raise SystemExit('FAIL ' + msg)

# Inbox footer must be small and non-redundant.
req('↑↓浏览 · F筛选' in ui, 'clean browse/filter footer missing')
req('ENTER回复/播放' in ui and 'ENTER回复/播放 · V广播' not in ui, 'reply/play footer cleanup missing')
req('ENTER回复/播放 · T新建' in ui, 'new/diagnostics footer missing')
req('T 新建消息   ·   V 广播语音   ·   S 通讯诊断' not in ui, 'empty-state legacy V broadcast hint remains')
req("g_mode == COMM_INBOX && cmd == 'v'" not in ui, 'Inbox V broadcast shortcut must be removed')
req('T新建 / M节点 / O发件' not in ui, 'legacy crowded inbox footer still present')

# O Outbox and M Nodes are no longer Inbox shortcuts.
req("if(g_mode == COMM_INBOX && cmd == 'o')" not in ui, 'Inbox O shortcut still active')
req("if(g_mode == COMM_INBOX && cmd == 'm')" not in ui, 'Inbox M shortcut still active')
req("if(g_mode == COMM_INBOX && cmd == 's')" in ui, 'Inbox diagnostics shortcut missing')

# Node diagnostics live under the diagnostics screen and return there.
req('M节点列表' in ui, 'diagnostics node-list footer missing')
req("else if(g_mode == COMM_DIAG && cmd == 'm')" in ui, 'diagnostics M route missing')
req('g_nodesReturnMode = COMM_DIAG;' in ui, 'node diagnostics return target missing')
req('g_mode = g_nodesReturnMode;' in ui, 'node diagnostics Back route missing')
req('Mesh节点诊断' in ui, 'node diagnostics title missing')

# Timeline must use stable columns and dashed dividers instead of one packed metadata string.
req('void drawDashedDivider' in ui and 'drawFastHLine' in ui, 'dashed message divider missing')
req('drawDashedDivider(display, y + 72);' in ui, 'message divider not applied per row')
req('display.fillRect(30, y + 8, 4, 54' in ui, 'quiet selected-row rail missing')
req('FT02_DrawTextPack(display, ft02_cjk_20r, peer, 44, y + 22);' in ui, 'peer fixed column missing')
req('FT02_DrawTextPack(display, ft02_cjk_20r, when, 500, y + 22);' in ui, 'time fixed column missing')
req('FT02_DrawTextPack(display, ft02_cjk_20r, state, 636, y + 22);' in ui, 'state fixed column missing')
req('历史 %u/%u   未读 %u   筛选 %s   Mesh %s' in ui, 'clean summary line missing')

print('PASS A1.5.14 communication UI cleanup / diagnostics nesting / dashed timeline guard')
