# Pantheon logo (zenith oculus)

## Source

- **`pantheon-oculus-straight-on-zenith.png`** — original render (no post-processing in the logo pipeline).

## Pipeline

Centered square crop → resize → circular alpha only (no color edits, no lip scripts).

## Files in `docs/`

| File | Role |
| :--- | :--- |
| `pantheon-logo-zenith-circle-only.png` | Circle mark only, transparent (4096). |
| `pantheon-logo-zenith-lockup.png` | Mark + **PANTHEON** wordmark (4096 canvas). |
| `pantheon-logo-zenith-same-size-no-text.png` | Same canvas height as lockup; transparent below the circle. |
| `pantheon-logo-zenith-*-hero.png` / `.webp` | **720px-wide** derivatives for README and web (smaller git size). |

## Regenerate

```bash
python3 tools/generate_pantheon_zenith_logos.py
python3 tools/generate_pantheon_zenith_logos.py --width 6144 --out-dir /path/to/export
```
