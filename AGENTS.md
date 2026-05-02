# Agent notes

## Cursor Cloud specific instructions

- **Build (EE):** From repo root, `make -f Makefile.world` (requires PS2SDK / `ee-gcc` on `PATH`; see `Makefile.world`). Default output: `floor.elf`.
- **Emulator (PCSX2):** Local verification is the source of truth for visuals (GPU backend, colors, and moiré on the checker floor). This cloud image may not ship a PS2 BIOS; place yours in `~/.config/PCSX2/bios/` and point `inis/PCSX2.ini` at it.
- **Headless smoke + capture (this VM):** `dbus-run-session -- xvfb-run -a -s "-screen 0 1280x720x24"` then run the extracted AppImage’s `squashfs-root/usr/bin/pcsx2-qt /path/to/floor.elf`. Pair with `ffmpeg -f x11grab` on the same `DISPLAY` to record boot → white hold → Path1 grid/sky. **Software** renderer captures often look darker / flatter than your desktop PCSX2; use them for “does the sequence run,” not final art approval.
- **If using PCSX2 AppImage here:** FUSE may be missing — extract instead of running the AppImage directly:  
  `./pcsx2-*.AppImage --appimage-extract` then run `squashfs-root/usr/bin/pcsx2-qt` with `LD_LIBRARY_PATH` set to `squashfs-root/usr/lib`. Install **`libxcb-cursor0`** if Qt reports the xcb platform plugin failing.
- **Flicker / Z-fight:** See `BETA_RELEASE.md` — do not enable double-sided Path1 floor with overlapping coplanar passes; hybrid mode skips the CPU GIF floor when the Path1 floor job is on to avoid Z-fighting.
- **Strict Path1-only:** `make -f Makefile.world EE_CFLAGS='-DPANTHEON_RENDER_PROFILE=1'` (disables CPU fallback paths; documented in `BETA_RELEASE.md`).

For product behavior and pinned defaults, prefer **`README.md`** and **`BETA_RELEASE.md`** over duplicating them here.
