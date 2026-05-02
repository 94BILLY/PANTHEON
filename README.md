# Pantheon

**Versioning:** [Semantic Versioning 2.0.0](https://semver.org/). Current pre-release: **`v1.0.0-beta.2`** (see git tag; peel commit with `git rev-parse v1.0.0-beta.2^{}`). First RTM will be **`v1.0.0`** (no prerelease suffix). [Tags on GitHub](https://github.com/94BILLY/PANTHEON/tags).

**PANTHEON** is a high-performance **Path 1** stack for the **PlayStation 2**, built to hold a **locked 60 FPS** baseline. It enforces a strict **manager–worker** split: the **Emotion Engine** acts as a **DMA/VIF1 conductor**—it sequences **128-bit** command chains and feeds the bus—while **Vector Unit 1** runs dedicated **VLIW microcode** (`shader.vsm`) that transforms vertices, packs GIF primitives, and drives the **Graphics Synthesizer** with **XGKICK**. The EE is not a software rasterizer; it **orchestrates** work the hardware was meant to do.

At the silicon boundary the pipeline is **deterministic**: **quadword-aligned** structures, **burst-friendly** unpack layouts, a fixed **VRAM word map**, and guardrails (e.g. **near–Z** rejects) so what you enqueue is what the GS consumes. Authoring stays honest through an offline **Softimage `.hrc` → C headers** bridge (`hrc2ps2.py`)—geometry lands in the ROM image already padded and aligned for **VIF1** and **VU1**.

No middleware pretense, no engine-within-the-engine—**EE schedules, VU1 computes, GS draws.**

## What you get

- **Path 1 end-to-end** — EE builds chains; VU1 owns transform and kick; contract lives in `pantheon_path1_contract.h`.
- **VRAM discipline** — Linear allocator, page/block alignment, boot-time layout audit when telemetry is on.
- **Hybrid & strict profiles** — Default avoids coplanar floor Z-fight; strict mode validates full Path 1 floor + skydome (see [`BETA_RELEASE.md`](BETA_RELEASE.md)).
- **Softimage-native asset path** — `.hrc` through `hrc2ps2.py` into headers the DMA path consumes as-is.
- **Atmosphere & boot** — Timecycle-smoothed sky; luma ramp + **WWW.94BILLY.COM** title (bitmap glyphs, stagger + optional wave; no texture upload for the logo).

## Phase 1 — Golden build (locked)

Baseline **`pantheon-base-60fps`**:

- Locked **60 FPS** Path 1 rendering for floor grid, skydome, and boot overlay.
- **GTA-style** orbital camera with deadzone-aware analog input.
- **Zero-overlap** VRAM layout verified under telemetry.

## Roadmap

- **Phase 2** — VRAM texture foundation: host **IMAGE** uploads, **STQ**, `shader.vsm` sampling path.
- **Terrain / chunking** — EE-side batching to respect the **16 KB** VU1 data ceiling at scale.
- **Atmosphere** — Continued **San Andreas–style** lerp between discrete timecycle palettes.

**Target platform:** PS2 (PCSX2 for iteration; real hardware supported).

**Build:** `make -f Makefile.world` → `floor.elf`. Pinned defaults and strict Path 1 flags: [`BETA_RELEASE.md`](BETA_RELEASE.md).

**Docs:** [`GETTING_STARTED.md`](GETTING_STARTED.md) · [`FLIGHT_LOG.md`](FLIGHT_LOG.md) · [`HANDOFF.md`](HANDOFF.md) · [`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md) · [`CHANGELOG.md`](CHANGELOG.md)

**Repository policy:** Public as a **showcase / devlog-style** view of the work. There is **no** `LICENSE` file; rights are **not** granted to copy, redistribute, or reuse the code without written permission (see notice below). **Issues, pull requests, and contributions are not accepted.**

**Official Repository:** https://github.com/94BILLY/Pantheon  
**Author:** [94BILLY](https://github.com/94BILLY)

**Beta baseline (pinned defaults + strict Path1 note):** see [`BETA_RELEASE.md`](BETA_RELEASE.md).

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
