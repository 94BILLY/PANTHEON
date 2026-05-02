# Changelog

All notable changes to this project are documented here. Versioning follows [Semantic Versioning](https://semver.org/).

## Unreleased

### Changed

- **Boot:** Match GitHub-style intro on the stable baseline — **WWW.94BILLY.COM**, longer white hold (180 frames), default glyph wave (~2.25f). Path1 still waits until **full** `PANTHEON_BOOT_REVEAL_TOTAL_FRAMES` (no early-world strobing); `vif_packets`, solid floor, and Z/clear path unchanged from the flicker-fixed tree.

### Removed

- **Repo hygiene:** NotebookLM snapshot (`_notebooklm/`), Cursor editor rules (`.cursor/rules/`), PS2SDK-template `cube/` sample, legacy root `Makefile`, `fix_shader.py`, `obj2ps2.py`, and `scripts/export_notebooklm_snapshot.py`. Public tree is **Path 1 + docs + asset cruncher** only; use `Makefile.world` and `GETTING_STARTED.md`.

## [1.0.0-beta.2] - 2026-05-02

### Added

- **`GETTING_STARTED.md`** — toolchain, PCSX2, build profiles, asset pipeline, troubleshooting.
- **`FLIGHT_LOG.md`** — development journal.
- **`HANDOFF.md`** — SCE reference paths and documentation index.

### Changed

- **README:** documentation index; pre-release tag **`v1.0.0-beta.2`**.
- **Git:** stop tracking `__pycache__/*.pyc` and `_backup_archive/*`; expand `.gitignore` for local junk.

## [1.0.0-beta.1] - 2026-05-02

### Added

- SemVer pre-release tag **`v1.0.0-beta.1`** on hybrid + boot baseline (commit `21e157a`).

### Changed

- **Boot:** Default-on luma ramp; **WWW.94BILLY.COM** title with stagger + glyph wave; Path1 enabled only after full boot duration (reduces clear/world strobing).
- **Hybrid profile:** CPU GIF exported floor + Path1 **skydome**; Path1 **floor tiles disabled** by default to prevent coplanar Z-fighting with the CPU deck.
- **Strict profile** (`PANTHEON_RENDER_PROFILE=1`): Path1 floor tiles on, CPU overlay off (VU1 validation).
- **Docs:** `BETA_RELEASE.md` aligned with `floor.c` defaults; README versioning line.

[1.0.0-beta.2]: https://github.com/94BILLY/PANTHEON/tags
[1.0.0-beta.1]: https://github.com/94BILLY/PANTHEON/tags
