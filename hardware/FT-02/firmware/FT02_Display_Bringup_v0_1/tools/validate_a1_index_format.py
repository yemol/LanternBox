#!/usr/bin/env python3
"""Host-side validator for FT-02 PBF Persistent Index A1.

This mirrors the firmware's FTPBI1 disk format and scans an untouched OSM PBF
with only the Python standard library. It is a validation tool, not required by
FT-02 at runtime.
"""

from __future__ import annotations

import argparse
import json
import struct
import time
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

HEADER = struct.Struct("<8sHHHHQIIIIIQQQQQIIIIiiii12s")
ENTRY = struct.Struct("<QIIIIIIIiiiiBBH8s")
assert HEADER.size == 128
assert ENTRY.size == 64

MAGIC = b"FTPBI1\0\0"
VERSION = 1
FLAG_SOURCE_BOUNDS = 0x0001
ENTRY_FLAG_NODE_BOUNDS = 0x0001


def read_varint(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while pos < len(data) and shift <= 63:
        b = data[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not (b & 0x80):
            return value, pos
        shift += 7
    raise ValueError("invalid varint")


def zigzag(value: int) -> int:
    return (value >> 1) ^ -(value & 1)


def fields(data: bytes) -> Iterator[tuple[int, int, int | bytes]]:
    pos = 0
    while pos < len(data):
        key, pos = read_varint(data, pos)
        field = key >> 3
        wire = key & 7
        if wire == 0:
            value, pos = read_varint(data, pos)
            yield field, wire, value
        elif wire == 1:
            value = data[pos : pos + 8]
            if len(value) != 8:
                raise ValueError("truncated fixed64")
            pos += 8
            yield field, wire, value
        elif wire == 2:
            length, pos = read_varint(data, pos)
            value = data[pos : pos + length]
            if len(value) != length:
                raise ValueError("truncated bytes")
            pos += length
            yield field, wire, value
        elif wire == 5:
            value = data[pos : pos + 4]
            if len(value) != 4:
                raise ValueError("truncated fixed32")
            pos += 4
            yield field, wire, value
        else:
            raise ValueError(f"unsupported wire {wire}")


def packed_varints(data: bytes) -> Iterator[int]:
    pos = 0
    while pos < len(data):
        value, pos = read_varint(data, pos)
        yield value


@dataclass
class Bounds:
    valid: bool = False
    min_lat: int = 0
    min_lon: int = 0
    max_lat: int = 0
    max_lon: int = 0

    def add(self, lat: int, lon: int) -> None:
        if not self.valid:
            self.valid = True
            self.min_lat = self.max_lat = lat
            self.min_lon = self.max_lon = lon
            return
        self.min_lat = min(self.min_lat, lat)
        self.max_lat = max(self.max_lat, lat)
        self.min_lon = min(self.min_lon, lon)
        self.max_lon = max(self.max_lon, lon)

    def merge(self, other: "Bounds") -> None:
        if other.valid:
            self.add(other.min_lat, other.min_lon)
            self.add(other.max_lat, other.max_lon)


def nanodeg_to_e7(value: int) -> int:
    return int(value / 100)


def parse_blob_header(data: bytes) -> tuple[str, int]:
    block_type = None
    data_size = None
    for field, wire, value in fields(data):
        if field == 1 and wire == 2:
            block_type = bytes(value).decode("ascii")
        elif field == 3 and wire == 0:
            data_size = int(value)
    if block_type is None or data_size is None:
        raise ValueError("invalid BlobHeader")
    return block_type, data_size


def parse_blob(data: bytes) -> tuple[bytes, int, int]:
    raw = None
    raw_size = 0
    zlib_data = None
    for field, wire, value in fields(data):
        if field == 1 and wire == 2:
            raw = bytes(value)
        elif field == 2 and wire == 0:
            raw_size = int(value)
        elif field == 3 and wire == 2:
            zlib_data = bytes(value)
        elif field in (4, 5, 6, 7) and wire == 2:
            raise ValueError("unsupported compression")
    if raw is not None:
        return raw, len(raw), 0
    if zlib_data is None or raw_size <= 0:
        raise ValueError("invalid Blob")
    inflated = zlib.decompress(zlib_data)
    if len(inflated) != raw_size:
        raise ValueError("raw_size mismatch")
    return inflated, len(zlib_data), 1


def parse_header_bounds(data: bytes) -> Bounds:
    result = Bounds()
    for field, wire, value in fields(data):
        if field != 1 or wire != 2:
            continue
        vals: dict[int, int] = {}
        for f, w, v in fields(bytes(value)):
            if w == 0 and f in (1, 2, 3, 4):
                vals[f] = zigzag(int(v))
        if all(k in vals for k in (1, 2, 3, 4)):
            result.valid = True
            result.min_lon = nanodeg_to_e7(vals[1])
            result.max_lon = nanodeg_to_e7(vals[2])
            result.max_lat = nanodeg_to_e7(vals[3])
            result.min_lat = nanodeg_to_e7(vals[4])
        break
    return result


def parse_regular_node(data: bytes, gran: int, lat_off: int, lon_off: int, bounds: Bounds) -> None:
    lat = lon = None
    for field, wire, value in fields(data):
        if field == 8 and wire == 0:
            lat = zigzag(int(value))
        elif field == 9 and wire == 0:
            lon = zigzag(int(value))
    if lat is not None and lon is not None:
        bounds.add(nanodeg_to_e7(lat_off + gran * lat), nanodeg_to_e7(lon_off + gran * lon))


def parse_dense_nodes(data: bytes, gran: int, lat_off: int, lon_off: int, bounds: Bounds) -> int:
    ids: list[int] = []
    lats: list[int] = []
    lons: list[int] = []
    for field, wire, value in fields(data):
        if field == 1:
            if wire == 2:
                ids.extend(packed_varints(bytes(value)))
            elif wire == 0:
                ids.append(int(value))
        elif field == 8 and wire == 2:
            lats.extend(packed_varints(bytes(value)))
        elif field == 9 and wire == 2:
            lons.extend(packed_varints(bytes(value)))
    if len(lats) != len(lons):
        raise ValueError("dense lat/lon mismatch")
    lat = lon = 0
    for encoded_lat, encoded_lon in zip(lats, lons):
        lat += zigzag(encoded_lat)
        lon += zigzag(encoded_lon)
        bounds.add(nanodeg_to_e7(lat_off + gran * lat), nanodeg_to_e7(lon_off + gran * lon))
    return len(ids)


def parse_primitive_block(data: bytes) -> tuple[int, int, int, Bounds]:
    gran = 100
    lat_off = 0
    lon_off = 0
    groups: list[bytes] = []
    for field, wire, value in fields(data):
        if field == 2 and wire == 2:
            groups.append(bytes(value))
        elif field == 17 and wire == 0:
            gran = int(value)
        elif field == 19 and wire == 0:
            lat_off = int(value)
            if lat_off >= 1 << 63:
                lat_off -= 1 << 64
        elif field == 20 and wire == 0:
            lon_off = int(value)
            if lon_off >= 1 << 63:
                lon_off -= 1 << 64

    nodes = ways = relations = 0
    bounds = Bounds()
    for group in groups:
        for field, wire, value in fields(group):
            if wire != 2:
                continue
            message = bytes(value)
            if field == 1:
                nodes += 1
                parse_regular_node(message, gran, lat_off, lon_off, bounds)
            elif field == 2:
                nodes += parse_dense_nodes(message, gran, lat_off, lon_off, bounds)
            elif field == 3:
                ways += 1
            elif field == 4:
                relations += 1
    return nodes, ways, relations, bounds


def source_signature(path: Path) -> int:
    size = path.stat().st_size
    sample_size = 32 * 1024
    positions = [0, max(0, size // 2 - sample_size // 2), max(0, size - sample_size)]
    crc = zlib.crc32(struct.pack("<Q", size))
    with path.open("rb") as f:
        last = None
        for pos in positions:
            if pos == last:
                continue
            last = pos
            f.seek(pos)
            data = f.read(min(sample_size, size - pos))
            crc = zlib.crc32(struct.pack("<Q", pos), crc)
            crc = zlib.crc32(data, crc)
    return crc & 0xFFFFFFFF


def build(source: Path, output: Path | None) -> dict[str, object]:
    started = time.monotonic()
    size = source.stat().st_size
    signature = source_signature(source)
    entries: list[bytes] = []
    source_bounds = Bounds()
    observed_bounds = Bounds()

    file_blocks = header_blocks = data_blocks = 0
    total_nodes = total_ways = total_relations = 0
    raw_total = compressed_total = 0
    max_blob = max_raw = 0

    with source.open("rb") as f:
        while True:
            offset = f.tell()
            prefix = f.read(4)
            if not prefix:
                break
            if len(prefix) != 4:
                raise ValueError("truncated prefix")
            header_len = int.from_bytes(prefix, "big")
            blob_header = f.read(header_len)
            if len(blob_header) != header_len:
                raise ValueError("truncated header")
            block_type, blob_len = parse_blob_header(blob_header)
            blob = f.read(blob_len)
            if len(blob) != blob_len:
                raise ValueError("truncated blob")
            raw, compressed_len, compression = parse_blob(blob)

            nodes = ways = relations = 0
            bounds = Bounds()
            block_type_id = 0
            flags = 0
            if block_type == "OSMHeader":
                block_type_id = 1
                header_blocks += 1
                source_bounds.merge(parse_header_bounds(raw))
            elif block_type == "OSMData":
                block_type_id = 2
                data_blocks += 1
                nodes, ways, relations, bounds = parse_primitive_block(raw)
                if bounds.valid:
                    flags |= ENTRY_FLAG_NODE_BOUNDS
                    observed_bounds.merge(bounds)
                total_nodes += nodes
                total_ways += ways
                total_relations += relations

            entries.append(ENTRY.pack(
                offset, header_len, blob_len, len(raw), compressed_len,
                nodes, ways, relations,
                bounds.min_lat if bounds.valid else 0,
                bounds.min_lon if bounds.valid else 0,
                bounds.max_lat if bounds.valid else 0,
                bounds.max_lon if bounds.valid else 0,
                block_type_id, compression, flags, b"\0" * 8,
            ))
            file_blocks += 1
            raw_total += len(raw)
            compressed_total += compressed_len
            max_blob = max(max_blob, blob_len)
            max_raw = max(max_raw, len(raw))

    if not source_bounds.valid:
        source_bounds.merge(observed_bounds)

    entry_bytes = b"".join(entries)
    elapsed_ms = int((time.monotonic() - started) * 1000)
    header = HEADER.pack(
        MAGIC, VERSION, HEADER.size, ENTRY.size,
        FLAG_SOURCE_BOUNDS if source_bounds.valid else 0,
        size, signature, len(entries), file_blocks, header_blocks, data_blocks,
        total_nodes, total_ways, total_relations, raw_total, compressed_total,
        max_blob, max_raw, elapsed_ms, zlib.crc32(entry_bytes) & 0xFFFFFFFF,
        source_bounds.min_lat if source_bounds.valid else 0,
        source_bounds.min_lon if source_bounds.valid else 0,
        source_bounds.max_lat if source_bounds.valid else 0,
        source_bounds.max_lon if source_bounds.valid else 0,
        b"\0" * 12,
    )

    if output is not None:
        output.write_bytes(header + entry_bytes)

    return {
        "source_bytes": size,
        "source_signature": f"0x{signature:08X}",
        "index_bytes": len(header) + len(entry_bytes),
        "entries": len(entries),
        "file_blocks": file_blocks,
        "header_blocks": header_blocks,
        "data_blocks": data_blocks,
        "nodes": total_nodes,
        "ways": total_ways,
        "relations": total_relations,
        "raw_payload_bytes": raw_total,
        "compressed_payload_bytes": compressed_total,
        "max_blob_bytes": max_blob,
        "max_raw_block_bytes": max_raw,
        "bounds_e7": {
            "min_lat": source_bounds.min_lat,
            "min_lon": source_bounds.min_lon,
            "max_lat": source_bounds.max_lat,
            "max_lon": source_bounds.max_lon,
        },
        "elapsed_ms": elapsed_ms,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()

    report = build(args.source, args.output)
    text = json.dumps(report, ensure_ascii=False, indent=2)
    print(text)
    if args.report:
        args.report.write_text(text + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
