# Pantheon — beta release (floor + intro + sky)

This document pins the **default** configuration that matches:

`git pull && make -f Makefile.world clean && make -f Makefile.world`

## Git tag

- **`v0.9.0-beta`** — potential final beta baseline (hybrid render + walkable floor + boot intro + timecycle sky).

Recreate locally after pull:

```bash
git checkout main
git pull
git rev-parse HEAD
```

## What the default build is

| Area | Behavior |
|------|----------|
| **Render** | **Hybrid** (`PANTHEON_RENDER_PROFILE=0`): CPU GIF path draws debug/export floor + Path1 draws skydome/floor. Most stable for visibility. |
| **Floor** | World-anchored tiles (`PANTHEON_TRIAGE_FLOOR_FOLLOW_PLAYER=0`), SoftImage XY→XZ swizzle on. |
| **Intro** | Boot luma ramp + title `WWW.94BILLY.COM`; glyphs use libdraw-correct GS coords (START/END offset vs framebuffer origin). |
| **Sky** | `g_day01` advances over `PANTHEON_DAY_CYCLE_SECONDS` (default 24 min); `PANTHEON_ATMO_SMOOTH_ALPHA` lerps atmosphere to avoid stair-stepping. |
| **Start palette** | **`PANTHEON_WEATHER_OVERCAST`** at **`g_day01=0.2`** → baby-blue biased overcast daylight. |
| **Dusk / purple** | **CLEAR** weather, slots 5→6 (`pantheon_timecycle.h`): strengthened magenta/purple dusk toward night. |

## Strict Path1-only (`EE_CFLAGS` build)

```bash
make -f Makefile.world clean && make -f Makefile.world EE_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'
```

This sets **`PATH1_AB_CPU_OVERLAY=0`**: no CPU GIF floor/overlay — only VU1 Path1 meshes. Use for validating the Path1 pipeline; if Path1 floor fails, you may see sky-only until that path is fixed.

## Flicker note

Keep **`PANTHEON_FLOOR_PATH1_DOUBLE_SIDED=0`** (default): double-sided coplanar floor tris can Z-fight on some PCSX2 backends.
