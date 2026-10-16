from pathlib import Path
root=Path(__file__).resolve().parents[1]
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')

def req(cond,msg):
    if not cond:
        raise SystemExit('FAIL '+msg)

req('COMM_FILTER' in ui, 'filter mode missing')
req('void drawFilter(FT02Display& display)' in ui, 'filter page missing')
req('"全部消息", "广播消息", "私聊消息"' in ui, 'filter choices missing')
req('ENTER应用筛选' in ui and 'B返回消息' in ui, 'filter footer missing')
req('filterShortcut' in ui and "raw == static_cast<uint8_t>('F')" in ui and "raw == static_cast<uint8_t>('[')" in ui, 'robust F aliases missing')
req('[COMM-FILTER] open' in ui and '[COMM-FILTER] applied' in ui, 'filter trace logs missing')
req("g_historyFilter = static_cast<HistoryFilter>(g_filterSelection);" in ui, 'filter apply missing')
req('g_mode = COMM_FILTER;' in ui, 'Inbox F does not open filter page')
req('if(g_mode == COMM_FILTER)' in ui, 'filter input handler missing')

# Voice page layout guard.
req('constexpr int BAR_X = 80;' in ui and 'constexpr int BAR_W = 640;' in ui, 'stable voice progress geometry missing')
req('drawCenteredTextScaled(display, ft02_cjk_24b, state, 400, 250, 4, 3);' in ui, 'CJK-safe voice state not centered')
req('"分片 %03u / %03u"' in ui, 'fixed chunk row missing')
req('BAR_X + BAR_W - pctW' in ui, 'right-aligned percent missing')
req('请保持设备开启，发送完成后自动返回' in ui, 'TX status hint missing')
req('"发送中"' in ui and '"等待发送完成"' in ui, 'clean TX footer missing')
req('FT02_CommunicationCommitPartial(display, 56, 204, 688, 208);' in ui, 'voice partial refresh geometry missing')

print('PASS A1.5.15 filter UX + voice page polish')
