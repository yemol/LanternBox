#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CAD="$ROOT/cad"; OUT="$ROOT/output"; LOG="$ROOT/logs"
NAME="FT02-LR01_v1.1_Fabrication_Release"
SCH="$CAD/$NAME.kicad_sch"; PCB="$CAD/$NAME.kicad_pcb"
KICAD="/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
[ -x "$KICAD" ] || KICAD="$(command -v kicad-cli || true)"
[ -x "$KICAD" ] || { echo "kicad-cli not found"; exit 2; }
[ -f "$SCH" ] && [ -f "$PCB" ] || { echo "Production SCH/PCB missing"; exit 3; }
mkdir -p "$OUT/gerber" "$OUT/drill" "$OUT/pdf" "$LOG"

"$KICAD" sch erc --output "$LOG/ERC.txt" --severity-all --exit-code-violations "$SCH"
"$KICAD" pcb drc --output "$LOG/DRC.txt" --severity-all --schematic-parity --refill-zones --save-board --exit-code-violations "$PCB"

"$KICAD" sch export pdf --output "$OUT/${NAME}_Schematic.pdf" "$SCH"
"$KICAD" sch export bom --output "$OUT/FT02-LR01_v1.1_BOM.csv" --exclude-dnp \
  --fields "Reference,Value,Footprint,Manufacturer,MPN,JLCPCB,DNP,Notes" "$SCH"

rm -rf "$OUT/gerber"; mkdir -p "$OUT/gerber"
"$KICAD" pcb export gerbers --output "$OUT/gerber" --board-plot-params --check-zones "$PCB"

rm -rf "$OUT/drill"; mkdir -p "$OUT/drill"
"$KICAD" pcb export drill --output "$OUT/drill" --format excellon --generate-map --map-format pdf \
  --generate-report --report-path "$OUT/drill/Drill_Report.txt" "$PCB"

"$KICAD" pcb export pos --output "$OUT/FT02-LR01_v1.1_CPL.csv" --side front --format csv \
  --units mm --smd-only --exclude-dnp "$PCB"

"$KICAD" pcb export pdf --output "$OUT/${NAME}_PCB.pdf" \
  --layers "F.Cu,F.Mask,F.Silkscreen,Edge.Cuts" \
  --mode-single --check-zones "$PCB"

if command -v sips >/dev/null 2>&1; then
  sips -s format png "$OUT/${NAME}_PCB.pdf" --out "$OUT/FT02-LR01_v1.1_PCB_Preview.png" >/dev/null 2>&1 || true
fi

"$KICAD" pcb export stats --output "$OUT/BOARD_STATS.txt" --format report --units mm "$PCB"
"$KICAD" pcb export ipcd356 --output "$OUT/${NAME}.d356" "$PCB"

echo "ERC/DRC/export automation passed."
echo "Overall release still requires RF/orientation/BOM/CPL/firmware audits."
