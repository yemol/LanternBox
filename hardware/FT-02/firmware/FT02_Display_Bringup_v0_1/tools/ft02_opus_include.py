# PlatformIO pre-build helper for PCMFlowOpus' vendored libopus headers.
# PCMFlowOpus keeps the standard C API below src/external/opus/include.
Import("env")
from pathlib import Path

libdeps_root = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
expected = libdeps_root / "PCMFlowOpus" / "src" / "external" / "opus" / "include"
env.Append(CPPPATH=[str(expected)])

# VCS package directory names can vary. Discover any installed copy by opus.h
# rather than depending on PlatformIO's package folder spelling.
if libdeps_root.exists():
    for child in libdeps_root.iterdir():
        candidates = [
            child / "src" / "external" / "opus" / "include",
            child / "external" / "opus" / "include",
        ]
        for inc in candidates:
            if inc != expected and (inc / "opus.h").exists():
                env.Append(CPPPATH=[str(inc)])
