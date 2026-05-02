# Pantheon

**PANTHEON** is a **Path 1** PlayStation 2 stack: the **EE** runs as **DMA/VIF1 conductor**, **VU1** runs **`shader.vsm`** (transform, GIF pack, **XGKICK** to **GS**). Not Path 3. Not “engine” abstraction—**silicon-shaped** work: quadword alignment, unpack layout, VRAM map, **near–Z** rejects, contract in `pantheon_path1_contract.h`.

Offline mesh paths: **Softimage `.hrc` → `hrc2ps2.py`**, **Wavefront `.obj` → `obj2ps2.py`**—data lands in the shape the bus expects.

If that paragraph reads like noise, **you’re not the audience yet**. The PS2 doesn’t negotiate. Go read **EE Users Manual / VU / GS**, SCE samples, and **late-era shipping titles**—then come back. The gap between “homebrew demo” and **this** will make sense.

**This repository is a public record of the work**—not a product, not a starter kit, not an invitation to fork for your ROM. **No `LICENSE`.** No rights to copy, redistribute, or reuse without **written permission**. **Issues, pull requests, and contributions are not accepted.**

---

## What’s in the tree

- **Path 1 end-to-end** — EE chains, VU1 kick, contract headers.
- **VRAM discipline** — linear allocator, alignment, optional boot layout audit.
- **Hybrid & strict profiles** — default avoids CPU vs VU1 floor Z-fight; strict validates full Path1 floor + skydome ([`BETA_RELEASE.md`](BETA_RELEASE.md)).
- **Skydome + timecycle** — smoothed palettes; defaults documented (long day cycle, overcast bias).
- **Boot** — luma ramp + **WWW.94BILLY.COM** bitmap title; Path1 world gated until reveal completes.

## Baseline

Locked **60 FPS** target for this whitebox: floor grid, skydome, boot overlay, orbit camera. **Pantheon the game** is intended to **grow from this same codebase**; what you see today is the **foundation**, not the shipping title.

## Roadmap (directional)

- Phase 2 — texture / **STQ** / sampling in microcode.
- Terrain / chunking — respect **16 KB** VU1 data ceiling at scale.
- Atmosphere — continued timecycle work.

---

**Versioning:** [SemVer 2.0](https://semver.org/). Pre-release tag **`v1.0.0-beta`**; next **`v1.0.1-beta`**, …; first RTM **`v1.0.0`**. [Tags](https://github.com/94BILLY/PANTHEON/tags).

**Reproducibility (for people who compile):** `make -f Makefile.world` → `floor.elf`. Pinned defaults and flags: [`BETA_RELEASE.md`](BETA_RELEASE.md). Deeper toolchain notes: [`GETTING_STARTED.md`](GETTING_STARTED.md) (technical appendix, not a hand-hold).

**Docs:** [`GETTING_STARTED.md`](GETTING_STARTED.md) · [`BETA_RELEASE.md`](BETA_RELEASE.md) · [`FLIGHT_LOG.md`](FLIGHT_LOG.md) · [`HANDOFF.md`](HANDOFF.md) · [`BASELINE_ACCEPTANCE.md`](BASELINE_ACCEPTANCE.md) · [`CHANGELOG.md`](CHANGELOG.md)

**Repository:** [github.com/94BILLY/Pantheon](https://github.com/94BILLY/Pantheon) · **94BILLY**

© 2026 94BILLY. All rights reserved. Viewing permitted. No reuse without written permission.
