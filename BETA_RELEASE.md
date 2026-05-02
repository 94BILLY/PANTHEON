# Pantheon — beta release (floor + boot + sky)

This document pins the **default** configuration that matches:

`git pull && make -f Makefile.world clean && make -f Makefile.world`

## Git tag

- **`v1.0.0-beta.1`** — SemVer pre-release: hybrid render (CPU walkable deck + Path1 skydome), boot sequence (**94BILLY STUDIOS**), timecycle sky, anti–Z-fight floor split. Points to commit `134e663`.

Recreate this tree locally:

```bash
git fetch origin tag v1.0.0-beta.1
git checkout v1.0.0-beta.1
git rev-parse HEAD
```

## What the default build is

| Area | Behavior |
|------|----------|
| **Render** | **Hybrid** (`PANTHEON_RENDER_PROFILE=0`): CPU GIF path draws the exported Softimage floor + **Path1 draws skydome only** (`PANTHEON_PATH1_FLOOR_TILES=0`). Avoids coplanar CPU vs VU1 floor Z-fight. |
| **Floor** | World-anchored tiles (`PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER=0`), SoftImage XY→XZ swizzle on (`PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE`). |
| **Boot** | **On by default** (`PANTHEON_BOOT_REVEAL_ENABLE=1`): luma ramp + bitmap title **94BILLY STUDIOS**; Path1 world starts **after** `PANTHEON_BOOT_REVEAL_TOTAL_FRAMES` (~344 frames at default step/hold). Glyph wave default **off** (`PANTHEON_BOOT_TEXT_WAVE_AMP=0`). **Disable boot:** `EE_CFLAGS='-DPANTHEON_BOOT_REVEAL_ENABLE=0'`. |
| **Sky** | `g_day01` advances over `PANTHEON_DAY_CYCLE_SECONDS` (default 24 min); `PANTHEON_ATMO_SMOOTH_ALPHA` lerps atmosphere to avoid stair-stepping. |
| **Start palette** | **`PANTHEON_WEATHER_OVERCAST`** at **`g_day01=0.2`** → baby-blue biased overcast daylight. |
| **Dusk / purple** | **CLEAR** weather, slots 5→6 (`pantheon_timecycle.h`): strengthened magenta/purple dusk toward night. |

## Boot font

- **Current:** Tiny **bitmap** font in C (`pantheon_boot_glyph_rows`) — zero extra VRAM, no texture uploads.
- **Serif / Times look:** Pre-render to a paletted texture offline and draw with `draw_rect_textured`, or add more glyph definitions.
- **Disable motion** (static title / less sparkle): `EE_CFLAGS='-DPANTHEON_BOOT_TEXT_ANIMATE=0'` (wave amp is already 0 by default).

## Strict Path1-only (`EE_CFLAGS` build)

```bash
make -f Makefile.world clean && make -f Makefile.world EE_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'
```

This sets **`PATH1_AB_CPU_OVERLAY=0`** and **`PANTHEON_PATH1_FLOOR_TILES=1`**: no CPU GIF floor/overlay — VU1 Path1 skydome **and** floor tiles. Use for validating the Path1 floor pipeline.

## Flicker note

Keep **`PANTHEON_FLOOR_PATH1_DOUBLE_SIDED=0`** (default): double-sided coplanar floor tris can Z-fight on some PCSX2 backends. In hybrid profile, Path1 floor tiles are off so the deck is a single CPU GIF layer.
