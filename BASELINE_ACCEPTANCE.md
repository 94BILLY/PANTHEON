# Pantheon Baseline Acceptance

Baseline milestone: `pantheon-baseline-60fps-path1-controls`

## Runtime Matrix (PASS)

- Geometry visible: PASS
- Skydome visible: PASS
- Floor grid visible: PASS
- GTA camera controls working: PASS
- Player movement controls working: PASS
- Stable 60 FPS: PASS

## Build artifact

- Single output name: **`floor.elf`** (profile selected at compile time via `EE_EXTRA_CFLAGS`; see [`BETA_RELEASE.md`](BETA_RELEASE.md)).

## Regression Gate

Any render-path, microcode, or pipeline change must preserve the full PASS matrix above in at least one playable profile before merge.

## Strict Path 1 Gate

- Validation profile: `PANTHEON_RENDER_PROFILE=1`
- Required checks:
  - floor visible (Path 1 VIF1/VU1)
  - skydome visible (Path 1 VIF1/VU1)
  - camera and movement controls stable
  - 60 FPS sustained

## Minimal Telemetry Keys

- `verts` (batch vertices)
- `vif_qw` (submitted VIF qwords)
- `nearz` (near-Z threshold hit count)
- `tiles` (tile submissions in frame budget)

## Dynamic sky (strict Path 1 courier)

- Skydome vertex RGB comes from `apply_sky_color`: linear RGB blend `g_atmosphere.sky_horizon` → `g_atmosphere.sky_top` by latitude `t` (`pantheon_sample_timecycle` already linear between Morning…Night slot pairs).
- GIF clear RGB in `render_clear_and_setup` uses the same `g_atmosphere.sky_horizon` bytes so clears match the skydome rim (no mismatch flash).
- `shader.vsm`: VF09 is courier-only (EE-packed RGBAQ float); `ftoi0` applies only to VF10/VF11 for screen integer positions.

## GS VRAM phase (boot printf)

On startup, `pantheon_vram_log_layout` prints word-address ranges for FBP0, FBP1, ZBP, and optional TBP (when `PANTHEON_TEXTURE_PHASE=1`). Any `WARNING` line indicates overlap and must be treated as a hard bug before relying on textures.

Default build keeps **`PANTHEON_TEXTURE_PHASE=0`** so strict Path 1 behavior is unchanged; enable texture upload slice with:

`make -f Makefile.world PANTHEON_TEXTURE_PHASE=1`

