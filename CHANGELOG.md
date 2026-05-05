# Changelog

All notable changes to this project are documented here. Versioning follows [Semantic Versioning](https://semver.org/).

Pre-release tags use **`vMAJOR.MINOR.PATCH-beta`** (e.g. **`v1.0.0-beta`**, then **`v1.0.1-beta`** as the beta line advances). First stable release will be **`v1.0.0`** (no prerelease suffix).

## Unreleased

### Added

- **IOP audio path (beta):** `floor.c` loads **`LIBSD`** / **`AUDSRV`**, links **`-laudsrv`**, parses embedded **theme** WAV (`theme_wav_data.h` from `theme_intro.wav`), with **`theme.ads`** / **`theme_ads_data.h`** as ADPCM-style companion data. Reference WAV duplicated at repo root and **`docs/media/theme_intro.wav`**.
- **`docs/pantheon-landing.html`** — WordPress Custom HTML aligned with live 94billy.com block: logo, responsive type, Path 1 pipeline section title, wallpaper strip; **Phase 1** bullets for DualShock, hybrid walkable floor, IOP **audsrv** theme audio (beta); **in progress** audio polish callout; install notes for **audsrv** / `-laudsrv`; strict build uses **`EE_EXTRA_CFLAGS`**.
- **`docs/PANTHEON_MARKETING.md`** — v2 X/Reddit copy: accurate hybrid vs strict, **`v1.0.0-beta`**, tracked media paths, policy reminder; GitHub Social preview called out as manual step.
- **`obj2ps2.py`** — Wavefront `.obj` → C for Path 1–style vertex emission (Blender / DCC route); documented in README, `GETTING_STARTED.md`, and `docs/pantheon-landing.html`.
- **`docs/screenshot_beta1.png`** — README hero still (retail PS2 capture; same frame as `still-world-hero.png`).
- **`docs/pantheon-94billy.html`** — dark, self-contained landing for **94billy.com/pantheon** (Criterion-style: hero, stats, CTAs, `shader.vsm` excerpt).

### Changed

- **`Makefile.world`:** `EE_LIBS` includes **`-laudsrv`** for `audsrv` linkage.
- **README:** Phase 1 baseline bullets expanded (tiled floor / hybrid deck, audio beta, reference WAV); roadmap **Phase 2** calls out audio + texturing; **Repository policy** states sole maintainer **94BILLY** and GitHub collaborator hygiene; core file table lists theme assets.
- **`GETTING_STARTED.md`:** **Common issues** entry for **`cannot find -laudsrv`**.
- **Docs tone:** README, `GETTING_STARTED.md` (opening), `BETA_RELEASE.md` (opening), and `docs/pantheon-landing.html` reframed as **public record / showcase** for readers who already understand Path 1—explicit boundaries (no license, no contributions); reproducibility without “starter kit” positioning.
- **README:** Centered hero (title, tagline, flat-square badges, captioned hero still); section rules; overview blockquote; `bash` quickstart; tables for baseline, media, docs, roadmap; maintainer metadata in `<details>`; aligned with **v1.0.0-Beta** / landing tagline. **Repository policy** as blockquote + centered footer table.
- **GETTING_STARTED.md:** **TL;DR** at top; **Common issues** moved to end (after **References**).
- **`docs/pantheon-landing.html`:** WordPress block from shipped **pantheon-page** template; media filled with `raw.githubusercontent.com` URLs; **`docs/still-boot-title-4586.png`** (boot title frame from capture). Release tag text aligned to **v1.0.0-beta**.
- **BASELINE_ACCEPTANCE.md:** Removed stale `floor_hybrid_baseline.elf` / `floor_path1_strict.elf` names; single artifact **`floor.elf`** + pointer to `BETA_RELEASE.md`.
- **Positioning:** Crunchers and asset sections are described as **reference-tree / reproducibility**, not a general game-making invitation (`README.md`, `GETTING_STARTED.md`, `docs/pantheon-landing.html`).
- **Public hygiene:** Removed optional dev-only atmosphere NDJSON hook from `floor.c` (no machine-local paths). `GETTING_STARTED.md` audience wording neutralized; `VIEW_PANTHEON_MEDIA.html` no longer links to gitignored capture masters. **`HANDOFF.md`:** optional `git filter-repo` / BFG note for maintainers who must scrub old history blobs.

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
- **Release defaults:** **`EE_EXTRA_CFLAGS`** appends `-DPANTHEON_*` tuners without clobbering `-D_EE`.
- **Tuners:** `PANTHEON_INITIAL_WEATHER` (default overcast); docs for day-cycle and CLEAR weather via `EE_EXTRA_CFLAGS`.
- **Docs:** `BETA_RELEASE.md` aligned with `floor.c` defaults; boot font note; `EE_EXTRA_CFLAGS` examples throughout.

### Removed

- **Repo hygiene:** NotebookLM snapshot (`_notebooklm/`), IDE-only rules (not shipped), PS2SDK-template `cube/` sample, legacy root `Makefile`, `fix_shader.py`, `obj2ps2.py`, and `scripts/export_notebooklm_snapshot.py`. Public tree is **Path 1 + docs + asset cruncher** only; use `Makefile.world` and `GETTING_STARTED.md`.

*Earlier work used tags `v1.0.0-beta.1` and `v1.0.0-beta.2`; those names are retired in favor of **`v1.0.0-beta`** as the single canonical pre-1.0 tag for this baseline.*

[1.0.0-beta]: https://github.com/94BILLY/PANTHEON/releases
