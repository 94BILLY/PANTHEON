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

---

The landing HTML uses **`https://94billy.com/wp-content/uploads/2026/05/`** plus these filenames. WordPress may change the month folder when you upload — if images 404, open **Media →** each file **→ Copy URL** and replace URLs in `docs/pantheon-landing.html` (and the hero `url()` in `docs/pantheon-94billy.html`).

**`docs/pantheon-landing.html` (94billy.com block)** uses **only one** image asset: **`pantheon-loop-boot-to-world.gif`** — same bytes as **`docs/media/loop-boot-to-world.gif`** (README hero). No other landing images.

Upload from your Pantheon repo:

| Save as (Media Library filename) | Source in repo |
| :--- | :--- |
| `pantheon-loop-boot-to-world.gif` | **`docs/media/loop-boot-to-world.gif`** |

Optional (not used by current landing HTML):

| Save as | Source |
| :--- | :--- |
| `pantheon-logo-zenith.webm` | `docs/media/pantheon-logo-zenith.webm` |
| `pantheon-logo-zenith-no-text-hero.png` | `docs/pantheon-logo-zenith-no-text-hero.png` |
| `pantheon-screenshot-beta1.png` | `docs/screenshot_beta1.png` |

After upload, URLs should look like:

`https://94billy.com/wp-content/uploads/2026/05/pantheon-logo-zenith-no-text-hero.png`

If your install uses a different year/month path, do a find/replace on that base path in the HTML files.
