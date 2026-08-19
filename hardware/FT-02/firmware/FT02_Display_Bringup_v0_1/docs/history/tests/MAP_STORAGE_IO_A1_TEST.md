# FT-02 v2.74t Map Storage I/O A1 test

## Purpose
Measure and reduce raw `.osm.pbf` I/O overhead without changing the verified SD electrical profile.

## SD profile remains frozen
- FSPI
- SPI Mode 3
- 40 MHz
- SCK 42 / MOSI 2 / MISO 1 / CS 41
- CMD18 multi-block read
- 16 KiB transfer chunks

## Test A: existing regional cache
Open a location whose `.osm.pbc5.<cell>` already exists.
Expected: `SD cache hit` or `RAM cache hit`. A raw `[MAP-IO-A1]` line may not appear because the raw PBF is not needed.

## Test B: new region / forced cache miss
Choose a region that has no `.osm.pbc5.<cell>` cache, or remove only that target regional cache file.
Wait for the first build to complete and capture:

```text
[MAP-IO-A1] raw_pbf blocks=... seeks=... reads=... bytes=... seek=... ms read=... ms inflate=... ms node_pass=... ms way_pass=... ms build=... ms
```

### Acceptance
- `seeks` should be approximately equal to `blocks`.
- `reads` should be approximately equal to `blocks`.
- There must no longer be the old extra tiny prefix read / second seek for each block.
- Map output and generated regional cache must remain identical in function.

## Interpretation
- Large `read ms`: continue storage/read-ahead optimization.
- Small `read ms`, large `inflate ms`: optimize decompression.
- Small `read/inflate`, large `way_pass`: improve spatial indexing or prebuild regional caches on the Mac.
