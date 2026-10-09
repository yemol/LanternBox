#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
inp=(root/'src/FT02_InputManager.cpp').read_text()
epd=(root/'src/FT02_EpdInteractiveRuntime.cpp').read_text()

def req(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

req('FT02_INPUT_DISCRETE_BOUNCE_MS = 35' in inp, 'short bounce guard')
req('FT02_INPUT_DISCRETE_FAST_REPRESS_MS = 270' in inp, 'fast re-press window')
req('FT02_INPUT_REPEAT_START_MIN_MS = 270' in inp, 'repeat start lower bound')
req('FT02_INPUT_REPEAT_START_MAX_MS = 340' in inp, 'repeat start upper bound')
req('FT02_INPUT_REPEAT_BURST_GAP_MS = 125' in inp, 'repeat burst gap')
req('FT02_INPUT_REPEAT_DRAIN_MAX = 8' in inp, 'repeat queue drain bound')
req('FT02_DrainQueuedSameDiscreteRaw' in inp, 'stale repeat backlog drain')
req('FT02_INPUT_DISCRETE_RELEASE_QUIET_MS' not in inp, 'old 650ms synthetic release lock still present')
req('g_waitForInitialRelease' not in epd, 'EPD still owns synthetic release gate')
req('InputManager A2 owns held-repeat suppression' in epd, 'EPD A2 ownership comment')
req('static_cast<uint8_t>(raw) == 0xFFu' in inp, '0xFF bus-glitch drop')

# Behavioral model of the A2 timing state. Factory repeat is 300ms then 50ms.
BOUNCE=35
FAST=270
RMIN=270
RMAX=340
BURST_GAP=125

def run(events):
    # events is [(t, raw, queued_same_after)] where queued_same_after emulates
    # stale repeat bytes already waiting in CardKB2's 32-byte I2C queue.
    tracked=None
    accepted_t=0
    last_seen=0
    burst=False
    out=[]
    for t,raw,queued_same in events:
        if tracked != raw:
            tracked=raw; accepted_t=t; last_seen=t; burst=False; out.append((t,raw)); continue
        da=t-accepted_t
        ds=t-last_seen
        last_seen=t
        if da <= BOUNCE:
            continue
        if burst:
            if ds <= BURST_GAP:
                continue
            if queued_same:
                last_seen=t
                continue
            burst=False; accepted_t=t; out.append((t,raw)); continue
        if da < FAST:
            accepted_t=t; out.append((t,raw)); continue
        if RMIN <= da <= RMAX:
            burst=True
            continue
        if queued_same:
            burst=True
            continue
        accepted_t=t; out.append((t,raw))
    return out

# 1) Fast same-key taps survive.
req(run([(0,'x',False),(90,'x',False),(190,'x',False)]) == [(0,'x'),(90,'x'),(190,'x')], 'fast same-key taps swallowed')
# 2) Held key repeat stream is suppressed.
req(run([(0,'x',False),(300,'x',False),(350,'x',False),(400,'x',False),(450,'x',False)]) == [(0,'x')], 'hold repeat leaked')
# 3) Re-press after release is accepted immediately after repeat burst goes quiet.
req(run([(0,'x',False),(300,'x',False),(350,'x',False),(600,'x',False)]) == [(0,'x'),(600,'x')], 'post-release re-press blocked')
# 4) Long blocked section with queued repeats must not look like a new press.
req(run([(0,'x',False),(700,'x',True)]) == [(0,'x')], 'stale repeat backlog leaked')
# 5) Different key is immediate.
req(run([(0,'x',False),(40,'c',False)]) == [(0,'x'),(40,'c')], 'different key delayed')
print('PASS Keyboard Adaptive A2: fast taps accepted, 300/50ms hold repeats suppressed, backlog drained')
