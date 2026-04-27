# Pantheon — Cursor agent handoff

## What this project is

**Pantheon** is a **PlayStation 2** custom engine (EE + **Path 1 / VU1**): EE builds DMA/VIF1 packets; **`shader.vsm`** (assembled with `dvp-as`) transforms vertices and kicks GIF. **GIF** path uses libdraw for clear + optional CPU overlays. Goal long-term: **open-world** scale, **Softimage 3.9** (NT4) authored assets, **Bliss-style** sky/grass look; pipeline is being hardened first.

## Source of truth: official SCE tree (Path 1)

For **Path 1** (EE manager/conductor, DMA/VIF1 packets, VU1 microcode, XGKICK → GIF → GS, quadword alignment, register semantics), treat the **local SCE install tree** as authoritative—not generic web writeups.

- **Canonical paths:** Windows `C:\PS2Tools\sce\` and WSL `/mnt/c/PS2Tools/sce/` (same tree).
- **Use it for:** hardware manuals (GS, EE, VU), IOP/DMA/VIF reference material under that tree, and **EE samples** (e.g. `ee/sample/basic3d/vu1/` — `readme.txt`, sources) when validating Path 1 patterns against first-party examples.
- **Builds vs. docs:** [`Makefile.world`](Makefile.world) builds against **`PS2SDK`** includes and libs. The SCE tree is the **documentation and reference-sample** authority when something is ambiguous or when matching official hardware wording; it is not an extra `-I` path unless you add that explicitly later.

Examples under that tree: `Docs&Training/HardwareManuals/GS_Users_Manual.pdf`, `ee/sample/basic3d/vu1/readme.txt`.

## Repo layout (Linux/WSL)

| Path | Role |
|------|------|
| [`floor.c`](floor.c) | Main EE entry: pad, camera, GIF clear/floor overlay, VU1 skydome + floor chains |
| [`shader.vsm`](shader.vsm) | VU1 microcode (vertex × MVP, RGBAQ + XYZ2) |
| [`Makefile.world`](Makefile.world) | `make -f Makefile.world` → `floor.elf` |
| [`hrc2ps2.py`](hrc2ps2.py) | ASCII HRCH → `*_data.h` (`PantheonVertex` / `PantheonTriangle`, quad→tri fan) |
| [`floor_data.h`](floor_data.h) | Crunched floor mesh (generated; do not hand-edit) |
| [`skydome_data.h`](skydome_data.h) | Crunched skydome (58 verts, 112 tris as of last export) |
| [`skydome.hrc`](skydome.hrc) | ASCII source for cruncher (when present) |

**Note:** [`goldensphere_data.h`](goldensphere_data.h) may be an empty stub from a binary `.hrc` run; safe to ignore or delete after confirming unused.

## Asset pipeline (authoritative)

1. **Softimage 3.9 (NT4):** Save **binary** `.hrc` (e.g. under `SOFT3D...\databases\...` or `C:\temp\export.hrc`).
2. **Convert to ASCII:** `hrcConvert.exe <infile> <outfile> -a` — **two paths, then `-a`**. Use **`3D\bin\hrcConvert.exe`**, **not** `sgi\bin` (wrong DLLs on NT). Full path example:  
   `C:\Softimage\SOFT3D_3.9.2\3D\bin\hrcConvert.exe C:\temp\export.hrc C:\temp\export_ascii.hrc -a`
3. **WSL:** `python3 hrc2ps2.py export_ascii.hrc` (or rename to `skydome.hrc` first so output is `skydome_data.h`).
4. **Build:** `cd /path/to/Pantheon && make -f Makefile.world` (needs `PS2SDK`, `ee-gcc`, `dvp-as`).

User’s **Desktop batch** on NT converts `export.hrc` → ASCII after fixing `hrcConvert.exe` (was 0-byte / wrong tree once).

## Engine behavior (current)

- **VU1 memory layout (Path 1):** VIF unpacks vertices into VU mem starting at quadword **8**; the microprogram must **not** write GIF output into addresses that overlap that window (e.g. a batch of 108 verts uses 216 qwords from 8 upward). Place the GIF tag and `xgkick` buffer in high mem (e.g. **240+**) and stream vertex output from **241+** so `sq` does not clobber input while the loop runs. [`shader.vsm`](shader.vsm) and [`floor.c`](floor.c) `render_path1_tris()` must stay in sync on those addresses.
- **`USE_VU1_SKYDOME_MESH`** = `(SKYDOME_TRI_COUNT > 0)` from [`skydome_data.h`](skydome_data.h).
- **Skydome winding:** triangles expanded as **(a, c, b)** in `init_flat_skydome()` so **inner** surface faces the camera (viewer under the dome).
- **Bliss-style look (vertex colors + clear):** `init_flat_skydome()` does per-vertex **blue gradient + sin “cloud”** puff; clear color is soft blue; CPU floor overlay biased green; VU1 floor packed color grass-ish.
- **CPU procedural sky** only when **`SKYDOME_TRI_COUNT == 0`** (`#if !USE_VU1_SKYDOME_MESH` block).
- **`PANTHEON_DEBUG_LOG`:** default **0**; set **1** via `-DPANTHEON_DEBUG_LOG=1` on `ee-gcc` line in Makefile for pad JSON logs to `host:` / local file.

## PCSX2 / testing

- Run **`floor.elf`** from **`host:`** with PCSX2 host root set to Pantheon dir (user uses `\\wsl.localhost\Ubuntu\home\marvin\Pantheon`).
- Emulator log noise (`ParsePlayedTimeLine`, `Recompiler Reset`) is usually benign.

## Softimage — user in progress

User is on **laptop** continuing **Softimage 3.9**: cutting **lower hemisphere** off a sphere for a true **upper skydome**. Had **tagged bottom points (red)**; **Delete** did nothing — likely need **Delete** submenu for **tagged components**, **polygon tag + delete**, or **freeze/collapse** primitive before component delete. **Boolean** subtract box below y=0 is a fallback.

## Likely next tasks for the agent

1. After user supplies new **`skydome.hrc` / `skydome_data.h`**, verify **`SKYDOME_TRI_COUNT`**, rebuild, visual check in PCSX2.
2. **TIM + UV** path for clouds (extend `hrc2ps2.py` + VU1 shader when ready).
3. **Terrain chunking** for VU1 16KB / batch limits (flight log Day 5 theme).
4. Keep **`HANDOFF.md`** updated if architecture shifts.

## Build one-liner

```bash
cd /home/marvin/Pantheon && make -f Makefile.world
```

## Contact / context

Conversation + flight logs referenced **Notebook LM**, **ND/Kojima-style** frame contracts, and the SCE tree above. Plan file may exist under `.cursor/plans/` with skydome + product vision notes.
