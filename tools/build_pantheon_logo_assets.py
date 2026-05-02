#!/usr/bin/env python3
"""
Build Pantheon logo assets from docs/pantheon-interior-oculus-sky-pov.png

Outputs (under docs/):
  - pantheon-logo-circle-1024.png   — circular mark, transparent (RGBA)
  - pantheon-logo-lockup-vertical.png — mark + PANTHEON (transparent bg, dark text)
  - pantheon-logo-lockup-on-dark.png  — same layout, light text for dark UI
  - pantheon-logo-lockup-horizontal.png — mark left, wordmark right

Typography: uses Liberation Serif if Times New Roman is unavailable (set
PANTHEON_LOGO_FONT to a .ttf path to override). Brand target: Times New Roman.
"""
from __future__ import annotations

import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "docs" / "pantheon-interior-oculus-sky-pov.png"
OUT_DIR = REPO / "docs"

# Oculus crop: sky blob from blue opening (see THE_ONE / prior art)
FRAC = 0.62  # oculus radius / (crop_side/2); higher = larger hole / more sky


def _find_font() -> str:
    env = os.environ.get("PANTHEON_LOGO_FONT")
    if env and Path(env).is_file():
        return env
    candidates = [
        "/usr/share/fonts/truetype/msttcorefonts/Times_New_Roman.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf",
    ]
    for c in candidates:
        if Path(c).is_file():
            return c
    raise SystemExit("No suitable serif font found; set PANTHEON_LOGO_FONT")


def build_circle_mark() -> Image.Image:
    import numpy as np

    if not SRC.is_file():
        raise SystemExit(f"Missing source: {SRC}")

    im = Image.open(SRC).convert("RGB")
    w, h = im.size
    a = np.asarray(im, dtype=np.float32)
    R, G, B = a[..., 0], a[..., 1], a[..., 2]
    Y = 0.2126 * R + 0.7152 * G + 0.0722 * B
    sky = ((B - R) > 25) & (Y > 120)
    ys, xs = np.where(sky)
    if xs.size == 0:
        raise SystemExit("Sky mask empty; cannot locate oculus")
    cx = float(xs.mean())
    cy = float(ys.mean())

    xmin, xmax = int(xs.min()), int(xs.max())
    ymin, ymax = int(ys.min()), int(ys.max())
    r_bbox = 0.5 * min(xmax - xmin + 1, ymax - ymin + 1)
    r_est = float(r_bbox * 0.92)

    half = int(max(110, min(min(w, h) // 2 - 1, r_est / FRAC)))
    side = 2 * half

    def clamp_crop(cx_: float, cy_: float, side_: int) -> tuple[int, int, int, int]:
        half_ = side_ // 2
        x0 = int(round(cx_ - half_))
        y0 = int(round(cy_ - half_))
        x1 = x0 + side_
        y1 = y0 + side_
        if x0 < 0:
            x1 -= x0
            x0 = 0
        if y0 < 0:
            y1 -= y0
            y0 = 0
        if x1 > w:
            shift = x1 - w
            x0 -= shift
            x1 = w
        if y1 > h:
            shift = y1 - h
            y0 -= shift
            y1 = h
        x0 = max(0, x0)
        y0 = max(0, y0)
        cw, ch = x1 - x0, y1 - y0
        if cw < side_ or ch < side_:
            side_ = min(cw, ch)
            half_ = side_ // 2
            xc_ = int(round(cx_))
            yc_ = int(round(cy_))
            x0 = max(0, min(w - side_, xc_ - half_))
            y0 = max(0, min(h - side_, yc_ - half_))
            x1 = x0 + side_
            y1 = y0 + side_
        return x0, y0, x1, y1

    x0, y0, x1, y1 = clamp_crop(cx, cy, side)
    crop = im.crop((x0, y0, x1, y1)).resize((1024, 1024), Image.LANCZOS)

    mask = Image.new("L", (1024, 1024), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, 1023, 1023), fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(radius=1.2))

    out = crop.convert("RGBA")
    out.putalpha(mask)
    return out


def _wordmark_image(text: str, font_path: str, size: int, fill: tuple[int, int, int]) -> Image.Image:
    font = ImageFont.truetype(font_path, size)
    tmp = Image.new("RGBA", (4, 4), (0, 0, 0, 0))
    dr = ImageDraw.Draw(tmp)
    bbox = dr.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    pad = 8
    im = Image.new("RGBA", (tw + pad * 2, th + pad * 2), (0, 0, 0, 0))
    dr = ImageDraw.Draw(im)
    dr.text((pad - bbox[0], pad - bbox[1]), text, font=font, fill=fill + (255,))
    return im


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    font_path = _find_font()

    mark = build_circle_mark()
    circle_path = OUT_DIR / "pantheon-logo-circle-1024.png"
    mark.save(circle_path)
    # Back-compat / single-name convenience
    mark.save(OUT_DIR / "pantheon-oculus-circle-transparent.png")

    word = "PANTHEON"
    wm_dark = _wordmark_image(word, font_path, 86, (20, 20, 22))
    wm_light = _wordmark_image(word, font_path, 86, (245, 245, 248))

    gap = 36
    # Vertical lockup
    mw, mh = mark.size
    ww, wh = wm_dark.size
    vw = max(mw, ww)
    vh = mh + gap + wh
    vert = Image.new("RGBA", (vw, vh), (0, 0, 0, 0))
    vert.paste(mark, ((vw - mw) // 2, 0), mark)
    vert.paste(wm_dark, ((vw - ww) // 2, mh + gap), wm_dark)
    vert.save(OUT_DIR / "pantheon-logo-lockup-vertical.png")

    vert_dark = Image.new("RGBA", (vw, vh), (0, 0, 0, 0))
    vert_dark.paste(mark, ((vw - mw) // 2, 0), mark)
    vert_dark.paste(wm_light, ((vw - ww) // 2, mh + gap), wm_light)
    vert_dark.save(OUT_DIR / "pantheon-logo-lockup-on-dark.png")

    # Horizontal lockup
    gap_h = 48
    hw = mw + gap_h + ww
    hh = max(mh, wh)
    horiz = Image.new("RGBA", (hw, hh), (0, 0, 0, 0))
    horiz.paste(mark, (0, (hh - mh) // 2), mark)
    horiz.paste(wm_dark, (mw + gap_h, (hh - wh) // 2), wm_dark)
    horiz.save(OUT_DIR / "pantheon-logo-lockup-horizontal.png")

    print("Wrote:")
    for p in (
        circle_path,
        OUT_DIR / "pantheon-oculus-circle-transparent.png",
        OUT_DIR / "pantheon-logo-lockup-vertical.png",
        OUT_DIR / "pantheon-logo-lockup-on-dark.png",
        OUT_DIR / "pantheon-logo-lockup-horizontal.png",
    ):
        print(" ", p.relative_to(REPO))


if __name__ == "__main__":
    main()
