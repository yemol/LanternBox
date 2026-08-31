#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "[1/4] Python dependencies..."
python3 -m pip install -r tools/requirements_p0.txt

echo "[2/4] Fetching ONLY fully approved A6 P0 sources..."
mkdir -p "$PWD/P0_SOURCE_STAGING"
python3 tools/fetch_p0_sources.py --output "$PWD/P0_SOURCE_STAGING"

echo "[3/4] Building runtime library through A6 P0 Admission Gate..."
python3 tools/build_ft02_field_library.py "$PWD/P0_SOURCE_STAGING" "$PWD/SD_ROOT/library"

echo "[4/4] Validating A6 P0-only library..."
python3 tools/validate_ft02_field_library.py "$PWD/SD_ROOT/library"
