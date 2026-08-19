#!/usr/bin/env python3
"""Build FT-02 spatial source tiles for one map region.

Example:
  python3 tools/build_ft02_map_source_tiles.py \
    /path/shanghai-260726.osm.pbf \
    --region shanghai \
    --bbox 120.80,30.60,122.20,31.90 \
    --output-root /Volumes/FT02/maps/regions \
    --zoom 11

A2 also pre-builds the firmware-compatible .osm.pbi beside every tile and
writes a per-region tiles.tsv integrity manifest. Use --index-only to add
indexes to an already-generated tile set without extracting the PBFs again.

Requires the `osmium` command line utility. The script generates overlapped
Web-Mercator source tiles named z<z>_x<x>_y<y>.osm.pbf and updates regions.tsv.
The overlap is intentional: FT-02 regional caches are larger than the visible
screen, so a center close to a source-tile edge still needs neighboring map
objects.
"""
from __future__ import annotations
import argparse, json, math, os, shutil, subprocess, sys, tempfile
from pathlib import Path
from validate_a1_index_format import build as build_pbi, source_signature as pbf_source_signature, HEADER as PBI_HEADER, MAGIC as PBI_MAGIC, VERSION as PBI_VERSION

WEBMERC_MAX_LAT = 85.05112878

def clamp_lat(lat: float) -> float:
    return max(-WEBMERC_MAX_LAT, min(WEBMERC_MAX_LAT, lat))

def lon_to_xf(lon: float, z: int) -> float:
    return (lon + 180.0) / 360.0 * (1 << z)

def lat_to_yf(lat: float, z: int) -> float:
    lat = clamp_lat(lat)
    r = math.radians(lat)
    return (1.0 - math.asinh(math.tan(r)) / math.pi) * 0.5 * (1 << z)

def x_to_lon(x: float, z: int) -> float:
    return x / (1 << z) * 360.0 - 180.0

def y_to_lat(y: float, z: int) -> float:
    n = math.pi - 2.0 * math.pi * y / (1 << z)
    return math.degrees(math.atan(math.sinh(n)))

def tile_range(bbox, z):
    minlon, minlat, maxlon, maxlat = bbox
    x0 = max(0, int(math.floor(lon_to_xf(minlon, z))))
    x1 = min((1 << z)-1, int(math.floor(lon_to_xf(maxlon, z))))
    y0 = max(0, int(math.floor(lat_to_yf(maxlat, z))))
    y1 = min((1 << z)-1, int(math.floor(lat_to_yf(minlat, z))))
    return x0, x1, y0, y1

def expanded_tile_bbox(x: int, y: int, z: int, overlap: float):
    # Expand in tile-coordinate space. This behaves consistently in Web Mercator.
    left = x - overlap
    right = x + 1.0 + overlap
    top = y - overlap
    bottom = y + 1.0 + overlap
    return [x_to_lon(left,z), y_to_lat(bottom,z), x_to_lon(right,z), y_to_lat(top,z)]

def update_manifest(path: Path, region: str, bbox, zoom: int, tile_root_on_sd: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    rows=[]
    if path.exists():
        rows=[ln.rstrip('\n') for ln in path.read_text(encoding='utf-8').splitlines() if ln.strip()]
    header="# region_id\tmin_lon\tmin_lat\tmax_lon\tmax_lat\ttile_zoom\ttile_root"
    rows=[r for r in rows if r.startswith('#') or not r.split('\t',1)[0]==region]
    if not rows or not any(r.startswith('#') for r in rows): rows.insert(0,header)
    minlon,minlat,maxlon,maxlat=bbox
    rows.append(f"{region}\t{minlon:.7f}\t{minlat:.7f}\t{maxlon:.7f}\t{maxlat:.7f}\t{zoom}\t{tile_root_on_sd}")
    path.write_text('\n'.join(rows)+'\n',encoding='utf-8')


def read_pbi_pair(path: Path, pbf: Path):
    if not path.exists() or path.stat().st_size < PBI_HEADER.size:
        return None
    raw = path.read_bytes()[:PBI_HEADER.size]
    fields = PBI_HEADER.unpack(raw)
    magic, version, header_bytes, entry_bytes = fields[:4]
    source_bytes = fields[5]
    source_signature = fields[6]
    entry_count = fields[7]
    if magic != PBI_MAGIC or version != PBI_VERSION or header_bytes != 128 or entry_bytes != 64:
        return None
    expected = 128 + entry_count * 64
    if path.stat().st_size != expected or source_bytes != pbf.stat().st_size:
        return None
    if source_signature != pbf_source_signature(pbf):
        return None
    return {"source_bytes": source_bytes, "source_signature": source_signature, "entries": entry_count, "index_bytes": expected}

def write_tile_manifest(path: Path, rows):
    header = "# z\tx\ty\tpbf\tpbi\tpbf_bytes\tpbi_bytes\tsource_signature\tentries"
    lines=[header]
    for r in rows:
        lines.append(f"{r['z']}\t{r['x']}\t{r['y']}\t{r['pbf']}\t{r['pbi']}\t{r['pbf_bytes']}\t{r['pbi_bytes']}\t0x{r['source_signature']:08X}\t{r['entries']}")
    path.write_text("\n".join(lines)+"\n", encoding="utf-8")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('source_pbf', type=Path)
    ap.add_argument('--region', required=True)
    ap.add_argument('--bbox', required=True, help='minLon,minLat,maxLon,maxLat')
    ap.add_argument('--output-root', type=Path, required=True, help='directory corresponding to /maps/regions on SD')
    ap.add_argument('--zoom', type=int, default=11)
    ap.add_argument('--overlap', type=float, default=0.08, help='fraction of one source tile added around each tile')
    ap.add_argument('--batch-size', type=int, default=8)
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--index-only', action='store_true', help='do not re-extract tiles; build/verify .pbi for existing tile PBFs')
    ap.add_argument('--force-index', action='store_true', help='rebuild .pbi even when an apparently matching pair exists')
    args=ap.parse_args()
    if shutil.which('osmium') is None and not args.dry_run and not args.index_only:
        sys.exit('ERROR: osmium command not found. macOS: brew install osmium-tool')
    if not args.source_pbf.exists() and not args.index_only and not args.dry_run: sys.exit(f'ERROR: source not found: {args.source_pbf}')
    try: bbox=tuple(float(v) for v in args.bbox.split(','))
    except Exception: sys.exit('ERROR: --bbox must be minLon,minLat,maxLon,maxLat')
    if len(bbox)!=4 or bbox[0]>=bbox[2] or bbox[1]>=bbox[3]: sys.exit('ERROR: invalid bbox')
    if args.zoom < 8 or args.zoom > 14: sys.exit('ERROR: --zoom expected 8..14')
    if not (0.0 <= args.overlap <= 0.25): sys.exit('ERROR: --overlap expected 0..0.25')

    region_dir=args.output_root/args.region
    tiles_dir=region_dir/'tiles'
    tiles_dir.mkdir(parents=True,exist_ok=True)
    x0,x1,y0,y1=tile_range(bbox,args.zoom)
    jobs=[]
    for y in range(y0,y1+1):
        for x in range(x0,x1+1):
            out=f"z{args.zoom}_x{x}_y{y}.osm.pbf"
            jobs.append((x,y,out,expanded_tile_bbox(x,y,args.zoom,args.overlap)))
    print(f'[MAP-PACK-A2] region={args.region} z={args.zoom} tiles={len(jobs)} x={x0}..{x1} y={y0}..{y1} overlap={args.overlap:.3f}')

    if not args.dry_run and not args.index_only:
        for start in range(0,len(jobs),max(1,args.batch_size)):
            batch=jobs[start:start+max(1,args.batch_size)]
            cfg={'directory':str(tiles_dir),'extracts':[]}
            for x,y,out,b in batch:
                cfg['extracts'].append({'output':out,'description':f'{args.region} z{args.zoom} x{x} y{y}','bbox':b})
            with tempfile.NamedTemporaryFile('w',suffix='.json',delete=False,encoding='utf-8') as tf:
                json.dump(cfg,tf,ensure_ascii=False,indent=2)
                cfg_path=tf.name
            try:
                cmd=['osmium','extract','--config',cfg_path,'--strategy','complete_ways','--option','relations=false','--set-bounds','--overwrite',str(args.source_pbf)]
                print(f'[MAP-PACK-A2] batch {start+1}-{start+len(batch)}/{len(jobs)}')
                subprocess.run(cmd,check=True)
            finally:
                os.unlink(cfg_path)

    tile_manifest_rows=[]
    if not args.dry_run:
        for n,(x,y,out,b) in enumerate(jobs, start=1):
            pbf_path=tiles_dir/out
            pbi_path=pbf_path.with_suffix('.pbi')
            if not pbf_path.exists():
                if args.index_only:
                    print(f'[MAP-PACK-A2] missing tile skipped {pbf_path.name}')
                    continue
                sys.exit(f'ERROR: extracted tile missing: {pbf_path}')
            pair=None if args.force_index else read_pbi_pair(pbi_path,pbf_path)
            if pair is None:
                print(f'[MAP-PACK-A2] build index {n}/{len(jobs)} {pbf_path.name}')
                report=build_pbi(pbf_path,pbi_path)
                pair={
                    'source_bytes': report['source_bytes'],
                    'source_signature': int(report['source_signature'],16),
                    'entries': report['entries'],
                    'index_bytes': report['index_bytes'],
                }
            else:
                print(f'[MAP-PACK-A2] index pair OK {n}/{len(jobs)} {pbf_path.name}')
            tile_manifest_rows.append({
                'z':args.zoom,'x':x,'y':y,
                'pbf':pbf_path.name,'pbi':pbi_path.name,
                'pbf_bytes':pbf_path.stat().st_size,'pbi_bytes':pbi_path.stat().st_size,
                'source_signature':pair['source_signature'],'entries':pair['entries'],
            })
        write_tile_manifest(region_dir/'tiles.tsv',tile_manifest_rows)
        print(f'[MAP-PACK-A2] pair manifest={region_dir / "tiles.tsv"} pairs={len(tile_manifest_rows)}')

    # Runtime path is always SD-root absolute, independent of the Mac mount point.
    tile_root_on_sd=f'/maps/regions/{args.region}/tiles'
    update_manifest(args.output_root/'regions.tsv',args.region,bbox,args.zoom,tile_root_on_sd)
    print(f'[MAP-PACK-A2] manifest={args.output_root / "regions.tsv"}')
    print(f'[MAP-PACK-A2] tile_root={tiles_dir}')
    if args.dry_run:
        for x,y,out,b in jobs[:8]: print(f'  {out} bbox={b}')
        if len(jobs)>8: print(f'  ... {len(jobs)-8} more')

if __name__=='__main__': main()
