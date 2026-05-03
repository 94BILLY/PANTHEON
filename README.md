<h1 align="center" style="margin-top:0;margin-bottom:0;">
  <img src="docs/pantheon-logo-zenith-no-text-hero.png" alt="Pantheon — oculus mark" width="280" /><br />
  <span style="display:inline-block;margin-top:2px;">PANTHEON</span>
</h1>

<p align="center">Unified Path 1 game engine for the Sony® PlayStation 2. VU1-native. Locked 60 FPS.</p>

<p align="center">
  <img src="https://img.shields.io/badge/License-All%20Rights%20Reserved-red.svg" alt="License: All Rights Reserved" />
  <a href="https://github.com/94BILLY/PANTHEON/releases/tag/v1.0.0-beta"><img src="https://img.shields.io/badge/Status-v1.0.0--Beta-blue.svg" alt="Status: v1.0.0-Beta" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Platform-PlayStation%202-000000.svg" alt="Platform: PlayStation 2" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Architecture-Path%201-2ea44f.svg" alt="Architecture: Path 1" /></a>
  <a href="https://github.com/ps2dev/ps2sdk"><img src="https://img.shields.io/badge/SDK-ps2sdk-orange.svg" alt="SDK: ps2sdk" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Compiler-dvp--as-purple.svg" alt="Compiler: dvp-as" /></a>
</p>

<p align="center">
  <img src="docs/media/loop-boot-to-world.gif" alt="Boot sequence through Path 1 world — retail PS2 capture" width="720" />
</p>

<p align="center"><sub>Boot → world · Loop from retail PS2 capture</sub></p>

---

## Overview

Pantheon enforces a strict **Path 1 pipeline**: the Emotion Engine sequences 128-bit DMA/VIF command chains; VU1 executes `shader.vsm` (a bespoke VLIW microprogram compiled with `dvp-as`) that transforms vertices, packs GIF tags, and fires XGKICK directly to the Graphics Synthesizer. Geometry is authored in Softimage 3D, crunched offline via `hrc2ps2.py` and `obj2ps2.py`, and delivered as C arrays.

**The EE never touches a vertex.**

### Phase 1 Baseline (v1.0.0-Beta)

- **Boot title** — 94BILLY luma ramp reveal sequence
- **Timecycle skydome** — San Andreas-style 8-slot weather + 4-channel RGB interpolation
- **Walkable floor** — Path 1 mesh rendering
- **Orbit camera** — Spherical telemetry, deadzone + smoothing
- **DualShock input** — Analog pad capture and edge detection

**Build profiles:**
- **Hybrid (default)** — CPU GIF floor + Path 1 skydome (avoids coplanar Z-fight)
- **Strict Path 1** — VU1 renders all geometry; CPU GIF overlay disabled

---

## Build

```bash
git clone https://github.com/94BILLY/PANTHEON.git
cd PANTHEON
make -f Makefile.world
```

Output: `floor.elf`

Run in **PCSX2 2.7.x** or deploy to real PS2 hardware via OPL/FreeMcBoot.

**Requirements:** `ps2sdk` · `ee-gcc` 15.2.0 · `dvp-as` · Python 3.x

Setup guide: [`GETTING_STARTED.md`](GETTING_STARTED.md)

---

## Core Files

| File | Purpose |
| :--- | :--- |
| `pantheon_path1_contract.h` | EE ↔ VU1 compile-time memory layout contract |
| `pantheon_vram.h` / `pantheon_vram.c` | GS VRAM allocator (linear word-bump over 4 MB) |
| `shader.vsm` | VU1 microprogram — transform, matrix math, GIF packing, XGKICK |
| `floor.c` | EE conductor — DMA sequencing, boot, world loop |
| `hrc2ps2.py` | Softimage 3D `.hrc` → C array mesh cruncher |

---

## Documentation

| Document | Content |
| :--- | :--- |
| [`GETTING_STARTED.md`](GETTING_STARTED.md) | Build setup, PCSX2 config, profile flags, asset pipeline |
| [`BETA_RELEASE.md`](BETA_RELEASE.md) | v1.0.0-Beta scope, acceptance criteria, hybrid vs strict modes |
| [`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md) | Phase 1 acceptance gates and test procedures |
| [`FLIGHT_LOG.md`](FLIGHT_LOG.md) | Development journal — debugging log, learnings, architecture decisions |
| [`DOCS_INDEX.md`](DOCS_INDEX.md) | Full documentation index |
| [`CHANGELOG.md`](CHANGELOG.md) | Release history |

---

## Media

**Gallery (Retail PS2 captures):**  
[`docs/media/VIEW_PANTHEON_MEDIA.html`](docs/media/VIEW_PANTHEON_MEDIA.html) — Stills and loops under `docs/media/`

**Logo generation:**  
Source: [`docs/pantheon-oculus-straight-on-zenith.png`](docs/pantheon-oculus-straight-on-zenith.png)  
Generator: [`tools/generate_pantheon_zenith_logos.py`](tools/generate_pantheon_zenith_logos.py)  
Details: [`docs/LOGO.md`](docs/LOGO.md)

---

## Roadmap

- **Phase 2 — Texturing** — STQ coordinates, texture sampling in VU1, GS VRAM upload
- **Phase 3 — Scale** — World chunking within VU1 data memory limits
- **Phase 4 — Atmosphere** — Full dynamic timecycle, day/night sky transition

---

## Repository Policy

Public technical record. **No `LICENSE` file; all rights reserved.**

- Issues, pull requests, and unsolicited contributions are **not accepted**
- Redistribution requires written permission from 94BILLY
- Source code and assets are provided for educational and reference purposes

---

## Audience

This codebase is targeted at developers comfortable with:
- **PS2SDK** and `ps2-gcc`
- **Emotion Engine** (EE Core, DMA, VIF)
- **Vector Unit 1** (VU1, microcode, VLIW instruction pairing)
- **Graphics Synthesizer** (GIF packets, register writes, GS memory layout)
- **Bare-metal C** (no OS abstractions, direct hardware access)

**Entry point:** Start with `pantheon_path1_contract.h` and `shader.vsm`.

---

<p align="center">
  <img src="docs/media/Pantheon%20Oculus%20Official%20Wallpaper.png" alt="Pantheon Oculus Official Wallpaper" width="100%" />
</p>

<p align="center">
  <a href="https://github.com/94BILLY/PANTHEON">github.com/94BILLY/PANTHEON</a><br />
  <strong>94BILLY</strong> · <a href="https://www.94billy.com/PANTHEON">94billy.com/PANTHEON</a>
</p>

<p align="center"><sub>© 2026 94BILLY · All rights reserved</sub></p>

---

<details>
<summary><strong>GitHub repository metadata</strong></summary>

**Description (one line):**
```
Bare-metal Path 1 PlayStation 2 engine. VU1 microcode. Softimage 3D pipeline. 60 FPS.
```

**Topics:**
```
ps2 playstation2 ps2sdk homebrew vu1 path1 gamedev openworld softimage bare-metal demoscene
```

**Language:**
```
C (95%), Assembly (5%)
```

</details>
