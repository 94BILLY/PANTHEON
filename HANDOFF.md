# Handoff — Path 1 reference

## Source of truth: SCE tree (Path 1)

For EE DMA/VIF1, VU1 (`shader.vsm`), XGKICK, GIF/GS semantics, alignment, and hardware-facing details, treat the local Sony SCE kit as the primary **documentation and sample** authority next to [ps2sdk](https://github.com/ps2dev/ps2sdk):

| Host | Path |
|------|------|
| Windows | `C:\PS2Tools\sce\` |
| WSL (same tree) | `/mnt/c/PS2Tools/sce/` |

**Builds** use `PS2SDK` from the environment. The SCE tree does not replace ps2dev toolchains; it clarifies manuals, headers, and official EE patterns.

## Repo entry points

| Document | Purpose |
|----------|---------|
| [GETTING_STARTED.md](GETTING_STARTED.md) | Toolchain, build, PCSX2, profiles, asset pipeline |
| [BETA_RELEASE.md](BETA_RELEASE.md) | Pinned default configuration for the current beta tag |
| [BASELINE_ACCEPTANCE.md](BASELINE_ACCEPTANCE.md) | Acceptance and regression gates |
| [FLIGHT_LOG.md](FLIGHT_LOG.md) | Development journal |

## Optional: scrubbing old git history

If a string that must not appear **anywhere in past commits** (not only on current `main`) was ever pushed, normal commits do not remove it from history blobs. Maintainers can use **`git filter-repo`** (recommended) or **BFG Repo-Cleaner** to rewrite the default branch, then **force-push** and have all clones re-fetch or re-clone. This is disruptive; only do it if compliance or release policy requires a clean full history. For most public repos, a clean **tip of `main`** is enough for what visitors see.
