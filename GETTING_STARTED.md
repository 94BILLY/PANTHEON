# Pantheon — build and run

## TL;DR

1. Build [ps2toolchain](https://github.com/ps2dev/ps2toolchain) (`ee-gcc` + `dvp-as`) and install [ps2sdk](https://github.com/ps2dev/ps2sdk) — see **Step 1** and **Step 2** below if you have not already.
2. `git clone https://github.com/94BILLY/PANTHEON.git`
3. `cd PANTHEON && make -f Makefile.world`
4. Open **`floor.elf`** in **PCSX2** (or deploy to hardware — **Running on Real PS2 Hardware**).

Experienced PS2 devs often stop here. Everyone else: read the full guide.

**How to read this page:** Requirements → Steps 1–5 → **Build profiles** → **Asset pipeline** (only if you replace meshes) → **Understanding `shader.vsm`** → **Running on Real PS2 Hardware** → **References** → **Common issues** (last). For *what the default binary does* in one screen, open **[`BETA_RELEASE.md`](BETA_RELEASE.md)** first.

**Audience:** people who are **already building PS2 ELFs** and want the same **reproducible** commands as this repository documents. This is not a conceptual introduction to Path 1—use the **EE / VIF / VU / GS** manuals and **ps2sdk** samples for that foundation. For the big-picture story, pair this file with **HANDOFF.md** and official manuals.

**Pantheon** is **Path 1** at the metal: the **EE** issues **DMA/VIF** work; **VU1** runs **`shader.vsm`** and **XGKICK**; the **GS** draws—without a software renderer doing the geometry on the main CPU.

---

## What you are working with

- The Emotion Engine (EE) CPU writes DMA command chains and sends them over
  the VIF1 bus. It does not process geometry.
- Vector Unit 1 (VU1) runs a custom VLIW microprogram (`shader.vsm`) that
  transforms vertices, builds GIF packets, and fires XGKICK directly to the
  Graphics Synthesizer.
- The Graphics Synthesizer (GS) rasterizes. That is all it does.

This is the same pipeline architecture used by Naughty Dog, Kojima Productions,
Rockstar North, and Team Silent on production PS2 titles. There are no wrappers
between your code and the silicon.

---

## System Requirements

### Development Machine

| Requirement | Details |
|-------------|---------|
| OS | Windows 10/11 with WSL2 (Ubuntu 22.04 recommended) |
| RAM | 8 GB minimum |
| Disk | 10 GB free for toolchain build |
| Internet | Required for toolchain download |

### Required Software

| Tool | Version | Purpose |
|------|---------|---------|
| ps2sdk | current `main` | PS2 runtime libraries |
| ee-gcc | 15.x typical | EE C compiler (mips64r5900el); run `ee-gcc --version` — varies with ps2toolchain |
| dvp-as | current | VU1 assembler for `.vsm` microprograms |
| mips64r5900el-ps2-elf-strip | current | ELF strip tool |
| Python | 3.x | Asset pipeline (`hrc2ps2.py`, `obj2ps2.py`) |
| PCSX2 | 2.7.x | PS2 emulator for development |

### Optional (Asset Creation)

| Tool | Version | Purpose |
|------|---------|---------|
| Softimage 3D | 3.92 | 3D modelling — the original tool used by Kojima and Rockstar |
| Windows NT 4.0 VM | — | Required to run Softimage 3.92 |
| hrcConvert.exe | — | Converts Softimage binary `.hrc` to ASCII |
| VirtualBox / VMware | current | Hosts the NT VM |

You do not need Softimage to build and run Pantheon. The repository includes
pre-crunched geometry headers (`floor_data.h`, `skydome_data.h`). Softimage
is required only if you are **regenerating those headers** or replacing meshes **inside this reference tree**—not as encouragement to fork into a general “make a game” workflow.

---

## Step 1 — Install the PS2 Toolchain

### In WSL2 (Ubuntu)

```bash
# Install build dependencies
sudo apt update
sudo apt install -y build-essential cmake git python3 texinfo libgmp-dev \
    libmpfr-dev libmpc-dev

# Clone and build ps2toolchain (this compiles ee-gcc and dvp-as from source)
git clone https://github.com/ps2dev/ps2toolchain.git
cd ps2toolchain
./toolchain.sh
```

This takes 30–60 minutes. It builds the full MIPS EE cross-compiler and the
DVP assembler for VU1 microcode.

### Set Environment Variables

Add to `~/.bashrc`:

```bash
export PS2DEV=/usr/local/ps2dev
export PS2SDK=$PS2DEV/ps2sdk
export PATH=$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/dvp/bin:$PATH
```

Then:

```bash
source ~/.bashrc
```

### Verify

```bash
ee-gcc --version       # should show mips64r5900el-ps2-elf (version depends on toolchain)
dvp-as --version       # VU assembler
which dvp-as           # should be /usr/local/ps2dev/dvp/bin/dvp-as
```

---

## Step 2 — Install ps2sdk

```bash
git clone https://github.com/ps2dev/ps2sdk.git
cd ps2sdk
make
make install
```

Verify:

```bash
ls $PS2SDK/ee/lib      # should contain libdraw.a, libgraph.a, libmath3d.a etc.
```

---

## Step 3 — Clone Pantheon

```bash
git clone https://github.com/94BILLY/Pantheon.git
cd Pantheon
```

---

## Step 4 — Build

```bash
make -f Makefile.world
```

On success this produces `floor.elf` and strips it.

Expected output:

```
ee-gcc -D_EE -G0 -O2 -Wall -fno-lto ... -c floor.c -o floor.o
dvp-as -o shader.o shader.vsm
ee-gcc ... -o floor.elf floor.o shader.o pantheon_vram.o pantheon_texture_host.o ...
mips64r5900el-ps2-elf-strip --strip-all floor.elf
```

If `dvp-as` is not found, your `PATH` is not set correctly. See Step 1.

---

## Step 5 — Run in PCSX2

### PCSX2 Setup (Windows)

1. Download PCSX2 2.7.x from [pcsx2.net](https://pcsx2.net)
2. Provide a PS2 BIOS image (required — not included)
3. In PCSX2: `File → Boot File` → select `floor.elf`

### WSL → Windows file path

```bash
# Copy ELF to Windows Desktop for PCSX2 access
cp floor.elf /mnt/c/Users/YOUR_USERNAME/Desktop/pantheon.elf
```

### Expected behaviour on first boot

- Black screen with luma ramp
- **WWW.94BILLY.COM** title appears (staggered reveal + gentle wave)
- Sky fades in (timecycle atmosphere, overcast daylight by default)
- Green floor deck renders
- Left stick moves player. Right stick orbits camera.
- L1/R1 zoom camera.

---

## Build Profiles

### Default — Hybrid (recommended)

```bash
make -f Makefile.world
```

CPU GIF path renders the walkable floor deck. Path 1 VU1 renders the skydome.
No coplanar Z-fighting. This is the stable baseline.

### Strict Path 1 Validation

```bash
make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'
```

VU1 renders both skydome and floor tiles. CPU GIF overlay disabled.
Use this to validate the Path 1 floor pipeline in isolation.
The CPU deck is off — only VU1 geometry appears.

### No Boot Sequence

```bash
make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_BOOT_REVEAL_ENABLE=0'
```

Skips the luma ramp and URL title. World renders on frame 1.
Useful for fast iteration during development.

### Texture Phase Preview (Phase 2)

```bash
make -f Makefile.world PANTHEON_TEXTURE_PHASE=1
```

Enables the host texture upload slice. Allocates a 64×64 PSMCT32 block in
VRAM, fills it with a checker pattern, and uploads via GIF IMAGE/TRX path
with TEXFLUSH. Requires the VRAM allocator to have sufficient headroom.
This is an early Phase 2 preview — no texture sampling in `shader.vsm` yet.

### Combined flags

```bash
make -f Makefile.world \
    EE_EXTRA_CFLAGS='-DPANTHEON_RENDER_PROFILE=1 -DPANTHEON_BOOT_REVEAL_ENABLE=0 -DPANTHEON_MIN_TELEMETRY=1'
```

---

## Telemetry

Enable boot-time `printf` output:

```bash
make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_MIN_TELEMETRY=1'
```

PCSX2 will print VRAM layout, frame counter, camera position, VIF1 quadword
counts, and near-Z cull hits to the console. Disable for release builds.

---

## Asset pipeline — mesh regeneration (reference tree)

This documents the **Softimage → header** and **OBJ → .c** paths used to keep `floor_data.h` / `skydome_data.h` (and related) consistent with `floor.c`. Skip if you only build against the committed headers.

### Requirements

- Softimage 3D 3.92 running on a Windows NT 4.0 VM
- `hrcConvert.exe` on the NT VM
- Python 3.x on your development machine
- WSL shared folder or network share between NT VM and host

### Step 1 — Model in Softimage 3D 3.92

- Work in real-world scale. 1 Softimage unit = 1 PS2 world unit.
- Keep faces as quads. The cruncher handles triangulation automatically.
- Y-up in Softimage. The cruncher maps to the PS2 floor convention used in `floor.c` (winding and XY→XZ handling are pinned in [`BETA_RELEASE.md`](BETA_RELEASE.md)).
- Export as `.hrc` (Softimage binary hierarchy format).

### Step 2 — Convert to ASCII

On the NT VM:

```
hrcConvert.exe model.hrc model_ascii.hrc -a
```

Copy `model_ascii.hrc` to your WSL environment.

### Step 3 — Run the Cruncher

```bash
python3 hrc2ps2.py model_ascii.hrc
```

Output: `model_data.h`

**Skydome (shipping headers):** to match the repo’s gradient behavior when regenerating from `skydome.hrc`, use:

```bash
python3 hrc2ps2.py skydome.hrc -o skydome_data.h --color-mode gradient
```

The cruncher performs:
- Quad → triangle splitting
- W=1.0f padding on all vertices for 128-bit quadword alignment
- Axis mapping to PS2 world/floor conventions (see `hrc2ps2.py` and `BETA_RELEASE.md`)
- `PantheonVertex` struct emission with `__attribute__((aligned(16)))`
- `PantheonTriangle` index array emission

### Wavefront OBJ (Blender, Maya, etc.)

Export a triangulated (or n-gon) mesh as `.obj`, then from the repo root:

```bash
python3 obj2ps2.py your_model.obj your_model_data.c your_array_name
```

This emits a `.c` file with `PantheonVertex` arrays and a `PantheonVIFHeader` stub. Wiring it to the EE DMA path is **your** integration burden; the committed reference uses **`hrc2ps2.py`** headers for the shipped floor/skydome. OBJ is an alternate crunch path documented for completeness, not an invitation to treat the repo as a game template.

### Step 4 — Include in your build

```c
#include "model_data.h"
// model_vertices[MODEL_VERT_COUNT]
// model_indices[MODEL_TRI_COUNT]
```

Pass to `render_path1_tris()` the same way `floor_data.h` and
`skydome_data.h` are used in `floor.c`.

### Geometry budget per VU1 batch

VU1 data memory is 16 KB (1024 quadwords).
The Path 1 contract reserves addresses 0–7 for constants (perspective,
screen bias, MVP matrix) and address 240 for the GIF tag output.
This gives a practical per-batch limit of:

```
PATH1_VU_MAX_BATCH_VERTS = 72 vertices per submit
```

For larger meshes, the engine batches automatically in `render_path1_tris()`.

---

## Understanding `shader.vsm`

`shader.vsm` is the VU1 microprogram. It is the most important file in the
repository. If you want to extend Pantheon — add texture coordinates, lighting,
fog — this is where that work happens.

### VU1 data memory layout (from `pantheon_path1_contract.h`)

```
Address 0–1   : Perspective constants
Address 2–3   : Screen bias
Address 4–7   : MVP matrix (4 rows)
Address 8+    : Vertex input payload (position, color, STQ per vertex)
Address 240   : GIF tag output (XGKICK fires from here)
Address 241+  : Transformed vertex output stream
```

### Build and link

`shader.vsm` is assembled by `dvp-as` into `shader.o` and linked into
`floor.elf` as a standard ELF object. The symbol `PantheonShader` is the
entry point. It resolves to a non-zero address at link time — if it resolves
to 0, `dvp-as` did not run or the `.vutext` section is missing.

### Extending the microprogram

To add a new per-vertex attribute (e.g. fog density):

1. Add the attribute to `PantheonVertex` in `floor_data.h`
2. Update `pantheon_path1_contract.h` with the new VU1 address
3. Update the UNPACK payload builder in `floor.c`
4. Add the attribute read and math to `shader.vsm`
5. The compile-time `#error` guards in `pantheon_path1_contract.h` will
   catch any address overlap before the build succeeds

---

## Running on Real PS2 Hardware

### Requirements

- PS2 Fat or Slim with FreeMcBoot or OPL installed
- USB drive or network share accessible via OPL
- `floor.elf` built and stripped

### Deploy

```bash
# Rename for OPL compatibility if needed
cp floor.elf PANTHEON.ELF
```

Copy to USB drive root or your OPL network share.
Launch via OPL → Applications → PANTHEON.ELF.

Tested on PS2 Fat (SCPH-50000) with FreeMcBoot.
PS3 BC (backward-compatible) also works for upscaled output.

---

## References

- [ps2dev/ps2sdk](https://github.com/ps2dev/ps2sdk)
- [ps2dev/ps2toolchain](https://github.com/ps2dev/ps2toolchain)
- [ps2dev/dvp](https://github.com/ps2dev/dvp)
- [PCSX2](https://pcsx2.net)
- Sony PS2 Runtime Library Release 3.0 — official VU1 programming samples
- Funslower / soopadoopa (2001, scene.org) — Path 1 demoscene reference
- `FLIGHT_LOG.md` — development journal, Day 1 → Day 7
- `HANDOFF.md` — SCE tree paths and doc index

---

## Common issues

**Black screen, no boot title**  
VU1 microprogram not executing. Check that `shader.o` was built by `dvp-as`
and linked. `PantheonShader` must resolve to a non-zero address. Enable
`PANTHEON_MIN_TELEMETRY=1` and check PCSX2 console for boot output.

**Grey screen (world not rendering)**  
DMA chain built but GS not receiving valid packets. Enable telemetry and
check VIF1 quadword count. Batch size may exceed 255 QW limit.
Check `PATH1_VU_MAX_BATCH_VERTS` against your mesh vertex count.

**Flickering floor**  
Coplanar Z-fight between CPU GIF deck and Path 1 floor tiles.
Use the default hybrid profile (`PANTHEON_RENDER_PROFILE=0`) which keeps
the two floor layers on separate render passes, or disable one.

**`dvp-as: command not found`**  
`/usr/local/ps2dev/dvp/bin` is not in your `PATH`. See Step 1.

**`cannot find -ldraw`**  
ps2sdk not installed or `PS2SDK` env var not set. See Step 2.
