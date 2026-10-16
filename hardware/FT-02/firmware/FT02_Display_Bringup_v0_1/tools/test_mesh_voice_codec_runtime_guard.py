from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(errors='ignore')
codec=(root/'src/FT02_MeshVoiceCodec.cpp').read_text(errors='ignore')
audio=(root/'src/FT02_MeshVoiceAudio.cpp').read_text(errors='ignore')
checks={
 'loop_stack_30k':'SET_LOOP_TASK_STACK_SIZE(30 * 1024)' in main,
 'codec_rtc_magic':'FT02_CODEC_TRACE_MAGIC' in codec and 'RTC_NOINIT_ATTR' in codec,
 'create_begin':'FT02_CODEC_TRACE_CREATE_BEGIN' in codec,
 'create_done':'FT02_CODEC_TRACE_CREATE_DONE' in codec,
 'frame_begin':'FT02_CODEC_TRACE_FRAME_BEGIN' in codec,
 'frame_done':'FT02_CODEC_TRACE_FRAME_DONE' in codec,
 'boot_report':'FT02_MeshVoiceCodecReportResetTrace();' in audio,
 'internal_heap_diag':'codecInternalLargest()' in codec and 'codecInternalFree()' in codec,
}
bad=[k for k,v in checks.items() if not v]
for k,v in checks.items(): print(('PASS' if v else 'FAIL'), k)
if bad: raise SystemExit(1)
print('PASS Mesh voice speech-codec runtime guard')
