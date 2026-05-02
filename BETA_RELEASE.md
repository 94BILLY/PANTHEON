# Pantheon — pinned baseline (floor + boot + sky)

**Internal record:** default `#define` / profile behavior for the **`v1.0.0-beta`** tree. For builders who already have **PS2SDK** and want to match the author’s hybrid vs strict configuration—not a tutorial.

Reproduces with:

`git pull && make -f Makefile.world clean && make -f Makefile.world`

## Git tag

- **`v1.0.0-beta`** — Canonical SemVer pre-release for this baseline: hybrid render, **WWW.94BILLY.COM** boot (luma ramp + stagger + glyph wave), timecycle sky, anti–Z-fight floor split, docs pack, `EE_EXTRA_CFLAGS` tuners. Peel commit: `git rev-parse v1.0.0-beta^{}`. **Next beta bumps** use patch: **`v1.0.1-beta`**, **`v1.0.2-beta`**, … until RTM **`v1.0.0`**.

Recreate this tree locally:

```bash
git fetch origin tag v1.0.0-beta
git checkout v1.0.0-beta
git rev-parse HEAD
```

## What the default build is

| Area | Behavior |
|------|----------|
| **Render** | **Hybrid** (`PANTHEON_RENDER_PROFILE=0`): CPU GIF path draws the exported Softimage floor + **Path1 draws skydome only** (`PANTHEON_PATH1_FLOOR_TILES=0`). Avoids coplanar CPU vs VU1 floor Z-fight. |
| **Floor** | World-anchored tiles (`PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER=0`), SoftImage XY→XZ swizzle on (`PANTHEON_TRIAGE_FLOOR_YZ_SWIZZLE`). |
| **Boot** | **On by default** (`PANTHEON_BOOT_REVEAL_ENABLE=1`): luma ramp + **WWW.94BILLY.COM** bitmap title (staggered reveal + sine wave on glyphs); Path1 world starts **after** `PANTHEON_BOOT_REVEAL_TOTAL_FRAMES` (~434 frames at default step/hold with 180-frame white dwell). Set **`PANTHEON_BOOT_TEXT_WAVE_AMP=0`** for static glyphs if a backend sparkles. **Disable boot:** `make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_BOOT_REVEAL_ENABLE=0'`. |
| **Sky** | `g_day01` advances over `PANTHEON_DAY_CYCLE_SECONDS` (default **1440 s** = **24 min** for a full day); `PANTHEON_ATMO_SMOOTH_ALPHA` lerps atmosphere to avoid stair-stepping. **Short sessions** (a few minutes) often look “stuck on blue” — that’s normal: slow cycle + **overcast** daylight bias. Tune locally with `EE_EXTRA_CFLAGS` (see `Makefile.world`): e.g. `make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_DAY_CYCLE_SECONDS=120'` (2 min cycle) or `EE_EXTRA_CFLAGS='-DPANTHEON_INITIAL_WEATHER=PANTHEON_WEATHER_CLEAR'` for stronger dusk/night contrast. |
| **Start palette** | **`PANTHEON_WEATHER_OVERCAST`** at **`g_day01=0.2`** → baby-blue biased overcast daylight. |
| **Dusk / purple** | **CLEAR** weather, slots 5→6 (`pantheon_timecycle.h`): strengthened magenta/purple dusk toward night. |

## Boot font

- **Current:** Tiny **bitmap** font in C (`pantheon_boot_glyph_rows`) — zero extra VRAM, no texture uploads.
- **Serif / Times look:** Pre-render to a paletted texture offline and draw with `draw_rect_textured`, or add more glyph definitions.
- **Disable motion** (static title / less sparkle): `make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_BOOT_TEXT_ANIMATE=0'`, or set **`PANTHEON_BOOT_TEXT_WAVE_AMP=0`** (default wave is non-zero for the GitHub-style glyph wobble).

## Strict Path1-only (`EE_EXTRA_CFLAGS` build)

```bash
make -f Makefile.world clean && make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'
```

This sets **`PATH1_AB_CPU_OVERLAY=0`** and **`PANTHEON_PATH1_FLOOR_TILES=1`**: no CPU GIF floor/overlay — VU1 Path1 skydome **and** floor tiles. Use for validating the Path1 floor pipeline.

## Flicker note

Keep **`PANTHEON_FLOOR_PATH1_DOUBLE_SIDED=0`** (default): double-sided coplanar floor tris can Z-fight on some PCSX2 backends. In hybrid profile, Path1 floor tiles are off so the deck is a single CPU GIF layer.
