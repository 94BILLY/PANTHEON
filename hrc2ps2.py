"""Pantheon cruncher: ASCII Softimage .hrc -> C header arrays."""

import argparse
import os
import re
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


def parse_hrc(path, source_up):
    with open(path, "rb") as hrc_file:
        raw_data = hrc_file.read()
    binary_guess = is_probably_binary(raw_data)

    # First pass: normal text decode for ASCII-exported HRCH files.
    text_content = raw_data.decode("latin-1", errors="ignore")
    vertices, indices, uvs = extract_mesh_data(text_content, source_up)
    if vertices and indices:
        return vertices, indices, uvs, binary_guess, False

    # Fallback for binary Softimage exports: scrape from known payload offset.
    # This mirrors the documented offset path used when hrcConvert -a is unavailable.
    binary_offset = 0x035A
    if len(raw_data) > binary_offset:
        scraped_text = raw_data[binary_offset:].decode("latin-1", errors="ignore")
        vertices, indices, uvs = extract_mesh_data(scraped_text, source_up)
        if vertices and indices:
            return vertices, indices, uvs, binary_guess, True

    if not vertices:
        raise ValueError(
            "No vertex positions found. Use ASCII export or run hrcConvert -a, "
            "or verify the binary payload format."
        )
    raise ValueError("No polygon nodes found to triangulate.")

    return vertices, indices, uvs, binary_guess, False


def compute_color(vertices, idx, color_mode, bounds):
    if color_mode == "flat":
        return 128.0, 128.0, 128.0, 128.0
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
            r, g, b, a = compute_color(vertices, idx, color_mode, bounds)
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

        out.write(
            "typedef struct {\n"
            "    float s, t, q, pad;\n"
            "} __attribute__((aligned(16))) PantheonUV;\n\n"
        )
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
        vertices, indices, uvs, binary_guess, used_binary_scrape = parse_hrc(input_file, args.source_up)
        if args.check_only:
            print(
                "CHECK: "
                f"verts={len(vertices)} tris={len(indices)} uvs={len(uvs)} "
                f"source_up={args.source_up} binary_guess={int(binary_guess)} "
                f"used_binary_scrape={int(used_binary_scrape)}"
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
        f"color_mode={args.color_mode}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
