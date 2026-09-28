"""Rebuild, independently verify and export only schematic review artifacts."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

BASE=Path(__file__).resolve().parents[1]
CLI=shutil.which("kicad-cli") or "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
SCH="FT02_PowerModule_V2.kicad_sch"


def main():
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE="1")
    commands=[
        [sys.executable,"tools/build_schematic.py"],
        [CLI,"sch","export","netlist","--format","kicadxml","-o","review/reference.net.xml",SCH],
        [CLI,"sch","erc","--format","json","-o","review/erc.json",SCH],
        [sys.executable,"tools/verify_schematic.py"],
        [sys.executable,"tools/pd_budget.py"],
        [sys.executable,"-m","unittest","discover","-s","tools","-p","test_*.py","-v"],
        [CLI,"sch","export","pdf","-o","review/FT02_PowerModule_V2_PD_Review.pdf",SCH],
    ]
    for command in commands:
        result=subprocess.run(command,cwd=BASE,env=env,text=True,capture_output=True)
        print(result.stdout,result.stderr,flush=True)
        if "unittest" in command:
            (BASE/"review/test_results.txt").write_text(result.stdout+result.stderr)
        if result.returncode:
            raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
