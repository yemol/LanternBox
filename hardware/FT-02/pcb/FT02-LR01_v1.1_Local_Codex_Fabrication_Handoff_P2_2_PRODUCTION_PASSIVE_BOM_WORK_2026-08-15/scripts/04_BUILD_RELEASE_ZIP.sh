#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; OUT="$ROOT/output"
NAME="FT02-LR01_v1.1_Fabrication_Release"
GATE="$OUT/FABRICATION_RELEASE_GATE.md"
[ -f "$GATE" ] || { echo "Missing release gate"; exit 2; }
grep -Eq '^Overall:[[:space:]]*PASS[[:space:]]*$' "$GATE" || { echo "Gate not PASS; refusing release ZIP"; exit 3; }
cd "$ROOT"
rm -f "$OUT/$NAME.zip"
/usr/bin/zip -r "$OUT/$NAME.zip" cad output logs spec/LOCKED_BOM.csv spec/LOCKED_GPIO_CONTRACT.csv spec/LOCKED_FPC_CONTRACT.csv spec/LOCKED_GNSS_5PIN.csv >/dev/null
echo "Created: $OUT/$NAME.zip"
