# Pantheon

**Pantheon** is a **Path 1** PlayStation 2 engine: the **Emotion Engine** issues **DMA/VIF1** work; **VU1** runs **`shader.vsm`** (transform, GIF pack, **XGKICK** to the **Graphics Synthesizer**). In-tree crunchers (**`obj2ps2.py`**, **`hrc2ps2.py`**) exist to align mesh data to the same bus format the **reference `floor.elf`** uses—**not** positioned as a kit for shipping unrelated titles.

**Build:** `make -f Makefile.world` → **`floor.elf`** (stripped EE ELF).

---

## At a glance

| | |
|---|---|
| **What you get** | Boot sequence (**WWW.94BILLY.COM**), timecycle skydome, walkable floor grid, third-person orbit camera, DualShock movement |
| **Default render** | **Hybrid** — CPU GIF draws the floor deck; **Path 1** draws the skydome (avoids coplanar Z-fight). **Strict Path 1** turns on VU1 floor tiles and turns off the CPU overlay — see [`BETA_RELEASE.md`](BETA_RELEASE.md) |
| **Target** | **60 FPS** reference whitebox; acceptance criteria in [`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md) |

---

## Who this is for

- Builders who already have **PS2SDK** / **ee-gcc** and want a **reproducible Path 1** reference.
- Anyone reading the repo as **engineering documentation** (memory contract in **`pantheon_path1_contract.h`**, VU program in **`shader.vsm`**).

Path 1 is not abstracted away. If EE / VIF / VU / GS manuals are new to you, start with Sony and **ps2sdk** samples, then return—**[`GETTING_STARTED.md`](GETTING_STARTED.md)** assumes you are compiling PS2 software.

---

## Repository policy

**Public record of the work** — not a product warranty, not permission to fork for redistribution. **No `LICENSE` file.** **All rights reserved.** No **issues**, no **pull requests**, no **contributions** without **written permission**. Viewing the source does not grant a license to copy or reuse.

---

## Documentation

| Document | Use |
|----------|-----|
| [**GETTING_STARTED.md**](GETTING_STARTED.md) | Toolchain, build, PCSX2, profiles, asset pipeline, troubleshooting |
| [**BETA_RELEASE.md**](BETA_RELEASE.md) | Pinned defaults for **`v1.0.0-beta`**; hybrid vs strict one-liners |
| [**HANDOFF.md**](HANDOFF.md) | Doc index + local SCE reference paths |
| [**BASELINE_ACCEPTANCE.md**](BASELINE_ACCEPTANCE.md) | Regression / acceptance gates |
| [**CHANGELOG.md**](CHANGELOG.md) | Release notes |
| [**FLIGHT_LOG.md**](FLIGHT_LOG.md) | Development journal |

**Versioning:** [SemVer 2.0](https://semver.org/). Pre-release tag **`v1.0.0-beta`**. [Tags →](https://github.com/94BILLY/PANTHEON/tags)

---

## Roadmap (directional)

- Texture / **STQ** / sampling in microcode  
- Terrain / chunking within **VU1** data limits  
- Atmosphere / timecycle  

---

**Repository:** [github.com/94BILLY/Pantheon](https://github.com/94BILLY/Pantheon) · **94BILLY** · [94billy.com](https://www.94billy.com)

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
