# Pantheon images on 94billy.com (WordPress)

The landing HTML uses **`https://94billy.com/wp-content/uploads/2026/05/`** plus these filenames. WordPress may change the month folder when you upload — if images 404, open **Media →** each file **→ Copy URL** and replace the five URLs in `docs/pantheon-landing.html` (and the hero `url()` in `docs/pantheon-94billy.html`).

Upload from your Pantheon repo (same bytes, these names on disk in `docs/` or `docs/media/`):

| Save as (Media Library filename) | Source in repo |
| :--- | :--- |
| `pantheon-logo-zenith.webm` | `docs/media/pantheon-logo-zenith.webm` (VP9 + alpha, 512×512, ~2 s loop — site header) |
| `pantheon-logo-zenith-no-text-hero.png` | `docs/pantheon-logo-zenith-no-text-hero.png` (optional fallback elsewhere) |
| `pantheon-still-world-hero.png` | `docs/media/still-world-hero.png` |
| `pantheon-still-boot-title-4586.png` | `docs/still-boot-title-4586.png` |
| `pantheon-loop-world-orbit.gif` | `docs/media/loop-world-orbit.gif` |
| `pantheon-screenshot-beta1.png` | `docs/screenshot_beta1.png` |

After upload, URLs should look like:

`https://94billy.com/wp-content/uploads/2026/05/pantheon-logo-zenith-no-text-hero.png`

If your install uses a different year/month path, do a find/replace on that base path in the HTML files.
