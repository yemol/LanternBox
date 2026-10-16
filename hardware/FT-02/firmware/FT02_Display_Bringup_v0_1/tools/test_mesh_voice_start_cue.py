from pathlib import Path
root=Path(__file__).resolve().parents[1]
proto=(root/'src/FT02_MeshVoiceProtocol.h').read_text(errors='ignore')
audio=(root/'src/FT02_MeshVoiceAudio.cpp').read_text(errors='ignore')
audioh=(root/'src/FT02_MeshVoiceAudio.h').read_text(errors='ignore')
codec=(root/'src/FT02_MeshVoiceCodec.cpp').read_text(errors='ignore')
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
uih=(root/'src/FT02_CommunicationNodeUI.h').read_text(errors='ignore')
main=(root/'src/main.cpp').read_text(errors='ignore')
checks=[
 ('15s tx cap', 'FT02_VOICE_TX_MAX_DURATION_MS = 15000u' in proto),
 ('700ms minimum preroll', 'FT02_VOICE_CAPTURE_PREROLL_MS = 700u' in proto),
 ('raw fail-safe supports visible cue+15s', 'FT02_VOICE_CAPTURE_RAW_MAX_MS' in audio and 'FT02_VOICE_CAPTURE_MAX_PREROLL_MS = 5000u' in proto),
 ('dynamic cue trim state', 'g_captureTrimMs' in audio and 'g_userCueArmed' in audio),
 ('effective elapsed starts from real cue', 'rawElapsedMs - g_captureTrimMs' in audio),
 ('15s auto-stop from real cue', 'g_status.elapsedMs >= FT02_VOICE_TX_MAX_DURATION_MS' in audio and 'FT02_AudioLogRequestTransientStop()' in audio),
 ('audio cue mark API', 'FT02_MeshVoiceAudioMarkUserStartCue' in audio and 'FT02_MeshVoiceAudioMarkUserStartCue' in audioh),
 ('codec accepts dynamic trim', 'uint32_t trimLeadMs' in codec and 'skipSamples' in codec),
 ('ui timer 15s', '/ 00:15' in ui and '最长15秒' in ui),
 ('pending tells wait', '请等待“开始说话”提示' in ui),
 ('positive start cue', '● 开始说话' in ui),
 ('cue ui api', 'FT02_CommunicationUIVoiceCaptureCueReady' in ui and 'FT02_CommunicationUIVoiceCaptureCueReady' in uih),
 ('cue partial api', 'FT02_DrawCommunicationVoiceStartCuePartial' in ui and 'FT02_DrawCommunicationVoiceStartCuePartial' in uih),
 ('capture before cue delay', 'FT02_CommunicationUIStartPendingVoiceCapture()' in main and 'delay(FT02_VOICE_CAPTURE_PREROLL_MS)' in main),
 ('cue after minimum preroll', main.index('delay(FT02_VOICE_CAPTURE_PREROLL_MS)') < main.index('FT02_CommunicationUIVoiceCaptureCueReady()')),
 ('real cue timestamp after physical refresh', main.index('FT02_DrawCommunicationVoiceStartCuePartial(display)') < main.index('FT02_MeshVoiceAudioMarkUserStartCue()')),
]
failed=[]
for name,ok in checks:
 print(('PASS' if ok else 'FAIL'),name)
 if not ok: failed.append(name)
if failed: raise SystemExit('FAIL start cue: '+', '.join(failed))
print('PASS Mesh voice 15 s + capture-first preroll + actual cue-bound trim/window')
