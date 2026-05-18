# v1.0.1-Beta Release Notes

## What's New

### Phase 2 — Scale

This release introduces **world scaling infrastructure** and **production-ready documentation** for monolithic asset integration.

#### 31×31 Dynamic Treadmill Floor
- Player-anchored tile grid snaps origin to nearest tile boundary
- Y=0.0f ground invariant maintained at locked 60 FPS
- Seamless world expansion without visible tile transitions
- Full coordinate space ready for large-scale world content

#### Zero-Pop Boot Sequence
- Eliminated intro flickering artifact
- Path 1 world rendering strictly gated until 94BILLY STUDIOS reveal completes
- Z-fight elimination via deferred rendering

#### Optimized Mesh Slicing
- `hrc2ps2.py` automatically partitions assets into VU1-safe batches (≤72 verts)
- Respects 16KB VU1 data memory ceiling
- Ready for monolithic asset ingestion via `--slice` flag

#### Hardware-Ready Audio
- IOP/SPU2 infrastructure complete
- Gated via `#ifdef PANTHEON_HARDWARE_TARGET` for clean PCSX2 emulator builds
- Real PS2 hardware builds retain full audsrv support

#### Production Documentation
- `ENGINE_STATE.md` — canonical technical specification
- VRAM map, VU1 contract, build flags, toolchain paths
- Hardware/emulator build examples included

---

## Field Demonstration

**Boot → Skydome → Floor (Retail PS2 capture — 60 FPS locked)**

![Pantheon v1.0.0 boot to world](https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/media/loop-boot-to-world.gif)

---

**v1.0.1-Beta Floor Expansion (Dynamic treadmill in action)**

https://94billy.com/wp-content/uploads/2026/05/pantheon-v1.0.1-beta-floor-expansion.mp4

31×31 dynamic treadmill demonstrating seamless world expansion, zero-pop boot sequence, and locked 60 FPS stability across floor tile transitions.

---

## Stability & Verification

- **PCSX2 2.7.x:** Full emulator validation, clean builds without audio state corruption
- **PS2 Hardware:** Retail hardware capture demonstrates deterministic 60 FPS locked rendering
- **Path 1 Pipeline:** Strict EE → VIF1 → VU1 → XGKICK → GS contract verified at compile time

---

## Documentation

- **Getting Started:** [`GETTING_STARTED.md`](GETTING_STARTED.md)
- **Engine State:** [`ENGINE_STATE.md`](ENGINE_STATE.md)
- **Changelog:** [`CHANGELOG.md`](CHANGELOG.md)
- **Full Documentation:** [`DOCS_INDEX.md`](DOCS_INDEX.md)

---

## Build

```bash
git clone https://github.com/94BILLY/PANTHEON.git
cd PANTHEON
make -f Makefile.world
```

Output: `floor.elf` — ready for PCSX2 2.7.x or PS2 hardware deployment.

**Hardware build with audio:**
```bash
make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_HARDWARE_TARGET'
```

---

## Requirements

- `ps2sdk` (with `audsrv`)
- `ee-gcc` 15.x
- `dvp-as` (VU1 microcode assembler)
- Python 3.x
- PCSX2 2.7.x (for emulation)

Setup guide: [`GETTING_STARTED.md`](GETTING_STARTED.md)

---

## Roadmap

- **Phase 3 — Texturing** — STQ coordinates, texture sampling in VU1, GS VRAM upload
- **Phase 4 — Atmosphere** — Full dynamic timecycle, day/night transition
- **Phase 5 — Monolithic Assets** — Large model ingestion via automatic mesh slicing

---

**v1.0.1-Beta** — March 23rd, 2026

*"Imagination unleashed."*
