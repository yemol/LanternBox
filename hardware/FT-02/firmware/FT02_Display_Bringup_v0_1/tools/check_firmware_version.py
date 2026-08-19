#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src" / "FT02_BuildInfo.h"
text = HEADER.read_text(encoding="utf-8")
match = re.search(r'#define\s+FT02_FIRMWARE_VERSION\s+"([^"]+)"', text)
if not match:
    print("FAIL: FT02_FIRMWARE_VERSION not found")
    sys.exit(1)
version = match.group(1)

required = {
    ROOT / "src" / "FT02_HomeUI.cpp": "FT02_FIRMWARE_VERSION_LABEL",
    ROOT / "src" / "FT02_LocationRecorder.cpp": "FT02_FIRMWARE_VERSION",
    ROOT / "src" / "main.cpp": "FT02_FIRMWARE_BUILD_LABEL",
}
for path, token in required.items():
    body = path.read_text(encoding="utf-8")
    if token not in body:
        print(f"FAIL: {path.relative_to(ROOT)} does not use {token}")
        sys.exit(1)

legacy = []
for path in (ROOT / "src").glob("*.cpp"):
    body = path.read_text(encoding="utf-8")
    for m in re.finditer(r'(?<![A-Za-z0-9_])v\d+\.\d+[A-Za-z]?', body):
        value = m.group(0)
        # Historical comments/log labels are allowed, but stale UI/runtime literals are not.
        line = body.count("\n", 0, m.start()) + 1
        snippet = body.splitlines()[line - 1]
        if 'Serial.' in snippet or '版本：' in snippet or 'FIRMWARE_VERSION =' in snippet:
            legacy.append(f"{path.name}:{line}: {snippet.strip()}")
if legacy:
    print("FAIL: hard-coded runtime version literals remain")
    print("\n".join(legacy))
    sys.exit(1)

print(f"PASS: firmware version {version} is centralized in FT02_BuildInfo.h")
