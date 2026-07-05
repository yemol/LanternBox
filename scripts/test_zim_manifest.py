import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ZIM_DIR = ROOT / "data" / "kiwix" / "zim"
MANIFEST_FILE = ROOT / "data" / "kiwix" / "zim_manifest.json"

VALID_ROLES = {"decision", "lookup", "fallback", "support"}
VALID_USAGE_POLICIES = {
    "ai_retrieval_allowed",
    "lookup_only",
    "fallback_only",
    "background_support_only",
    "language_support_only",
}


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)


def load_manifest():
    if not MANIFEST_FILE.exists():
        fail(f"manifest missing: {MANIFEST_FILE}")

    try:
        payload = json.loads(MANIFEST_FILE.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"manifest JSON parse error: {exc}")

    if isinstance(payload, dict):
        payload = payload.get("zim_files", [])
    if not isinstance(payload, list):
        fail("manifest must be a list or an object with zim_files")

    return payload


def main() -> None:
    manifest = load_manifest()
    entries_by_filename = {}

    for index, entry in enumerate(manifest):
        if not isinstance(entry, dict):
            fail(f"manifest entry #{index} is not an object")

        filename = str(entry.get("filename") or "").strip()
        role = str(entry.get("role") or "").strip()
        usage_policy = str(entry.get("usage_policy") or "").strip()

        if not filename:
            fail(f"manifest entry #{index} missing filename")
        if filename in entries_by_filename:
            fail(f"duplicate manifest filename: {filename}")
        entries_by_filename[filename] = entry

        if role not in VALID_ROLES:
            fail(f"unknown role for {filename}: {role}")
        if usage_policy not in VALID_USAGE_POLICIES:
            fail(f"unknown usage_policy for {filename}: {usage_policy}")

        lower = filename.lower()
        if "_maxi_" in lower and usage_policy == "ai_retrieval_allowed":
            fail(f"maxi marked as ai_retrieval_allowed: {filename}")
        if ".stackexchange.com_en_all_" in lower and usage_policy == "ai_retrieval_allowed":
            fail(f"English StackExchange marked as ai_retrieval_allowed: {filename}")
        if "_mini_" in lower and usage_policy == "ai_retrieval_allowed":
            fail(f"mini marked as ai_retrieval_allowed: {filename}")

    zim_files = {path.name for path in ZIM_DIR.glob("*.zim")} if ZIM_DIR.exists() else set()
    manifest_files = set(entries_by_filename)
    missing = sorted(zim_files - manifest_files)
    extra = sorted(manifest_files - zim_files)

    if missing:
        fail("ZIM files missing from manifest: " + ", ".join(missing))
    if extra:
        fail("manifest entries without local ZIM file: " + ", ".join(extra))

    print(
        json.dumps(
            {
                "ok": True,
                "manifest": str(MANIFEST_FILE),
                "zim_count": len(zim_files),
                "manifest_count": len(manifest_files),
            },
            ensure_ascii=False,
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
