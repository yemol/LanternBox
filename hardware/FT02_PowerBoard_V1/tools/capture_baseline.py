"""Record a read-only source baseline without staging pre-existing user changes."""

import hashlib
import json
from pathlib import Path
import subprocess
from datetime import datetime, timezone


ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parents[1] / "review/git_baseline.json"


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args])


def main():
    if OUT.exists():
        raise SystemExit("Baseline already exists; refusing to overwrite historical evidence")
    rows = []
    for record in git("status", "--porcelain=v1", "-z").decode().split("\0"):
        if not record:
            continue
        status, name = record[:2], record[3:]
        if name.startswith("hardware/FT02_PowerBoard_V1/"):
            continue
        path = ROOT / name
        rows.append({"status": status, "path": name,
                     "sha256": hashlib.sha256(path.read_bytes()).hexdigest()
                     if path.is_file() else None})
    sources = [
        "AGENTS.md ", "CODEX_NOTES.md",
        "hardware/FT-02/docs/FT02_hardware_pin_map_v1_1.md",
        "hardware/FT-02/docs/FT02_hardware_research_new_chat_brief_v1_1.md",
        "hardware/FT-02/firmware/FT02_Display_Bringup_v0_1/docs/FT02_MAX17048_BATTERY_MONITOR.md",
        "hardware/FT-02/firmware/FT02_Display_Bringup_v0_1/AUDIO_ON_DEMAND_A1.md",
        "hardware/CardKB2_FT02_LOW_POWER_A2_DFS20_IDF61/POWER_TEST.md",
    ]
    data = {
        "captured_utc": datetime.now(timezone.utc).isoformat(),
        "repository": str(ROOT), "head": git("rev-parse", "HEAD").decode().strip(),
        "branch": git("branch", "--show-current").decode().strip(),
        "pre_existing_status": rows,
        "source_hashes": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources},
        "scope": "Existing user changes are not included in PowerBoard commits.",
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")
    print(f"Recorded HEAD {data['head']}; {len(rows)} existing status entries")


if __name__ == "__main__":
    main()
