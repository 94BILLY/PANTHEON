# Pantheon — beta release (floor + intro + sky)

This document pins the **default** configuration that matches:

`git pull && make -f Makefile.world clean && make -f Makefile.world`

## Git tag

- **`v0.9.0-beta`** — early beta marker; newer tags on `main` may supersede (e.g. `v0.9.3-beta`).

Recreate locally after pull:

```bash
git checkout main
git pull
git rev-parse HEAD
```

## What the default build is

| Area | Behavior |
|------|----------|
| **Render** | **Hybrid** (`PANTHEON_RENDER_PROFILE=0`): Path1 draws **skydome then floor** (floor last so walkable plane wins Z inside the dome). **CPU GIF floor is skipped when Path1 floor runs** — avoids Z-fight. |
| **Floor** | World-anchored tiles (`PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER=0`), SoftImage XY→XZ swizzle on. Path1 floor matches **v0.9.x beta sandbox grid** by default (`PANTHEON_PATH1_FLOOR_BETA_GRID=1`): bright green **major lines** + **brighter checker cells** (cells were darkened in old tags and read as “no floor” in orbit + low-bitrate video). Player snap uses **tiled support** so the deck stays under you on every patch. Uniform slab: `EE_CFLAGS='-DPANTHEON_PATH1_FLOOR_BETA_GRID=0'`. |
| **Intro** | Boot luma ramp + `WWW.94BILLY.COM`; libdraw GS coords fixed. **Staggered letter reveal** + **gentle vertical sine wave** on by default (`PANTHEON_BOOT_TEXT_WAVE_AMP` ~2.25f, slightly slower phase). For zero wave on strict PCSX2: `EE_CFLAGS='-DPANTHEON_BOOT_TEXT_WAVE_AMP=0.0f'`. |
| **Sky** | `g_day01` advances over `PANTHEON_DAY_CYCLE_SECONDS` (default 24 min); `PANTHEON_ATMO_SMOOTH_ALPHA` lerps atmosphere to avoid stair-stepping. |
| **Start palette** | **`PANTHEON_WEATHER_OVERCAST`** at **`g_day01=0.2`** → baby-blue biased overcast daylight. |
| **Dusk / purple** | **CLEAR** weather, slots 5→6 (`pantheon_timecycle.h`): strengthened magenta/purple dusk toward night. |

## Boot font / “Times New Roman”

- **Current:** Tiny **bitmap** font in C (`pantheon_boot_glyph_rows`) — zero extra VRAM, no texture uploads.
- **Serif / Times look:** The EE has **no built-in TTF**. Practical options:
  1. **Pre-render** your string (or full alphabet) to a **paletted 4/8 bpp texture** offline, **upload once** at boot, draw with `draw_rect_textured` — closest to “real” Times on PS2.
  2. Keep bitmaps but **add more glyph definitions** (still no engine change outside boot).
- **Disable boot motion** (static title):  
  `EE_CFLAGS='-DPANTHEON_BOOT_TEXT_ANIMATE=0'`

## Strict Path1-only (`EE_CFLAGS` build)

```bash
make -f Makefile.world clean && make -f Makefile.world EE_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'
```

This sets **`PATH1_AB_CPU_OVERLAY=0`**: no CPU GIF floor/overlay — only VU1 Path1 meshes. Use for validating the Path1 pipeline; if Path1 floor fails, you may see sky-only until that path is fixed. In that mode the **CPU exported floor can run again** as the only floor path (see `floor.c` job gating).

## Flicker note

- Keep **`PANTHEON_FLOOR_PATH1_DOUBLE_SIDED=0`** (default): double-sided coplanar floor tris can Z-fight on some PCSX2 backends.
- **Do not** draw CPU GIF floor and Path1 floor on the **same** plane in the same frame (default hybrid now avoids this).
