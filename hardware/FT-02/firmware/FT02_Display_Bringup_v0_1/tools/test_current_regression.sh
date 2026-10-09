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
python3 tools/test_epd_swspi_a1.py
python3 tools/test_epd_swspi_overlay.py
python3 tools/test_mesh_hwspi_a1_wiring.py
python3 tools/test_mesh_tx_a1_wiring.py
python3 tools/test_mesh_tx_a1_crypto.py
python3 tools/test_mesh_p2p_nodedb_a1.py
bash tools/test_mesh_p2p_protocol_host.sh
bash tools/test_mesh_p2p_pki_syntax.sh
bash tools/test_mesh_hwspi_a1_syntax.sh
bash tools/test_mesh_tx_a1_service_syntax.sh
bash tools/test_mesh_tx_a1_comm_syntax.sh
python3 tools/test_audio_regression.py
python3 tools/test_audio_power_lifecycle.py
python3 tools/test_battery_monitor.py
python3 tools/test_input_epd_regression.py
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
python3 tools/test_mesh_sleep_irq_lifecycle_rc2.py
python3 tools/test_system_health_a2.py
python3 tools/test_compass_gnss_control.py
python3 tools/test_function_surface_audit.py
python3 tools/test_field_library_v082.py
python3 tools/test_low_power_a2.py
bash tools/test_meshservice_a1.sh
python3 tools/test_stable_r2_status_ui.py

printf '\nCURRENT REGRESSION: PASS\n'
