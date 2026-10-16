from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
store = (ROOT / 'src/FT02_MeshMessageStore.cpp').read_text()
codec = (ROOT / 'src/FT02_MeshVoiceCodec.cpp').read_text()
board = (ROOT / 'boards/esp32-s3-devkitc1-n16r8.json').read_text()
transport = (ROOT / 'src/FT02_MeshVoiceTransport.cpp').read_text()

checks = []
checks.append(('board_has_psram', 'BOARD_HAS_PSRAM' in board and '"psram_type": "opi"' in board))
checks.append(('history_not_static_bss', 'DiskRecord g_records[FT02_MESH_HISTORY_LIMIT]' not in store))
checks.append(('history_psram_alloc', 'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT' in store and 'heap_caps_calloc' in store))
checks.append(('codec_large_globals_removed', not re.search(r'\bg_(captureFrame|codecPcm|playFrame|encoded)\s*\[', codec)))
checks.append(('codec_scratch_psram', 'CODEC_SCRATCH_BYTES' in codec and 'heap_caps_malloc' in codec and 'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT' in codec))
checks.append(('codec_scratch_freed', codec.count('freeScratch(scratch)') >= 8))
checks.append(('transport_payload_not_static_bss', 'uint8_t data[FT02_VOICE_MAX_ENCODED_BYTES]' not in transport))
checks.append(('transport_payload_psram', 'heap_caps_malloc(FT02_VOICE_MAX_ENCODED_BYTES' in transport and 'g_rxBuffers' in transport and 'g_txBuffer' in transport))

bad = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(('PASS' if ok else 'FAIL'), name)
if bad:
    raise SystemExit('Mesh PSRAM memory layout regression failed: ' + ', '.join(bad))

# Architectural estimate, packed DiskRecord is 328 B. Opus widened the
# per-frame scratch to 60 ms / 16 kHz while retaining the 49.5 KiB legacy
# encoded area; transport still keeps 2 RX + 1 TX legacy-sized PSRAM windows.
history_bytes = 328 * 100
codec_bytes = 2646*2 + 960*2 + 2646*2 + 49500
transport_bytes = 49500 * 3
print(f'PASS Mesh PSRAM memory layout: ~{history_bytes + codec_bytes + transport_bytes} bytes moved out of static internal DRAM')
