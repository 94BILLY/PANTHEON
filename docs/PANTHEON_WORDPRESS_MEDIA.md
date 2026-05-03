# Pantheon images on 94billy.com (WordPress)

## `pantheon-logo-zenith.webm` (release / site)

**In the repo:** `docs/media/pantheon-logo-zenith.webm` (~89 KB, VP9 + alpha, 512×512, 2 s loop).

**On your PC after `git pull`:**
```powershell
cd C:\path\to\Pantheon
git pull
copy docs\media\pantheon-logo-zenith.webm $env:USERPROFILE\Desktop\
```

**If `https://raw.githubusercontent.com/.../pantheon-logo-zenith.webm` returns 404**  
GitHub often does not serve `raw` for **private** repos without auth. Use either:

- **GitHub web:** open the file in the repo → **Download raw file**, or  
- **Git archive:**  
  `git archive origin main docs/media/pantheon-logo-zenith.webm | tar -x`

Then upload that `.webm` to **WordPress → Media** as **`pantheon-logo-zenith.webm`** (or attach to a **GitHub Release** as a binary asset).

**Landing page:** In `docs/pantheon-landing.html`, uncomment the **`<video>...</video>`** block under the title (or paste the same snippet from the comment into your Custom HTML block).

---

The landing HTML uses **`https://94billy.com/wp-content/uploads/2026/05/`** plus these filenames. WordPress may change the month folder when you upload — if images 404, open **Media →** each file **→ Copy URL** and replace the five URLs in `docs/pantheon-landing.html` (and the hero `url()` in `docs/pantheon-94billy.html`).

Upload from your Pantheon repo (same bytes, these names on disk in `docs/` or `docs/media/`):

| Save as (Media Library filename) | Source in repo |
| :--- | :--- |
| `pantheon-logo-zenith.webm` | `docs/media/pantheon-logo-zenith.webm` (optional — VP9 + alpha; use if you add a `<video>` mark back to the header) |
| `pantheon-logo-zenith-no-text-hero.png` | `docs/pantheon-logo-zenith-no-text-hero.png` (README / other) |
| `pantheon-still-world-hero.png` | `docs/media/still-world-hero.png` |
| `pantheon-still-boot-title-4586.png` | `docs/still-boot-title-4586.png` |
| `pantheon-loop-world-orbit.gif` | `docs/media/loop-world-orbit.gif` |
| `pantheon-screenshot-beta1.png` | `docs/screenshot_beta1.png` |

After upload, URLs should look like:

`https://94billy.com/wp-content/uploads/2026/05/pantheon-logo-zenith-no-text-hero.png`

If your install uses a different year/month path, do a find/replace on that base path in the HTML files.
