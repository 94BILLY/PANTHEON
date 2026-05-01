Pantheon NotebookLM snapshots (UTF-8 text)

Primary file for NotebookLM:
  _notebooklm/pantheon_code_snapshot_full.txt  (~215 KB)

Includes: README, BASELINE, Makefiles, Path1/VU/VRAM/texture headers & sources,
floor.c, shader.vsm, generated floor_data.h / skydome_data.h, hrc2ps2/obj2ps2/fix_shader,
cube samples, and skydome.hrc.

Each section starts with:
  ================================================================================
  ### FILE: <relative/path>
  ================================================================================

Regenerate:
  python3 scripts/export_notebooklm_snapshot.py

Excluded by design: *.elf, binaries, __pycache__, large flight logs.
