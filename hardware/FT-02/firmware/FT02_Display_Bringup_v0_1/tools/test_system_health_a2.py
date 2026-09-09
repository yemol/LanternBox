#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def text(rel):
    return (ROOT / rel).read_text(encoding='utf-8')

def req(cond, msg):
    if not cond:
        raise SystemExit(f'FAIL: {msg}')

build = text('src/FT02_BuildInfo.h')
selftest = text('src/FT02_SystemSelfTest.cpp')
lr01 = text('src/FT02_LR01HostRuntime.cpp')
rec = text('src/FT02_LocationRecorder.cpp')
recui = text('src/FT02_LocationRecorderUI.cpp')
mapui = text('src/FT02_PbfMapUI.cpp')

req('"v2.75k67"' in build, 'firmware version')
req('lr01.uartErrors == 0u ? FT02_SELFTEST_PASS : FT02_SELFTEST_WARN' not in selftest,
    'Host UART must not warn forever on historical uartErrors')
req('本次新增 RX+%lu UART+%lu 复位+%lu' in selftest,
    'communication delta warning missing')
req('当前正常·累计 RX%lu UART%lu 复位%lu' in selftest,
    'communication historical counters should be informational')
req('FT02_COMM_UART_WARN_TOTAL = 10u' in selftest,
    'absolute error threshold missing')
req('低电量 %.0f%%' in selftest and '严重低电量 %.0f%%' in selftest,
    'battery low warning should explain itself')
req('Host UART在线·启动协议帧未捕获' in selftest,
    'missed boot frame must not render as A0')
req('markProtocolA2Feature();' in lr01 and 'if(g_state.protocolVersion == 0u) g_state.protocolVersion = 2u;' in lr01,
    'A2 extension inference missing')
req('GNSS_POWER_STATE ' in lr01 and 'COMPASS_CAL_STATE ' in lr01,
    'A2 inference sources missing')
req('无法开始：GNSS已关闭，请先在设备状态开启' in rec,
    'location recorder start must honor GNSS OFF')
req('记点失败：GNSS已关闭' in rec,
    'manual point must explain GNSS OFF')
req('自动记录暂停：GNSS已关闭' in rec,
    'active auto track must pause on GNSS OFF')
req('无法开启自动：GNSS已关闭' in rec,
    'auto track enable must honor GNSS OFF')
req('if(!FT02_GnssPowerDesiredEnabled()) return "已关闭";' in recui,
    'recorder UI GNSS OFF state missing')
req('GNSS已关闭·请在设备状态中开启' in recui,
    'recorder UI guidance missing')
req('snprintf(speedText, sizeof(speedText), "GNSS关闭")' in mapui,
    'map footer GNSS OFF state missing')

print('PASS k67 self-test-health=A2 protocol-A0-fix=OK historical-UART-no-duplicate-warn=OK GNSS-off-UX=recorder+map')
