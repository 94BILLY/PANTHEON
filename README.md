# Pantheon

**Versioning:** [Semantic Versioning 2.0.0](https://semver.org/). Current pre-release: **`v1.0.0-beta.2`** (see git tag; peel commit with `git rev-parse v1.0.0-beta.2^{}`). First RTM will be **`v1.0.0`** (no prerelease suffix). [Tags on GitHub](https://github.com/94BILLY/PANTHEON/tags).

**PANTHEON** is a deterministic data-orchestration and spatial rendering matrix engineered for highly asymmetric, heterogeneous silicon environments.

Operating at the bare-metal layer, it enforces a rigorous manager–worker execution protocol: the Primary Logic Matrix sequences high-bandwidth 128-bit DMA/VIF command chains while an isolated VLIW vector co-processor executes cycle-accurate microcode. The result is mathematically deterministic performance with zero-overhead hardware authority.

## Core Capabilities

- **Asymmetric Data Plane Orchestration** — Strict conductor-node protocol that decouples stream orchestration from spatial compute.
- **Double-Buffered Asynchronous Pipeline** — Zero-latency handoff between command preparation and vector-unit execution.
- **Strategic VRAM Word-Map** — Explicit 4 MB linear allocator enforcing 8 KB page alignment and 256-byte block boundaries.
- **Production-Grade Asset Bridge** — Automated offline pipeline that ingests native Softimage `.hrc` hierarchies and flattens them into hardware-safe sequential arrays.
- **Hardware-Level Safeguards** — Active Near-Z culling, quadword alignment enforcement, and chromatic stability across dynamic atmospheric transitions.

## Phase 1 — Golden Build (Locked)

The engine has reached its Phase 1 baseline (`pantheon-base-60fps`):

- Stable, locked 60 FPS asynchronous rendering in a pure Path 1 environment.
- Pure Path 1 rendering of complex environment geometry and atmospheric skydomes.
- Precision-sampled GTA-style orbital camera telemetry with deadzone-aware analog input.
- Verified zero-overlap memory layout audited via boot-time telemetry.

## Key Architectural Invariants

- **Manager–Worker Separation**: The host processor acts solely as a stream orchestrator; all geometric transformation, perspective division, and raster output is delegated to the vector co-processor.
- **Deterministic Memory Discipline**: Linear word-bump allocator with strict 16-byte quadword alignment for all DMA-facing structures.
- **VLIW Execution Parallelism**: Bespoke microprogram (`shader.vsm`) utilizing dual-issue floating-point and integer math with direct XGKICK to the rasterizer.
- **Automated Telemetry Bridge**: Offline asset-compression pipeline (`hrc2ps2.py`) that enforces hardware-safe alignment and padding.

## Current Roadmap

- **Phase 2**: VRAM Texture Foundation — IMAGE-mode host uploads and STQ homogeneous coordinate parsing.
- **Terrain / Scene Chunking**: EE-side spatial partitioning to manage large world data within the 16 KB co-processor limit.
- **Atmospheric Pacing**: San Andreas-style temporal lerping between discrete timecycle palettes.

## Platform Implementation Context

While designed as a study in modern asymmetric computational topologies, Pantheon is currently implemented as a **Path 1 PlayStation 2 engine**. It rejects modern abstraction layers in favor of treating legacy silicon with the same discipline applied to contemporary high-performance systems.

**Build:** `make -f Makefile.world` → `floor.elf`. Pinned defaults and strict Path 1 flags: [`BETA_RELEASE.md`](BETA_RELEASE.md).

**Docs:** [`GETTING_STARTED.md`](GETTING_STARTED.md) · [`FLIGHT_LOG.md`](FLIGHT_LOG.md) · [`HANDOFF.md`](HANDOFF.md) · [`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md) · [`CHANGELOG.md`](CHANGELOG.md)

**Repository policy:** Public as a **showcase / devlog-style** view of the work. There is **no** `LICENSE` file; rights are **not** granted to copy, redistribute, or reuse the code without written permission (see notice below). **Issues, pull requests, and contributions are not accepted.**

**Official Repository:** https://github.com/94BILLY/Pantheon  
**Author:** [94BILLY](https://github.com/94BILLY)

**Beta baseline (pinned defaults + strict Path1 note):** see [`BETA_RELEASE.md`](BETA_RELEASE.md).

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
