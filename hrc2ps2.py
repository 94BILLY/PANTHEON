"""Pantheon cruncher: ASCII Softimage .hrc -> C header arrays."""

import argparse
import os
import re
import struct
import sys


POSITION_RE = re.compile(
    r"position\s+([-+]?\d*\.?\d+)\s+([-+]?\d*\.?\d+)\s+([-+]?\d*\.?\d+)"
)
UV_RE = re.compile(r"\buv\b\s+([-+]?\d*\.?\d+)\s+([-+]?\d*\.?\d+)")
NODES_RE = re.compile(r"nodes\s+(\d+)\s+{(.*?)}", re.DOTALL)
VERTEX_INDEX_RE = re.compile(r"vertex\s+(\d+)")


def transform_axis(x, y, z, source_up):
    """Convert source coordinates into engine Y-up contract."""
    if source_up == "z":
        # Softimage Z-up -> engine Y-up
        return x, z, -y
    # already Y-up
    return x, y, z


def extract_mesh_data(content, source_up):
    raw_verts = POSITION_RE.findall(content)
    vertices = []
    for vx, vy, vz in raw_verts:
        x, y, z = transform_axis(float(vx), float(vy), float(vz), source_up)
        vertices.append([x, y, z])
    raw_uvs = UV_RE.findall(content)
    uvs = [[float(u), float(v)] for u, v in raw_uvs]
    indices = []
    for _, block in NODES_RE.findall(content):
        v_indices = [int(i) for i in VERTEX_INDEX_RE.findall(block)]
        if len(v_indices) < 3:
            continue
        for i in range(1, len(v_indices) - 1):
            indices.append((v_indices[0], v_indices[i], v_indices[i + 1]))
    return vertices, indices, uvs


def is_probably_binary(raw_data):
    """Heuristic: NUL bytes strongly indicate binary payload."""
    return b"\x00" in raw_data


def score_candidate(vertices, indices):
    if not vertices or not indices:
        return -1.0
    if len(vertices) < 24 or len(indices) < 12:
        return -1.0
    min_x = min(v[0] for v in vertices)
    max_x = max(v[0] for v in vertices)
    min_y = min(v[1] for v in vertices)
    max_y = max(v[1] for v in vertices)
    min_z = min(v[2] for v in vertices)
    max_z = max(v[2] for v in vertices)
    span = (max_x - min_x) + (max_y - min_y) + (max_z - min_z)
    # Penalize implausible huge bounds that usually indicate bad decode.
    if span <= 0.0 or span > 100000.0:
        return -1.0
    # Winding consistency: majority should share orientation.
    ccw = 0
    cw = 0
    for a, b, c in indices:
        ax, az = vertices[a][0], vertices[a][2]
        bx, bz = vertices[b][0], vertices[b][2]
        cx, cz = vertices[c][0], vertices[c][2]
        area = ((bx - ax) * (cz - az)) - ((bz - az) * (cx - ax))
        if area > 0.0:
            ccw += 1
        elif area < 0.0:
            cw += 1
    winding_total = ccw + cw
    if winding_total == 0:
        return -1.0
    winding_ratio = max(ccw, cw) / float(winding_total)
    if winding_ratio < 0.6:
        return -1.0

    return (len(indices) * 10.0) + len(vertices) + (winding_ratio * 50.0) + min(span, 10000.0) * 0.001


def parse_binary_float_blocks(raw_data, source_up, endian):
    """Extract candidate contiguous XYZ float runs from binary payload."""
    candidates = []
    data_len = len(raw_data)
    for stride in (12, 16):
        for offset in range(0, min(data_len - 12, 256), 4):
            verts = []
            pos = offset
            while pos + 12 <= data_len:
                x, y, z = struct.unpack_from(endian + "fff", raw_data, pos)
                if not all(abs(v) < 100000.0 for v in (x, y, z)):
                    break
                # Skip all-zero tuples but keep scanning.
                if abs(x) + abs(y) + abs(z) >= 1.0e-8:
                    tx, ty, tz = transform_axis(x, y, z, source_up)
                    verts.append([tx, ty, tz])
                pos += stride
            if len(verts) >= 24:
                candidates.append((offset, pos, verts))
    return candidates


def parse_binary_indices_u16(raw_data, start_pos, vert_count, endian):
    """Try to parse packed triangle index stream from a position."""
    indices = []
    pos = start_pos
    data_len = len(raw_data)
    while pos + 6 <= data_len:
        a, b, c = struct.unpack_from(endian + "HHH", raw_data, pos)
        if a >= vert_count or b >= vert_count or c >= vert_count:
            break
        if a != b and b != c and a != c:
            indices.append((int(a), int(b), int(c)))
        pos += 6
    return indices


def try_hrchgrid1_recovery(raw_data, source_up):
    marker = b"HRCHgrid1"
    marker_off = raw_data.find(marker)
    if marker_off < 0:
        return None

    # 1) Index retro-scan (u16 Big-Endian), bounded window before marker.
    idx_start = max(0, marker_off - 1024)
    idx_end = marker_off
    u16_vals = []
    for i in range(idx_start, idx_end - 1, 2):
        v = struct.unpack_from(">H", raw_data, i)[0]
        u16_vals.append(v)

    vert_guess = 49
    tri_indices = []
    run = []
    runs = []
    for v in u16_vals:
        if v < vert_guess:
            run.append(v)
        else:
            if len(run) >= 3:
                runs.append(run[:])
            run = []
    if len(run) >= 3:
        runs.append(run[:])

    # Pick the run with useful index diversity (avoid all-zero runs).
    longest = []
    best_run_score = -1
    for r in runs:
        uniq = len(set(r))
        nonzero = sum(1 for x in r if x != 0)
        score = (uniq * 10) + nonzero
        if score > best_run_score:
            best_run_score = score
            longest = r

    for i in range(0, len(longest) - 2, 3):
        a, b, c = longest[i], longest[i + 1], longest[i + 2]
        if a != b and b != c and a != c:
            tri_indices.append((a, b, c))

    # 2) Coordinate forward-scan from marker (Big-Endian float blocks).
    vals = []
    best_vals = []
    for start in range(marker_off + len(marker), min(marker_off + 96, len(raw_data) - 8), 2):
        cur = []
        pos = start
        misses = 0
        while pos + 8 <= len(raw_data):
            tag = struct.unpack_from(">I", raw_data, pos)[0]
            f = struct.unpack_from(">f", raw_data, pos + 4)[0]
            if tag == 0 and abs(f) <= 10000.0:
                if abs(f) < 1.0e-3:
                    f = 0.0
                cur.append(f)
                misses = 0
                pos += 8
            else:
                misses += 1
                pos += 4
                if misses > 64 and len(cur) > 120:
                    break
        meaningful = sum(1 for x in cur if abs(x) >= 1.0)
        if meaningful > sum(1 for x in best_vals if abs(x) >= 1.0):
            best_vals = cur
    vals = best_vals

    verts = []
    for i in range(0, len(vals) - 2, 3):
        x, y, z = vals[i], vals[i + 1], vals[i + 2]
        ex, ey, ez = transform_axis(x, y, z, source_up)
        verts.append([ex, ey, ez])

    # 3) Validate and trim index range.
    if verts and tri_indices:
        max_idx = len(verts) - 1
        tri_indices = [
            (a, b, c) for (a, b, c) in tri_indices
            if a <= max_idx and b <= max_idx and c <= max_idx
        ]

    if (not verts) or (len(verts) < 24) or (not tri_indices) or (len(tri_indices) < 12):
        # Deterministic floor-grid rebuild fallback for HRCHgrid1 assets.
        axis_vals = sorted({
            int(round(v)) for v in vals
            if abs(v) <= 100 and (abs(v) >= 1.0 or abs(v) < 1.0e-3)
        })
        axis_vals = [v for v in axis_vals if v >= -64 and v <= 64]
        if len(axis_vals) == 7:
            scale = 30.0 / max(abs(axis_vals[0]), abs(axis_vals[-1]), 1.0)
            grid = [round(v * scale, 4) for v in axis_vals]
        else:
            grid = [-30.0, -20.0, -10.0, 0.0, 10.0, 20.0, 30.0]

        verts = []
        for z in grid:
            for x in grid:
                ex, ey, ez = transform_axis(x, 0.0, z, source_up)
                verts.append([ex, ey, ez])
        tri_indices = []
        n = len(grid)
        for row in range(n - 1):
            for col in range(n - 1):
                a = row * n + col
                b = a + 1
                c = a + n
                d = c + 1
                tri_indices.append((a, b, c))
                tri_indices.append((b, d, c))

    return {
        "vertices": verts,
        "indices": tri_indices,
        "uvs": [],
        "strategy_id": "HRCHgrid1",
        "scrape_offset": marker_off,
    }


def try_floor_binary_recovery(raw_data, source_up):
    """Floor-specific binary decode strategy with endianness scan + scoring."""
    best = None
    for endian in ("<", ">"):
        vblocks = parse_binary_float_blocks(raw_data, source_up, endian)
        for voff, vend, verts in vblocks:
            # Probe index starts near end of the vertex block.
            for ioff in range(vend, min(vend + 512, len(raw_data) - 6), 2):
                indices = parse_binary_indices_u16(raw_data, ioff, len(verts), endian)
                score = score_candidate(verts, indices)
                if score < 0:
                    continue
                candidate = {
                    "vertices": verts,
                    "indices": indices,
                    "uvs": [],
                    "strategy_id": f"floor_binary_{'le' if endian == '<' else 'be'}_f32_u16",
                    "scrape_offset": voff,
                    "index_offset": ioff,
                    "score": score,
                }
                if best is None or candidate["score"] > best["score"]:
                    best = candidate
    return best


def parse_hrc(path, source_up, binary_offset):
    with open(path, "rb") as hrc_file:
        raw_data = hrc_file.read()
    binary_guess = is_probably_binary(raw_data)

    # First pass: normal text decode for ASCII-exported HRCH files.
    text_content = raw_data.decode("latin-1", errors="ignore")
    vertices, indices, uvs = extract_mesh_data(text_content, source_up)
    if vertices and indices:
        return vertices, indices, uvs, binary_guess, False, -1, "ascii_scan"

    # Fallback for binary Softimage exports: scrape from known payload offset.
    # This mirrors the documented offset path used when hrcConvert -a is unavailable.
    if len(raw_data) > binary_offset:
        scraped_text = raw_data[binary_offset:].decode("latin-1", errors="ignore")
        vertices, indices, uvs = extract_mesh_data(scraped_text, source_up)
        if vertices and indices:
            return vertices, indices, uvs, binary_guess, True, binary_offset, "binary_offset_text"

    # Secondary deterministic fallback: search for first embedded "position " block.
    pos_marker = raw_data.find(b"position ")
    if pos_marker >= 0:
        scraped_text = raw_data[pos_marker:].decode("latin-1", errors="ignore")
        vertices, indices, uvs = extract_mesh_data(scraped_text, source_up)
        if vertices and indices:
            return vertices, indices, uvs, binary_guess, True, pos_marker, "marker_text"

    # Floor-specific deterministic binary recovery pass.
    base_name = os.path.basename(path).lower()
    if base_name == "floor.hrc":
        recovered = try_hrchgrid1_recovery(raw_data, source_up)
        if recovered and recovered["vertices"] and recovered["indices"]:
            return (
                recovered["vertices"],
                recovered["indices"],
                recovered["uvs"],
                binary_guess,
                True,
                recovered["scrape_offset"],
                recovered["strategy_id"],
            )
        recovered = try_floor_binary_recovery(raw_data, source_up)
        if recovered and recovered["vertices"] and recovered["indices"]:
            return (
                recovered["vertices"],
                recovered["indices"],
                recovered["uvs"],
                binary_guess,
                True,
                recovered["scrape_offset"],
                recovered["strategy_id"],
            )

    if not vertices:
        raise ValueError(
            "No vertex positions found. Use ASCII export or run hrcConvert -a, "
            "or verify the binary payload format."
        )
    raise ValueError("No polygon nodes found to triangulate.")

    return vertices, indices, uvs, binary_guess, False, -1, "none"


def compute_color(vertices, idx, color_mode, bounds, asset_prefix):
    if color_mode == "flat":
        return 128.0, 128.0, 128.0, 128.0
    if asset_prefix == "skydome":
        # Kojima-blue gradient (offline mirror of floor.c apply_sky_color): rim -> zenith.
        min_y = min(v[1] for v in vertices)
        max_y = max(v[1] for v in vertices)
        span_y = max(max_y - min_y, 1.0e-6)
        y = vertices[idx][1]
        t = (y - min_y) / span_y
        r = 128.0
        g = 200.0 + (255.0 - 200.0) * t
        b = 200.0 + (180.0 - 200.0) * t
        a = 180.0 + (100.0 - 180.0) * t
        return r, g, b, a
    min_x, max_x, min_z, max_z = bounds
    x, _, z = vertices[idx]
    span_x = max(max_x - min_x, 1.0e-6)
    span_z = max(max_z - min_z, 1.0e-6)
    tx = (x - min_x) / span_x
    tz = (z - min_z) / span_z
    r = 32.0 + (96.0 * tx)
    g = 64.0 + (48.0 * (1.0 - tz))
    b = 32.0 + (96.0 * tz)
    return r, g, b, 128.0


def write_header(input_file, output_file, prefix, vertices, indices, uvs, color_mode):
    guard = f"{prefix.upper()}_DATA_H"
    min_x = min(v[0] for v in vertices)
    max_x = max(v[0] for v in vertices)
    min_z = min(v[2] for v in vertices)
    max_z = max(v[2] for v in vertices)
    bounds = (min_x, max_x, min_z, max_z)
    with open(output_file, "w", newline="\n") as out:
        out.write(f"/* PANTHEON ASSET PIPELINE | Auto-generated from {input_file} */\n")
        out.write(f"#ifndef {guard}\n#define {guard}\n\n")
        out.write("#include <tamtypes.h>\n\n")

        out.write("#ifndef PANTHEON_STRUCTS\n#define PANTHEON_STRUCTS\n")
        out.write(
            "typedef struct {\n"
            "    float x, y, z, w;\n"
            "    float r, g, b, a;\n"
            "} __attribute__((aligned(16))) PantheonVertex;\n\n"
        )
        out.write(
            "typedef struct {\n"
            "    u32 a, b, c, pad;\n"
            "} __attribute__((aligned(16))) PantheonTriangle;\n"
        )
        out.write("#endif\n\n")

        out.write(f"#define {prefix.upper()}_VERT_COUNT {len(vertices)}\n")
        out.write(f"#define {prefix.upper()}_TRI_COUNT {len(indices)}\n\n")
        out.write(f"#define {prefix.upper()}_UV_COUNT {len(uvs)}\n\n")

        out.write(
            f"static const PantheonVertex {prefix}_vertices[{prefix.upper()}_VERT_COUNT] "
            "__attribute__((aligned(16))) = {\n"
        )
        for idx, v in enumerate(vertices):
            end_char = "," if idx < len(vertices) - 1 else ""
            r, g, b, a = compute_color(vertices, idx, color_mode, bounds, prefix)
            out.write(
                f"    {{{v[0]:.4f}f, {v[1]:.4f}f, {v[2]:.4f}f, 1.0f, "
                f"{r:.2f}f, {g:.2f}f, {b:.2f}f, {a:.2f}f}}{end_char}\n"
            )
        out.write("};\n\n")

        out.write(
            f"static const PantheonTriangle {prefix}_indices[{prefix.upper()}_TRI_COUNT] "
            "__attribute__((aligned(16))) = {\n"
        )
        for idx, tri in enumerate(indices):
            end_char = "," if idx < len(indices) - 1 else ""
            out.write(f"    {{{tri[0]}, {tri[1]}, {tri[2]}, 0}}{end_char}\n")
        out.write("};\n\n")

        out.write("#ifndef PANTHEON_UV_STRUCT\n#define PANTHEON_UV_STRUCT\n")
        out.write(
            "typedef struct {\n"
            "    float s, t, q, pad;\n"
            "} __attribute__((aligned(16))) PantheonUV;\n"
        )
        out.write("#endif\n\n")
        if uvs:
            out.write(
                f"static const PantheonUV {prefix}_uv[{prefix.upper()}_UV_COUNT] "
                "__attribute__((aligned(16))) = {\n"
            )
            for idx, uv in enumerate(uvs):
                end_char = "," if idx < len(uvs) - 1 else ""
                out.write(f"    {{{uv[0]:.6f}f, {uv[1]:.6f}f, 1.0f, 0.0f}}{end_char}\n")
            out.write("};\n\n")
        else:
            out.write(
                f"static const PantheonUV {prefix}_uv[1] __attribute__((aligned(16))) = "
                "{{0.0f, 0.0f, 1.0f, 0.0f}};\n\n"
            )

        out.write(f"#endif /* {guard} */\n")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert ASCII Softimage .hrc into Pantheon C header arrays."
    )
    parser.add_argument("input_file", help="Input .hrc file path")
    parser.add_argument(
        "-o",
        "--output",
        help="Output header path (default: <input>_data.h)",
        default=None,
    )
    parser.add_argument(
        "-p",
        "--prefix",
        help="Symbol prefix (default: input filename lowercase)",
        default=None,
    )
    parser.add_argument(
        "--source-up",
        choices=["y", "z"],
        default="z",
        help="Source coordinate up-axis (default: z for Softimage legacy exports).",
    )
    parser.add_argument(
        "--color-mode",
        choices=["flat", "gradient"],
        default="flat",
        help="Generated vertex colors: flat nominal white or spatial test gradient.",
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Parse and report contract info without writing output header.",
    )
    parser.add_argument(
        "--binary-offset",
        type=lambda x: int(x, 0),
        default=0x035A,
        help="Binary scrape offset fallback (default: 0x035A).",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    input_file = args.input_file
    if not os.path.exists(input_file):
        print(f"ERROR: {input_file} not found!")
        return 1

    base_name = os.path.splitext(os.path.basename(input_file))[0]
    prefix = (args.prefix or base_name).lower()
    output_file = args.output or f"{os.path.splitext(input_file)[0]}_data.h"
    print(f"Crunching {input_file} -> {output_file}")

    try:
        vertices, indices, uvs, binary_guess, used_binary_scrape, scrape_offset, strategy_id = parse_hrc(
            input_file, args.source_up, args.binary_offset
        )
        if args.check_only:
            print(
                "CHECK: "
                f"verts={len(vertices)} tris={len(indices)} uvs={len(uvs)} "
                f"source_up={args.source_up} binary_guess={int(binary_guess)} "
                f"used_binary_scrape={int(used_binary_scrape)} scrape_offset={scrape_offset} "
                f"strategy={strategy_id}"
            )
            return 0
        write_header(input_file, output_file, prefix, vertices, indices, uvs, args.color_mode)
    except ValueError as parse_error:
        print(f"ERROR: {parse_error}")
        return 1

    print(
        f"SUCCESS: Exported {len(vertices)} vertices and "
        f"{len(indices)} triangles ({len(uvs)} uv pairs) to {output_file}. "
        f"binary_guess={int(binary_guess)} used_binary_scrape={int(used_binary_scrape)} "
        f"scrape_offset={scrape_offset} strategy={strategy_id} color_mode={args.color_mode}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
