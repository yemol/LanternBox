# FT-02 Map Pack Builder A2 Test

## Goal
Eliminate the 6-7 second first-use `.pbi` build on FT-02 by generating a firmware-compatible FTPBI1 index beside every source tile on the Mac.

## Existing Shanghai tiles: fastest upgrade
If the Z11 PBF tiles are already on the SD card, do **not** extract them again. Run:

```bash
python3 tools/build_ft02_map_source_tiles.py \
  /path/to/shanghai-260726.osm.pbf \
  --region shanghai \
  --bbox 120.80,30.60,122.20,31.90 \
  --output-root /Volumes/YOUR_SD/maps/regions \
  --zoom 11 \
  --index-only
```

`--index-only` does not read the monolithic source PBF. The positional source argument is kept for command compatibility.

Expected files for every tile:

```text
z11_x1715_y836.osm.pbf
z11_x1715_y836.osm.pbi
```

The packer also writes:

```text
/maps/regions/shanghai/tiles.tsv
```

with PBF/PBI size, source signature and index entry count for each pair.

## Fresh pack
For a fresh Shanghai pack:

```bash
brew install osmium-tool
python3 tools/build_ft02_map_source_tiles.py \
  /path/to/shanghai-260726.osm.pbf \
  --region shanghai \
  --bbox 120.80,30.60,122.20,31.90 \
  --output-root /Volumes/YOUR_SD/maps/regions \
  --zoom 11
```

A2 extracts each source tile and then builds its `.osm.pbi` on the host.

## Device acceptance
Visit a source tile that has never been used to build a `.pbc5` regional cache.

Expected:

```text
[MAP-SOURCE-A2] ... tile pair pbf=1 pbi=1 mode=host-prebuilt
[PBF-A1] persistent index loaded
```

Must **not** appear during normal use:

```text
[PBF-A1] index missing; first build required
[PBF-A1] persistent index build begin
```

The device-side rebuild remains as recovery if a `.pbi` is missing or invalid.
