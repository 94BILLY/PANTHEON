# Pantheon website landing (Nova-style)

**File:** `pantheon-landing-nova-style.html`

WordPress: add a **Custom HTML** block and paste the full contents.

## Before publishing

1. **Logo** — Replace the `<img src="...">` in the header with your Pantheon mark (upload to Media Library and use that URL), or add `docs/pantheon-mark.png` to the repo and point `raw` URL to it.
2. **Video** — Replace the gray placeholder `div` in section 2 with your real `<iframe>` (same markup as Nova; only change `src` to `https://www.youtube.com/embed/YOUR_ID?rel=0&modestbranding=1`).
3. **Hero image** — Optional wide screenshot under section 4; replace `pantheon-hero-placeholder.png` or use a WP media URL.
4. **Version banner** — Edit the `v1.0.0-beta.1` line to match your current Git tag.
5. **Rights** — Add a footer line on the live site if you want (“All rights reserved” / no license), separate from this template.

## Repo layout

Keeping the HTML in `docs/` avoids coupling the PS2 build to WordPress; you can copy-paste or sync to `94billy.com` when you update copy.
