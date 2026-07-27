#!/usr/bin/env python3
"""
Build a geographic FT-02 offline map package from Geofabrik-style OSM shapefiles.

The script never downloads data. It reads either:
  1. a Geofabrik `*-free.shp.zip` archive, or
  2. an extracted directory containing `gis_osm_*.shp` files.

Output remains compatible with FT02 v1.93 firmware:
  <output>/map.cfg
  <output>/<zoom>/<x>/<y>.ftm
  <output>/preview.png
  <output>/map_manifest.json

Dependencies:
  python3 -m pip install pillow geopandas shapely pyproj fiona
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import zipfile
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Iterator, Sequence

try:
    import geopandas as gpd
    from PIL import Image, ImageDraw, ImageFont
    from shapely.geometry import (
        GeometryCollection,
        LineString,
        MultiLineString,
        MultiPoint,
        MultiPolygon,
        Point,
        Polygon,
        box,
    )
except ImportError as exc:
    raise SystemExit(
        "Missing dependency. Install with: "
        "python3 -m pip install pillow geopandas shapely pyproj fiona"
    ) from exc

TILE_SIZE = 256
STRIDE = TILE_SIZE // 8
FTM_HEADER = struct.Struct("<4sHHHBBI")
MAX_MERCATOR_LAT = 85.05112878

ROAD_FILES = (
    "gis_osm_roads_free_1.shp",
    "gis_osm_railways_free_1.shp",
)
WATER_FILES = (
    "gis_osm_water_a_free_1.shp",
    "gis_osm_waterways_free_1.shp",
)
BUILDING_FILES = ("gis_osm_buildings_a_free_1.shp",)
PLACE_FILES = ("gis_osm_places_free_1.shp",)
POI_FILES = ("gis_osm_pois_free_1.shp",)

MAJOR_ROADS = {
    "motorway",
    "motorway_link",
    "trunk",
    "trunk_link",
    "primary",
    "primary_link",
}
MEDIUM_ROADS = {
    "secondary",
    "secondary_link",
    "tertiary",
    "tertiary_link",
}
MINOR_ROADS = {
    "residential",
    "living_street",
    "unclassified",
    "service",
    "road",
}
PATHS = {
    "pedestrian",
    "footway",
    "path",
    "cycleway",
    "track",
    "steps",
}


@dataclass(frozen=True)
class Bounds:
    west: float
    south: float
    east: float
    north: float

    def validate(self) -> None:
        if not (-180.0 <= self.west < self.east <= 180.0):
            raise ValueError("invalid west/east longitude range")
        if not (-90.0 <= self.south < self.north <= 90.0):
            raise ValueError("invalid south/north latitude range")

    @property
    def center_lon(self) -> float:
        return (self.west + self.east) / 2.0

    @property
    def center_lat(self) -> float:
        return (self.south + self.north) / 2.0

    def as_geopandas_bbox(self) -> tuple[float, float, float, float]:
        return (self.west, self.south, self.east, self.north)


@dataclass(frozen=True)
class TileRange:
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
    def tile_count(self) -> int:
        return self.columns * self.rows


class SourceCatalog:
    def __init__(self, source: Path):
        self.source = source.resolve()
        if not self.source.exists():
            raise FileNotFoundError(f"source does not exist: {self.source}")
        self.is_zip = self.source.is_file() and self.source.suffix.lower() == ".zip"
        self._zip_names: set[str] = set()
        if self.is_zip:
            with zipfile.ZipFile(self.source) as archive:
                self._zip_names = set(archive.namelist())
        elif not self.source.is_dir():
            raise ValueError("source must be a .zip archive or an extracted directory")

    def _find_zip_member(self, filename: str) -> str | None:
        matches = [name for name in self._zip_names if Path(name).name == filename]
        if not matches:
            return None
        matches.sort(key=len)
        return matches[0]

    def has(self, filename: str) -> bool:
        if self.is_zip:
            return self._find_zip_member(filename) is not None
        return (self.source / filename).exists()

    def uri(self, filename: str) -> str | None:
        if self.is_zip:
            member = self._find_zip_member(filename)
            if member is None:
                return None
            return f"zip://{self.source}!{member}"
        path = self.source / filename
        return str(path) if path.exists() else None


def clamp_latitude(latitude: float) -> float:
    return max(-MAX_MERCATOR_LAT, min(MAX_MERCATOR_LAT, latitude))


def lonlat_to_global_pixel(lon: float, lat: float, zoom: int) -> tuple[float, float]:
    lat = clamp_latitude(lat)
    scale = TILE_SIZE * (1 << zoom)
    x = (lon + 180.0) / 360.0 * scale
    lat_rad = math.radians(lat)
    y = (
        1.0
        - math.asinh(math.tan(lat_rad)) / math.pi
    ) / 2.0 * scale
    return x, y


def global_pixel_to_lonlat(x: float, y: float, zoom: int) -> tuple[float, float]:
    scale = TILE_SIZE * (1 << zoom)
    lon = x / scale * 360.0 - 180.0
    mercator_y = math.pi * (1.0 - 2.0 * y / scale)
    lat = math.degrees(math.atan(math.sinh(mercator_y)))
    return lon, lat


def tile_range_for_bounds(bounds: Bounds, zoom: int) -> TileRange:
    west_x, north_y = lonlat_to_global_pixel(bounds.west, bounds.north, zoom)
    east_x, south_y = lonlat_to_global_pixel(bounds.east, bounds.south, zoom)

    min_x = math.floor(west_x / TILE_SIZE)
    min_y = math.floor(north_y / TILE_SIZE)
    max_x = max(min_x, math.ceil(east_x / TILE_SIZE) - 1)
    max_y = max(min_y, math.ceil(south_y / TILE_SIZE) - 1)

    limit = (1 << zoom) - 1
    return TileRange(
        max(0, min_x),
        max(0, min_y),
        min(limit, max_x),
        min(limit, max_y),
    )


def read_layer(catalog: SourceCatalog, filename: str, bounds: Bounds):
    uri = catalog.uri(filename)
    if uri is None:
        return None

    try:
        frame = gpd.read_file(
            uri,
            bbox=bounds.as_geopandas_bbox(),
        )
    except Exception as exc:
        print(f"WARN: failed to read {filename}: {exc}", file=sys.stderr)
        return None

    if frame.empty:
        return frame

    if frame.crs is None:
        frame = frame.set_crs("EPSG:4326", allow_override=True)
    elif str(frame.crs).upper() != "EPSG:4326":
        frame = frame.to_crs("EPSG:4326")

    clip_box = box(bounds.west, bounds.south, bounds.east, bounds.north)
    frame = frame[frame.geometry.notna()].copy()
    frame = frame[frame.geometry.intersects(clip_box)].copy()
    return frame


def iter_lines(geometry) -> Iterator[LineString]:
    if geometry is None or geometry.is_empty:
        return
    if isinstance(geometry, LineString):
        yield geometry
    elif isinstance(geometry, MultiLineString):
        yield from geometry.geoms
    elif isinstance(geometry, Polygon):
        yield LineString(geometry.exterior.coords)
        for ring in geometry.interiors:
            yield LineString(ring.coords)
    elif isinstance(geometry, MultiPolygon):
        for polygon in geometry.geoms:
            yield from iter_lines(polygon)
    elif isinstance(geometry, GeometryCollection):
        for child in geometry.geoms:
            yield from iter_lines(child)


def iter_polygons(geometry) -> Iterator[Polygon]:
    if geometry is None or geometry.is_empty:
        return
    if isinstance(geometry, Polygon):
        yield geometry
    elif isinstance(geometry, MultiPolygon):
        yield from geometry.geoms
    elif isinstance(geometry, GeometryCollection):
        for child in geometry.geoms:
            yield from iter_polygons(child)


def iter_points(geometry) -> Iterator[Point]:
    if geometry is None or geometry.is_empty:
        return
    if isinstance(geometry, Point):
        yield geometry
    elif isinstance(geometry, MultiPoint):
        yield from geometry.geoms
    elif isinstance(geometry, GeometryCollection):
        for child in geometry.geoms:
            yield from iter_points(child)


def world_to_canvas(
    lon: float,
    lat: float,
    zoom: int,
    tile_range: TileRange,
    scale: int,
) -> tuple[int, int]:
    world_x, world_y = lonlat_to_global_pixel(lon, lat, zoom)
    x = int(round((world_x - tile_range.min_x * TILE_SIZE) * scale))
    y = int(round((world_y - tile_range.min_y * TILE_SIZE) * scale))
    return x, y


def geometry_points(
    line: LineString,
    zoom: int,
    tile_range: TileRange,
    scale: int,
) -> list[tuple[int, int]]:
    return [
        world_to_canvas(lon, lat, zoom, tile_range, scale)
        for lon, lat, *_ in line.coords
    ]


def road_class(row) -> str:
    value = None
    for key in ("fclass", "highway", "type"):
        if key in row and row[key] is not None:
            value = str(row[key]).strip().lower()
            if value:
                break
    return value or "unknown"


def road_style(fclass: str, detail: str, scale: int) -> tuple[int, int] | None:
    if fclass in MAJOR_ROADS:
        return (0, 4 * scale)
    if fclass in MEDIUM_ROADS:
        return (0, 3 * scale)
    if fclass in MINOR_ROADS:
        return (25, 2 * scale)
    if fclass in PATHS:
        if detail == "emergency":
            return None
        return (80, max(1, scale))
    if detail == "dense":
        return (100, max(1, scale))
    return None


def draw_roads(
    draw: ImageDraw.ImageDraw,
    frame,
    zoom: int,
    tile_range: TileRange,
    scale: int,
    detail: str,
) -> int:
    if frame is None or frame.empty:
        return 0

    drawn = 0
    styled_rows: list[tuple[int, int, object]] = []
    for _, row in frame.iterrows():
        style = road_style(road_class(row), detail, scale)
        if style is None:
            continue
        tone, width = style
        styled_rows.append((tone, width, row.geometry))

    # Draw smaller roads first and major roads last.
    styled_rows.sort(key=lambda item: item[1])
    for tone, width, geometry in styled_rows:
        for line in iter_lines(geometry):
            points = geometry_points(line, zoom, tile_range, scale)
            if len(points) >= 2:
                draw.line(points, fill=tone, width=width, joint="curve")
                drawn += 1
    return drawn


def draw_railways(
    draw: ImageDraw.ImageDraw,
    frame,
    zoom: int,
    tile_range: TileRange,
    scale: int,
) -> int:
    if frame is None or frame.empty:
        return 0
    drawn = 0
    for _, row in frame.iterrows():
        for line in iter_lines(row.geometry):
            points = geometry_points(line, zoom, tile_range, scale)
            if len(points) < 2:
                continue
            draw.line(points, fill=55, width=max(1, scale))
            # Sparse square markers make railways distinct on 1-bit output.
            for index in range(0, len(points), 3):
                x, y = points[index]
                radius = max(1, scale)
                draw.rectangle((x - radius, y - radius, x + radius, y + radius), fill=0)
            drawn += 1
    return drawn


def draw_water(
    draw: ImageDraw.ImageDraw,
    frame,
    zoom: int,
    tile_range: TileRange,
    scale: int,
) -> int:
    if frame is None or frame.empty:
        return 0
    drawn = 0
    for _, row in frame.iterrows():
        geometry = row.geometry
        polygons = list(iter_polygons(geometry))
        if polygons:
            for polygon in polygons:
                exterior = geometry_points(
                    LineString(polygon.exterior.coords), zoom, tile_range, scale
                )
                if len(exterior) >= 3:
                    draw.polygon(exterior, fill=235, outline=0)
                    min_x, min_y, max_x, max_y = polygon.bounds
                    c0 = world_to_canvas(min_x, max_y, zoom, tile_range, scale)
                    c1 = world_to_canvas(max_x, min_y, zoom, tile_range, scale)
                    left, top = min(c0[0], c1[0]), min(c0[1], c1[1])
                    right, bottom = max(c0[0], c1[0]), max(c0[1], c1[1])
                    spacing = 10 * scale
                    for offset in range(left - (bottom - top), right + spacing, spacing):
                        draw.line(
                            (offset, bottom, offset + (bottom - top), top),
                            fill=170,
                            width=max(1, scale),
                        )
                    draw.line(exterior + [exterior[0]], fill=0, width=max(1, scale))
                    drawn += 1
        else:
            for line in iter_lines(geometry):
                points = geometry_points(line, zoom, tile_range, scale)
                if len(points) >= 2:
                    draw.line(points, fill=45, width=2 * scale)
                    drawn += 1
    return drawn


def draw_buildings(
    draw: ImageDraw.ImageDraw,
    frame,
    zoom: int,
    tile_range: TileRange,
    scale: int,
    detail: str,
) -> int:
    if detail == "emergency" or frame is None or frame.empty:
        return 0
    drawn = 0
    for _, row in frame.iterrows():
        for polygon in iter_polygons(row.geometry):
            points = geometry_points(
                LineString(polygon.exterior.coords), zoom, tile_range, scale
            )
            if len(points) >= 3:
                fill = 185 if detail == "street" else 150
                draw.polygon(points, fill=fill, outline=80)
                drawn += 1
    return drawn


def load_font(path: Path | None, size: int):
    if path is not None:
        try:
            return ImageFont.truetype(str(path), size=size)
        except OSError as exc:
            raise SystemExit(f"cannot load font {path}: {exc}") from exc
    return ImageFont.load_default()


def draw_places(
    draw: ImageDraw.ImageDraw,
    frame,
    zoom: int,
    tile_range: TileRange,
    scale: int,
    font,
    max_labels: int,
) -> int:
    if frame is None or frame.empty or max_labels <= 0:
        return 0

    candidates = []
    for _, row in frame.iterrows():
        name = ""
        for key in ("name", "name_en"):
            if key in row and row[key] is not None:
                name = str(row[key]).strip()
                if name:
                    break
        if not name:
            continue
        rank = 99
        fclass = str(row.get("fclass", "")).lower()
        if fclass in {"city", "town"}:
            rank = 0
        elif fclass in {"village", "suburb"}:
            rank = 1
        elif fclass in {"neighbourhood", "locality"}:
            rank = 2
        for point in iter_points(row.geometry):
            candidates.append((rank, name, point))

    candidates.sort(key=lambda item: (item[0], len(item[1])))
    occupied: list[tuple[int, int, int, int]] = []
    drawn = 0

    for _, name, point in candidates:
        if drawn >= max_labels:
            break
        x, y = world_to_canvas(point.x, point.y, zoom, tile_range, scale)
        bbox = draw.textbbox((x, y), name, font=font, stroke_width=1 * scale)
        box_with_margin = (bbox[0] - 3, bbox[1] - 2, bbox[2] + 3, bbox[3] + 2)
        if any(
            not (
                box_with_margin[2] < other[0]
                or box_with_margin[0] > other[2]
                or box_with_margin[3] < other[1]
                or box_with_margin[1] > other[3]
            )
            for other in occupied
        ):
            continue
        draw.rectangle(box_with_margin, fill=255)
        draw.text(
            (x, y),
            name,
            font=font,
            fill=0,
            stroke_width=max(1, scale // 2),
            stroke_fill=255,
        )
        occupied.append(box_with_margin)
        drawn += 1

    return drawn


def write_ftm(path: Path, tile: Image.Image, threshold: int) -> None:
    gray = tile.convert("L")
    payload = bytearray(TILE_SIZE * STRIDE)
    pixels = gray.load()

    for y in range(TILE_SIZE):
        for x in range(TILE_SIZE):
            if pixels[x, y] < threshold:
                payload[y * STRIDE + x // 8] |= 0x80 >> (x % 8)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        FTM_HEADER.pack(
            b"FTM1",
            TILE_SIZE,
            TILE_SIZE,
            STRIDE,
            1,
            0,
            len(payload),
        )
        + payload
    )


def default_start_tile(bounds: Bounds, zoom: int, tile_range: TileRange) -> tuple[int, int]:
    center_x, center_y = lonlat_to_global_pixel(bounds.center_lon, bounds.center_lat, zoom)
    center_tile_x = int(center_x // TILE_SIZE)
    center_tile_y = int(center_y // TILE_SIZE)

    # v1.93 viewport is 4 x 2 tiles. Start near the center of that viewport.
    start_x = center_tile_x - 2
    start_y = center_tile_y - 1

    max_start_x = max(tile_range.min_x, tile_range.max_x - 3)
    max_start_y = max(tile_range.min_y, tile_range.max_y - 1)
    start_x = max(tile_range.min_x, min(start_x, max_start_x))
    start_y = max(tile_range.min_y, min(start_y, max_start_y))
    return start_x, start_y


def write_map_config(
    output: Path,
    name: str,
    zoom: int,
    bounds: Bounds,
    tile_range: TileRange,
    start_x: int,
    start_y: int,
) -> None:
    config = "\n".join(
        [
            "FTMAP1",
            f"name={name}",
            "attribution=(c) OpenStreetMap contributors",
            f"zoom={zoom}",
            f"min_x={tile_range.min_x}",
            f"max_x={tile_range.max_x}",
            f"min_y={tile_range.min_y}",
            f"max_y={tile_range.max_y}",
            f"start_x={start_x}",
            f"start_y={start_y}",
            # v1.93 ignores unknown keys; these prepare geographic navigation.
            "projection=web_mercator",
            f"west={bounds.west:.7f}",
            f"south={bounds.south:.7f}",
            f"east={bounds.east:.7f}",
            f"north={bounds.north:.7f}",
            f"center_lon={bounds.center_lon:.7f}",
            f"center_lat={bounds.center_lat:.7f}",
            "source=openstreetmap",
            "license=ODbL-1.0",
            "",
        ]
    )
    (output / "map.cfg").write_text(config, encoding="utf-8")


def build_map(args: argparse.Namespace) -> dict:
    bounds = Bounds(args.west, args.south, args.east, args.north)
    bounds.validate()
    if not (0 <= args.zoom <= 22):
        raise ValueError("zoom must be between 0 and 22")
    if not (1 <= args.threshold <= 254):
        raise ValueError("threshold must be between 1 and 254")

    tile_range = tile_range_for_bounds(bounds, args.zoom)
    if tile_range.tile_count > args.max_tiles:
        raise ValueError(
            f"requested area needs {tile_range.tile_count} tiles, "
            f"above --max-tiles={args.max_tiles}. Reduce bbox or zoom."
        )

    scale = args.render_scale
    width = tile_range.columns * TILE_SIZE * scale
    height = tile_range.rows * TILE_SIZE * scale
    if width * height > args.max_render_pixels:
        raise ValueError(
            f"render canvas {width}x{height} exceeds --max-render-pixels"
        )

    catalog = SourceCatalog(args.source)
    print(f"Source: {catalog.source}")
    print(
        f"Tiles: z{args.zoom} "
        f"x={tile_range.min_x}..{tile_range.max_x} "
        f"y={tile_range.min_y}..{tile_range.max_y} "
        f"({tile_range.columns}x{tile_range.rows})"
    )

    canvas = Image.new("L", (width, height), 255)
    draw = ImageDraw.Draw(canvas)

    stats: dict[str, int] = {}

    water_polygons = read_layer(catalog, WATER_FILES[0], bounds)
    water_lines = read_layer(catalog, WATER_FILES[1], bounds)
    stats["water_polygons"] = draw_water(
        draw, water_polygons, args.zoom, tile_range, scale
    )
    stats["waterways"] = draw_water(
        draw, water_lines, args.zoom, tile_range, scale
    )

    if args.detail != "emergency":
        buildings = read_layer(catalog, BUILDING_FILES[0], bounds)
    else:
        buildings = None
    stats["buildings"] = draw_buildings(
        draw, buildings, args.zoom, tile_range, scale, args.detail
    )

    roads = read_layer(catalog, ROAD_FILES[0], bounds)
    railways = read_layer(catalog, ROAD_FILES[1], bounds)
    stats["roads"] = draw_roads(
        draw, roads, args.zoom, tile_range, scale, args.detail
    )
    stats["railways"] = draw_railways(
        draw, railways, args.zoom, tile_range, scale
    )

    places = read_layer(catalog, PLACE_FILES[0], bounds)
    font = load_font(args.font, args.label_size * scale)
    stats["labels"] = draw_places(
        draw,
        places,
        args.zoom,
        tile_range,
        scale,
        font,
        args.max_labels,
    )

    if scale > 1:
        canvas = canvas.resize(
            (tile_range.columns * TILE_SIZE, tile_range.rows * TILE_SIZE),
            Image.Resampling.LANCZOS,
        )

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    preview = canvas.convert("L")
    preview.save(output / "preview.png")

    for row in range(tile_range.rows):
        for column in range(tile_range.columns):
            tile_x = tile_range.min_x + column
            tile_y = tile_range.min_y + row
            tile = preview.crop(
                (
                    column * TILE_SIZE,
                    row * TILE_SIZE,
                    (column + 1) * TILE_SIZE,
                    (row + 1) * TILE_SIZE,
                )
            )
            write_ftm(
                output / str(args.zoom) / str(tile_x) / f"{tile_y}.ftm",
                tile,
                args.threshold,
            )

    start_x, start_y = default_start_tile(bounds, args.zoom, tile_range)
    write_map_config(
        output,
        args.name,
        args.zoom,
        bounds,
        tile_range,
        start_x,
        start_y,
    )

    manifest = {
        "format": "FT02_MAP_PACKAGE_1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "name": args.name,
        "source": str(catalog.source),
        "data": "OpenStreetMap / Geofabrik",
        "license": "ODbL-1.0",
        "attribution": "(c) OpenStreetMap contributors",
        "zoom": args.zoom,
        "detail": args.detail,
        "threshold": args.threshold,
        "bbox": {
            "west": bounds.west,
            "south": bounds.south,
            "east": bounds.east,
            "north": bounds.north,
        },
        "tile_range": {
            "min_x": tile_range.min_x,
            "max_x": tile_range.max_x,
            "min_y": tile_range.min_y,
            "max_y": tile_range.max_y,
            "columns": tile_range.columns,
            "rows": tile_range.rows,
            "count": tile_range.tile_count,
        },
        "start_tile": {"x": start_x, "y": start_y},
        "render_stats": stats,
    }
    (output / "map_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    print(f"Output: {output}")
    print(f"Generated {tile_range.tile_count} FTM tiles")
    print("Render stats:", json.dumps(stats, ensure_ascii=False))
    return manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build FT-02 monochrome map tiles from OSM shapefiles."
    )
    parser.add_argument("source", type=Path, help="Geofabrik .shp.zip or directory")
    parser.add_argument("output", type=Path, help="output maps/current directory")
    parser.add_argument("--west", required=True, type=float)
    parser.add_argument("--south", required=True, type=float)
    parser.add_argument("--east", required=True, type=float)
    parser.add_argument("--north", required=True, type=float)
    parser.add_argument("--zoom", type=int, default=15)
    parser.add_argument("--name", default="FT-02 Offline Map")
    parser.add_argument(
        "--detail",
        choices=("emergency", "street", "dense"),
        default="street",
        help="emergency=major roads only, street=normal, dense=include more paths/buildings",
    )
    parser.add_argument("--threshold", type=int, default=180)
    parser.add_argument("--render-scale", type=int, choices=(1, 2, 3, 4), default=2)
    parser.add_argument("--font", type=Path, help="optional TTF/OTF font for labels")
    parser.add_argument("--label-size", type=int, default=12)
    parser.add_argument("--max-labels", type=int, default=80)
    parser.add_argument("--max-tiles", type=int, default=512)
    parser.add_argument("--max-render-pixels", type=int, default=180_000_000)
    return parser.parse_args()


def main() -> None:
    try:
        build_map(parse_args())
    except (ValueError, FileNotFoundError) as exc:
        raise SystemExit(f"ERROR: {exc}") from exc


if __name__ == "__main__":
    main()
