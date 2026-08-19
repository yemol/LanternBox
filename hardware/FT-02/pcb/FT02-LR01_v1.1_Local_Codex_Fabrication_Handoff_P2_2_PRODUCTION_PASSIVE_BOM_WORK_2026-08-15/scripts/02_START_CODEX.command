#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PROMPT='Read AGENTS.md and CODEX_MASTER_TASK.md. Execute the FT02-LR01 v1.1 manufacturing closure end-to-end. Do not add or redesign any features. Preserve the Semtech E512V01A RF reference exactly. Do not mark fabrication release PASS unless every mandatory gate passes.'
command -v codex >/dev/null 2>&1 || { echo "Codex not found. Install with: curl -fsSL https://chatgpt.com/codex/install.sh | sh"; exit 2; }
if [ ! -d .git ]; then
  git init
  git add .
  git commit -m "FT02-LR01 v1.1 fabrication handoff" || true
fi
if command -v pbcopy >/dev/null 2>&1; then
  printf "%s" "$PROMPT" | pbcopy
  echo "Task copied. When Codex opens: Command+V, then Enter."
else
  echo "$PROMPT"
fi
exec codex
