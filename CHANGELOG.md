# Changelog

All notable changes to this project are documented here. Versioning follows [Semantic Versioning](https://semver.org/).

## [1.0.0-beta.1] - 2026-05-02

### Added

- SemVer pre-release tag **`v1.0.0-beta.1`** on hybrid + boot baseline (commit `134e663`).

### Changed

- **Boot:** Default-on luma ramp; title **94BILLY STUDIOS**; Path1 enabled only after full boot duration (reduces clear/world strobing).
- **Hybrid profile:** CPU GIF exported floor + Path1 **skydome**; Path1 **floor tiles disabled** by default to prevent coplanar Z-fighting with the CPU deck.
- **Strict profile** (`PANTHEON_RENDER_PROFILE=1`): Path1 floor tiles on, CPU overlay off (VU1 validation).
- **Docs:** `BETA_RELEASE.md` aligned with `floor.c` defaults; README versioning line.

[1.0.0-beta.1]: https://github.com/94BILLY/PANTHEON/tags
