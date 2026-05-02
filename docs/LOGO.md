# Pantheon mark

## Source of truth (code / vector)

**`pantheon-logo-mark.svg`** — Plain XML geometry you can version in Git, diff, and scale without loss.

**Raw file on GitHub (swap branch/tag as needed):**  
`https://github.com/94BILLY/Pantheon/raw/main/docs/pantheon-logo-mark.svg`

- **Website:** `<img src="https://github.com/94BILLY/Pantheon/raw/main/docs/pantheon-logo-mark.svg" width="114" height="114" alt="Pantheon" />` or upload the same file to WordPress Media and point at your CDN. **Inline SVG** is best if you want `currentColor` / CSS theming without editing the file per theme.
- **Everywhere else:** Export PNG/WebP at any size from the SVG (Figma, Inkscape, `rsvg-convert`, etc.). The old **`pantheon-logo-mark-timeless.png`** is optional reference only; prefer regenerating from SVG when you change the mark.

## One-file “rooted in code” workflow

1. Edit **`pantheon-logo-mark.svg`** (coordinates only — no filters, no embedded bitmaps).
2. Commit the change; Git history is the logo history.
3. Re-export raster fallbacks for email clients or legacy CMS if needed.

## Optional wordmark

Keep type separate: set “PANTHEON” in CSS with a neutral system font stack or your site font; do not bake text into the SVG unless you lock a specific wordmark version.

## Color

Default fill in the SVG is **`#1a1a1a`**. For a white mark on dark backgrounds, duplicate the file or use inline SVG + CSS:

```html
<style>.pantheon-mark path, .pantheon-mark rect { fill: #fff; }</style>
```

Or replace `fill="#1a1a1a"` with `fill="currentColor"` in the SVG and set `color` on the parent.
