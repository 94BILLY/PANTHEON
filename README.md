# Pantheon

Pantheon is a Path 1 PlayStation 2 engine. The Emotion Engine submits DMA/VIF1 work; VU1 runs `shader.vsm` for transforms, GIF packing, and XGKICK to the Graphics Synthesizer. The repository includes `obj2ps2.py` and `hrc2ps2.py` so mesh data can be emitted in the same layout the reference binary expects. The tree is maintained as an engineering record and reproducibility baseline, not as a general-purpose template for unrelated products.

Build the reference ELF with:

```text
make -f Makefile.world
```

Output: `floor.elf` (stripped).

## Media (real hardware)

Captures from a **retail PS2** (not emulation). Local gallery: [`docs/media/VIEW_PANTHEON_MEDIA.html`](docs/media/VIEW_PANTHEON_MEDIA.html). Asset names describe **behavior**: `still-*` (frame), `loop-*` (repeat), `clip-*` (short beat).

**Social:** [`loop-boot-to-world.gif`](docs/media/loop-boot-to-world.gif) (boot → `WWW.94BILLY.COM` → world), [`loop-world-vertical.gif`](docs/media/loop-world-vertical.gif), [`clip-power-on.gif`](docs/media/clip-power-on.gif), [`loop-world-wide.gif`](docs/media/loop-world-wide.gif). **Stills:** [`still-ps2-particles.png`](docs/media/still-ps2-particles.png), [`still-rgb-proof.png`](docs/media/still-rgb-proof.png) (`IMG_4587.MOV` **1:11**).

**Landing / OG:** [`still-world-hero.png`](docs/media/still-world-hero.png); primary loop: [`loop-world-orbit.gif`](docs/media/loop-world-orbit.gif).

Phone masters (`IMG_4586.MOV`, `IMG_4587.MOV`) are **local** and gitignored.

![Pantheon Path 1 — real PS2 capture](docs/media/loop-world-orbit.gif)

## Scope

The reference `floor.elf` is a fixed-timeline whitebox: boot title, outdoor sky, ground plane, orbit camera, and controller input.

Included in this baseline:

- Boot sequence (WWW.94BILLY.COM)
- Timecycle skydome
- Walkable floor grid
- Third-person orbit camera
- DualShock movement and look

By default the build uses a hybrid path: CPU GIF draws the floor; Path 1 draws the skydome. That split avoids two floor layers fighting at the same depth. For a single-Path1 floor pass, use the strict profile described in BETA_RELEASE.md.

Pinned defaults and flags for the current beta tag are in BETA_RELEASE.md. Acceptance checks are in BASELINE_ACCEPTANCE.md. Target frame rate for this whitebox is 60 Hz.

## Audience

The project is intended for developers who already build PS2 software with PS2SDK and want a concrete Path 1 reference. Readers treating the repo as documentation should start with `pantheon_path1_contract.h` and `shader.vsm`. Familiarity with EE, VIF, VU, and GS documentation is assumed; GETTING_STARTED.md covers toolchain, build, PCSX2, and mesh regeneration for this tree only.

## Repository policy

This repository is a public record of the work. There is no LICENSE file. All rights are reserved. Issues, pull requests, and unsolicited contributions are not accepted. Redistribution or reuse requires written permission. Access to the source does not grant a license.

## Documentation

- GETTING_STARTED.md — Toolchain, build, PCSX2, build profiles, asset pipeline notes, troubleshooting
- BETA_RELEASE.md — Pinned defaults for the v1.0.0-beta tag (hybrid vs strict)
- HANDOFF.md — Documentation index and optional local SCE reference paths
- BASELINE_ACCEPTANCE.md — Acceptance criteria
- CHANGELOG.md — Version history
- FLIGHT_LOG.md — Development log

Versioning follows Semantic Versioning. The current pre-release tag is v1.0.0-beta. Tags: https://github.com/94BILLY/PANTHEON/tags

## Roadmap

- Texture coordinates, STQ, and sampling in the VU1 program
- Terrain and chunking within VU1 data limits
- Further atmosphere and timecycle work

---

Repository: https://github.com/94BILLY/Pantheon  
94BILLY — https://www.94billy.com

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
