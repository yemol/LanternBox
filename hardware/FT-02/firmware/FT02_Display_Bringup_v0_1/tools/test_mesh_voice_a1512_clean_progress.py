from pathlib import Path
root=Path(__file__).resolve().parents[1]
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text()
audio=(root/'src/FT02_MeshVoiceAudio.cpp').read_text()
codec=(root/'src/FT02_MeshVoiceCodec.cpp').read_text()
proto=(root/'src/FT02_MeshVoiceProtocol.h').read_text()

def req(c,m):
    if not c: raise SystemExit('FAIL '+m)
req('snprintf(timer, sizeof(timer), "发送进度")' in ui, 'TX header must be static to prevent e-paper digit ghosting')
req('"分片 %03u/%03u   %03u%%"' in ui, 'TX numeric row must use fixed-width digits')
req('FT02_CommunicationCommitPartial(display, 120, 232, 560, 190);' in ui, 'TX partial window must cover timer/bar/numeric row')
req('FT02_AudioLogStartTransientRecording' in audio, 'Mesh voice must reuse AudioLog transient capture')
req('FT02_VOICE_TX_CODEC = FT02_VOICE_CODEC_OPUS_6000' in proto, 'TX must restore proven Opus 6000 quality')
req('OPUS_SET_BITRATE(6000)' in codec and 'OPUS_SET_COMPLEXITY(7)' in codec, 'Opus quality settings mismatch')
req('FT02_VOICE_CHUNK_BYTES = 192u' in proto and 'FT02_VOICE_WIRE_MAX_BYTES = 204u' in proto, 'private-safe fragmentation mismatch')
print('PASS A1.5.13 AudioLog/Opus6000 + private-safe fragmentation')
