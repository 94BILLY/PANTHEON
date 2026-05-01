# Agent notes

## Cursor Cloud specific instructions

- **Build (EE):** From repo root, `make -f Makefile.world` (requires PS2SDK / `ee-gcc` on `PATH`; see `Makefile.world`). Default output: `floor.elf`.
- **Emulator (PCSX2):** Local verification is the source of truth for visuals. This cloud image may not ship a PS2 BIOS; without BIOS + GPU, Qt-based PCSX2 often **blocks or produces no console output** under `xvfb-run` even when the binary starts (X11/dbus traffic only).
- **If using PCSX2 AppImage here:** FUSE may be missing — extract instead of running the AppImage directly:  
  `./pcsx2-*.AppImage --appimage-extract` then run `squashfs-root/usr/bin/pcsx2-qt` with `LD_LIBRARY_PATH` set to `squashfs-root/usr/lib`. Install **`libxcb-cursor0`** if Qt reports the xcb platform plugin failing.
- **Flicker / Z-fight:** See `BETA_RELEASE.md` — do not enable double-sided Path1 floor with overlapping coplanar passes; hybrid mode skips the CPU GIF floor when the Path1 floor job is on to avoid Z-fighting.
- **Strict Path1-only:** `make -f Makefile.world EE_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'` (disables CPU fallback paths; documented in `BETA_RELEASE.md`).

For product behavior and pinned defaults, prefer **`README.md`** and **`BETA_RELEASE.md`** over duplicating them here.
