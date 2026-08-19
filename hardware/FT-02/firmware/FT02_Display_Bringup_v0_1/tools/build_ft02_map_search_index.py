#!/usr/bin/env python3
"""Build the FT-02 Map Search A4 disk index from an OSM PBF.

A4 replaces the A3 "load the whole search.bin into PSRAM" design with two
files:

  *.search.idx  small fixed 256-bucket directory
  *.search.dat  fixed-size records grouped by bucket

Each place is written into the bucket of every distinct Unicode codepoint in
its normalized name. A query therefore needs only the bucket for the query's
first codepoint, while exact, prefix *and contains* matching still work. Hash
collisions are harmless because firmware verifies the full name before adding
a result.

The firmware keeps only the ~2 KiB bucket directory in normal RAM and streams
8 records at a time from .search.dat. It does not allocate the complete index
in PSRAM.

Requires pyosmium:
    python3 -m pip install osmium

Example:
    python3 tools/build_ft02_map_search_index.py \
      /Volumes/SD/maps/raw/shanghai-260726.osm.pbf \
      /Volumes/SD/maps/raw/shanghai-260726.osm.search.idx \
      /Volumes/SD/maps/raw/shanghai-260726.osm.search.dat

If output_dat is omitted it is derived from output_idx by replacing .idx with
.dat.
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    import osmium
except ImportError:
    print("ERROR: pyosmium is required. Install with: python3 -m pip install osmium", file=sys.stderr)
    raise SystemExit(2)

MAGIC = b"FTMSA4I\0"
VERSION = 4
BUCKET_COUNT = 256
HEADER = struct.Struct("<8sHHHHIII")
BUCKET = struct.Struct("<II")  # data byte offset, record count
RECORD = struct.Struct("<72s72sffB3x")
assert RECORD.size == 156
assert BUCKET.size == 8


def clean(value: str | None) -> str:
    if not value:
        return ""
    return value.strip().replace("\t", " ").replace("\r", " ").replace("\n", " ")


def utf8_fixed(text: str, size: int) -> bytes:
    raw = clean(text).encode("utf-8")
    if len(raw) < size:
        return raw + b"\0" * (size - len(raw))
    raw = raw[: size - 1]
    while raw and (raw[-1] & 0xC0) == 0x80:
        raw = raw[:-1]
    return raw + b"\0" * (size - len(raw))


def norm_name(text: str) -> str:
    return clean(text).casefold()


def bucket_for_codepoint(cp: int) -> int:
    return ((cp * 2654435761) & 0xFFFFFFFF) >> 24


def buckets_for_name(name: str) -> set[int]:
    normalized = norm_name(name)
    return {bucket_for_codepoint(ord(ch)) for ch in normalized if ch}


AMENITY = {
    "school": ("学校", 35), "university": ("大学", 30), "college": ("学校", 35),
    "hospital": ("医院", 25), "clinic": ("诊所", 35), "pharmacy": ("药店", 45),
    "restaurant": ("餐厅", 45), "cafe": ("咖啡馆", 48), "fast_food": ("餐饮", 48),
    "bank": ("银行", 42), "atm": ("ATM", 52), "police": ("公安/警务", 28),
    "fire_station": ("消防站", 30), "post_office": ("邮局", 42),
    "parking": ("停车场", 55), "bus_station": ("公交枢纽", 24),
    "library": ("图书馆", 35), "theatre": ("剧院", 38), "cinema": ("电影院", 42),
    "place_of_worship": ("宗教场所", 50), "townhall": ("政府机构", 30),
}
TOURISM = {
    "hotel": ("酒店", 42), "hostel": ("住宿", 48), "museum": ("博物馆", 30),
    "attraction": ("景点", 32), "viewpoint": ("观景点", 38), "gallery": ("展馆", 38),
}
LEISURE = {
    "park": ("公园", 24), "garden": ("花园", 32), "sports_centre": ("体育场所", 35),
    "stadium": ("体育场", 30), "playground": ("游乐场", 45),
}
SHOP = {"mall": ("商场", 32), "supermarket": ("超市", 38), "department_store": ("商场", 36)}
ROAD_TYPES = {
    "motorway", "trunk", "primary", "secondary", "tertiary", "residential",
    "unclassified", "service", "living_street", "pedestrian"
}


@dataclass
class Category:
    family: str
    label: str
    priority: int


def category_for(tags) -> Category | None:
    if tags.get("public_transport") in {"stop_position", "platform"}:
        return None
    place = tags.get("place")
    if place == "square": return Category("place", "广场", 8)
    if place == "city": return Category("place", "城市", 10)
    if place == "town": return Category("place", "城镇", 12)
    if place in {"suburb", "quarter", "neighbourhood"}: return Category("place", "地区", 20)

    railway = tags.get("railway")
    if railway == "station":
        is_subway = tags.get("station") == "subway" or tags.get("subway") == "yes"
        return Category("rail", "地铁站" if is_subway else "火车站", 12)
    if railway == "subway_entrance": return Category("rail_entrance", "地铁入口", 36)
    if railway == "halt": return Category("rail", "铁路站", 24)
    if tags.get("public_transport") == "station": return Category("transit", "公共交通站", 18)
    if tags.get("highway") == "bus_stop": return Category("bus", "公交站", 34)

    val = tags.get("amenity")
    if val in AMENITY:
        label, priority = AMENITY[val]; return Category("amenity:" + val, label, priority)
    val = tags.get("tourism")
    if val in TOURISM:
        label, priority = TOURISM[val]; return Category("tourism:" + val, label, priority)
    val = tags.get("leisure")
    if val in LEISURE:
        label, priority = LEISURE[val]; return Category("leisure:" + val, label, priority)
    val = tags.get("shop")
    if val in SHOP:
        label, priority = SHOP[val]; return Category("shop:" + val, label, priority)

    if tags.get("aeroway") == "aerodrome": return Category("airport", "机场", 18)
    if tags.get("building") == "train_station": return Category("rail", "车站", 18)
    if tags.get("historic"): return Category("historic", "历史地点", 42)
    if tags.get("natural") in {"water", "peak", "wood"}:
        labels = {"water": "水域", "peak": "山峰", "wood": "林地"}
        return Category("natural:" + tags.get("natural"), labels[tags.get("natural")], 44)
    if tags.get("waterway") in {"river", "canal"}: return Category("waterway", "河流/水道", 45)
    highway = tags.get("highway")
    if highway in ROAD_TYPES: return Category("road", "道路", 55)
    if tags.get("office"): return Category("office", "机构", 55)
    return None


def context_for(tags) -> str:
    parts: list[str] = []
    for key in ("addr:district", "addr:suburb", "addr:city"):
        value = clean(tags.get(key))
        if value and value not in parts:
            parts.append(value); break
    for key in ("network", "operator", "ref", "addr:street"):
        value = clean(tags.get(key))
        if value and value not in parts: parts.append(value)
        if len(parts) >= 2: break
    return " · ".join(parts)


def detail_for(cat: Category, tags) -> str:
    context = context_for(tags)
    return f"{cat.label} · {context}" if context else cat.label


@dataclass
class Entry:
    name: str
    detail: str
    lat: float
    lon: float
    priority: int
    family: str


class Builder(osmium.SimpleHandler):
    def __init__(self):
        super().__init__()
        self.entries: dict[tuple[str, str, int, int], Entry] = {}
        self.raw_named = 0
        self.filtered = 0
        self.duplicate_replaced = 0

    def emit(self, tags, lat: float, lon: float) -> None:
        if not (math.isfinite(lat) and math.isfinite(lon)): return
        cat = category_for(tags)
        if cat is None:
            self.filtered += 1; return
        names: list[str] = []
        for key in ("name:zh", "name", "official_name", "short_name", "alt_name"):
            value = tags.get(key)
            if value:
                for part in value.split(";"):
                    part = clean(part)
                    if part and part not in names: names.append(part)
        if not names: return
        self.raw_named += 1
        detail = detail_for(cat, tags)
        grid_lat = round(lat * 1000)
        grid_lon = round(lon * 1000)
        for name in names:
            key = (norm_name(name), cat.family, grid_lat, grid_lon)
            entry = Entry(name, detail, lat, lon, cat.priority, cat.family)
            old = self.entries.get(key)
            if old is None:
                self.entries[key] = entry
            elif entry.priority < old.priority or (entry.priority == old.priority and len(entry.detail) > len(old.detail)):
                self.entries[key] = entry
                self.duplicate_replaced += 1

    def node(self, n):
        if n.location.valid(): self.emit(n.tags, n.location.lat, n.location.lon)

    def way(self, w):
        if not (w.tags.get("name") or w.tags.get("name:zh")): return
        coords = [(nd.location.lat, nd.location.lon) for nd in w.nodes if nd.location.valid()]
        if not coords: return
        self.emit(w.tags,
                  sum(x for x, _ in coords) / len(coords),
                  sum(y for _, y in coords) / len(coords))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source_pbf")
    ap.add_argument("output_idx")
    ap.add_argument("output_dat", nargs="?")
    args = ap.parse_args()

    idx_path = Path(args.output_idx)
    dat_path = Path(args.output_dat) if args.output_dat else idx_path.with_suffix(".dat")

    h = Builder()
    h.apply_file(args.source_pbf, locations=True, idx="flex_mem")
    entries = list(h.entries.values())
    entries.sort(key=lambda e: (norm_name(e.name), e.priority, e.detail, e.lat, e.lon))

    buckets: list[list[Entry]] = [[] for _ in range(BUCKET_COUNT)]
    for entry in entries:
        for bucket_id in buckets_for_name(entry.name):
            buckets[bucket_id].append(entry)

    offsets: list[tuple[int, int]] = []
    data_records = 0
    byte_offset = 0
    dat_path.parent.mkdir(parents=True, exist_ok=True)
    with dat_path.open("wb") as dat:
        for bucket in buckets:
            bucket.sort(key=lambda e: (norm_name(e.name), e.priority, e.detail, e.lat, e.lon))
            offsets.append((byte_offset, len(bucket)))
            for e in bucket:
                dat.write(RECORD.pack(
                    utf8_fixed(e.name, 72), utf8_fixed(e.detail, 72),
                    float(e.lat), float(e.lon), int(e.priority),
                ))
            data_records += len(bucket)
            byte_offset += len(bucket) * RECORD.size

    idx_path.parent.mkdir(parents=True, exist_ok=True)
    with idx_path.open("wb") as idx:
        idx.write(HEADER.pack(MAGIC, VERSION, BUCKET_COUNT, RECORD.size, 0,
                              len(entries), data_records, 0))
        for offset, count in offsets:
            idx.write(BUCKET.pack(offset, count))

    nonempty = sum(1 for _, count in offsets if count)
    largest = max((count for _, count in offsets), default=0)
    print(f"PASS: unique={len(entries)} data_records={data_records} buckets_nonempty={nonempty}/{BUCKET_COUNT}")
    print(f"PASS: idx={idx_path} ({idx_path.stat().st_size / 1024:.1f} KiB)")
    print(f"PASS: dat={dat_path} ({dat_path.stat().st_size / 1024:.1f} KiB) largest_bucket={largest} records")
    print(f"INFO: named={h.raw_named} filtered={h.filtered} duplicate_replaced={h.duplicate_replaced}")


if __name__ == "__main__":
    main()
