#!/usr/bin/env python3
"""
FT-02 OSM PBF Map Builder v1.95

Pure-Python reader for standard .osm.pbf extracts. It does not need osmium,
GDAL, pyrosm, or a network connection.

The builder creates an FT-02 map package compatible with firmware v1.93+:
  <output>/map.cfg
  <output>/<zoom>/<x>/<y>.ftm
  <output>/preview.png
  <output>/map_manifest.json

The first pass stores node coordinates only inside the requested tile window
plus a margin. The second pass resolves road/water/rail/building ways and
renders them to a monochrome map canvas.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import time
import zlib
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterator

from PIL import Image, ImageDraw, ImageFont

TILE_SIZE = 256
STRIDE = TILE_SIZE // 8
FTM_HEADER = struct.Struct("<4sHHHBBI")
MAX_MERCATOR_LAT = 85.05112878

MAJOR_ROADS = {
    "motorway", "motorway_link", "trunk", "trunk_link",
    "primary", "primary_link",
}
MEDIUM_ROADS = {
    "secondary", "secondary_link", "tertiary", "tertiary_link",
}
MINOR_ROADS = {
    "residential", "living_street", "unclassified", "service", "road",
}
PATHS = {"pedestrian", "footway", "path", "cycleway", "track", "steps"}


class PBFError(RuntimeError):
    pass


def read_varint(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    length = len(data)
    while pos < length:
        byte = data[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        if not (byte & 0x80):
            return value, pos
        shift += 7
        if shift > 70:
            raise PBFError("varint too long")
    raise PBFError("truncated varint")


def zigzag(value: int) -> int:
    return (value >> 1) ^ -(value & 1)


def iter_fields(data: bytes) -> Iterator[tuple[int, int, object]]:
    pos = 0
    length = len(data)
    while pos < length:
        key, pos = read_varint(data, pos)
        field = key >> 3
        wire = key & 7
        if wire == 0:
            value, pos = read_varint(data, pos)
            yield field, wire, value
        elif wire == 1:
            if pos + 8 > length:
                raise PBFError("truncated fixed64")
            yield field, wire, data[pos:pos + 8]
            pos += 8
        elif wire == 2:
            size, pos = read_varint(data, pos)
            end = pos + size
            if end > length:
                raise PBFError("truncated length-delimited field")
            yield field, wire, data[pos:end]
            pos = end
        elif wire == 5:
            if pos + 4 > length:
                raise PBFError("truncated fixed32")
            yield field, wire, data[pos:pos + 4]
            pos += 4
        else:
            raise PBFError(f"unsupported protobuf wire type {wire}")


def packed_varints(data: bytes, signed: bool = False) -> list[int]:
    values: list[int] = []
    pos = 0
    while pos < len(data):
        value, pos = read_varint(data, pos)
        values.append(zigzag(value) if signed else value)
    return values


def parse_blob_header(data: bytes) -> tuple[str, int]:
    block_type = ""
    data_size = -1
    for field, wire, value in iter_fields(data):
        if field == 1 and wire == 2:
            block_type = bytes(value).decode("utf-8", "replace")
        elif field == 3 and wire == 0:
            data_size = int(value)
    if not block_type or data_size < 0:
        raise PBFError("invalid BlobHeader")
    return block_type, data_size


def parse_blob(data: bytes) -> bytes:
    raw = None
    raw_size = None
    zlib_data = None
    for field, wire, value in iter_fields(data):
        if field == 1 and wire == 2:
            raw = bytes(value)
        elif field == 2 and wire == 0:
            raw_size = int(value)
        elif field == 3 and wire == 2:
            zlib_data = bytes(value)
    if raw is not None:
        payload = raw
    elif zlib_data is not None:
        payload = zlib.decompress(zlib_data)
    else:
        raise PBFError("unsupported Blob compression (expected raw or zlib)")
    if raw_size is not None and len(payload) != raw_size:
        raise PBFError(f"Blob raw_size mismatch: expected {raw_size}, got {len(payload)}")
    return payload


def iter_pbf_blocks(path: Path, wanted_type: str | None = None) -> Iterator[tuple[str, bytes]]:
    with path.open("rb") as handle:
        while True:
            size_bytes = handle.read(4)
            if not size_bytes:
                return
            if len(size_bytes) != 4:
                raise PBFError("truncated block header length")
            header_size = struct.unpack(">I", size_bytes)[0]
            if header_size <= 0 or header_size > 64 * 1024:
                raise PBFError(f"invalid BlobHeader size {header_size}")
            header = handle.read(header_size)
            if len(header) != header_size:
                raise PBFError("truncated BlobHeader")
            block_type, data_size = parse_blob_header(header)
            blob = handle.read(data_size)
            if len(blob) != data_size:
                raise PBFError("truncated Blob")
            if wanted_type is None or block_type == wanted_type:
                yield block_type, parse_blob(blob)


def parse_string_table(message: bytes) -> list[str]:
    strings: list[str] = []
    for field, wire, value in iter_fields(message):
        if field == 1 and wire == 2:
            strings.append(bytes(value).decode("utf-8", "replace"))
    return strings


def decode_tags(keys: list[int], vals: list[int], strings: list[str]) -> dict[str, str]:
    tags: dict[str, str] = {}
    for key_id, value_id in zip(keys, vals):
        if key_id < len(strings) and value_id < len(strings):
            tags[strings[key_id]] = strings[value_id]
    return tags


@dataclass(frozen=True)
class TileWindow:
    zoom: int
    min_x: int
    min_y: int
    max_x: int
    max_y: int

    @property
    def columns(self) -> int:
        return self.max_x - self.min_x + 1

    @property
    def rows(self) -> int:
        return self.max_y - self.min_y + 1

    @property
    def width(self) -> int:
        return self.columns * TILE_SIZE

    @property
    def height(self) -> int:
        return self.rows * TILE_SIZE


@dataclass
class Feature:
    kind: str
    subtype: str
    name: str
    coords: list[tuple[float, float]]
    closed: bool = False


def clamp_lat(lat: float) -> float:
    return max(-MAX_MERCATOR_LAT, min(MAX_MERCATOR_LAT, lat))


def lonlat_to_global_pixel(lon: float, lat: float, zoom: int) -> tuple[float, float]:
    lat = clamp_lat(lat)
    scale = TILE_SIZE * (1 << zoom)
    x = (lon + 180.0) / 360.0 * scale
    lat_rad = math.radians(lat)
    y = (1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * scale
    return x, y


def global_pixel_to_lonlat(x: float, y: float, zoom: int) -> tuple[float, float]:
    scale = TILE_SIZE * (1 << zoom)
    lon = x / scale * 360.0 - 180.0
    merc_y = math.pi * (1.0 - 2.0 * y / scale)
    lat = math.degrees(math.atan(math.sinh(merc_y)))
    return lon, lat


def window_from_center(lon: float, lat: float, zoom: int, columns: int, rows: int) -> TileWindow:
    px, py = lonlat_to_global_pixel(lon, lat, zoom)
    center_x = int(px // TILE_SIZE)
    center_y = int(py // TILE_SIZE)
    min_x = center_x - columns // 2
    min_y = center_y - rows // 2
    return TileWindow(zoom, min_x, min_y, min_x + columns - 1, min_y + rows - 1)


def window_bounds(window: TileWindow, margin_tiles: float = 0.0) -> tuple[float, float, float, float]:
    pad = margin_tiles * TILE_SIZE
    west, north = global_pixel_to_lonlat(
        window.min_x * TILE_SIZE - pad,
        window.min_y * TILE_SIZE - pad,
        window.zoom,
    )
    east, south = global_pixel_to_lonlat(
        (window.max_x + 1) * TILE_SIZE + pad,
        (window.max_y + 1) * TILE_SIZE + pad,
        window.zoom,
    )
    return west, south, east, north


def parse_primitive_block(payload: bytes) -> tuple[list[str], list[bytes], int, int, int]:
    string_table: list[str] = []
    groups: list[bytes] = []
    granularity = 100
    lat_offset = 0
    lon_offset = 0
    for field, wire, value in iter_fields(payload):
        if field == 1 and wire == 2:
            string_table = parse_string_table(bytes(value))
        elif field == 2 and wire == 2:
            groups.append(bytes(value))
        elif field == 17 and wire == 0:
            granularity = int(value)
        elif field == 19 and wire == 0:
            lat_offset = zigzag(int(value))
        elif field == 20 and wire == 0:
            lon_offset = zigzag(int(value))
    return string_table, groups, granularity, lat_offset, lon_offset


def decode_normal_node(message: bytes, strings: list[str], granularity: int, lat_offset: int, lon_offset: int):
    node_id = None
    raw_lat = None
    raw_lon = None
    keys: list[int] = []
    vals: list[int] = []
    for field, wire, value in iter_fields(message):
        if field == 1 and wire == 0:
            node_id = zigzag(int(value))
        elif field == 2 and wire == 2:
            keys = packed_varints(bytes(value))
        elif field == 3 and wire == 2:
            vals = packed_varints(bytes(value))
        elif field == 8 and wire == 0:
            raw_lat = zigzag(int(value))
        elif field == 9 and wire == 0:
            raw_lon = zigzag(int(value))
    if node_id is None or raw_lat is None or raw_lon is None:
        return None
    lat = 1e-9 * (lat_offset + granularity * raw_lat)
    lon = 1e-9 * (lon_offset + granularity * raw_lon)
    return node_id, lon, lat, decode_tags(keys, vals, strings)


def iter_dense_nodes(message: bytes, strings: list[str], granularity: int, lat_offset: int, lon_offset: int):
    ids: list[int] = []
    lats: list[int] = []
    lons: list[int] = []
    keys_vals: list[int] = []
    for field, wire, value in iter_fields(message):
        if field == 1 and wire == 2:
            ids = packed_varints(bytes(value), signed=True)
        elif field == 8 and wire == 2:
            lats = packed_varints(bytes(value), signed=True)
        elif field == 9 and wire == 2:
            lons = packed_varints(bytes(value), signed=True)
        elif field == 10 and wire == 2:
            keys_vals = packed_varints(bytes(value))
    if not (len(ids) == len(lats) == len(lons)):
        raise PBFError("DenseNodes arrays have different lengths")

    current_id = 0
    current_lat = 0
    current_lon = 0
    kv_pos = 0
    for delta_id, delta_lat, delta_lon in zip(ids, lats, lons):
        current_id += delta_id
        current_lat += delta_lat
        current_lon += delta_lon
        tags: dict[str, str] = {}
        while kv_pos < len(keys_vals):
            key_id = keys_vals[kv_pos]
            kv_pos += 1
            if key_id == 0:
                break
            if kv_pos >= len(keys_vals):
                raise PBFError("truncated DenseNodes keys_vals")
            value_id = keys_vals[kv_pos]
            kv_pos += 1
            if key_id < len(strings) and value_id < len(strings):
                tags[strings[key_id]] = strings[value_id]
        lat = 1e-9 * (lat_offset + granularity * current_lat)
        lon = 1e-9 * (lon_offset + granularity * current_lon)
        yield current_id, lon, lat, tags


def decode_way(message: bytes, strings: list[str]):
    way_id = 0
    keys: list[int] = []
    vals: list[int] = []
    refs_delta: list[int] = []
    for field, wire, value in iter_fields(message):
        if field == 1 and wire == 0:
            way_id = int(value)
        elif field == 2 and wire == 2:
            keys = packed_varints(bytes(value))
        elif field == 3 and wire == 2:
            vals = packed_varints(bytes(value))
        elif field == 8 and wire == 2:
            refs_delta = packed_varints(bytes(value), signed=True)
    refs: list[int] = []
    current = 0
    for delta in refs_delta:
        current += delta
        refs.append(current)
    return way_id, decode_tags(keys, vals, strings), refs


def classify_feature(tags: dict[str, str], detail: str) -> tuple[str, str] | None:
    highway = tags.get("highway", "")
    if highway:
        if highway in PATHS and detail == "emergency":
            return None
        if highway in MAJOR_ROADS | MEDIUM_ROADS | MINOR_ROADS | PATHS or detail == "dense":
            return "road", highway
    railway = tags.get("railway", "")
    if railway and railway not in {"abandoned", "razed", "proposed"}:
        return "rail", railway
    waterway = tags.get("waterway", "")
    if waterway:
        return "water", waterway
    natural = tags.get("natural", "")
    water = tags.get("water", "")
    landuse = tags.get("landuse", "")
    if natural == "water" or water or landuse in {"reservoir", "basin"}:
        return "water_area", water or natural or landuse
    if detail == "dense" and "building" in tags:
        return "building", tags.get("building", "yes")
    return None


def preferred_name(tags: dict[str, str]) -> str:
    for key in ("name:zh", "name", "name:en"):
        value = tags.get(key, "").strip()
        if value:
            return value
    return ""


def collect_nodes(path: Path, bounds: tuple[float, float, float, float]):
    west, south, east, north = bounds
    coords: dict[int, tuple[float, float]] = {}
    labels: list[tuple[float, float, str, int]] = []
    blocks = 0
    nodes_seen = 0
    kept = 0
    start = time.time()

    for _, payload in iter_pbf_blocks(path, "OSMData"):
        blocks += 1
        strings, groups, granularity, lat_offset, lon_offset = parse_primitive_block(payload)
        for group in groups:
            for field, wire, value in iter_fields(group):
                if field == 1 and wire == 2:
                    node = decode_normal_node(bytes(value), strings, granularity, lat_offset, lon_offset)
                    if node is None:
                        continue
                    node_id, lon, lat, tags = node
                    nodes_seen += 1
                    if west <= lon <= east and south <= lat <= north:
                        coords[node_id] = (lon, lat)
                        kept += 1
                        name = preferred_name(tags)
                        if name:
                            rank = label_rank(tags)
                            if rank is not None:
                                labels.append((lon, lat, name, rank))
                elif field == 2 and wire == 2:
                    for node_id, lon, lat, tags in iter_dense_nodes(
                        bytes(value), strings, granularity, lat_offset, lon_offset
                    ):
                        nodes_seen += 1
                        if west <= lon <= east and south <= lat <= north:
                            coords[node_id] = (lon, lat)
                            kept += 1
                            name = preferred_name(tags)
                            if name:
                                rank = label_rank(tags)
                                if rank is not None:
                                    labels.append((lon, lat, name, rank))
        if blocks % 50 == 0:
            print(f"Pass 1: blocks={blocks} nodes={nodes_seen} kept={kept}", file=sys.stderr)
    print(
        f"Pass 1 complete: blocks={blocks}, nodes={nodes_seen}, kept={kept}, "
        f"labels={len(labels)}, {time.time() - start:.1f}s",
        file=sys.stderr,
    )
    return coords, labels


def label_rank(tags: dict[str, str]) -> int | None:
    place = tags.get("place", "")
    if place in {"city", "town"}:
        return 0
    if place in {"suburb", "borough", "district"}:
        return 1
    if place in {"quarter", "neighbourhood", "village"}:
        return 2
    railway = tags.get("railway", "")
    if railway in {"station", "halt"}:
        return 3
    public_transport = tags.get("public_transport", "")
    if public_transport == "station":
        return 3
    amenity = tags.get("amenity", "")
    if amenity in {"hospital", "university"}:
        return 4
    return None


def split_resolved_runs(refs: list[int], coords: dict[int, tuple[float, float]]) -> list[list[tuple[float, float]]]:
    runs: list[list[tuple[float, float]]] = []
    current: list[tuple[float, float]] = []
    for ref in refs:
        point = coords.get(ref)
        if point is None:
            if len(current) >= 2:
                runs.append(current)
            current = []
        else:
            current.append(point)
    if len(current) >= 2:
        runs.append(current)
    return runs


def collect_ways(path: Path, coords: dict[int, tuple[float, float]], detail: str) -> list[Feature]:
    features: list[Feature] = []
    blocks = 0
    ways_seen = 0
    selected = 0
    start = time.time()
    for _, payload in iter_pbf_blocks(path, "OSMData"):
        blocks += 1
        strings, groups, _, _, _ = parse_primitive_block(payload)
        for group in groups:
            for field, wire, value in iter_fields(group):
                if field != 3 or wire != 2:
                    continue
                _, tags, refs = decode_way(bytes(value), strings)
                ways_seen += 1
                classification = classify_feature(tags, detail)
                if classification is None:
                    continue
                kind, subtype = classification
                name = preferred_name(tags)
                runs = split_resolved_runs(refs, coords)
                for run in runs:
                    closed = len(run) >= 4 and run[0] == run[-1]
                    features.append(Feature(kind, subtype, name, run, closed))
                    selected += 1
        if blocks % 50 == 0:
            print(f"Pass 2: blocks={blocks} ways={ways_seen} features={selected}", file=sys.stderr)
    print(
        f"Pass 2 complete: blocks={blocks}, ways={ways_seen}, features={selected}, "
        f"{time.time() - start:.1f}s",
        file=sys.stderr,
    )
    return features


def canvas_point(lon: float, lat: float, window: TileWindow, scale: int) -> tuple[int, int]:
    px, py = lonlat_to_global_pixel(lon, lat, window.zoom)
    return (
        int(round((px - window.min_x * TILE_SIZE) * scale)),
        int(round((py - window.min_y * TILE_SIZE) * scale)),
    )


def road_width(subtype: str, detail: str, scale: int) -> tuple[int, int] | None:
    if subtype in MAJOR_ROADS:
        return 0, 5 * scale
    if subtype in MEDIUM_ROADS:
        return 0, 3 * scale
    if subtype in MINOR_ROADS:
        return 35, 2 * scale
    if subtype in PATHS:
        if detail == "emergency":
            return None
        return 90, max(1, scale)
    if detail == "dense":
        return 110, max(1, scale)
    return None


def draw_features(image: Image.Image, features: list[Feature], window: TileWindow, scale: int, detail: str):
    draw = ImageDraw.Draw(image)
    counts: dict[str, int] = {}

    # Areas first.
    for feature in features:
        if feature.kind not in {"water_area", "building"} or not feature.closed:
            continue
        points = [canvas_point(lon, lat, window, scale) for lon, lat in feature.coords]
        if len(points) < 4:
            continue
        if feature.kind == "water_area":
            draw.polygon(points, fill=240, outline=50)
            # Sparse hatching remains recognizable after 1-bit conversion.
            xs = [p[0] for p in points]
            ys = [p[1] for p in points]
            left, right = max(0, min(xs)), min(image.width, max(xs))
            top, bottom = max(0, min(ys)), min(image.height, max(ys))
            for offset in range(left - (bottom - top), right + 12 * scale, 12 * scale):
                draw.line((offset, bottom, offset + (bottom - top), top), fill=185, width=scale)
        else:
            draw.polygon(points, fill=215, outline=140)
        counts[feature.kind] = counts.get(feature.kind, 0) + 1

    # Water lines and railways below roads.
    for feature in features:
        if feature.kind not in {"water", "rail"}:
            continue
        points = [canvas_point(lon, lat, window, scale) for lon, lat in feature.coords]
        if len(points) < 2:
            continue
        if feature.kind == "water":
            draw.line(points, fill=45, width=3 * scale, joint="curve")
        else:
            draw.line(points, fill=65, width=max(1, scale), joint="curve")
            for index in range(0, len(points), 3):
                x, y = points[index]
                r = max(1, scale)
                draw.rectangle((x-r, y-r, x+r, y+r), fill=0)
        counts[feature.kind] = counts.get(feature.kind, 0) + 1

    roads: list[tuple[int, int, Feature]] = []
    for feature in features:
        if feature.kind != "road":
            continue
        style = road_width(feature.subtype, detail, scale)
        if style is None:
            continue
        tone, width = style
        roads.append((width, tone, feature))
    roads.sort(key=lambda item: item[0])
    for width, tone, feature in roads:
        points = [canvas_point(lon, lat, window, scale) for lon, lat in feature.coords]
        if len(points) >= 2:
            # White casing separates major roads in dense urban areas.
            if width >= 3 * scale:
                draw.line(points, fill=255, width=width + 2 * scale, joint="curve")
            draw.line(points, fill=tone, width=width, joint="curve")
            counts["road"] = counts.get("road", 0) + 1
    return counts


def load_font(font_path: Path | None, size: int):
    candidates = []
    if font_path is not None:
        candidates.append(font_path)
    candidates.extend([
        Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"),
        Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"),
        Path("/usr/share/fonts/truetype/arphic-gbsn00lp/gbsn00lp.ttf"),
    ])
    for candidate in candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size=size)
    return ImageFont.load_default()


def draw_labels(image: Image.Image, labels, features: list[Feature], window: TileWindow, scale: int, font_path: Path | None, max_labels: int):
    draw = ImageDraw.Draw(image)
    font = load_font(font_path, 15 * scale)
    small_font = load_font(font_path, 12 * scale)
    candidates: list[tuple[int, float, float, str, object]] = []
    for lon, lat, name, rank in labels:
        candidates.append((rank, lon, lat, name, font if rank <= 1 else small_font))

    # Add a small number of named major roads using the midpoint of the longest resolved segment.
    road_names: dict[str, Feature] = {}
    for feature in features:
        if feature.kind != "road" or not feature.name or feature.subtype not in MAJOR_ROADS | MEDIUM_ROADS:
            continue
        old = road_names.get(feature.name)
        if old is None or len(feature.coords) > len(old.coords):
            road_names[feature.name] = feature
    for feature in road_names.values():
        lon, lat = feature.coords[len(feature.coords)//2]
        candidates.append((5, lon, lat, feature.name, small_font))

    candidates.sort(key=lambda item: (item[0], len(item[3])))
    occupied: list[tuple[int, int, int, int]] = []
    drawn = 0
    for rank, lon, lat, name, selected_font in candidates:
        if drawn >= max_labels:
            break
        if len(name) > 14:
            name = name[:14]
        x, y = canvas_point(lon, lat, window, scale)
        bbox = draw.textbbox((x, y), name, font=selected_font, anchor="mm", stroke_width=2*scale)
        margin = 3 * scale
        candidate_box = (bbox[0]-margin, bbox[1]-margin, bbox[2]+margin, bbox[3]+margin)
        if candidate_box[2] < 0 or candidate_box[0] >= image.width or candidate_box[3] < 0 or candidate_box[1] >= image.height:
            continue
        if any(not (candidate_box[2] < b[0] or candidate_box[0] > b[2] or candidate_box[3] < b[1] or candidate_box[1] > b[3]) for b in occupied):
            continue
        draw.text((x, y), name, font=selected_font, fill=0, anchor="mm", stroke_width=2*scale, stroke_fill=255)
        occupied.append(candidate_box)
        drawn += 1
    return drawn


def write_ftm(path: Path, tile: Image.Image, threshold: int):
    gray = tile.convert("L")
    payload = bytearray(TILE_SIZE * STRIDE)
    pixels = gray.load()
    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            if pixels[x, y] < threshold:
                payload[y * STRIDE + x // 8] |= 0x80 >> (x % 8)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(FTM_HEADER.pack(b"FTM1", TILE_SIZE, TILE_SIZE, STRIDE, 1, 0, len(payload)) + payload)


def render_package(source: Path, output: Path, window: TileWindow, detail: str, scale: int, threshold: int, font_path: Path | None, max_labels: int, name: str):
    bounds = window_bounds(window, margin_tiles=1.0)
    print(f"Tile window z{window.zoom}: x={window.min_x}..{window.max_x}, y={window.min_y}..{window.max_y}")
    print(f"Geographic bounds with margin: {bounds}")
    coords, labels = collect_nodes(source, bounds)
    features = collect_ways(source, coords, detail)

    canvas = Image.new("L", (window.width * scale, window.height * scale), 255)
    counts = draw_features(canvas, features, window, scale, detail)
    label_count = draw_labels(canvas, labels, features, window, scale, font_path, max_labels)

    if scale > 1:
        final = canvas.resize((window.width, window.height), Image.Resampling.LANCZOS)
    else:
        final = canvas

    output.mkdir(parents=True, exist_ok=True)
    final.save(output / "preview.png")

    for row in range(window.rows):
        for col in range(window.columns):
            tile = final.crop((col*TILE_SIZE, row*TILE_SIZE, (col+1)*TILE_SIZE, (row+1)*TILE_SIZE))
            x = window.min_x + col
            y = window.min_y + row
            write_ftm(output / str(window.zoom) / str(x) / f"{y}.ftm", tile, threshold)

    start_x = max(window.min_x, min(window.max_x - 3, window.min_x + (window.columns - 4)//2))
    start_y = max(window.min_y, min(window.max_y - 1, window.min_y + (window.rows - 2)//2))
    cfg = "\n".join([
        "FTMAP1",
        f"name={name}",
        "attribution=(c) OpenStreetMap contributors",
        f"zoom={window.zoom}",
        f"min_x={window.min_x}",
        f"max_x={window.max_x}",
        f"min_y={window.min_y}",
        f"max_y={window.max_y}",
        f"start_x={start_x}",
        f"start_y={start_y}",
        "",
    ])
    (output / "map.cfg").write_text(cfg, encoding="utf-8")
    west, south, east, north = window_bounds(window, margin_tiles=0)
    manifest = {
        "format": "FTMAP1",
        "builder": "ft02_osm_pbf_builder.py v1.95",
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "source_file": source.name,
        "name": name,
        "attribution": "© OpenStreetMap contributors",
        "projection": "Web Mercator / WGS84",
        "zoom": window.zoom,
        "tile_bounds": {"min_x": window.min_x, "max_x": window.max_x, "min_y": window.min_y, "max_y": window.max_y},
        "geographic_bounds": {"west": west, "south": south, "east": east, "north": north},
        "start": {"x": start_x, "y": start_y},
        "tile_count": window.columns * window.rows,
        "detail": detail,
        "render_scale": scale,
        "threshold": threshold,
        "feature_counts": counts,
        "label_count": label_count,
    }
    (output / "map_manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Generated {manifest['tile_count']} FTM tiles in {output}")
    print(f"Feature counts: {counts}; labels={label_count}")


def main():
    parser = argparse.ArgumentParser(description="Build FT-02 FTM tiles directly from an OSM PBF extract")
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--center-lon", type=float, required=True)
    parser.add_argument("--center-lat", type=float, required=True)
    parser.add_argument("--zoom", type=int, default=15)
    parser.add_argument("--columns", type=int, default=8)
    parser.add_argument("--rows", type=int, default=6)
    parser.add_argument("--detail", choices=["emergency", "street", "dense"], default="street")
    parser.add_argument("--scale", type=int, choices=[1, 2, 3, 4], default=2)
    parser.add_argument("--threshold", type=int, default=165)
    parser.add_argument("--font", type=Path)
    parser.add_argument("--max-labels", type=int, default=55)
    parser.add_argument("--name", default="Shanghai Central")
    args = parser.parse_args()

    if not args.source.exists():
        raise SystemExit(f"Source file not found: {args.source}")
    if args.columns < 4 or args.rows < 2:
        raise SystemExit("FT-02 requires at least 4 columns and 2 rows")
    if not (0 <= args.zoom <= 22):
        raise SystemExit("--zoom must be 0..22")
    window = window_from_center(args.center_lon, args.center_lat, args.zoom, args.columns, args.rows)
    render_package(args.source, args.output, window, args.detail, args.scale, args.threshold, args.font, args.max_labels, args.name)


if __name__ == "__main__":
    main()
