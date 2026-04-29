# Pantheon Baseline Acceptance

Baseline milestone: `pantheon-baseline-60fps-path1-controls`

## Runtime Matrix (PASS)

- Geometry visible: PASS
- Skydome visible: PASS
- Floor grid visible: PASS
- GTA camera controls working: PASS
- Player movement controls working: PASS
- Stable 60 FPS: PASS

## Build Profiles Verified

- Hybrid baseline build: `floor_hybrid_baseline.elf`
- Strict Path 1 build: `floor_path1_strict.elf`

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

