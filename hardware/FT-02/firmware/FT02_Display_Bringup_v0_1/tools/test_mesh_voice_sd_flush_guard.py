from pathlib import Path
s=Path('src/FT02_AudioLog.cpp').read_text()
checks={
 'rtc_trace':'RTC_NOINIT_ATTR uint32_t g_ft02AudioStopTracePhase' in s,
 'boot_report':'[AUDIO-RESET-TRACE] previous phase=' in s,
 'chunked_flush':'while(offset < totalBytes)' in s and 'vTaskDelay(1);' in s,
 'internal_bounce':'memcpy(g_ft02AudioMonoBuffer' in s,
 'no_giant_psram_fwrite':'fwrite(\n        g_ft02AudioRecording.psramStage' not in s,
 'flush_trace':'FT02_AUDIO_STOP_TRACE_FLUSH_WRITE' in s,
}
for k,v in checks.items():
    print(('PASS' if v else 'FAIL'), k)
if not all(checks.values()): raise SystemExit(1)
print('PASS Mesh voice SD flush/watchdog guard')
