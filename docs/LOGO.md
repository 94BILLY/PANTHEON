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
| `media/pantheon-logo-zenith.webm` | **VP9 + alpha** WebM (512×512, 2 s loop). **Release path:** clone/pull repo → this file; if `raw.githubusercontent.com` 404s (private repo), download from GitHub UI or `git archive` (see `docs/PANTHEON_WORDPRESS_MEDIA.md`). |

Encode (from circle asset):

```bash
ffmpeg -y -framerate 1 -loop 1 -i docs/pantheon-logo-zenith-circle-only.png \
  -vf "scale=512:512:flags=lanczos,format=rgba" -t 2 \
  -c:v libvpx-vp9 -pix_fmt yuva420p -auto-alt-ref 0 -b:v 0 -crf 32 -an \
  docs/media/pantheon-logo-zenith.webm
```

## Regenerate

```bash
python3 tools/generate_pantheon_zenith_logos.py
python3 tools/generate_pantheon_zenith_logos.py --width 6144 --out-dir /path/to/export
```
