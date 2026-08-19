#!/bin/bash
set -u
KICAD="/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
fail=0
for cmd in git python3 codex; do
  if command -v "$cmd" >/dev/null 2>&1; then echo "[OK] $cmd: $(command -v "$cmd")"; else echo "[MISSING] $cmd"; fail=1; fi
done
if [ -x "$KICAD" ]; then
  echo "[OK] kicad-cli: $KICAD"
  "$KICAD" version || true
elif command -v kicad-cli >/dev/null 2>&1; then
  echo "[OK] kicad-cli: $(command -v kicad-cli)"
  kicad-cli version || true
else
  echo "[MISSING] kicad-cli"
  echo "Expected: $KICAD"
  fail=1
fi
if [ "$fail" -ne 0 ]; then
  echo
  echo "Install Codex CLI:"
  echo "curl -fsSL https://chatgpt.com/codex/install.sh | sh"
  echo "Install KiCad 10 stable, then rerun."
  exit 2
fi
echo "[OK] Environment ready."
echo "Next: bash scripts/02_START_CODEX.command"
