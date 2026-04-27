"""Pantheon cruncher: ASCII Softimage .hrc -> C header arrays."""

import argparse
import os
import re
import sys


POSITION_RE = re.compile(
    r"position\s+([-+]?\d*\.?\d+)\s+([-+]?\d*\.?\d+)\s+([-+]?\d*\.?\d+)"
)
NODES_RE = re.compile(r"nodes\s+(\d+)\s+{(.*?)}", re.DOTALL)
VERTEX_INDEX_RE = re.compile(r"vertex\s+(\d+)")


def extract_mesh_data(content):
    raw_verts = POSITION_RE.findall(content)
    vertices = [[float(v[0]), float(v[1]), float(v[2])] for v in raw_verts]
    indices = []
    for _, block in NODES_RE.findall(content):
        v_indices = [int(i) for i in VERTEX_INDEX_RE.findall(block)]
        if len(v_indices) < 3:
            continue
        for i in range(1, len(v_indices) - 1):
            indices.append((v_indices[0], v_indices[i], v_indices[i + 1]))
    return vertices, indices


def parse_hrc(path):
    with open(path, "rb") as hrc_file:
        raw_data = hrc_file.read()

    # First pass: normal text decode for ASCII-exported HRCH files.
    text_content = raw_data.decode("latin-1", errors="ignore")
    vertices, indices = extract_mesh_data(text_content)
    if vertices and indices:
        return vertices, indices

    # Fallback for binary Softimage exports: scrape from known payload offset.
    # This mirrors the documented offset path used when hrcConvert -a is unavailable.
    binary_offset = 0x035A
    if len(raw_data) > binary_offset:
        scraped_text = raw_data[binary_offset:].decode("latin-1", errors="ignore")
        vertices, indices = extract_mesh_data(scraped_text)
        if vertices and indices:
            return vertices, indices

    if not vertices:
        raise ValueError(
            "No vertex positions found. Use ASCII export or run hrcConvert -a, "
            "or verify the binary payload format."
        )
    raise ValueError("No polygon nodes found to triangulate.")

    return vertices, indices


def write_header(input_file, output_file, prefix, vertices, indices):
    guard = f"{prefix.upper()}_DATA_H"
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

        out.write(
            f"static const PantheonVertex {prefix}_vertices[{prefix.upper()}_VERT_COUNT] "
            "__attribute__((aligned(16))) = {\n"
        )
        for idx, v in enumerate(vertices):
            end_char = "," if idx < len(vertices) - 1 else ""
            out.write(
                f"    {{{v[0]:.4f}f, {v[1]:.4f}f, {v[2]:.4f}f, 1.0f, "
                f"128.0f, 128.0f, 128.0f, 128.0f}}{end_char}\n"
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
        vertices, indices = parse_hrc(input_file)
        write_header(input_file, output_file, prefix, vertices, indices)
    except ValueError as parse_error:
        print(f"ERROR: {parse_error}")
        return 1

    print(
        f"SUCCESS: Exported {len(vertices)} vertices and "
        f"{len(indices)} triangles to {output_file}."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
