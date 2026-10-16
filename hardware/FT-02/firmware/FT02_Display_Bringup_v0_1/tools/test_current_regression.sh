#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
# Shared logging header must be a PlatformIO local library dependency, not project-only include/.
test -f lib/FT02Log/src/FT02_Log.h || { echo "FAIL: FT02Log shared header missing"; exit 1; }
test -f lib/FT02Log/library.json || { echo "FAIL: FT02Log library manifest missing"; exit 1; }
test ! -f include/FT02_Log.h || { echo "FAIL: stale project-only include/FT02_Log.h present"; exit 1; }
for f in lib/FT02Storage/src/FT02_Storage.cpp lib/FT02FieldManual/src/FT02_FieldManual.cpp lib/FT02PbfIndex/src/FT02_PbfIndex.cpp lib/FT02PbfMapRuntime/src/FT02_PbfMapRuntime.cpp; do
  grep -q '#include "FT02_Log.h"' "$f" || { echo "FAIL: shared log include missing in $f"; exit 1; }
done
echo "FT02Log shared-library visibility: PASS"
python3 tools/test_stable_r1_cleanup.py
python3 tools/test_docs_consolidation.py
python3 tools/test_submit_hygiene.py
python3 tools/test_epd_swspi_a1.py
python3 tools/test_epd_swspi_overlay.py
python3 tools/test_mesh_hwspi_a1_wiring.py
python3 tools/test_mesh_tx_a1_wiring.py
python3 tools/test_mesh_tx_a1_crypto.py
python3 tools/test_mesh_p2p_nodedb_a1.py
python3 tools/test_mesh_nodeinfo_identity_a1515d.py
bash tools/test_mesh_p2p_protocol_host.sh
bash tools/test_mesh_p2p_pki_syntax.sh
bash tools/test_mesh_hwspi_a1_syntax.sh
bash tools/test_mesh_tx_a1_service_syntax.sh
bash tools/test_mesh_tx_a1_comm_syntax.sh
bash tools/test_mesh_message_store_host.sh
bash tools/test_mesh_voice_protocol_host.sh
python3 tools/test_mesh_voice_quality_mode.py
python3 tools/test_mesh_voice_start_cue.py
bash tools/test_mesh_voice_transport_syntax.sh
bash tools/test_mesh_voice_codec_syntax.sh
bash tools/test_mesh_voice_audio_syntax.sh
python3 tools/test_mesh_voice_stack_ui_guard.py
python3 tools/test_mesh_voice_stop_handoff_guard.py
python3 tools/test_mesh_voice_tx_ui_closeout.py
python3 tools/test_mesh_voice_tx_progress_partial.py
python3 tools/test_mesh_voice_a1513_send_ui_private_fix.py
python3 tools/test_mesh_comm_ui_cleanup_a1514.py
python3 tools/test_mesh_comm_ui_t_only_a1514a.py
python3 tools/test_mesh_comm_footer_grouping_a1514b.py
python3 tools/test_mesh_comm_filter_voice_ui_a1515.py
python3 tools/test_mesh_voice_ui_scale_a1515a.py
python3 tools/test_mesh_voice_cjk_scale_a1515c.py
python3 tools/test_comm_ui_log_header_guard.py
python3 tools/test_mesh_psram_memory_layout.py
python3 tools/test_audio_regression.py
python3 tools/test_audio_power_lifecycle.py
python3 tools/test_battery_monitor.py
python3 tools/test_input_epd_regression.py
python3 tools/test_pinyin_a5_engine.py
python3 tools/test_pinyin_a5_direct_toggle_regression.py
python3 tools/test_keyboard_adaptive_a2.py
python3 tools/test_mode_sleep_a1_6.py
python3 tools/test_sleep_power_a4.py
python3 tools/test_keyboard_wake_bridge_a4.py
python3 tools/test_sleep_scl_ext1_rc3.py
python3 tools/test_sleep_state_machine_rc3.py
python3 tools/test_sleep_power_a5a_gnss_standby_only.py
python3 tools/test_sleep_power_a5b_sd_bus_quiesce.py
python3 tools/test_sleep_power_a5c_sleep_truth.py
python3 tools/test_sleep_power_a5c_stable_r1.py
python3 tools/test_second_sleep_closure_r12.py
python3 tools/test_sleep_energy_measurement_r13.py
python3 tools/test_mesh_sleep_irq_lifecycle_rc2.py
python3 tools/test_system_health_a2.py
python3 tools/test_compass_gnss_control.py
python3 tools/test_function_surface_audit.py
python3 tools/test_field_library_v082.py
python3 tools/test_low_power_a2.py
bash tools/test_meshservice_a1.sh
python3 tools/test_stable_r2_status_ui.py

python3 tools/test_session_isolation_r14.py
python3 tools/test_power_status_page_r15.py
python3 tools/test_mesh_diag_resync_closeout_a1515e.py
printf '\nCURRENT REGRESSION: PASS\n'
