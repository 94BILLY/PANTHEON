# Changelog

All notable changes to this project are documented here. Versioning follows [Semantic Versioning](https://semver.org/).

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

- **Boot:** Default-on luma ramp; title **94BILLY STUDIOS**; Path1 enabled only after full boot duration (reduces clear/world strobing).
- **Hybrid profile:** CPU GIF exported floor + Path1 **skydome**; Path1 **floor tiles disabled** by default to prevent coplanar Z-fighting with the CPU deck.
- **Strict profile** (`PANTHEON_RENDER_PROFILE=1`): Path1 floor tiles on, CPU overlay off (VU1 validation).
- **Docs:** `BETA_RELEASE.md` aligned with `floor.c` defaults; README versioning line.

[1.0.0-beta.2]: https://github.com/94BILLY/PANTHEON/tags
[1.0.0-beta.1]: https://github.com/94BILLY/PANTHEON/tags
