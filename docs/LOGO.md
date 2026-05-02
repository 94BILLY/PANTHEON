# Pantheon logo assets

## Files

| File | Use |
|------|-----|
| `pantheon-logo-circle-1024.png` | Circular oculus mark only (RGBA, 1024×1024). |
| `pantheon-oculus-circle-transparent.png` | Same mark (kept as a stable alias for older links). |
| `pantheon-logo-lockup-vertical.png` | Mark above **PANTHEON** — light backgrounds. |
| `pantheon-logo-lockup-on-dark.png` | Same layout, light wordmark — dark backgrounds. |
| `pantheon-logo-lockup-horizontal.png` | Mark left, wordmark right — light backgrounds. |

Wordmark is **Times New Roman** when available on the build machine; otherwise **Liberation Serif** (metrically similar). Override with:

`PANTHEON_LOGO_FONT=/path/to/Times_New_Roman.ttf python3 tools/build_pantheon_logo_assets.py`

## Regenerate

Source render: `pantheon-interior-oculus-sky-pov.png`.

```bash
python3 tools/build_pantheon_logo_assets.py
```

The script centers the crop on the detected sky/oculus opening and applies a soft circular mask.
