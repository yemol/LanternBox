#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/stub"
cat > "$TMP/stub/Arduino.h" <<'EOF'
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct FakeSerialType { template <typename... A> void printf(const char*, A...) {} template <typename T> void print(T) {} template <typename T> void println(T) {} void println() {} template <typename... A> void write(A...) {} void flush() {} };
extern FakeSerialType Serial;
unsigned long millis();
inline void yield() {}
EOF

g++ -std=gnu++17 -fsyntax-only \
  -I"$TMP/stub" \
  -I"$ROOT/src" \
  -I"$ROOT/lib/FT02Log/src" \
  -I"$ROOT/lib/FT02Storage/src" \
  "$ROOT/src/FT02_MeshVoiceAudio.cpp"
echo "PASS MeshVoiceAudio orchestration syntax"
