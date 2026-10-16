# PlatformIO pre-build helper for the Arduino libgsm layout.
# arduino-libgsm keeps public headers in inc/ instead of src/.
Import("env")
from pathlib import Path

libdeps_root = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
expected = libdeps_root / "arduino-libgsm" / "inc"
# Append the deterministic VCS package path even if dependency installation has
# not materialized it yet at pre-script evaluation time.
env.Append(CPPPATH=[str(expected)])
if libdeps_root.exists():
    for child in libdeps_root.iterdir():
        inc = child / "inc"
        if inc != expected and (inc / "gsm.h").exists():
            env.Append(CPPPATH=[str(inc)])
