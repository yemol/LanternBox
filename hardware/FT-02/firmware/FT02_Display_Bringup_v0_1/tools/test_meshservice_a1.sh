#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${TMPDIR:-/tmp}/ft02_mesh_protocol_test"
g++ -std=c++17 -Wall -Wextra -Werror \
  "$ROOT/src/FT02_MeshProtocol.cpp" \
  "$ROOT/tools/host_meshservice/test_mesh_protocol.cpp" \
  -I"$ROOT/src" -o "$BIN"
"$BIN"
