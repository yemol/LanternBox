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
main = text('src/main.cpp')
status = text('src/FT02_StatusBar.cpp')
rec = text('src/FT02_LocationRecorder.cpp')
recui = text('src/FT02_LocationRecorderUI.cpp')
mapui = text('src/FT02_PbfMapUI.cpp')

req('"v2.84-sleep-power-a5c-stable-r1.1"' in build, 'firmware version')
req(not (ROOT/'src/FT02_LR01HostRuntime.cpp').exists(), 'retired LR01 runtime cpp still present')
req(not (ROOT/'src/FT02_LR01HostRuntime.h').exists(), 'retired LR01 runtime header still present')
req('FT02_LR01Host' not in main and 'FT02_LR01Host' not in status, 'active code still references LR01 host runtime')
req('SX1262 · HW-SPI · LongFast' in selftest, 'self-test does not report current Core SX1262 path')
req('Mesh PKI' in selftest and 'FT02_MeshServicePkiReady' in selftest, 'self-test PKI health missing')
req('FT02_MeshServiceRawFrameCount' in selftest and 'FT02_MeshServiceDecodedFrameCount' in selftest, 'Mesh receive counters missing')
req('LR01已移除·LoRa待Core迁移' not in selftest and '等待Core独立SPI迁移' not in selftest, 'stale LR01 migration text remains')
req('低电量 %.0f%%' in selftest and '严重低电量 %.0f%%' in selftest, 'battery low warning should explain itself')
req('无法开始：GNSS已关闭，请切换户外模式' in rec, 'location recorder start must honor GNSS OFF')
req('记点失败：GNSS已关闭' in rec, 'manual point must explain GNSS OFF')
req('自动记录暂停：GNSS已关闭' in rec, 'active auto track must pause on GNSS OFF')
req('无法开启自动：GNSS已关闭' in rec, 'auto track enable must honor GNSS OFF')
req('if(!FT02_GnssPowerDesiredEnabled()) return "已关闭";' in recui, 'recorder UI GNSS OFF state missing')
req('GNSS已关闭·请切换户外模式' in recui, 'recorder UI guidance missing')
req('snprintf(speedText, sizeof(speedText), "GNSS关闭")' in mapui, 'map footer GNSS OFF state missing')

print('PASS stable-r1 self-test=CoreMesh LR01-runtime=removed GNSS-off-UX=recorder+map')
