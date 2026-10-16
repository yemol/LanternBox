from pathlib import Path
import re

ui = Path("src/FT02_CommunicationNodeUI.cpp").read_text(encoding="utf-8")
font_cpp = Path("src/FT02_GlobalCJKBoldFontData.cpp").read_text(encoding="utf-8")
renderer_h = Path("src/FT02_FontPackRenderer.h").read_text(encoding="utf-8")
renderer_cpp = Path("src/FT02_FontPackRenderer.cpp").read_text(encoding="utf-8")

def req(cond, msg):
    if not cond:
        raise SystemExit("FAIL: " + msg)

req('drawCenteredTextScaled(display, ft02_cjk_24b, state, 400, 250, 4, 3);' in ui,
    'voice state must use full-CJK 24b scaled 4/3')
req('drawCenteredText(display, ft02_title_32b, state, 400, 250);' not in ui,
    'partial title_32 font must not render Chinese voice state')
req('void FT02_DrawTextPackScaled(' in renderer_h and 'int FT02_TextWidthPackScaled(' in renderer_h,
    'scaled font API missing')
req('FT02_DrawTextPackScaledInternal' in renderer_cpp,
    'scaled renderer implementation missing')

# Every codepoint used by the large Chinese voice-state strings must exist in
# the full CJK bold pack; this catches a future regression before real-device tofu boxes.
states = [
    '准备录音', '待机', '● 开始说话', '麦克风稳定中',
    '正在保存语音', '正在压缩语音', '语音处理失败', '正在发送语音'
]
index_cps = {int(x, 16) for x in re.findall(r'\{0x([0-9A-Fa-f]+),\s*\d+\}', font_cpp)}
missing = sorted({ord(ch) for text in states for ch in text if ord(ch) not in index_cps})
req(not missing, 'full CJK bold pack missing voice-state cps: ' + ','.join(f'U+{cp:04X}' for cp in missing))
print('PASS A1.5.15c CJK-safe scaled voice status')
