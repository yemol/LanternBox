#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(errors='ignore')
ui=(root/'src/FT02_CommunicationNodeUI.cpp').read_text(errors='ignore')
uih=(root/'src/FT02_CommunicationNodeUI.h').read_text(errors='ignore')
proto=(root/'src/FT02_MeshVoiceProtocol.h').read_text(errors='ignore')
codec=(root/'src/FT02_MeshVoiceCodec.cpp').read_text(errors='ignore')
audio=(root/'src/FT02_AudioLog.cpp').read_text(errors='ignore')

def req(x,msg):
    if not x: raise SystemExit('FAIL: '+msg)

req('SET_LOOP_TASK_STACK_SIZE(30 * 1024)' in main, 'Codec2 loop stack must remain 30 KiB')
req('`切英文' not in ui and '`切中文' not in ui and '` 中英' not in ui, 'legacy backtick IME hint remains in Communication UI')
req('长按SYM切英文' in ui and '长按SYM切中文' in ui, 'direct-toggle hints missing')
req('FT02_VOICE_CHUNK_BYTES = 192u' in proto, 'voice chunk must retain PKI/Mesh headroom')
req('FT02_VOICE_WIRE_MAX_BYTES = 204u' in proto, 'wire max mismatch')
req('codecRuntimeHeadroom("encode-begin")' in codec, 'codec stack/heap diagnostics missing')

req('FT02_COMM_INPUT_REDRAW_THEN_START_VOICE' in uih, 'voice cue-before-capture input result missing')
handler=main[main.index('static bool FT02_HandleCommunicationInput'):main.index('static bool FT02_HandleHomeInput')]
voice_result=handler[handler.index('FT02_COMM_INPUT_REDRAW_THEN_START_VOICE'):]
req('FT02_RedrawCurrentPage();' in voice_result and 'FT02_CommunicationUIStartPendingVoiceCapture()' in voice_result,
    'main must commit voice cue before capture')
req(voice_result.index('FT02_RedrawCurrentPage();') < voice_result.index('FT02_CommunicationUIStartPendingVoiceCapture()'),
    'voice capture starts before EPD cue commit')

v_start=ui.index('FT02CommunicationInputResult beginVoice')
v_end=ui.index('const char* nodeName', v_start)
v_block=ui[v_start:v_end]
req('g_voiceStartPending = true' in v_block, 'voice start request does not arm deferred capture')
req('FT02_MeshVoiceAudioStartRecording(destination)' not in v_block,
    'voice input path still starts capture before redraw')

transient=audio[audio.index('bool FT02_AudioLogStartTransientRecording'):audio.index('bool FT02_AudioLogRequestTransientStop')]
service=audio[audio.index('static void FT02_AudioServiceTask(void*)'):audio.index('bool FT02_AudioLogBegin()')]
req('[AUDIO-MESH] arm begin' in transient, 'transient capture diagnostics missing')
req('(g_ft02AudioOwnerMask & FT02_AUDIO_OWNER_MESH_VOICE) == 0u' in service,
    'audio task does not gate idle RX while Mesh voice is arming')
req('FT02_AudioDrainIdleRx();' in service, 'idle RX drain unexpectedly removed')
req(transient.index('const esp_err_t error = i2s_channel_read(') < transient.index('g_ft02AudioState = FT02_AUDIO_LOG_RECORDING;'),
    'transient pre-start drain must finish before RECORDING state')

voice_input_start=ui.index('if(g_mode == COMM_VOICE)', ui.index('FT02_CommunicationUIHandleInput'))
voice_input_end=ui.index('const bool filterShortcut', voice_input_start)
voice_mode=ui[voice_input_start:voice_input_end]
req('FT02_MeshVoiceAudioStopRecording();' in voice_mode and 'return FT02_COMM_INPUT_NONE;' in voice_mode,
    'stop path may still force an e-paper refresh during post-roll')

print('PASS Mesh Voice stack/UI/I2S guard')
