# Pantheon

![version](https://img.shields.io/badge/version-v1.0.0--beta-blue)
![platform](https://img.shields.io/badge/platform-PlayStation%202-black)
![license](https://img.shields.io/badge/license-All%20Rights%20Reserved-red)
![fps](https://img.shields.io/badge/target-60%20FPS%20locked-brightgreen)

![Pantheon v1.0.0-beta — retail PS2, Path 1 whitebox](docs/screenshot_beta1.png)

Bare-metal **Path 1** PlayStation 2 engine: the **EE** issues **DMA/VIF1** work; **VU1** runs **`shader.vsm`** (transform, GIF pack, **XGKICK** to **GS**). The tree ships **`hrc2ps2.py`** and **`obj2ps2.py`** so mesh data matches the layout the reference **`floor.elf`** expects. This is an engineering record and reproducibility baseline—not a generic game template.

Build the reference ELF:

```text
git clone https://github.com/94BILLY/PANTHEON.git
cd PANTHEON
make -f Makefile.world
```

Output: **`floor.elf`** (stripped). Run in **PCSX2** or on hardware; full setup: **[`GETTING_STARTED.md`](GETTING_STARTED.md)**.

## Phase 1 baseline (v1.0.0-beta)

Whitebox: **WWW.94BILLY.COM** boot, timecycle skydome, walkable floor, third-person orbit camera, **60 Hz** target. Default build uses a **hybrid** profile (CPU GIF floor + Path 1 skydome) to avoid coplanar Z-fight; strict all-Path1 floor is documented in **[`BETA_RELEASE.md`](BETA_RELEASE.md)**.

**→ [`GETTING_STARTED.md`](GETTING_STARTED.md)** — build it.  
**→ [`FLIGHT_LOG.md`](FLIGHT_LOG.md)** — how it was built.

## Key files

| File | Role |
|------|------|
| [`pantheon_path1_contract.h`](pantheon_path1_contract.h) | EE ↔ VU1 memory layout and batch contract |
| [`pantheon_vram.h`](pantheon_vram.h) | GS VRAM allocator declarations |
| [`pantheon_vram.c`](pantheon_vram.c) | Linear bump allocator, alignment, layout telemetry |
| [`shader.vsm`](shader.vsm) | VU1 microprogram: transform, near–Z, GIF packets, XGKICK |
| [`floor.c`](floor.c) | EE conductor: DMA chains, boot, world |

## Media (real hardware)

Captures from a **retail PS2**. Gallery (local + GitHub links): [`docs/media/VIEW_PANTHEON_MEDIA.html`](docs/media/VIEW_PANTHEON_MEDIA.html).

**Loops:** [`loop-boot-to-world.gif`](docs/media/loop-boot-to-world.gif), [`loop-world-orbit.gif`](docs/media/loop-world-orbit.gif). **Stills:** [`still-rgb-proof.png`](docs/media/still-rgb-proof.png) (1:11 in `IMG_4587.MOV`), [`still-world-hero.png`](docs/media/still-world-hero.png) (same frame as screenshot above). Phone masters stay **local** (gitignored).

**Website:** visual landing for **94billy.com/pantheon** — [`docs/pantheon-94billy.html`](docs/pantheon-94billy.html) (dark, self-contained). WordPress / Classic block source: [`docs/pantheon-landing.html`](docs/pantheon-landing.html).

## Scope

The reference **`floor.elf`** is a fixed-timeline whitebox: boot title, outdoor sky, ground plane, orbit camera, DualShock input. Pinned defaults and flags: **[`BETA_RELEASE.md`](BETA_RELEASE.md)**. Acceptance: **[`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md)**.

## Audience

Developers who already build PS2 software with **PS2SDK** and want a concrete Path 1 reference. Start from **`pantheon_path1_contract.h`** and **`shader.vsm`**. EE / VIF / VU / GS familiarity is assumed.

## Repository policy

Public record of the work. **No** `LICENSE`; all rights reserved. Issues, pull requests, and unsolicited contributions are **not** accepted. Redistribution or reuse requires **written permission**.

## Documentation

- **[`GETTING_STARTED.md`](GETTING_STARTED.md)** — toolchain, build, PCSX2, profiles, assets, troubleshooting  
- **[`BETA_RELEASE.md`](BETA_RELEASE.md)** — hybrid vs strict defaults for **v1.0.0-beta**  
- **[`HANDOFF.md`](HANDOFF.md)** — doc index, optional SCE reference paths  
- **[`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md)** — acceptance criteria  
- **[`CHANGELOG.md`](CHANGELOG.md)** — version history  
- **[`FLIGHT_LOG.md`](FLIGHT_LOG.md)** — development log  

**Tags:** [github.com/94BILLY/PANTHEON/tags](https://github.com/94BILLY/PANTHEON/tags)

## Roadmap

### Phase 2 — Texture / STQ

- Texture coordinates, **STQ**, and sampling in the VU1 program.

### Phase 3 — Terrain / scale

- Terrain chunking within **16 KB** VU1 data limits.

### Phase 4 — Atmosphere

- Further timecycle and sky work.

---

**Repository:** [github.com/94BILLY/PANTHEON](https://github.com/94BILLY/PANTHEON) · **94BILLY** — [94billy.com](https://www.94billy.com)

### GitHub repository settings (maintainer)

Set **Description** (one line):

```text
Bare-metal Path 1 PlayStation 2 engine. VU1 microcode. Softimage 3D pipeline. 60 FPS locked.
```

**Topics:** `ps2`, `playstation2`, `homebrew`, `vu1`, `path1`, `gamedev`, `openworld`, `softimage`, `bare-metal`, `demoscene`

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
