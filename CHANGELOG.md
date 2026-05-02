# Changelog

All notable changes to this project are documented here. Versioning follows [Semantic Versioning](https://semver.org/).

Pre-release tags use **`vMAJOR.MINOR.PATCH-beta`** (e.g. **`v1.0.0-beta`**, then **`v1.0.1-beta`** as the beta line advances). First stable release will be **`v1.0.0`** (no prerelease suffix).

## Unreleased

### Added

- **`obj2ps2.py`** — Wavefront `.obj` → C for Path 1–style vertex emission (Blender / DCC route); documented in README, `GETTING_STARTED.md`, and `docs/pantheon-landing.html`.

### Changed

- **Docs tone:** README, `GETTING_STARTED.md` (opening), `BETA_RELEASE.md` (opening), and `docs/pantheon-landing.html` reframed as **public record / showcase** for readers who already understand Path 1—explicit boundaries (no license, no contributions); reproducibility without “starter kit” positioning.
- **README:** Restructured for **Apple-style scanability** (tables, short sections) while keeping **Sony-style technical precision** (Path 1, artifact name, doc map); reduced gatekeeping in the lede.
- **GETTING_STARTED.md:** Opening **navigation paragraph** (“how to read this page”) before audience disclaimer.
- **BASELINE_ACCEPTANCE.md:** Removed stale `floor_hybrid_baseline.elf` / `floor_path1_strict.elf` names; single artifact **`floor.elf`** + pointer to `BETA_RELEASE.md`.

## [1.0.0-beta] - 2026-05-02

### Added

- **`GETTING_STARTED.md`** — toolchain, PCSX2, build profiles, asset pipeline, troubleshooting.
- **`FLIGHT_LOG.md`** — development journal.
- **`HANDOFF.md`** — SCE reference paths and documentation index.
- SemVer pre-release tag **`v1.0.0-beta`** (canonical beta tag for this baseline).

### Changed

- **Boot:** Default-on luma ramp; **WWW.94BILLY.COM** title (stagger + glyph wave; longer white hold, ~2.25f wave). Path1 enabled only after full `PANTHEON_BOOT_REVEAL_TOTAL_FRAMES` (reduces clear/world strobing); separate `vif_packets`, solid floor, Z/clear path preserved.
- **Hybrid profile:** CPU GIF exported floor + Path1 **skydome**; Path1 **floor tiles disabled** by default to prevent coplanar Z-fighting with the CPU deck.
- **Strict profile** (`PANTHEON_RENDER_PROFILE=1`): Path1 floor tiles on, CPU overlay off (VU1 validation).
- **README:** documentation index; versioning line; sky/session expectations (see `BETA_RELEASE.md`).
- **Release defaults:** `PANTHEON_AGENT_SKY_LOG` defaults to **0**. **`EE_EXTRA_CFLAGS`** appends tuners without clobbering `-D_EE`.
- **Tuners:** `PANTHEON_INITIAL_WEATHER` (default overcast); docs for day-cycle and CLEAR weather via `EE_EXTRA_CFLAGS`.
- **Docs:** `BETA_RELEASE.md` aligned with `floor.c` defaults; boot font note; `EE_EXTRA_CFLAGS` examples throughout.

### Removed

- **Repo hygiene:** NotebookLM snapshot (`_notebooklm/`), Cursor editor rules (`.cursor/rules/`), PS2SDK-template `cube/` sample, legacy root `Makefile`, `fix_shader.py`, `obj2ps2.py`, and `scripts/export_notebooklm_snapshot.py`. Public tree is **Path 1 + docs + asset cruncher** only; use `Makefile.world` and `GETTING_STARTED.md`.

*Earlier work used tags `v1.0.0-beta.1` and `v1.0.0-beta.2`; those names are retired in favor of **`v1.0.0-beta`** as the single canonical pre-1.0 tag for this baseline.*

[1.0.0-beta]: https://github.com/94BILLY/PANTHEON/releases
