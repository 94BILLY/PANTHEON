#!/usr/bin/env python3
"""
Regenerate Pantheon zenith logos from docs/pantheon-oculus-straight-on-zenith.png

Pure pipeline: centered square crop → resize → circular alpha (no color edits).
Optional wordmark lockup. Writes hero WebP/PNG for the repo by default.

Usage:
  python3 tools/generate_pantheon_zenith_logos.py
  python3 tools/generate_pantheon_zenith_logos.py --width 6144 --out-dir /tmp/pantheon_export
"""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "docs" / "pantheon-oculus-straight-on-zenith.png"


def build_circle(square: Image.Image) -> Image.Image:
    w = square.size[0]
    ms = w * 2
    mb = Image.new("L", (ms, ms), 0)
    ImageDraw.Draw(mb).ellipse((0, 0, ms - 1, ms - 1), fill=255)
    mask = mb.resize((w, w), Image.Resampling.LANCZOS)
    out = square.convert("RGBA")
    out.putalpha(mask)
    return out


def font_path() -> str:
    for p in (
        "/usr/share/fonts/truetype/msttcorefonts/Times_New_Roman.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf",
    ):
        if Path(p).is_file():
            return p
    raise SystemExit("No Times New Roman / Liberation Serif found")


def wordmark_png(text: str, width_ref: int) -> Image.Image:
    fp = font_path()
    fs = int(round(320 * width_ref / 4096))
    pad = int(round(48 * width_ref / 4096))
    f = ImageFont.truetype(fp, fs)
    tmp = Image.new("RGBA", (8, 8), 0)
    d = ImageDraw.Draw(tmp)
    bb = d.textbbox((0, 0), text, font=f)
    tw, th = bb[2] - bb[0], bb[3] - bb[1]
    im = Image.new("RGBA", (tw + 2 * pad, th + 2 * pad), 0)
    d = ImageDraw.Draw(im)
    d.text((pad - bb[0], pad - bb[1]), text, font=f, fill=(22, 22, 24, 255))
    return im


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--width", type=int, default=4096, help="Output square width (default 4096)")
    ap.add_argument("--out-dir", type=Path, default=REPO / "docs", help="Directory for full-size PNGs")
    ap.add_argument("--hero-max", type=int, default=720, help="Max width for hero PNG/WebP (0=skip)")
    args = ap.parse_args()

    if not SRC.is_file():
        raise SystemExit(f"Missing source: {SRC}")

    im = Image.open(SRC).convert("RGB")
    w, h = im.size
    cx, cy = w // 2, h // 2
    half = min(cx, cy, w - cx, h - cy)
    sq = im.crop((cx - half, cy - half, cx + half, cy + half)).resize(
        (args.width, args.width), Image.Resampling.LANCZOS
    )

    args.out_dir.mkdir(parents=True, exist_ok=True)
    circle = build_circle(sq)
    circle.save(args.out_dir / "pantheon-logo-zenith-circle-only.png", compress_level=3)

    wm = wordmark_png("PANTHEON", args.width)
    gap = int(round(160 * args.width / 4096))
    mw, mh = circle.size
    ww, wh = wm.size
    vw, vh = max(mw, ww), mh + gap + wh
    lock = Image.new("RGBA", (vw, vh), (0, 0, 0, 0))
    lock.paste(circle, ((vw - mw) // 2, 0), circle)
    lock.paste(wm, ((vw - ww) // 2, mh + gap), wm)
    lock.save(args.out_dir / "pantheon-logo-zenith-lockup.png", compress_level=3)

    canvas = Image.new("RGBA", (vw, vh), (0, 0, 0, 0))
    canvas.paste(circle, ((vw - mw) // 2, 0), circle)
    canvas.save(args.out_dir / "pantheon-logo-zenith-same-size-no-text.png", compress_level=3)

    if args.hero_max > 0:
        hero_dir = args.out_dir

        def save_hero(src: Image.Image, base: str) -> None:
            w0 = src.size[0]
            if w0 > args.hero_max:
                nh = int(round(src.size[1] * args.hero_max / w0))
                small = src.resize((args.hero_max, nh), Image.Resampling.LANCZOS)
            else:
                small = src
            png = hero_dir / f"{base}-hero.png"
            webp = hero_dir / f"{base}-hero.webp"
            small.save(png, compress_level=9, optimize=True)
            small.save(webp, quality=88, method=6)
            print(png, webp)

        save_hero(lock, "pantheon-logo-zenith-lockup")
        save_hero(canvas, "pantheon-logo-zenith-no-text")

    print("OK", args.out_dir)


if __name__ == "__main__":
    main()
