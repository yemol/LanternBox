#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
allowed = {'README.md','ARCHITECTURE.md','TESTING.md','CHANGELOG.md'}
project_md = []
for p in ROOT.rglob('*.md'):
    rel = p.relative_to(ROOT)
    if rel.parts and rel.parts[0] == 'SD_ROOT':
        continue
    project_md.append(str(rel))
if set(project_md) != allowed:
    raise SystemExit('FAIL project markdown set: ' + repr(sorted(project_md)))
# Frozen SD docs stay present; their integrity is checked by FieldLibrary regression.
sd_md = list((ROOT/'SD_ROOT').rglob('*.md'))
if len(sd_md) != 11:
    raise SystemExit(f'FAIL frozen SD markdown count: {len(sd_md)} != 11')
print('PASS docs consolidation: project_md=4 frozen_sd_md=11 total_md=15')
