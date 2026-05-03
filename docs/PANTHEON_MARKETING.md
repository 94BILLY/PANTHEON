# Pantheon — social copy (v2)

Aligned with **`main` / `v1.0.0-beta`**: defensible claims, hybrid-vs-strict accuracy, and **tracked** media paths. Use this instead of ad-hoc drafts that reference non-existent files or old tag names.

**Policy reminder (paste when useful):** public **engineering record** — viewing on GitHub is welcome; **no `LICENSE`** (all rights reserved); **no issues/PRs/contributions** per repo policy.

---

## Pre-launch checklist (repo + GitHub UI)

1. **GitHub Social preview** (highest ROI, manual): Repo **Settings → General → Social preview** → upload a still such as [`still-world-hero.png`](media/still-world-hero.png) or the zenith logo [`pantheon-logo-zenith-no-text-hero.png`](pantheon-logo-zenith-no-text-hero.png). This controls unfurls on X / Reddit / Discord.
2. **Media you can attach or link** (all under `docs/` unless noted):
   - GIF: [`docs/media/loop-boot-to-world.gif`](media/loop-boot-to-world.gif) — boot → world (retail capture).
   - Still: [`docs/media/still-world-hero.png`](media/still-world-hero.png), [`docs/screenshot_beta1.png`](screenshot_beta1.png).
   - Logo: [`docs/pantheon-logo-zenith-no-text-hero.png`](pantheon-logo-zenith-no-text-hero.png) (README hero).
3. **Links:** site **https://94billy.com/pantheon/** · repo **https://github.com/94BILLY/PANTHEON** · baseline **[`BETA_RELEASE.md`](../BETA_RELEASE.md)** (hybrid vs strict).

---

## Accurate one-liners (pick one)

- **A:** Pantheon is a **Path 1 PlayStation 2** stack: EE builds **DMA/VIF1** chains; **VU1** runs **`shader.vsm`** → **XGKICK** to the **GS**. Default **`floor.elf`** is **hybrid** (CPU GIF floor + Path 1 skydome); **strict** all-Path-1 is a documented profile.
- **B:** Bare-metal **C + VU1** engine — **compile-time** contract in **`pantheon_path1_contract.h`**, **Softimage → `hrc2ps2.py`** mesh path, **60 FPS** target (PCSX2 + retail PS2 captures).

Do **not** claim global “first Path 1 homebrew engine” — not provable and invites nitpicks.

---

## X (Twitter) — sequence (stagger over days)

Attach **`loop-boot-to-world.gif`** on posts **1** and **4** where noted; use **logo** or **still** on 2–3.

### Post 1 — Launch (attach: `loop-boot-to-world.gif`)

Introducing **PANTHEON**

A **Path 1** PlayStation 2 engine: **EE** → **VIF1** → **VU1** (`shader.vsm`) → **XGKICK** → **GS**.

Bare-metal **C + VU1** assembly. Default build is **hybrid** (walkable floor on CPU GIF + Path 1 skydome); **strict** Path 1 floor is one `EE_EXTRA_CFLAGS` profile away — see repo **BETA_RELEASE**.

Shipped baseline: **`v1.0.0-beta`**. Captures on **retail PS2** + PCSX2.

https://94billy.com/pantheon/  
https://github.com/94BILLY/PANTHEON

`#PS2Dev` `#PlayStation2` `#GameDev` `#Homebrew` `#RetroGaming`

### Post 2 — Technical (attach: logo or `still-world-hero.png`)

How Pantheon is wired:

**EE → VIF1 → VU1 → XGKICK → GS**

The Emotion Engine issues **128-bit DMA** work. **VU1** runs a **dvp-as**-assembled microprogram. Geometry batches for the sky go that path. **Hybrid default:** the **deck** uses the **CPU GIF** path so it does not Z-fight the Path 1 sky — details in **BETA_RELEASE.md**.

No “middleware engine” — this is **hardware-shaped** code.

https://github.com/94BILLY/PANTHEON

`#PS2` `#VU1` `#BareMetal` `#GameEngine` `#PS2Dev`

### Post 3 — Asset pipeline (attach: logo or still)

Mesh: **Softimage 3D** (authoring on a **Windows NT** VM in the documented pipeline) → **`hrc2ps2.py`** → quadword-aligned **`PantheonVertex`** headers → **VIF1 UNPACK**. **OBJ** route: **`obj2ps2.py`**.

Same *class* of DCC + cruncher workflow late PS2 teams used; Pantheon is a **modern homebrew** baseline, not a studio product.

https://github.com/94BILLY/PANTHEON

`#Softimage` `#PS2Dev` `#GameDev` `#RetroTech` `#PlayStation2`

### Post 4 — Hardware (attach: `loop-boot-to-world.gif`)

**Pantheon** on **real PlayStation 2** hardware (captures in-repo), plus **PCSX2 2.7.x** for iteration.

**`floor.elf`**: **`v1.0.0-beta`** — boot, timecycle sky, walkable floor, orbit cam, **60 FPS** target.

https://94billy.com/pantheon/

`#PS2Homebrew` `#PlayStation2` `#RetroGaming` `#PS2Dev`

### Post 5 — Flight log (optional: logo)

Build narrative: **[`FLIGHT_LOG.md`](../FLIGHT_LOG.md)** (day-by-day engineering notes on `main`).

https://github.com/94BILLY/PANTHEON/blob/main/FLIGHT_LOG.md

`#GameDev` `#BuildInPublic` `#PS2`

---

## Reddit — sequence (different subs, different days; do not cross-post identical walls of text)

### r/ps2dev (post first — most technical)

**TITLE:**  
Showcase: **Pantheon** — Path 1 PS2 engine (**VU1** / **DMA**), **`v1.0.0-beta`** baseline

**BODY:**  
I’ve published **Pantheon**, a **Path 1–oriented** PlayStation 2 homebrew engine: EE constructs **DMA/VIF1** chains; **VU1** runs **`shader.vsm`** (assembled with **dvp-as**, linked as a normal EE object) and **XGKICK**s GIF work to the **GS**. Shared layout is enforced at compile time via **`pantheon_path1_contract.h`**.

**Default `floor.elf` profile:** **hybrid** — **CPU GIF** draws the exported Softimage **floor**; **Path 1** draws the **skydome** (avoids coplanar Z-fighting). **Strict** all-Path-1 floor is documented under **`BETA_RELEASE.md`** (`PANTHEON_RENDER_PROFILE=1` style build).

**Meshes:** Softimage **`.hrc`** → **`hrc2ps2.py`** (and **`.obj`** → **`obj2ps2.py`**). Repo is a **public record** (all rights reserved; not an open-contribution project).

- **Repo:** https://github.com/94BILLY/PANTHEON  
- **Site:** https://94billy.com/pantheon/  
- **Baseline:** https://github.com/94BILLY/PANTHEON/blob/main/BETA_RELEASE.md  

Happy to go deep on **VIF1** batching, **near-Z**, or the hybrid vs strict split.

---

### r/PS2

**TITLE:**  
Path 1 PS2 engine + Softimage mesh path — **Pantheon** (`v1.0.0-beta`)

**BODY:**  
**Pantheon** is a homebrew **PlayStation 2** engine focused on **Path 1** (VU1 doing transform + GIF pack + kick). Retail PS2 + PCSX2 captures are in the repo and on **94billy.com/pantheon**. Meshes come from a **Softimage → Python cruncher** pipeline documented in **GETTING_STARTED.md**.  

**Repo (read-only public record):** https://github.com/94BILLY/PANTHEON  
**Site:** https://94billy.com/pantheon/

---

### r/gamedev

**TITLE:**  
Bare-metal **PS2** engine — **DMA**, **VU1** microcode, **60 FPS** target (`v1.0.0-beta`)

**BODY:**  
Shipped a small **reference** PS2 executable (**`floor.elf`**) and engine sources: **EE** schedules DMA; **VU1** runs **`shader.vsm`**. Default profile is **hybrid** (CPU path for the walk deck + VU1 for sky) so the whitebox stays stable; **strict** Path 1 is documented for validation.

Day-by-day notes: **FLIGHT_LOG.md**. Not a generic “make a game” kit — a **repro baseline**.

https://github.com/94BILLY/PANTHEON

---

### r/emulation

**TITLE:**  
Custom **PS2** engine — **PCSX2 2.7.x** + **hardware** captures, Path 1 / VU1

**BODY:**  
**Pantheon** targets **PCSX2** for fast iteration and is captured on **real PS2** in-repo (`docs/media/`). Pipeline is **Path 1–centric** with a documented **hybrid** default vs **strict** profile.

https://github.com/94BILLY/PANTHEON

---

### r/retrogaming

**TITLE:**  
**PS2** whitebox engine — boot, sky, floor, orbit cam (**v1.0.0-beta**)

**BODY:**  
**Pantheon**: a **PlayStation 2** “engine record” with a **Softimage**-authored skydome path, timecycle, and walkable deck — see **94billy.com/pantheon** and the GitHub repo for GIFs/stills.

https://94billy.com/pantheon/  
https://github.com/94BILLY/PANTHEON

---

## Visual asset priority (use repo paths)

1. **`docs/media/loop-boot-to-world.gif`** — primary motion proof (boot → world).  
2. **`docs/media/still-world-hero.png`** or **`docs/screenshot_beta1.png`** — still for technical threads.  
3. **`docs/pantheon-logo-zenith-no-text-hero.png`** — brand lockup (README-aligned).

Raw URLs for attachments follow:  
`https://raw.githubusercontent.com/94BILLY/PANTHEON/main/docs/media/loop-boot-to-world.gif` (same pattern for other paths).

---

## Timing (suggested)

- **Day 0:** Set **GitHub Social preview** (manual).  
- **Day 1:** X Post 1 + **r/ps2dev**.  
- **Day 2:** X Post 2 + **r/PS2**.  
- **Day 3:** X Post 3 + **r/gamedev**.  
- **Day 4:** X Post 4 + **r/emulation**.  
- **Day 5:** X Post 5 + **r/retrogaming**.

Space posts so people who saw day 1 can find **FLIGHT_LOG** on day 5 without fatigue.
