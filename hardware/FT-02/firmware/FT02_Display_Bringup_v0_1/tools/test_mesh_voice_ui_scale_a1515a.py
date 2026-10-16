from pathlib import Path
ui = Path("src/FT02_CommunicationNodeUI.cpp").read_text(encoding="utf-8")

def req(cond, msg):
    if not cond:
        raise SystemExit("FAIL: " + msg)

req('drawCenteredTextScaled(display, ft02_cjk_24b, state, 400, 250, 4, 3);' in ui, 'CJK-safe ~32px main voice state missing')
req('constexpr int BAR_X = 80;' in ui and 'constexpr int BAR_W = 640;' in ui, '640px voice bar geometry missing')
req('constexpr int BAR_H = 30;' in ui, '30px voice bar height missing')
req('FT02_DrawTextPack(display, ft02_cjk_24b, chunks, BAR_X, 346);' in ui, '24px chunk metrics missing')
req('FT02_DrawTextPack(display, ft02_cjk_24b, pct, BAR_X + BAR_W - pctW, 346);' in ui, '24px percent metrics missing')
req('drawCenteredText(display, ft02_title_32b, timer, 400, 346);' in ui, '32px recording timer missing')
req('drawCenteredText(display, ft02_cjk_24r, "请保持设备开启，发送完成后自动返回", 400, 394);' in ui, '24px send hint missing')
req('FT02_CommunicationCommitPartial(display, 56, 204, 688, 208);' in ui, 'expanded voice partial refresh missing')
print('PASS A1.5.15a voice UI scale polish')
