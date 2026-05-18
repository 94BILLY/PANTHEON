# PANTHEON — Engine State v1.0.1-Beta

## VRAM Map (GS 4MB, 32-bit word addresses)

| Region | Start | Size | Alignment |
|:--|:--|:--|:--|
| Framebuffer 0 | 0x00000 | 512×448×32bpp | 8KB page |
| Framebuffer 1 | 0x28000 | 512×448×32bpp | 8KB page |
| Z-buffer | 0x50000 | 512×448×32bpp | 8KB page |
| Texture heap | 0x78000 | remainder | 256-byte block |

## VU1 Data Memory Contract

- Ceiling: 16KB (1024 QW)
- Max verts per batch: 72 (PATH1_VU_MAX_BATCH_VERTS)
- Quadword alignment enforced on all VIF payloads

## Build Flags

| Flag | Default | Effect |
|:--|:--|:--|
| `PANTHEON_HARDWARE_TARGET` | unset | Enables IOP audio (LIBSD + AUDSRV) |
| `PANTHEON_TILE_RADIUS` | 15 | Floor treadmill radius (31×31 = 961 tiles) |
| `PANTHEON_TILE_SUBMIT_BUDGET` | 961 | Max tiles per frame |
| `PANTHEON_TEXTURE_PHASE` | 0 | Enables host texture upload slice |

## Audio Status

**Hardware path:** LIBSD + AUDSRV (rom0), 48000Hz/16bit/stereo, 4096-byte chunk streaming.  
**PCSX2 path:** no-op (gated by `#ifdef PANTHEON_HARDWARE_TARGET`).  
**Theme data:** `theme_wav_data.h` (embedded PCM, linked but unused in emulator builds).

## Toolchain

- Compiler: `ee-gcc` (mips64r5900el)
- Assembler: `dvp-as` (VU1 microcode)
- Linker: `ee-gcc` with `-T$(PS2SDK)/ee/startup/linkfile`
- Strip: `/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-strip`
- SDK: ps2sdk @ `/usr/local/ps2dev/ps2sdk`

## Build Examples

**PCSX2 (clean emulator build):**
```bash
make -f Makefile.world
```

**Real PS2 hardware (with audio):**
```bash
make -f Makefile.world EE_EXTRA_CFLAGS='-DPANTHEON_HARDWARE_TARGET'
```
