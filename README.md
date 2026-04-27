# Pantheon

Pantheon is a PlayStation 2 engine project focused on **Path 1 rendering**: the Emotion Engine (EE) orchestrates DMA/VIF1 work while VU1 executes transform and kick logic directly to the GS.

The long-term goal is to build a professional-grade PS2 engine platform for full game production, with the same level of architectural intent that modern teams expect from engines like RAGE or Unreal in their own domains, but purpose-built for PS2 hardware realities.

## Vision

Pantheon is being designed as:

- A **Path 1-first engine architecture** (EE manager, VU1 worker).
- A **hardware-authentic pipeline** that respects PS2 constraints instead of abstracting them away.
- A **content pipeline for real production work**, starting with Softimage-authored assets and moving toward scalable world workflows.

In practical terms, Pantheon aims to become a reusable PS2 engine foundation for shipping large, cohesive experiences, not just isolated rendering demos.

## Why Pantheon Exists

Many homebrew graphics examples demonstrate effects, but fewer establish a cohesive engine direction around VU1/GS-era production patterns. Pantheon exists to close that gap by building a durable Path 1 framework with clear rules:

- EE schedules and streams.
- VU1 performs transform/packing/kick responsibilities.
- Data contracts stay explicit and stable between C and VU microcode.

## Current State

Pantheon currently has a stable baseline that includes:

- 60 FPS locked runtime baseline in current test scene.
- Path 1 floor and skydome rendering active.
- VU1 microprogram (`shader.vsm`) integrated into the build.
- GTA-style camera/input controls active in the current test harness.
- Asset ingestion flow from Softimage `.hrc` through conversion tooling into engine headers.

This baseline is tracked in git with a dedicated tag:

- `pantheon-base-60fps`

## Core Architecture Principles

Pantheon development is guided by the following principles:

- **EE as manager, VU1 as worker**  
  EE builds DMA/VIF command streams and manages frame orchestration; VU1 performs vertex path work and GS kick.

- **Path 1 memory contract discipline**  
  Shared address and packet contracts between `floor.c` and `shader.vsm` are treated as core engine interfaces.

- **Hardware-safe data rules**  
  16-byte alignment, bounded batch sizing, and explicit packing semantics are first-class requirements.

- **Evidence-driven iteration**  
  Runtime logs and verification loops are used to validate behavior before locking in architectural changes.

## Rendering Pipeline (Current)

At a high level:

1. EE builds command/data packets in C (`floor.c`).
2. VIF1 uploads constants, matrices, and vertex payloads.
3. VU1 (`shader.vsm`) transforms, packs, and performs `xgkick`.
4. GS receives and rasterizes output.

This is intentionally optimized around Path 1 behavior, not a generic abstraction layer.

## Asset Pipeline (Current)

Current pipeline centers on Softimage `.hrc` source assets:

1. Export `.hrc` from Softimage.
2. Convert to ASCII when needed (`hrcConvert.exe ... -a` on NT workflow).
3. Run `hrc2ps2.py` to produce `*_data.h` mesh payloads.
4. Build and run in the EE/VU1 test harness.

Primary generated asset headers in this repo include:

- `floor_data.h`
- `skydome_data.h`

## Build

Build the current executable with:

```bash
cd /home/marvin/Pantheon
make -f Makefile.world
```

Output:

- `floor.elf`

## Repository Landmarks

- `floor.c` - Main EE-side orchestration, render path setup, input/camera, packet assembly.
- `shader.vsm` - VU1 microprogram implementing transform/packing/kick behavior.
- `Makefile.world` - PS2SDK-based build entry for the current world/test executable.
- `hrc2ps2.py` - Asset conversion tool for `.hrc` mesh data.
- `HANDOFF.md` - Deep technical handoff and current implementation notes.

## Roadmap

Pantheon is in active engine-foundation phase. Planned work includes:

- Path 1 renderer hardening and regression-safe contracts.
- Terrain/scene chunking for VU memory and streaming constraints.
- Extended content pipeline (UV/texture workflows, richer material handling).
- Higher-level engine systems around world assembly and gameplay integration.
- Tooling and documentation quality suitable for sustained production use.

## Status

Pantheon is early but real: the core rendering path is active, a baseline is versioned, and the project direction is explicit.

The project is not affiliated with Sony. It is an independent PS2 engine effort built with respect for platform constraints and first-party-era architectural patterns.
