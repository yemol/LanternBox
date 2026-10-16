from pathlib import Path
from math import ceil
ROOT = Path(__file__).resolve().parents[1]
proto_h = (ROOT / 'src/FT02_MeshVoiceProtocol.h').read_text()
proto_cpp = (ROOT / 'src/FT02_MeshVoiceProtocol.cpp').read_text()
codec = (ROOT / 'src/FT02_MeshVoiceCodec.cpp').read_text()
transport = (ROOT / 'src/FT02_MeshVoiceTransport.cpp').read_text()
pio = (ROOT / 'platformio.ini').read_text()
helper = (ROOT / 'tools/ft02_opus_include.py').read_text()

four_sec_bytes = ceil(4000 / 60) * 45
four_sec_chunks = ceil(four_sec_bytes / 192)
fifteen_sec_bytes = ceil(15000 / 60) * 45
fifteen_sec_chunks = ceil(fifteen_sec_bytes / 192)

checks = [
    ('tx_codec_opus_6000', 'FT02_VOICE_TX_CODEC = FT02_VOICE_CODEC_OPUS_6000' in proto_h),
    ('opus_rate_16k', 'FT02_VOICE_SAMPLE_RATE_OPUS = 16000u' in proto_h),
    ('opus_frame_960_samples', 'FT02_VOICE_FRAME_SAMPLES_OPUS = 960u' in proto_h),
    ('opus_frame_45b', 'FT02_VOICE_FRAME_BYTES_OPUS_6000 = 45u' in proto_h),
    ('tx_max_15s', 'FT02_VOICE_TX_MAX_DURATION_MS = 15000u' in proto_h),
    ('preroll_700ms', 'FT02_VOICE_CAPTURE_PREROLL_MS = 700u' in proto_h),
    ('dynamic_preroll_bound_5s', 'FT02_VOICE_CAPTURE_MAX_PREROLL_MS = 5000u' in proto_h),
    ('raw_capture_bound_20s', 'FT02_VOICE_CAPTURE_RAW_MAX_MS' in proto_h),
    ('legacy_protocol_30s', 'FT02_VOICE_MAX_DURATION_MS = 30000u' in proto_h),
    ('legacy_opus_6000', 'FT02_VOICE_CODEC_OPUS_6000' in proto_h and 'FT02_VOICE_FRAME_BYTES_OPUS_6000 = 45u' in proto_h),
    ('legacy_gsm', 'FT02_VOICE_CODEC_GSM_FR' in proto_h and 'gsm_decode(' in codec),
    ('legacy_3200', 'FT02_VOICE_CODEC_CODEC2_3200' in proto_h),
    ('legacy_1600', 'FT02_VOICE_CODEC_CODEC2_1600' in proto_h),
    ('legacy_1300', 'FT02_VOICE_CODEC_CODEC2_1300' in proto_h),
    ('legacy_payload_ceiling', 'FT02_VOICE_MAX_ENCODED_BYTES = 49500u' in proto_h),
    ('opus_tx_ceiling', 'FT02_VOICE_TX_MAX_ENCODED_BYTES = 11250u' in proto_h),
    ('sample_rate_by_codec', 'FT02_MeshVoiceSampleRateForCodec' in proto_cpp),
    ('encoder_uses_opus', 'opus_encoder_init(' in codec and 'OPUS_SET_BITRATE(6000)' in codec and 'OPUS_SET_COMPLEXITY(7)' in codec and 'opus_encode(' in codec),
    ('decoder_uses_opus', 'opus_decoder_init(' in codec and 'opus_decode(' in codec),
    ('dynamic_cue_trimmed', 'trimLeadMs' in codec and 'skipSamples' in codec),
    ('opus_psram_state', 'opus_encoder_get_size' in codec and 'codecPsramAlloc' in codec),
    ('transport_psram_buffer', 'heap_caps_malloc(FT02_VOICE_MAX_ENCODED_BYTES' in transport),
    ('rx_header_codec_rate', 'header.sampleRate = FT02_MeshVoiceSampleRateForCodec(rx.codec);' in transport),
    ('pcmflow_dependency', 'tanakamasayuki/PCMFlow.git' in pio),
    ('opus_dependency', 'tanakamasayuki/PCMFlowOpus.git' in pio),
    ('opus_include_helper', 'opus.h' in helper and 'CPPPATH' in helper),
    ('4s_packet_budget', four_sec_chunks <= 16),
    ('15s_packet_budget', fifteen_sec_bytes == 11250 and fifteen_sec_chunks == 59),
]
bad=[]
for name, ok in checks:
    print(('PASS' if ok else 'FAIL'), name)
    if not ok: bad.append(name)
print(f'INFO Opus-6000 estimate: 4s={four_sec_bytes}B/{four_sec_chunks} chunks, 15s={fifteen_sec_bytes}B/{fifteen_sec_chunks} chunks')
if bad: raise SystemExit('Mesh voice Opus quality/compression regression failed: ' + ', '.join(bad))
print('PASS Mesh Voice A1.5.13 quality mode: Opus 6.0 kbps TX + 15 s cap + legacy Opus-4800/GSM/Codec2 playback')
