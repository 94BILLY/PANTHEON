#!/usr/bin/env python3
"""Emit NotebookLM-friendly text snapshots of Pantheon sources (UTF-8)."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "_notebooklm"

FILES = [
    "README.md",
    "BASELINE_ACCEPTANCE.md",
    "Makefile",
    "Makefile.world",
    "pantheon_path1_contract.h",
    "pantheon_timecycle.h",
    "pantheon_vram.h",
    "pantheon_vram.c",
    "pantheon_texture_host.h",
    "pantheon_texture_host.c",
    "floor_data.h",
    "skydome_data.h",
    "floor.c",
    "shader.vsm",
    "hrc2ps2.py",
    "obj2ps2.py",
    "fix_shader.py",
    "cube/Makefile",
    "cube/cube.c",
    "cube/oldfloor.c",
    "cube/mesh_data.c",
    "skydome.hrc",
]


def main() -> None:
    OUT_DIR.mkdir(exist_ok=True)
    parts = []
    missing = []
    for rel in FILES:
        p = ROOT / rel
        if not p.is_file():
            missing.append(rel)
            continue
        text = p.read_text(encoding="utf-8", errors="replace")
        banner = f"\n{'=' * 80}\n### FILE: {rel}\n{'=' * 80}\n\n"
        parts.append(banner + text + ("" if text.endswith("\n") else "\n"))

    header = (
        "Pantheon Engine — code snapshot for NotebookLM\n"
        "UTF-8 text; paths relative to repo root.\n\n"
    )
    out = OUT_DIR / "pantheon_code_snapshot_full.txt"
    out.write_text(header + "".join(parts), encoding="utf-8")
    print(f"Wrote {out.relative_to(ROOT)} ({out.stat().st_size // 1024} KB)")
    if missing:
        print("Skipped (missing):", ", ".join(missing))


if __name__ == "__main__":
    main()
